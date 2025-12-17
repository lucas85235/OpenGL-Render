#ifndef CPU_CUBEMAP_CONVERTER_HPP
#define CPU_CUBEMAP_CONVERTER_HPP

#include "../rhi/rhi_device.h"
#include <cmath>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

/**
 * @brief CPU-based equirectangular to cubemap conversion
 *
 * Samples an equirectangular HDR image and generates 6 cubemap faces.
 * This approach is simpler than GPU-based render-to-texture but slower.
 * Suitable for offline/loading-time conversion.
 */
class CpuCubemapConverter {
private:
  // Convert float32 to float16 (IEEE 754 half-precision)
  static uint16_t FloatToHalf(float value) {
    uint32_t f = *reinterpret_cast<uint32_t *>(&value);
    uint32_t sign = (f >> 31) & 0x1;
    int32_t exponent = ((f >> 23) & 0xFF) - 127 + 15;
    uint32_t mantissa = f & 0x7FFFFF;

    if (exponent <= 0) {
      return static_cast<uint16_t>(sign << 15);
    } else if (exponent >= 31) {
      return static_cast<uint16_t>((sign << 15) | (31 << 10));
    }
    return static_cast<uint16_t>((sign << 15) | (exponent << 10) |
                                 (mantissa >> 13));
  }

  static glm::vec3 GetCubeFaceDirection(int face, float u, float v) {
    // u, v are in [0, 1], convert to [-1, 1]
    float s = u * 2.0f - 1.0f;
    float t = v * 2.0f - 1.0f;

    switch (face) {
    case 0: // +X
      return glm::normalize(glm::vec3(1.0f, -t, -s));
    case 1: // -X
      return glm::normalize(glm::vec3(-1.0f, -t, s));
    case 2: // +Y
      return glm::normalize(glm::vec3(s, 1.0f, t));
    case 3: // -Y
      return glm::normalize(glm::vec3(s, -1.0f, -t));
    case 4: // +Z
      return glm::normalize(glm::vec3(s, -t, 1.0f));
    case 5: // -Z
      return glm::normalize(glm::vec3(-s, -t, -1.0f));
    default:
      return glm::vec3(0.0f);
    }
  }

  static glm::vec2 DirectionToEquirectUV(const glm::vec3 &dir) {
    float phi = std::atan2(dir.z, dir.x);
    float theta = std::asin(glm::clamp(dir.y, -1.0f, 1.0f));

    float u = (phi / (2.0f * 3.14159265359f)) + 0.5f;
    float v = (theta / 3.14159265359f) + 0.5f;

    return glm::vec2(u, v);
  }

public:
  /**
   * @brief Convert equirectangular HDR data to cubemap faces
   * @param hdrData Raw HDR float data (RGB or RGBA, row-major)
   * @param hdrWidth Width of HDR image
   * @param hdrHeight Height of HDR image
   * @param hdrChannels Number of channels (3 or 4)
   * @param faceSize Output cubemap face size
   * @param device RHI device for uploading to cubemap
   * @param cubemap Target cubemap texture handle
   */
  static bool Convert(const float *hdrData, int hdrWidth, int hdrHeight,
                      int hdrChannels, int faceSize, RHI::IDevice *device,
                      RHI::TextureHandle cubemap) {
    if (!hdrData || !device || !RHI::IsValid(cubemap)) {
      std::cerr << "[CpuCubemap] Invalid parameters" << std::endl;
      return false;
    }

    std::cout << "[CpuCubemap] Converting " << hdrWidth << "x" << hdrHeight
              << " HDR to " << faceSize << "x" << faceSize << " cubemap..."
              << std::endl;

    // Allocate buffers: float32 for processing, uint16 (half) for upload
    std::vector<float> faceDataF32(faceSize * faceSize * 4);
    std::vector<uint16_t> faceDataF16(faceSize * faceSize * 4);

    for (int face = 0; face < 6; ++face) {
      std::fill(faceDataF32.begin(), faceDataF32.end(), 0.0f);

      for (int y = 0; y < faceSize; ++y) {
        for (int x = 0; x < faceSize; ++x) {
          float u = (x + 0.5f) / faceSize;
          float v = (y + 0.5f) / faceSize;

          glm::vec3 dir = GetCubeFaceDirection(face, u, v);
          glm::vec2 equirectUV = DirectionToEquirectUV(dir);

          // Sample HDR with bilinear interpolation
          float srcX = equirectUV.x * (hdrWidth - 1);
          float srcY = (1.0f - equirectUV.y) * (hdrHeight - 1); // Flip Y

          int x0 = static_cast<int>(srcX);
          int y0 = static_cast<int>(srcY);
          int x1 = std::min(x0 + 1, hdrWidth - 1);
          int y1 = std::min(y0 + 1, hdrHeight - 1);

          float fx = srcX - x0;
          float fy = srcY - y0;

          auto sample = [&](int sx, int sy) -> glm::vec3 {
            int idx = (sy * hdrWidth + sx) * hdrChannels;
            return glm::vec3(hdrData[idx], hdrData[idx + 1], hdrData[idx + 2]);
          };

          glm::vec3 c00 = sample(x0, y0);
          glm::vec3 c10 = sample(x1, y0);
          glm::vec3 c01 = sample(x0, y1);
          glm::vec3 c11 = sample(x1, y1);

          glm::vec3 c0 = glm::mix(c00, c10, fx);
          glm::vec3 c1 = glm::mix(c01, c11, fx);
          glm::vec3 color = glm::mix(c0, c1, fy);

          int outIdx = (y * faceSize + x) * 4;
          faceDataF32[outIdx + 0] = color.r;
          faceDataF32[outIdx + 1] = color.g;
          faceDataF32[outIdx + 2] = color.b;
          faceDataF32[outIdx + 3] = 1.0f;
        }
      }

      // Convert float32 to float16 for RGBA16F texture
      for (size_t i = 0; i < faceDataF32.size(); ++i) {
        faceDataF16[i] = FloatToHalf(faceDataF32[i]);
      }

      // Upload face to cubemap (now as half-float data)
      RHI::CubemapFace rhiFace = static_cast<RHI::CubemapFace>(face);
      device->UpdateTextureCubeFace(cubemap, rhiFace, faceDataF16.data(), 0);

      std::cout << "[CpuCubemap] Face " << face << " converted" << std::endl;
    }

    device->GenerateMipmaps(cubemap);

    std::cout << "[CpuCubemap] Conversion complete" << std::endl;
    return true;
  }
};

#endif // CPU_CUBEMAP_CONVERTER_HPP
