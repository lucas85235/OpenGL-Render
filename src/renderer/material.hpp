#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "../rhi/rhi_device.h"
#include "texture.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <vector>

struct MaterialProperties {
  glm::vec3 albedo = glm::vec3(1.0f);
  float metallic = 0.0f;
  float roughness = 0.5f;
  float ao = 1.0f;
  glm::vec3 emission = glm::vec3(0.0f);
  float emissionStrength = 0.0f;
  glm::vec3 ambient = glm::vec3(0.3f);
  glm::vec3 diffuse = glm::vec3(1.0f);
  glm::vec3 specular = glm::vec3(0.5f);
  float shininess = 32.0f;
};

class Material {
private:
  std::string name;
  MaterialProperties properties;
  std::vector<std::shared_ptr<Texture>> textures;

public:
  Material(const std::string &materialName = "Default") : name(materialName) {}

  void AddTexture(std::shared_ptr<Texture> texture) {
    if (texture && texture->IsLoaded()) {
      textures.push_back(texture);
    }
  }

  bool LoadTexture(const std::string &path, TextureType type,
                   const TextureParams &params = TextureParams()) {
    auto &manager = TextureManager::GetInstance();
    auto texture = manager.LoadTexture(path, type, params);
    if (texture) {
      AddTexture(texture);
      return true;
    }
    return false;
  }

  void Apply(RHI::IDevice *dev, RHI::ShaderHandle shader) const {
    if (!dev)
      return;

    // Bind textures by TYPE to correct slot (not by array index)
    for (const auto &tex : textures) {
      uint32_t slot = GetSlotForTextureType(tex->GetType());
      std::cout << "[Material] Binding " << TextureTypeToString(tex->GetType())
                << " to slot " << slot << std::endl;
      tex->Bind(slot);
    }

    dev->SetUniform(shader, "material.albedo", &properties.albedo[0], 3);
    dev->SetUniform(shader, "material.metallic", properties.metallic);
    dev->SetUniform(shader, "material.roughness", properties.roughness);
    dev->SetUniform(shader, "material.ao", properties.ao);
    dev->SetUniform(shader, "material.emission", &properties.emission[0], 3);
    dev->SetUniform(shader, "material.emissionStrength",
                    properties.emissionStrength);
  }

  // Map texture type to shader slot (0=diffuse, 1=normal, 2=metallic, etc.)
  static uint32_t GetSlotForTextureType(TextureType type) {
    switch (type) {
    case TextureType::DIFFUSE:
      return 0;
    case TextureType::NORMAL:
      return 1;
    case TextureType::METALLIC:
      return 2;
    case TextureType::ROUGHNESS:
      return 3;
    case TextureType::AO:
      return 4;
    case TextureType::EMISSION:
      return 5;
    default:
      return 0;
    }
  }

  void SetName(const std::string &n) { name = n; }
  const std::string &GetName() const { return name; }
  MaterialProperties &GetProperties() { return properties; }
  const MaterialProperties &GetProperties() const { return properties; }

  void SetAlbedo(const glm::vec3 &albedo) { properties.albedo = albedo; }
  void SetMetallic(float metallic) { properties.metallic = metallic; }
  void SetRoughness(float roughness) { properties.roughness = roughness; }
  void SetAO(float ao) { properties.ao = ao; }
  void SetEmission(const glm::vec3 &emission) {
    properties.emission = emission;
  }
  void SetEmissionStrength(float strength) {
    properties.emissionStrength = strength;
  }
  void SetAmbient(const glm::vec3 &ambient) { properties.ambient = ambient; }
  void SetDiffuse(const glm::vec3 &diffuse) { properties.diffuse = diffuse; }
  void SetSpecular(const glm::vec3 &specular) {
    properties.specular = specular;
  }
  void SetShininess(float shininess) { properties.shininess = shininess; }

  size_t GetTextureCount() const { return textures.size(); }
  std::shared_ptr<Texture> GetTexture(size_t index) const {
    return index < textures.size() ? textures[index] : nullptr;
  }

  bool HasTextureType(TextureType type) const {
    for (const auto &tex : textures) {
      if (tex->GetType() == type)
        return true;
    }
    return false;
  }

  void Clear() { textures.clear(); }
};

class MaterialLibrary {
public:
  static Material CreateGold() {
    Material mat("Gold");
    mat.SetAlbedo(glm::vec3(1.0f, 0.765557f, 0.336057f));
    mat.SetMetallic(1.0f);
    mat.SetRoughness(0.3f);
    return mat;
  }

  static Material CreateSilver() {
    Material mat("Silver");
    mat.SetAlbedo(glm::vec3(0.972f, 0.960f, 0.915f));
    mat.SetMetallic(1.0f);
    mat.SetRoughness(0.2f);
    return mat;
  }

  static Material CreateCopper() {
    Material mat("Copper");
    mat.SetAlbedo(glm::vec3(0.955f, 0.637f, 0.538f));
    mat.SetMetallic(1.0f);
    mat.SetRoughness(0.4f);
    return mat;
  }

  static Material CreatePlastic() {
    Material mat("Plastic");
    mat.SetAlbedo(glm::vec3(1.0f, 0.0f, 0.0f));
    mat.SetMetallic(0.0f);
    mat.SetRoughness(0.6f);
    return mat;
  }

  static Material CreateRubber() {
    Material mat("Rubber");
    mat.SetAlbedo(glm::vec3(0.2f, 0.2f, 0.2f));
    mat.SetMetallic(0.0f);
    mat.SetRoughness(0.9f);
    return mat;
  }

  static Material CreateEmissive(const glm::vec3 &color,
                                 float strength = 1.0f) {
    Material mat("Emissive");
    mat.SetAlbedo(color);
    mat.SetEmission(color);
    mat.SetEmissionStrength(strength);
    return mat;
  }

  static Material CreatePhong(const glm::vec3 &diffuseColor) {
    Material mat("Phong");
    mat.SetDiffuse(diffuseColor);
    mat.SetAmbient(diffuseColor * 0.3f);
    mat.SetSpecular(glm::vec3(0.5f));
    mat.SetShininess(32.0f);
    return mat;
  }
};

#endif // MATERIAL_HPP