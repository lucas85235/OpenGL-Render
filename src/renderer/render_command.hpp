#ifndef RENDER_COMMAND_HPP
#define RENDER_COMMAND_HPP

#include <glm/glm.hpp>
#include <memory>
#include "mesh.hpp"
#include "material.hpp"

struct RenderCommand {
    Mesh* mesh;                 // Qual geometria?
    Material* material;         // Qual aparência?
    glm::mat4 transform;        // Onde está no mundo?
    
    // Distância da câmera (para ordenação)
    float distanceToCamera; 

    // Construtor auxiliar
    RenderCommand(Mesh* m, Material* mat, const glm::mat4& trans, float dist = 0.0f)
        : mesh(m), material(mat), transform(trans), distanceToCamera(dist) {}
};

#endif // RENDER_COMMAND_HPP