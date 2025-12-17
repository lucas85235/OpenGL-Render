#ifndef PBR_UTILS_HPP
#define PBR_UTILS_HPP

#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

#include "../rhi/rhi_device.h"
#include "cpu_cubemap_converter.hpp"
#include "texture.hpp"

// stb_image is already included via texture.hpp
// We just need to declare the float loader if not visible
extern "C" {
extern float *stbi_loadf(char const *filename, int *x, int *y,
                         int *channels_in_file, int desired_channels);
extern float *stbi_loadf_from_memory(unsigned char const *buffer, int len,
                                     int *x, int *y, int *channels_in_file,
                                     int desired_channels);
extern void stbi_image_free(void *retval_from_stbi_load);
extern void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);
}

namespace PBRUtils {

class EnvironmentMap {
private:
  static constexpr int CUBEMAP_SIZE = 1024;
  static constexpr int IRRADIANCE_SIZE = 32;
  static constexpr int PREFILTER_SIZE = 128;
  static constexpr int BRDF_SIZE = 512;

  RHI::IDevice *device = nullptr;
  IFileSystem *fs = nullptr;

  void SetupCaptureMatrices(glm::mat4 *views, glm::mat4 &proj) {
    proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    views[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, -1.0f, 0.0f));
    views[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                           glm::vec3(0.0f, -1.0f, 0.0f));
    views[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, 1.0f));
    views[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                           glm::vec3(0.0f, 0.0f, -1.0f));
    views[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                           glm::vec3(0.0f, -1.0f, 0.0f));
    views[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                           glm::vec3(0.0f, -1.0f, 0.0f));
  }

  RHI::CubemapFace IndexToFace(int i) {
    return static_cast<RHI::CubemapFace>(i);
  }

