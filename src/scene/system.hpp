#ifndef SCENE_SYSTEM_BASE_HPP
#define SCENE_SYSTEM_BASE_HPP

#include "../renderer/renderer.hpp"
#include <entt/entt.hpp>

// Forward declaration
class Scene;

class System {
public:
  virtual ~System() = default;

  // Called when the system is added to the scene
  virtual void OnStart(Scene *scene) {}

  // Called every frame to update logic
  virtual void OnUpdate(Scene *scene, float dt) {}

  // Called every frame to submit render commands
  virtual void OnRender(Scene *scene, Renderer &renderer) {}
};

#endif
