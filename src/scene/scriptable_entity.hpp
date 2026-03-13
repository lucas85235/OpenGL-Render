#ifndef SCRIPTABLE_ENTITY_HPP
#define SCRIPTABLE_ENTITY_HPP

#include <entt/entt.hpp>

class Scene;

// Base class for all user native scripts
class ScriptableEntity {
protected:
  entt::entity entityHandle{entt::null};
  Scene *scene = nullptr;

  friend class Scene;

public:
  virtual ~ScriptableEntity() {}

  template <typename T> T &GetComponent() {
    return scene->GetRegistry().template get<T>(entityHandle);
  }

  template <typename T> bool HasComponent() const {
    return scene->GetRegistry().template any_of<T>(entityHandle);
  }

  virtual void OnCreate() {}
  virtual void OnDestroy() {}
  virtual void OnUpdate(float dt) {}
};

#endif
