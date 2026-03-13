#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "../rhi/rhi_device.h"
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "../core/vfs/file_system.hpp"
#include "../vendor/stb_image.h"

enum class TextureType {
  DIFFUSE,
  SPECULAR,
  NORMAL,
  HEIGHT,
  AMBIENT,
  EMISSION,
  METALLIC,
  ROUGHNESS,
  AO,
  UNKNOWN
};

enum class TextureWrap {
  REPEAT,
  MIRRORED_REPEAT,
  CLAMP_TO_EDGE,
  CLAMP_TO_BORDER
};
enum class TextureFilter {
  NEAREST,
  LINEAR,
  NEAREST_MIPMAP_NEAREST,
  LINEAR_MIPMAP_LINEAR
};

struct TextureParams {
  TextureWrap wrapS = TextureWrap::REPEAT;
  TextureWrap wrapT = TextureWrap::REPEAT;
  TextureFilter minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR;
  TextureFilter magFilter = TextureFilter::LINEAR;
  bool generateMipmap = true;
  bool flipVertically = true;
};

class Texture {
private:
  RHI::IDevice *device = nullptr;
  RHI::TextureHandle textureHandle;
  RHI::SamplerHandle samplerHandle;
  std::string path;
  TextureType type = TextureType::UNKNOWN;
  int width = 0, height = 0, channels = 0;
  bool loaded = false;

  RHI::TextureFormat GetRHIFormat(TextureType texType) const {
    // Color textures need SRGB for correct gamma handling
    // Data textures (normal, metallic, roughness, ao) must be linear
    switch (texType) {
    case TextureType::DIFFUSE:
    case TextureType::EMISSION:
      return RHI::TextureFormat::SRGB8_Alpha8;
    default:
      return RHI::TextureFormat::RGBA8; // Linear for data textures
    }
  }

  RHI::TextureWrapMode ToRHIWrapMode(TextureWrap wrap) const {
    switch (wrap) {
    case TextureWrap::REPEAT:
      return RHI::TextureWrapMode::Repeat;
    case TextureWrap::MIRRORED_REPEAT:
      return RHI::TextureWrapMode::MirroredRepeat;
    case TextureWrap::CLAMP_TO_EDGE:
      return RHI::TextureWrapMode::ClampToEdge;
    case TextureWrap::CLAMP_TO_BORDER:
      return RHI::TextureWrapMode::ClampToBorder;
    default:
      return RHI::TextureWrapMode::Repeat;
    }
  }

  RHI::TextureFilterMode ToRHIFilterMode(TextureFilter filter) const {
    switch (filter) {
    case TextureFilter::NEAREST:
      return RHI::TextureFilterMode::Nearest;
    case TextureFilter::LINEAR:
      return RHI::TextureFilterMode::Linear;
    case TextureFilter::NEAREST_MIPMAP_NEAREST:
      return RHI::TextureFilterMode::NearestMipmapNearest;
    case TextureFilter::LINEAR_MIPMAP_LINEAR:
      return RHI::TextureFilterMode::LinearMipmapLinear;
    default:
      return RHI::TextureFilterMode::Linear;
    }
  }

  bool CreateTextureRHI(unsigned char *data, const TextureParams &params) {
    if (!device)
      return false;

    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = GetRHIFormat(type);
    texDesc.width = width;
    texDesc.height = height;
    texDesc.depth = 1;
    texDesc.mipLevels = params.generateMipmap ? 0 : 1;
    texDesc.data = data;

    textureHandle = device->CreateTexture(texDesc);
    if (!RHI::IsValid(textureHandle)) {
      std::cerr << "[Texture] Failed to create texture" << std::endl;
      return false;
    }

    if (params.generateMipmap) {
      device->GenerateMipmaps(textureHandle);
    }

    RHI::SamplerDescriptor samplerDesc;
    samplerDesc.wrapS = ToRHIWrapMode(params.wrapS);
    samplerDesc.wrapT = ToRHIWrapMode(params.wrapT);
    samplerDesc.minFilter = ToRHIFilterMode(params.minFilter);
    samplerDesc.magFilter = ToRHIFilterMode(params.magFilter);
    samplerHandle = device->CreateSampler(samplerDesc);

    return true;
  }

public:
  Texture() = default;
  explicit Texture(RHI::IDevice *dev) : device(dev) {}

