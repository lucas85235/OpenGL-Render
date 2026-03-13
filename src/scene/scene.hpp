#ifndef SCENE_SYSTEM_HPP
#define SCENE_SYSTEM_HPP

#include <entt/entt.hpp>
#include <iostream>
#include <memory>
#include <string>

#include "../renderer/renderer.hpp"
#include "components.hpp"
#include "particle_system.hpp"
#include "system.hpp"

class Entity;

// ==========================================
// SCENE (Gerenciador)
// ==========================================
class Scene {
private:
  entt::registry registry;
  std::string name = "Untitled";
  std::vector<std::shared_ptr<System>> systems;

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

  void AddSystem(std::shared_ptr<System> system) { systems.push_back(system); }

  void OnStart() {
    for (auto &system : systems) {
      system->OnStart(this);
    }
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
  // Update Native Scripts first
  registry.view<NativeScriptComponent>().each([=](auto entityID, auto &nsc) {
    if (!nsc.Instance) {
      nsc.Instance = nsc.InstantiateScript();
      nsc.Instance->entityHandle = entityID;
      nsc.Instance->scene = this;
      nsc.Instance->OnCreate();
    }
    nsc.Instance->OnUpdate(dt);
  });

  // Call assigned Systems
  for (auto &system : systems) {
    system->OnUpdate(this, dt);
  }
}

inline void Scene::OnRender(Renderer &renderer) {
  for (auto &system : systems) {
    system->OnRender(this, renderer);
  }
}

#include "scriptable_entity.hpp"

#endif