public:
  RHI::TextureHandle envCubemap;
  RHI::TextureHandle irradianceMap;
  RHI::TextureHandle prefilterMap;
  RHI::TextureHandle brdfLUTTexture;
  RHI::SamplerHandle cubemapSampler;

  EnvironmentMap() = default;

  explicit EnvironmentMap(RHI::IDevice *dev) : device(dev) {}

  ~EnvironmentMap() {
    if (device) {
      if (RHI::IsValid(envCubemap))
        device->DestroyTexture(envCubemap);
      if (RHI::IsValid(irradianceMap))
        device->DestroyTexture(irradianceMap);
      if (RHI::IsValid(prefilterMap))
        device->DestroyTexture(prefilterMap);
      if (RHI::IsValid(brdfLUTTexture))
        device->DestroyTexture(brdfLUTTexture);
      if (RHI::IsValid(cubemapSampler))
        device->DestroySampler(cubemapSampler);
    }
  }

  void Initialize(RHI::IDevice *dev, IFileSystem *fileSys) {
    device = dev;
    fs = fileSys;
  }

  void LoadFromHDR(const std::string &path) {
    if (!device) {
      std::cerr << "[IBL] No device set!" << std::endl;
      return;
    }

    std::cout << "[IBL] Loading HDR: " << path << std::endl;

    if (!fs) {
      std::cerr << "[IBL] FileSystem not set!" << std::endl;
      return;
    }

    // Load HDR data via VFS
    std::vector<uint8_t> fileData = fs->ReadFile(path);
    if (fileData.empty()) {
      std::cerr << "[IBL] Failed to read HDR file: " << path << std::endl;
      return;
    }

    int hdrWidth, hdrHeight, hdrChannels;
    stbi_set_flip_vertically_on_load(true);
    float *hdrData =
        stbi_loadf_from_memory(fileData.data(), fileData.size(), &hdrWidth,
                               &hdrHeight, &hdrChannels, 0);

    if (!hdrData) {
      std::cerr << "[IBL] Failed to load HDR: " << path << std::endl;
      return;
    }

    std::cout << "[IBL] HDR loaded: " << hdrWidth << "x" << hdrHeight << " ("
              << hdrChannels << " channels)" << std::endl;

    // Create environment cubemap
    RHI::TextureDescriptor cubemapDesc;
    cubemapDesc.type = RHI::TextureType::TextureCube;
    cubemapDesc.format = RHI::TextureFormat::RGBA16F;
    cubemapDesc.width = CUBEMAP_SIZE;
    cubemapDesc.height = CUBEMAP_SIZE;
    cubemapDesc.mipLevels = 1;
    cubemapDesc.generateMipmaps = true;

    envCubemap = device->CreateTexture(cubemapDesc);
    if (!RHI::IsValid(envCubemap)) {
      std::cerr << "[IBL] Failed to create environment cubemap" << std::endl;
      stbi_image_free(hdrData);
      return;
    }

    // Convert equirectangular HDR to cubemap using CPU
    CpuCubemapConverter::Convert(hdrData, hdrWidth, hdrHeight, hdrChannels,
                                 CUBEMAP_SIZE, device, envCubemap);

    stbi_image_free(hdrData);

    // Create sampler for cubemap
    RHI::SamplerDescriptor samplerDesc;
    samplerDesc.minFilter = RHI::TextureFilterMode::Linear;
    samplerDesc.magFilter = RHI::TextureFilterMode::Linear;
    samplerDesc.wrapS = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.wrapT = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.wrapR = RHI::TextureWrapMode::ClampToEdge;
    cubemapSampler = device->CreateSampler(samplerDesc);

    GenerateIrradianceMap();
    GeneratePrefilterMap();
    GenerateBRDFLUT();

    std::cout << "[IBL] Environment maps created successfully." << std::endl;
  }

  void GenerateIrradianceMap() {
    if (!device)
      return;

    RHI::TextureDescriptor desc;
    desc.type = RHI::TextureType::TextureCube;
    desc.format = RHI::TextureFormat::RGBA16F;
    desc.width = IRRADIANCE_SIZE;
    desc.height = IRRADIANCE_SIZE;
    desc.mipLevels = 1;

    irradianceMap = device->CreateTexture(desc);
    if (!RHI::IsValid(irradianceMap)) {
      std::cerr << "[IBL] Failed to create irradiance map" << std::endl;
    }
  }

  void GeneratePrefilterMap() {
    if (!device)
      return;

    RHI::TextureDescriptor desc;
    desc.type = RHI::TextureType::TextureCube;
    desc.format = RHI::TextureFormat::RGBA16F;
    desc.width = PREFILTER_SIZE;
    desc.height = PREFILTER_SIZE;
    desc.mipLevels = 5;
    desc.generateMipmaps = true;

    prefilterMap = device->CreateTexture(desc);
    if (!RHI::IsValid(prefilterMap)) {
      std::cerr << "[IBL] Failed to create prefilter map" << std::endl;
    }
  }

  void GenerateBRDFLUT() {
    if (!device)
      return;

    RHI::TextureDescriptor desc;
    desc.type = RHI::TextureType::Texture2D;
    desc.format = RHI::TextureFormat::RG16F;
    desc.width = BRDF_SIZE;
    desc.height = BRDF_SIZE;
    desc.mipLevels = 1;

    brdfLUTTexture = device->CreateTexture(desc);
    if (!RHI::IsValid(brdfLUTTexture)) {
      std::cerr << "[IBL] Failed to create BRDF LUT" << std::endl;
    }
  }

  RHI::TextureHandle GetCubemap() const { return envCubemap; }
  RHI::TextureHandle GetIrradianceMap() const { return irradianceMap; }
  RHI::TextureHandle GetPrefilterMap() const { return prefilterMap; }
  RHI::TextureHandle GetBrdfLUT() const { return brdfLUTTexture; }
  RHI::SamplerHandle GetSampler() const { return cubemapSampler; }

  bool IsValid() const { return RHI::IsValid(envCubemap); }

  // Delete copy
  EnvironmentMap(const EnvironmentMap &) = delete;
  EnvironmentMap &operator=(const EnvironmentMap &) = delete;

  // Allow move
  EnvironmentMap(EnvironmentMap &&other) noexcept
      : device(other.device), envCubemap(other.envCubemap),
        irradianceMap(other.irradianceMap), prefilterMap(other.prefilterMap),
        brdfLUTTexture(other.brdfLUTTexture) {
    other.device = nullptr;
    other.envCubemap = {};
    other.irradianceMap = {};
    other.prefilterMap = {};
    other.brdfLUTTexture = {};
  }

  EnvironmentMap &operator=(EnvironmentMap &&other) noexcept {
    if (this != &other) {
      // Destroy current
      if (device) {
        if (RHI::IsValid(envCubemap))
          device->DestroyTexture(envCubemap);
        if (RHI::IsValid(irradianceMap))
          device->DestroyTexture(irradianceMap);
        if (RHI::IsValid(prefilterMap))
          device->DestroyTexture(prefilterMap);
        if (RHI::IsValid(brdfLUTTexture))
          device->DestroyTexture(brdfLUTTexture);
      }

      // Move
      device = other.device;
      envCubemap = other.envCubemap;
      irradianceMap = other.irradianceMap;
      prefilterMap = other.prefilterMap;
      brdfLUTTexture = other.brdfLUTTexture;

      other.device = nullptr;
      other.envCubemap = {};
      other.irradianceMap = {};
      other.prefilterMap = {};
      other.brdfLUTTexture = {};
    }
    return *this;
  }
};

} // namespace PBRUtils

#endif // PBR_UTILS_HPP