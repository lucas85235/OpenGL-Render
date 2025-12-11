#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "../renderer/model.hpp"
#include "scene.hpp"

// Componente para Renderizar Modelos 3D
class MeshRenderer : public Component {
private:
  std::shared_ptr<Model> model;
  std::shared_ptr<Material> materialOverride;

public:
  MeshRenderer(std::shared_ptr<Model> m)
      : model(m), materialOverride(nullptr) {}

  void SetMaterial(std::shared_ptr<Material> mat) { materialOverride = mat; }

  void OnRender(Renderer &renderer) override {
    if (model) {
      // Se tiver override de material, aplicamos (lógica que você pode
      // aprimorar no Renderer) Por enquanto, vamos assumir que o Renderer usa o
      // material do Model ou aplicamos manualmente aqui se tiver acesso

      if (materialOverride) {
        model->SetMaterialAll(materialOverride);
      }

      renderer.Submit(model, entity->transform.GetMatrix());
    }
  }
};

// Componente para Mesh Simples (Chão, etc)
class SimpleMeshRenderer : public Component {
private:
  std::shared_ptr<Mesh> mesh; // Cópia ou ptr

public:
  SimpleMeshRenderer(std::shared_ptr<Mesh> m) : mesh(m) {}

  void SetMaterial(std::shared_ptr<Material> mat) { mesh->SetMaterial(mat); }

  void OnRender(Renderer &renderer) override {
    renderer.SubmitMesh(*mesh, entity->transform.GetMatrix());
  }
};

// Componente de Script para Girar Objetos
class RotatorScript : public Component {
private:
  glm::vec3 rotationSpeed;

public:
  RotatorScript(glm::vec3 speed) : rotationSpeed(speed) {}

  void OnUpdate(float dt) override {
    entity->transform.Rotation += rotationSpeed * dt;
  }
};

// Script para fazer o objeto flutuar (Senoide)
class FloaterScript : public Component {
private:
  float amplitude;
  float frequency;
  float startY;
  float time;

public:
  FloaterScript(float amp = 0.5f, float freq = 1.0f)
      : amplitude(amp), frequency(freq), startY(0), time(0) {}

  void OnStart() override { startY = entity->transform.Position.y; }

  void OnUpdate(float dt) override {
    time += dt;
    float newY = startY + std::sin(time * frequency) * amplitude;
    entity->transform.Position.y = newY;
  }
};

class DirectionalLightComponent : public Component {
public:
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 1.0f;
  glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f); // Default direction

  DirectionalLightComponent(glm::vec3 col = glm::vec3(1.0f),
                            float intens = 1.0f)
      : color(col), intensity(intens) {}

  void OnRender(Renderer &renderer) override {
    DirectionalLight light;
    light.color = color;
    light.intensity = intensity;

    // Use entity position as direction if valid, otherwise use default
    glm::vec3 pos = entity->transform.Position;
    float len = glm::length(pos);
    if (len > 0.001f) {
      light.direction = glm::normalize(-pos);
    } else {
      light.direction = glm::normalize(direction);
    }

    renderer.SubmitDirectionalLight(light);
  }
};

// Componente de Luz Pontual (Lâmpada)
class PointLightComponent : public Component {
public:
  glm::vec3 color;
  float intensity;
  float radius;

  PointLightComponent(glm::vec3 col, float intens = 10.0f, float rad = 10.0f)
      : color(col), intensity(intens), radius(rad) {}

  void OnRender(Renderer &renderer) override {
    PointLightData light;
    light.position = entity->transform.Position; // Pega posição da entidade
    light.color = color;
    light.intensity = intensity;
    light.radius = radius;

    renderer.SubmitPointLight(light);
  }
};

#endif
