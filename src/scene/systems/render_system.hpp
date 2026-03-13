#ifndef RENDER_SYSTEM_HPP
#define RENDER_SYSTEM_HPP

#include "../components.hpp"
#include "../scene.hpp"
#include "../system.hpp"

class RenderSystem : public System {
public:
  void OnRender(Scene *scene, Renderer &renderer) override {
    auto &registry = scene->GetRegistry();

    // Render Simple Meshes
    auto simpleMeshView =
        registry.view<TransformComponent, SimpleMeshRendererComponent>();
    for (auto entity : simpleMeshView) {
      auto [transform, meshComp] =
          simpleMeshView.get<TransformComponent, SimpleMeshRendererComponent>(
              entity);
      if (meshComp.mesh) {
        renderer.SubmitMesh(*meshComp.mesh, transform.GetMatrix());
      }
    }

    // Render PBR Meshes
    auto meshView = registry.view<TransformComponent, MeshRendererComponent>();
    for (auto entity : meshView) {
      auto [transform, meshComp] =
          meshView.get<TransformComponent, MeshRendererComponent>(entity);
      if (meshComp.model) {
        if (meshComp.materialOverride) {
          meshComp.model->SetMaterialAll(meshComp.materialOverride);
        }
        renderer.Submit(meshComp.model, transform.GetMatrix());
      }
    }

    // Submit Directional Lights
    auto dirLightView =
        registry.view<TransformComponent, DirectionalLightComponent>();
    for (auto entity : dirLightView) {
      auto [transform, lightComp] =
          dirLightView.get<TransformComponent, DirectionalLightComponent>(
              entity);
      DirectionalLight lightInfo;
      lightInfo.color = lightComp.color;
      lightInfo.intensity = lightComp.intensity;

      glm::vec3 pos = transform.Position;
      float len = glm::length(pos);
      if (len > 0.001f) {
        lightInfo.direction = glm::normalize(-pos);
      } else {
        lightInfo.direction = glm::normalize(lightComp.direction);
      }
      renderer.SubmitDirectionalLight(lightInfo);
    }

    // Submit Point Lights
    auto pointLightView =
        registry.view<TransformComponent, PointLightComponent>();
    for (auto entity : pointLightView) {
      auto [transform, lightComp] =
          pointLightView.get<TransformComponent, PointLightComponent>(entity);
      PointLightData lightInfo;
      lightInfo.position = transform.Position;
      lightInfo.color = lightComp.color;
      lightInfo.intensity = lightComp.intensity;
      lightInfo.radius = lightComp.radius;
      renderer.SubmitPointLight(lightInfo);
    }
  }
};

#endif
