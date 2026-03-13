#ifndef SCENE_SYSTEM_HPP
#define SCENE_SYSTEM_HPP

#include <entt/entt.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "../renderer/renderer.hpp"
#include "components.hpp"
#include "particle_system.hpp"

class Entity;

// ==========================================
// SCENE (Gerenciador)
// ==========================================
class Scene {
private:
  entt::registry registry;
  std::string name = "Untitled";

  friend class Entity;
  // TODO serializers might need friend access or views

public:
  Scene() = default;
  ~Scene() = default;

  void SetName(const std::string &n) { name = n; }
  std::string GetName() const { return name; }

  Entity CreateEntity(const std::string &name = "Entity");
  void DestroyEntity(Entity entity);

  // We likely don't need FindEntity by name often in ECS, but keep it for
  // compatibility Returns empty entity if not found
  Entity FindEntity(const std::string &name);

  void Clear() { registry.clear(); }

  size_t GetEntityCount() const {
    return registry.storage<entt::entity>()->size();
  }

  void OnStart() {
    // Initialization could happen here if necessary for components like Floater
  }

  void OnUpdate(float dt);

  void OnRender(Renderer &renderer);

  entt::registry &GetRegistry() { return registry; }
};

// ==========================================
// ENTITY (O Objeto Wrapper)
// ==========================================
class Entity {
private:
  entt::entity entityHandle{entt::null};
  Scene *scene = nullptr;

public:
  Entity() = default;
  Entity(entt::entity handle, Scene *scene)
      : entityHandle(handle), scene(scene) {}

  template <typename T, typename... Args> T &AddComponent(Args &&...args) {
    if (HasComponent<T>()) {
      std::cerr << "[Warning] Entity already has component!\n";
    }
    return scene->registry.emplace<T>(entityHandle,
                                      std::forward<Args>(args)...);
  }

  template <typename T> T &GetComponent() {
    return scene->registry.get<T>(entityHandle);
  }

  template <typename T> bool HasComponent() const {
    return scene->registry.any_of<T>(entityHandle);
  }

  template <typename T> void RemoveComponent() {
    scene->registry.erase<T>(entityHandle);
  }

  operator bool() const {
    return entityHandle != entt::null && scene != nullptr;
  }
  operator entt::entity() const { return entityHandle; }
  bool operator==(const Entity &other) const {
    return entityHandle == other.entityHandle && scene == other.scene;
  }
  bool operator!=(const Entity &other) const { return !(*this == other); }

  std::string GetName() { return GetComponent<TagComponent>().Tag; }

  TransformComponent &GetTransform() {
    return GetComponent<TransformComponent>();
  }
};

// Scene Implementations
inline Entity Scene::CreateEntity(const std::string &name) {
  Entity entity = {registry.create(), this};
  entity.AddComponent<TransformComponent>();
  entity.AddComponent<TagComponent>(name.empty() ? "Entity" : name);
  return entity;
}

inline void Scene::DestroyEntity(Entity entity) { registry.destroy(entity); }

inline Entity Scene::FindEntity(const std::string &name) {
  auto view = registry.view<TagComponent>();
  for (auto entity : view) {
    const auto &tag = view.get<TagComponent>(entity);
    if (tag.Tag == name) {
      return Entity{entity, this};
    }
  }
  return Entity{};
}

inline void Scene::OnUpdate(float dt) {
  // Update Rotator Scripts
  auto rotView = registry.view<TransformComponent, RotatorScriptComponent>();
  for (auto entity : rotView) {
    auto [transform, rotator] =
        rotView.get<TransformComponent, RotatorScriptComponent>(entity);
    transform.Rotation += rotator.rotationSpeed * dt;
  }

  // Update Floater Scripts
  auto floatView = registry.view<TransformComponent, FloaterScriptComponent>();
  for (auto entity : floatView) {
    auto [transform, floater] =
        floatView.get<TransformComponent, FloaterScriptComponent>(entity);
    if (!floater.initialized) {
      floater.startY = transform.Position.y;
      floater.initialized = true;
    }
    floater.time += dt;
    transform.Position.y =
        floater.startY +
        std::sin(floater.time * floater.frequency) * floater.amplitude;
  }
}

inline void Scene::OnRender(Renderer &renderer) {
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
        dirLightView.get<TransformComponent, DirectionalLightComponent>(entity);
    DirectionalLight lightInfo;
    lightInfo.color = lightComp.color;
    lightInfo.intensity = lightComp.intensity;

    // Use entity position as direction if valid, otherwise use default
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

  // Submit Particle Systems
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

#endif