  ~Texture() {
    if (loaded && device) {
      if (RHI::IsValid(samplerHandle))
        device->DestroySampler(samplerHandle);
      if (RHI::IsValid(textureHandle))
        device->DestroyTexture(textureHandle);
    }
  }

  void SetDevice(RHI::IDevice *dev) { device = dev; }

  bool LoadFromFile(IFileSystem *fs, const std::string &filepath,
                    TextureType texType,
                    const TextureParams &params = TextureParams()) {
    if (!device || !fs) {
      std::cerr << "[Texture] Device or FS not set" << std::endl;
      return false;
    }

    path = filepath;
    type = texType;

    // Read file via VFS
    std::vector<uint8_t> fileData = fs->ReadFile(filepath);
    if (fileData.empty()) {
      std::cerr << "[Texture] Failed to read file: " << filepath << std::endl;
      return false;
    }

    return LoadFromMemory(fileData.data(), fileData.size(), texType, params);
  }

  bool LoadFromMemory(unsigned char *data, int length, TextureType texType,
                      const TextureParams &params = TextureParams()) {
    if (!device)
      return false;

    type = texType;
    stbi_set_flip_vertically_on_load(params.flipVertically);

    // Force 4 channels (RGBA) for Vulkan alignment requirements
    unsigned char *imageData =
        stbi_load_from_memory(data, length, &width, &height, &channels, 4);

    if (!imageData) {
      std::cerr << "[Texture] Failed to load from memory: "
                << stbi_failure_reason() << std::endl;
      return false;
    }

    // Update channels to reflect actual output (always 4)
    channels = 4;
    bool result = CreateTextureRHI(imageData, params);
    stbi_image_free(imageData);

    if (result)
      loaded = true;
    return result;
  }

  bool LoadHDR(const std::string &filepath) {
    if (!device)
      return false;

    path = filepath;
    type = TextureType::UNKNOWN;
    stbi_set_flip_vertically_on_load(true);
    float *data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
      std::cerr << "[Texture] Failed to load HDR: " << filepath << std::endl;
      return false;
    }

    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = RHI::TextureFormat::RGBA16F;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.depth = 1;
    texDesc.mipLevels = 1;
    texDesc.data = data;

    textureHandle = device->CreateTexture(texDesc);
    stbi_image_free(data);

