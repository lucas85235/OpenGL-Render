#ifndef PARTICLE_SYSTEM_HPP
#define PARTICLE_SYSTEM_HPP

#include "../renderer/material.hpp"
#include "../renderer/mesh.hpp"
#include "../renderer/renderer.hpp"
#include "scene.hpp"
#include <functional>
#include <memory>
#include <vector>

using ParticlePositionFunction = std::function<glm::vec4(float index_t)>;

struct ParticleSystemParams {
  int amount = 1000;
  glm::vec3 basePosition = glm::vec3(0.0f);
  glm::vec3 velocity = glm::vec3(0.0f, 1.0f, 0.0f);
  glm::vec3 baseColor = glm::vec3(1.0f);
  glm::vec2 size = glm::vec2(0.1f);
  float radius = 1.0f;
  float angle = 0.0f;
  bool center = true;
  std::shared_ptr<Material> material;
  std::shared_ptr<Mesh> customMesh; // If null, Renderer will use a quad

  // Function to determine initial position (x,y,z) and maybe something else in
  // w
  ParticlePositionFunction positionFunction = [](float t) -> glm::vec4 {
    return glm::vec4(0.0f);
  };
};

class ParticleSystemComponent : public Component {
private:
  ParticleSystemParams params;
  std::shared_ptr<Texture> mappingTexture;
  int textureWidth = 0;
  bool initialized = false;

  void CreateMappingTexture(RHI::IDevice *device) {
    textureWidth = static_cast<int>(std::ceil(std::sqrt(params.amount)));
    int texSize = textureWidth * textureWidth;

    std::vector<float> data(texSize * 4, 0.0f);

    for (int i = 0; i < params.amount; ++i) {
      float t = static_cast<float>(i) / static_cast<float>(params.amount);
      glm::vec4 pos = params.positionFunction(t);
      data[i * 4 + 0] = pos.x;
      data[i * 4 + 1] = pos.y;
      data[i * 4 + 2] = pos.z;
      data[i * 4 + 3] = pos.w;
    }

    mappingTexture = std::make_shared<Texture>();
    if (!mappingTexture->CreateFromData(device, data.data(), textureWidth,
                                        textureWidth,
                                        RHI::TextureFormat::RGBA32F)) {
      std::cerr << "Failed to create GPU mapping texture for particles!"
                << std::endl;
      mappingTexture.reset();
    }
  }

public:
  ParticleSystemComponent(const ParticleSystemParams &p) : params(p) {}

  void OnRender(Renderer &renderer) override {
    if (!initialized && renderer.GetDevice()) {
      CreateMappingTexture(renderer.GetDevice());
      initialized = true;
    }

    if (initialized && mappingTexture) {
      // Create a ParticleRenderCommand
      ParticleRenderCommand pCmd;
      pCmd.amount = params.amount;
      pCmd.textureWidth = textureWidth;
      pCmd.mappingTexture = mappingTexture;
      pCmd.material = params.material;
      pCmd.customMesh = params.customMesh;
      pCmd.transform = entity->transform.GetMatrix();

      // Additional properties
      pCmd.velocity = params.velocity;
      pCmd.baseColor = params.baseColor;
      pCmd.size = params.size;
      pCmd.radius = params.radius;
      pCmd.angle = params.angle;
      pCmd.center = params.center;

      renderer.SubmitParticles(pCmd);
    }
  }

  // Setters/Getters
  void SetParams(const ParticleSystemParams &p) {
    params = p;
    initialized = false; // Recreate texture next frame
  }
};

#endif // PARTICLE_SYSTEM_HPP
