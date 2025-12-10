#ifndef TEXTURE_HPP
#define TEXTURE_HPP

#include "../rhi/rhi_device.h"
#include <GL/glew.h>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

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
  REPEAT = GL_REPEAT,
  MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
  CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
  CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER
};

enum class TextureFilter {
  NEAREST = GL_NEAREST,
  LINEAR = GL_LINEAR,
  NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
  LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR
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
  // RHI resources
  RHI::IDevice *device = nullptr;
  RHI::TextureHandle rhiTexture;
  RHI::SamplerHandle rhiSampler;
  bool useRHI = false;

  // Legacy OpenGL
  unsigned int id = 0;

  std::string path;
  TextureType type = TextureType::UNKNOWN;
  int width = 0;
  int height = 0;
  int channels = 0;
  bool loaded = false;

  RHI::TextureFormat GetRHIFormat(int chans, TextureType texType) const {
    if (texType == TextureType::DIFFUSE || texType == TextureType::EMISSION) {
      return chans == 4 ? RHI::TextureFormat::SRGB8_Alpha8
                        : RHI::TextureFormat::SRGB8;
    }
    return chans == 4 ? RHI::TextureFormat::RGBA8 : RHI::TextureFormat::RGB8;
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

public:
  Texture() = default;

  explicit Texture(RHI::IDevice *dev) : device(dev) {}

  ~Texture() {
    if (loaded) {
      if (useRHI && device) {
        if (RHI::IsValid(rhiSampler))
          device->DestroySampler(rhiSampler);
        if (RHI::IsValid(rhiTexture))
          device->DestroyTexture(rhiTexture);
      } else if (id != 0) {
        glDeleteTextures(1, &id);
      }
    }
  }

  void SetDevice(RHI::IDevice *dev) { device = dev; }

  bool LoadFromFile(const std::string &filepath, TextureType texType,
                    const TextureParams &params = TextureParams()) {
    path = filepath;
    type = texType;

    stbi_set_flip_vertically_on_load(params.flipVertically);
    unsigned char *data =
        stbi_load(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
      std::cerr << "[Texture] Failed to load: " << filepath << " - "
                << stbi_failure_reason() << std::endl;
      return false;
    }

    bool result = false;
    if (device) {
      result = LoadRHI(data, params);
    } else {
      result = LoadOpenGL(data, params);
    }

    stbi_image_free(data);

    if (result) {
      loaded = true;
      std::cout << "[Texture] Loaded: " << filepath << " (" << width << "x"
                << height << ")" << std::endl;
    }

    return result;
  }

private:
  bool LoadRHI(unsigned char *data, const TextureParams &params) {
    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = GetRHIFormat(channels, type);
    texDesc.width = width;
    texDesc.height = height;
    texDesc.depth = 1;
    texDesc.mipLevels = params.generateMipmap ? 0 : 1;
    texDesc.data = data;

    rhiTexture = device->CreateTexture(texDesc);
    if (!RHI::IsValid(rhiTexture)) {
      std::cerr << "[Texture] RHI: Failed to create texture" << std::endl;
      return false;
    }

    if (params.generateMipmap) {
      device->GenerateMipmaps(rhiTexture);
    }

    RHI::SamplerDescriptor samplerDesc;
    samplerDesc.wrapS = ToRHIWrapMode(params.wrapS);
    samplerDesc.wrapT = ToRHIWrapMode(params.wrapT);
    samplerDesc.minFilter = ToRHIFilterMode(params.minFilter);
    samplerDesc.magFilter = ToRHIFilterMode(params.magFilter);

    rhiSampler = device->CreateSampler(samplerDesc);
    useRHI = true;
    return true;
  }

  bool LoadOpenGL(unsigned char *data, const TextureParams &params) {
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_RGB;

    if (channels == 1) {
      format = GL_RED;
      internalFormat = GL_RED;
    } else if (channels == 3) {
      format = GL_RGB;
      internalFormat =
          (type == TextureType::DIFFUSE || type == TextureType::EMISSION)
              ? GL_SRGB
              : GL_RGB;
    } else if (channels == 4) {
      format = GL_RGBA;
      internalFormat =
          (type == TextureType::DIFFUSE || type == TextureType::EMISSION)
              ? GL_SRGB_ALPHA
              : GL_RGBA;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format,
                 GL_UNSIGNED_BYTE, data);

    if (params.generateMipmap) {
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, (GLint)params.wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, (GLint)params.wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    (GLint)params.minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    (GLint)params.magFilter);

    return true;
  }

public:
  bool LoadFromMemory(unsigned char *data, int length, TextureType texType,
                      const TextureParams &params = TextureParams()) {
    type = texType;
    stbi_set_flip_vertically_on_load(params.flipVertically);

    unsigned char *imageData =
        stbi_load_from_memory(data, length, &width, &height, &channels, 0);
    if (!imageData) {
      std::cerr << "[Texture] Failed to load from memory" << std::endl;
      return false;
    }

    bool result =
        device ? LoadRHI(imageData, params) : LoadOpenGL(imageData, params);
    stbi_image_free(imageData);

    if (result) {
      loaded = true;
      std::cout << "[Texture] Loaded from memory: " << width << "x" << height
                << std::endl;
    }
    return result;
  }

  bool LoadHDR(const std::string &filepath) {
    path = filepath;
    type = TextureType::UNKNOWN;

    stbi_set_flip_vertically_on_load(true);
    float *data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);

    if (!data) {
      std::cerr << "[Texture] Failed to load HDR: " << filepath << std::endl;
      return false;
    }

    if (device) {
      RHI::TextureDescriptor texDesc;
      texDesc.type = RHI::TextureType::Texture2D;
      texDesc.format = RHI::TextureFormat::RGBA16F;
      texDesc.width = width;
      texDesc.height = height;
      texDesc.depth = 1;
      texDesc.mipLevels = 1;
      texDesc.data = data;

      rhiTexture = device->CreateTexture(texDesc);
      useRHI = true;
    } else {
      glGenTextures(1, &id);
      glBindTexture(GL_TEXTURE_2D, id);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB,
                   GL_FLOAT, data);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    stbi_image_free(data);
    loaded = true;
    std::cout << "[Texture] HDR loaded: " << filepath << std::endl;
    return true;
  }