    if (RHI::IsValid(textureHandle)) {
      loaded = true;
      return true;
    }
    return false;
  }

  bool CreateFromData(RHI::IDevice *dev, float *data, int w, int h,
                      RHI::TextureFormat format) {
    if (!dev)
      return false;

    device = dev;
    width = w;
    height = h;
    channels = 4;
    type = TextureType::UNKNOWN;

    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = format;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.depth = 1;
    texDesc.mipLevels = 1;
    texDesc.data = data;

    textureHandle = device->CreateTexture(texDesc);
    if (!RHI::IsValid(textureHandle)) {
      return false;
    }

    RHI::SamplerDescriptor samplerDesc;
    samplerDesc.wrapS = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.wrapT = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.minFilter = RHI::TextureFilterMode::Nearest;
    samplerDesc.magFilter = RHI::TextureFilterMode::Nearest;
    samplerHandle = device->CreateSampler(samplerDesc);

    loaded = true;
    return true;
  }

  void Bind(RHI::ICommandList *cmdList, unsigned int slot = 0) const {
    if (cmdList && loaded) {
      cmdList->BindTexture(slot, textureHandle);
      cmdList->BindSampler(slot, samplerHandle);
    }
  }

  RHI::TextureHandle GetHandle() const { return textureHandle; }
  const std::string &GetPath() const { return path; }
  TextureType GetType() const { return type; }
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  int GetChannels() const { return channels; }
  bool IsLoaded() const { return loaded; }

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  Texture(Texture &&other) noexcept
      : device(other.device), textureHandle(other.textureHandle),
        samplerHandle(other.samplerHandle), path(std::move(other.path)),
        type(other.type), width(other.width), height(other.height),
        channels(other.channels), loaded(other.loaded) {
    other.device = nullptr;
    other.textureHandle = RHI::TextureHandle{0};
    other.samplerHandle = RHI::SamplerHandle{0};
    other.loaded = false;
  }

  Texture &operator=(Texture &&other) noexcept {
    if (this != &other) {
      if (loaded && device) {
        if (RHI::IsValid(samplerHandle))
          device->DestroySampler(samplerHandle);
        if (RHI::IsValid(textureHandle))
          device->DestroyTexture(textureHandle);
      }
      device = other.device;
      textureHandle = other.textureHandle;
      samplerHandle = other.samplerHandle;
      path = std::move(other.path);
      type = other.type;
      width = other.width;
      height = other.height;
      channels = other.channels;
      loaded = other.loaded;
      other.device = nullptr;
      other.textureHandle = RHI::TextureHandle{0};
      other.samplerHandle = RHI::SamplerHandle{0};
      other.loaded = false;
    }
    return *this;
  }
};

class TextureManager {
private:
  std::map<std::string, std::shared_ptr<Texture>> cache;
  RHI::IDevice *device = nullptr;
  IFileSystem *fs = nullptr;

public:
  TextureManager() = default;

  void Initialize(RHI::IDevice *dev, IFileSystem *fileSys) {
    device = dev;
    fs = fileSys;
  }

  std::shared_ptr<Texture>
  LoadTexture(const std::string &path, TextureType type,
              const TextureParams &params = TextureParams()) {
    auto it = cache.find(path);
    if (it != cache.end())
      return it->second;

    auto texture = std::make_shared<Texture>(device);
    if (texture->LoadFromFile(fs, path, type, params)) {
      cache[path] = texture;
      return texture;
    }
    return nullptr;
  }

  void ClearCache() { cache.clear(); }
  size_t GetCacheSize() const { return cache.size(); }
};

inline std::string TextureTypeToString(TextureType type) {
  switch (type) {
  case TextureType::DIFFUSE:
    return "texture_diffuse";
  case TextureType::SPECULAR:
    return "texture_specular";
  case TextureType::NORMAL:
    return "texture_normal";
  case TextureType::HEIGHT:
    return "texture_height";
  case TextureType::AMBIENT:
    return "texture_ambient";
  case TextureType::EMISSION:
    return "texture_emission";
  case TextureType::METALLIC:
    return "texture_metallic";
  case TextureType::ROUGHNESS:
    return "texture_roughness";
  case TextureType::AO:
    return "texture_ao";
  default:
    return "texture_unknown";
  }
}

inline TextureType TextureTypeFromString(const std::string &s) {
  if (s == "texture_diffuse")
    return TextureType::DIFFUSE;
  if (s == "texture_specular")
    return TextureType::SPECULAR;
  if (s == "texture_normal" || s == "texture_normals")
    return TextureType::NORMAL;
  if (s == "texture_height")
    return TextureType::HEIGHT;
  if (s == "texture_ambient")
    return TextureType::AMBIENT;
  if (s == "texture_emission")
    return TextureType::EMISSION;
  if (s == "texture_metallic")
    return TextureType::METALLIC;
  if (s == "texture_roughness")
    return TextureType::ROUGHNESS;
  if (s == "texture_ao")
    return TextureType::AO;
  return TextureType::UNKNOWN;
}

#endif // TEXTURE_HPP