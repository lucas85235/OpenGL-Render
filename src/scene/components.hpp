#ifndef COMPONENTS_HPP
#define COMPONENTS_HPP

#include "scene.hpp"
#include "../renderer/model.hpp"

// Componente para Renderizar Modelos 3D
class MeshRenderer : public Component {
private:
    std::shared_ptr<Model> model;
    std::shared_ptr<Material> materialOverride;

public:
    MeshRenderer(std::shared_ptr<Model> m) : model(m), materialOverride(nullptr) {}

    void SetMaterial(std::shared_ptr<Material> mat) {
        materialOverride = mat;
    }

    void OnRender(Renderer& renderer) override {
        if (model) {
            // Se tiver override de material, aplicamos (lógica que você pode aprimorar no Renderer)
            // Por enquanto, vamos assumir que o Renderer usa o material do Model 
            // ou aplicamos manualmente aqui se tiver acesso
            
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
    
    void SetMaterial(std::shared_ptr<Material> mat) {
        mesh->SetMaterial(mat);
    }

    void OnRender(Renderer& renderer) override {
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

#endif