  void Bind(unsigned int slot = 0) const {
    if (useRHI && device) {
      device->BindTexture(slot, rhiTexture);
      device->BindSampler(slot, rhiSampler);
    } else {
      glActiveTexture(GL_TEXTURE0 + slot);
      glBindTexture(GL_TEXTURE_2D, id);
    }
  }

  void Unbind() const {
    if (!useRHI) {
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  // Getters
  void setId(unsigned int v) { id = v; }
  void setPath(const std::string &p) { path = p; }
  void setType(TextureType t) { type = t; }

  unsigned int GetID() const { return id; }
  RHI::TextureHandle GetRHIHandle() const { return rhiTexture; }
  const std::string &GetPath() const { return path; }
  TextureType GetType() const { return type; }
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  int GetChannels() const { return channels; }
  bool IsLoaded() const { return loaded; }
  bool IsUsingRHI() const { return useRHI; }

  Texture(const Texture &) = delete;
  Texture &operator=(const Texture &) = delete;

  Texture(Texture &&other) noexcept
      : device(other.device), rhiTexture(other.rhiTexture),
        rhiSampler(other.rhiSampler), useRHI(other.useRHI), id(other.id),
        path(std::move(other.path)), type(other.type), width(other.width),
        height(other.height), channels(other.channels), loaded(other.loaded) {
    other.device = nullptr;
    other.rhiTexture = RHI::TextureHandle{0};
    other.rhiSampler = RHI::SamplerHandle{0};
    other.id = 0;
    other.loaded = false;
    other.useRHI = false;
  }

  Texture &operator=(Texture &&other) noexcept {
    if (this != &other) {
      if (loaded) {
        if (useRHI && device) {
          if (RHI::IsValid(rhiSampler))
            device->DestroySampler(rhiSampler);
          if (RHI::IsValid(rhiTexture))
            device->DestroyTexture(rhiTexture);
        } else if (id != 0) {
          glDeleteTextures(1, &id);
        }
      }

      device = other.device;
      rhiTexture = other.rhiTexture;
      rhiSampler = other.rhiSampler;
      useRHI = other.useRHI;
      id = other.id;
      path = std::move(other.path);
      type = other.type;
      width = other.width;
      height = other.height;
      channels = other.channels;
      loaded = other.loaded;

      other.device = nullptr;
      other.rhiTexture = RHI::TextureHandle{0};
      other.rhiSampler = RHI::SamplerHandle{0};
      other.id = 0;
      other.loaded = false;
      other.useRHI = false;
    }
    return *this;
  }
};

class TextureManager {
private:
  std::map<std::string, std::shared_ptr<Texture>> cache;
  RHI::IDevice *device = nullptr;

  static TextureManager *instance;
  TextureManager() {}

public:
  static TextureManager &GetInstance() {
    if (!instance) {
      instance = new TextureManager();
    }
    return *instance;
  }

  void SetDevice(RHI::IDevice *dev) { device = dev; }

  std::shared_ptr<Texture>
  LoadTexture(const std::string &path, TextureType type,
              const TextureParams &params = TextureParams()) {
    auto it = cache.find(path);
    if (it != cache.end()) {
      return it->second;
    }

    auto texture = std::make_shared<Texture>(device);
    if (texture->LoadFromFile(path, type, params)) {
      cache[path] = texture;
      return texture;
    }

    return nullptr;
  }

  void ClearCache() {
    cache.clear();
    std::cout << "[TextureManager] Cache cleared" << std::endl;
  }

  size_t GetCacheSize() const { return cache.size(); }

  void PrintCacheInfo() const {
    std::cout << "\n=== Texture Cache ===" << std::endl;
    std::cout << "Total: " << cache.size() << " textures" << std::endl;
    for (const auto &pair : cache) {
      std::cout << "- " << pair.first << std::endl;
    }
  }
};

TextureManager *TextureManager::instance = nullptr;

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