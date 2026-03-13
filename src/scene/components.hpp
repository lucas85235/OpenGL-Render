#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "../renderer/model.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>

// ==========================================
// 1. TRANSFORM (Dados espaciais)
// ==========================================
struct TransformComponent {
  glm::vec3 Position = glm::vec3(0.0f);
  glm::vec3 Rotation = glm::vec3(0.0f); // Euler angles
  glm::vec3 Scale = glm::vec3(1.0f);

  TransformComponent() = default;
  TransformComponent(const TransformComponent &) = default;
  TransformComponent(const glm::vec3 &translation) : Position(translation) {}

  glm::mat4 GetMatrix() const {
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1, 0, 0));
    model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0, 1, 0));
    model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0, 0, 1));
    model = glm::scale(model, Scale);
    return model;
  }
};

// ==========================================
// TAG (Nome da Entidade)
// ==========================================
struct TagComponent {
  std::string Tag;

  TagComponent() = default;
  TagComponent(const TagComponent &) = default;
  TagComponent(const std::string &tag) : Tag(tag) {}
};

// Componente para Renderizar Modelos 3D
struct MeshRendererComponent {
  std::shared_ptr<Model> model;
  std::shared_ptr<Material> materialOverride;

  MeshRendererComponent() = default;
  MeshRendererComponent(const MeshRendererComponent &) = default;
  MeshRendererComponent(std::shared_ptr<Model> m)
      : model(m), materialOverride(nullptr) {}
};

// Componente para Mesh Simples (Chão, etc)
struct SimpleMeshRendererComponent {
  std::shared_ptr<Mesh> mesh;

  SimpleMeshRendererComponent() = default;
  SimpleMeshRendererComponent(const SimpleMeshRendererComponent &) = default;
  SimpleMeshRendererComponent(std::shared_ptr<Mesh> m) : mesh(m) {}
};

// Componente de Script para Girar Objetos
struct RotatorScriptComponent {
  glm::vec3 rotationSpeed = glm::vec3(0.0f);

  RotatorScriptComponent() = default;
  RotatorScriptComponent(const RotatorScriptComponent &) = default;
  RotatorScriptComponent(glm::vec3 speed) : rotationSpeed(speed) {}
};

// Script para fazer o objeto flutuar (Senoide)
struct FloaterScriptComponent {
  float amplitude = 0.5f;
  float frequency = 1.0f;
  float startY = 0.0f;
  float time = 0.0f;
  bool initialized = false;

  FloaterScriptComponent() = default;
  FloaterScriptComponent(const FloaterScriptComponent &) = default;
  FloaterScriptComponent(float amp, float freq)
      : amplitude(amp), frequency(freq), startY(0), time(0),
        initialized(false) {}
};

struct DirectionalLightComponent {
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 1.0f;
  glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f); // Default direction

  DirectionalLightComponent() = default;
  DirectionalLightComponent(const DirectionalLightComponent &) = default;
  DirectionalLightComponent(glm::vec3 col, float intens = 1.0f)
      : color(col), intensity(intens) {}
};

// Componente de Luz Pontual (Lâmpada)
struct PointLightComponent {
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 10.0f;
  float radius = 10.0f;

  PointLightComponent() = default;
  PointLightComponent(const PointLightComponent &) = default;
  PointLightComponent(glm::vec3 col, float intens = 10.0f, float rad = 10.0f)
      : color(col), intensity(intens), radius(rad) {}
};

class ScriptableEntity; // Forward declaration

struct NativeScriptComponent {
  ScriptableEntity *Instance = nullptr;

  ScriptableEntity *(*InstantiateScript)() = nullptr;
  void (*DestroyScript)(NativeScriptComponent *) = nullptr;

  template <typename T> void Bind() {
    InstantiateScript = []() {
      return static_cast<ScriptableEntity *>(new T());
    };
    DestroyScript = [](NativeScriptComponent *self) {
      delete self->Instance;
      self->Instance = nullptr;
    };
  }
};

#endif
