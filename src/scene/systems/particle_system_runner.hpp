#ifndef PARTICLE_SYSTEM_RUNNER_HPP
#define PARTICLE_SYSTEM_RUNNER_HPP

#include "../particle_system.hpp"
#include "../scene.hpp"
#include "../system.hpp"

class ParticleSystemRunner : public System {
public:
  void OnRender(Scene *scene, Renderer &renderer) override {
    auto &registry = scene->GetRegistry();
    auto particleView =
        registry.view<TransformComponent, ParticleSystemComponent>();

    for (auto entity : particleView) {
      auto [transform, particle] =
          particleView.get<TransformComponent, ParticleSystemComponent>(entity);

      if (!particle.initialized && renderer.GetDevice()) {
        particle.CreateMappingTexture(renderer.GetDevice());
        particle.initialized = true;
      }

      if (particle.initialized && particle.mappingTexture) {
        ParticleRenderCommand pCmd;
        pCmd.amount = particle.params.amount;
        pCmd.textureWidth = particle.textureWidth;
        pCmd.mappingTexture = particle.mappingTexture;
        pCmd.material = particle.params.material;
        pCmd.customMesh = particle.params.customMesh;
        pCmd.transform = transform.GetMatrix();

        pCmd.velocity = particle.params.velocity;
        pCmd.baseColor = particle.params.baseColor;
        pCmd.size = particle.params.size;
        pCmd.radius = particle.params.radius;
        pCmd.angle = particle.params.angle;
        pCmd.center = particle.params.center;

        renderer.SubmitParticles(pCmd);
      }
    }
  }
};

#endif
