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

    void OnStart() override {
        startY = entity->transform.Position.y;
    }

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

    DirectionalLightComponent(glm::vec3 col = glm::vec3(1.0f), float intens = 1.0f) 
        : color(col), intensity(intens) {}

    void OnRender(Renderer& renderer) override {
        DirectionalLight light;
        light.color = color;
        light.intensity = intensity;
        
        // A direção vem da rotação da entidade!
        // Um vetor apontando pra frente (0,0,-1) rotacionado pela entidade
        // Mas simplificando: Vamos usar os Euler Angles da entidade como vetor de direção normalizado
        // Ou melhor: converter rotação (Quaternion/Euler) para vetor forward.
        
        // Hack rápido: Usar a posição como "direção" se a entidade estiver na origem, 
        // ou definir explicitamente. Vamos tentar extrair do Transform rotation.
        
        // Por simplicidade agora: vamos assumir que a "Rotation" do transform define o vetor direção
        // Mas converter Euler para Vetor é chato sem Quaternions. 
        // Vamos usar uma direção fixa baseada na rotação se você tiver essa lógica, 
        // ou usar um vetor 'Forward' calculado.
        
        // Vamos usar um vetor fixo rotacionado ou simplesmente passar um vetor customizado
        // Para este exemplo, vou assumir que a 'Position' da entidade define de onde o sol vem (apontando para 0,0,0)
        // Isso é comum em editores simples.
        light.direction = glm::normalize(-entity->transform.Position); 
        
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

    void OnRender(Renderer& renderer) override {
        PointLightData light;
        light.position = entity->transform.Position; // Pega posição da entidade
        light.color = color;
        light.intensity = intensity;
        light.radius = radius;

        renderer.SubmitPointLight(light);
    }
};

#endif
