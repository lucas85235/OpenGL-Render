#ifndef LIGHTING_HPP
#define LIGHTING_HPP

#include "../rhi/rhi_device.h"
#include <glm/glm.hpp>
#include <vector>

constexpr int MAX_POINT_LIGHTS = 16;
constexpr int MAX_SPOT_LIGHTS = 8;

struct DirectionalLight {
  glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 1.0f;

  glm::vec3 GetDirection() const { return glm::normalize(direction); }
};

struct PointLight {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 10.0f;
  float radius = 10.0f;

  float GetAttenuation(float distance) const {
    if (distance >= radius)
      return 0.0f;
    float attenuation = 1.0f - (distance / radius);
    return attenuation * attenuation;
  }
};

struct SpotLight {
  glm::vec3 position = glm::vec3(0.0f);
  glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 10.0f;
  float range = 20.0f;
  float innerAngle = 12.5f; // degrees
  float outerAngle = 17.5f; // degrees

  float GetCutoffCos() const { return cos(glm::radians(outerAngle)); }
  float GetInnerCutoffCos() const { return cos(glm::radians(innerAngle)); }
};

class LightManager {
private:
  DirectionalLight sunLight;
  std::vector<PointLight> pointLights;
  std::vector<SpotLight> spotLights;
  bool hasSunLight = false;

public:
  void Clear() {
    hasSunLight = false;
    pointLights.clear();
    spotLights.clear();
  }

  void SetSunLight(const DirectionalLight &light) {
    sunLight = light;
    hasSunLight = true;
  }

  void AddPointLight(const PointLight &light) {
    if (pointLights.size() < MAX_POINT_LIGHTS) {
      pointLights.push_back(light);
    }
  }

  void AddSpotLight(const SpotLight &light) {
    if (spotLights.size() < MAX_SPOT_LIGHTS) {
      spotLights.push_back(light);
    }
  }

  bool HasSunLight() const { return hasSunLight; }
  const DirectionalLight &GetSunLight() const { return sunLight; }
  const std::vector<PointLight> &GetPointLights() const { return pointLights; }
  const std::vector<SpotLight> &GetSpotLights() const { return spotLights; }
  size_t GetPointLightCount() const { return pointLights.size(); }
  size_t GetSpotLightCount() const { return spotLights.size(); }

  void ApplyToShader(RHI::IDevice *dev, RHI::ShaderHandle shader) const {
    if (!dev)
      return;

    // Directional light
    if (hasSunLight) {
      glm::vec3 dir = sunLight.GetDirection();
      dev->SetUniform(shader, "dirLight.direction", &dir[0], 3);
      dev->SetUniform(shader, "dirLight.color", &sunLight.color[0], 3);
      dev->SetUniform(shader, "dirLight.intensity", sunLight.intensity);
    }

    // Point lights
    int numLights = static_cast<int>(pointLights.size());
    dev->SetUniform(shader, "numPointLights", numLights);

    for (size_t i = 0; i < pointLights.size(); ++i) {
      std::string prefix = "pointLights[" + std::to_string(i) + "].";
      dev->SetUniform(shader, (prefix + "position").c_str(),
                      &pointLights[i].position[0], 3);
      dev->SetUniform(shader, (prefix + "color").c_str(),
                      &pointLights[i].color[0], 3);
      dev->SetUniform(shader, (prefix + "intensity").c_str(),
                      pointLights[i].intensity);
      dev->SetUniform(shader, (prefix + "radius").c_str(),
                      pointLights[i].radius);
    }

    // Spot lights
    int numSpots = static_cast<int>(spotLights.size());
    dev->SetUniform(shader, "numSpotLights", numSpots);

    for (size_t i = 0; i < spotLights.size(); ++i) {
      std::string prefix = "spotLights[" + std::to_string(i) + "].";
      glm::vec3 dir = glm::normalize(spotLights[i].direction);
      dev->SetUniform(shader, (prefix + "position").c_str(),
                      &spotLights[i].position[0], 3);
      dev->SetUniform(shader, (prefix + "direction").c_str(), &dir[0], 3);
      dev->SetUniform(shader, (prefix + "color").c_str(),
                      &spotLights[i].color[0], 3);
      dev->SetUniform(shader, (prefix + "intensity").c_str(),
                      spotLights[i].intensity);
      dev->SetUniform(shader, (prefix + "range").c_str(), spotLights[i].range);
      dev->SetUniform(shader, (prefix + "cutOff").c_str(),
                      spotLights[i].GetCutoffCos());
      dev->SetUniform(shader, (prefix + "outerCutOff").c_str(),
                      spotLights[i].GetInnerCutoffCos());
    }
  }
};

#endif // LIGHTING_HPP
