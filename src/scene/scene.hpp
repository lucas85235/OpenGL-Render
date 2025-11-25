#ifndef SCENE_SYSTEM_HPP
#define SCENE_SYSTEM_HPP

#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "../renderer/renderer.hpp" // Para os componentes de render saberem o que é renderer

// Forward declarations
class Entity;
class Scene;

// ==========================================
// 1. COMPONENT (Base)
// ==========================================
class Component {
protected:
    Entity* entity; // Referência ao dono

public:
    virtual ~Component() {}
    
    void SetEntity(Entity* e) { entity = e; }
    Entity* GetEntity() { return entity; }

    // Ciclo de vida
    virtual void OnStart() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender(Renderer& renderer) {}
};

// ==========================================
// 2. TRANSFORM (Dados espaciais)
// ==========================================
struct Transform {
    glm::vec3 Position = glm::vec3(0.0f);
    glm::vec3 Rotation = glm::vec3(0.0f); // Euler angles
    glm::vec3 Scale = glm::vec3(1.0f);

    glm::mat4 GetMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, Position);
        model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1,0,0));
        model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0,1,0));
        model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0,0,1));
        model = glm::scale(model, Scale);
        return model;
    }
};

// ==========================================
// 3. ENTITY (O Objeto)
// ==========================================
class Entity {
private:
    std::vector<std::shared_ptr<Component>> components;
    std::string name;
    bool active;

public:
    Transform transform; // Todo objeto tem transform por padrão

    Entity(const std::string& name) : name(name), active(true) {}

    void Start() {
        for(auto& c : components) c->OnStart();
    }

    void Update(float dt) {
        if (!active) return;
        for(auto& c : components) c->OnUpdate(dt);
    }

    void Render(Renderer& renderer) {
        if (!active) return;
        for(auto& c : components) c->OnRender(renderer);
    }

    // Sistema de Componentes (Estilo Unity)
    template <typename T, typename... Args>
    std::shared_ptr<T> AddComponent(Args&&... args) {
        // Cria o componente
        auto component = std::make_shared<T>(std::forward<Args>(args)...);
        component->SetEntity(this);
        components.push_back(component);
        
        // Se a cena já começou, poderíamos chamar OnStart() aqui
        return component;
    }

    template <typename T>
    std::shared_ptr<T> GetComponent() {
        for (auto& component : components) {
            if (std::dynamic_pointer_cast<T>(component))
                return std::dynamic_pointer_cast<T>(component);
        }
        return nullptr;
    }

    std::string GetName() const { return name; }
};

// ==========================================
// 4. SCENE (Gerenciador)
// ==========================================
class Scene {
private:
    std::vector<std::shared_ptr<Entity>> entities;

public:
    std::shared_ptr<Entity> CreateEntity(const std::string& name = "Entity") {
        auto entity = std::make_shared<Entity>(name);
        entities.push_back(entity);
        return entity;
    }

    void OnStart() {
        for(auto& e : entities) e->Start();
    }

    void OnUpdate(float dt) {
        for(auto& e : entities) e->Update(dt);
    }

    void OnRender(Renderer& renderer) {
        for(auto& e : entities) e->Render(renderer);
    }
};

#endif