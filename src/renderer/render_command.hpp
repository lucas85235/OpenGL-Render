#ifndef RENDER_COMMAND_HPP
#define RENDER_COMMAND_HPP

#include "material.hpp"
#include "mesh.hpp"
#include <glm/glm.hpp>
#include <memory>

struct RenderCommand {
  Mesh *mesh;          // Qual geometria?
  Material *material;  // Qual aparência?
  glm::mat4 transform; // Onde está no mundo?

  // Distância da câmera (para ordenação)
  float distanceToCamera;

  // Construtor auxiliar
  RenderCommand(Mesh *m, Material *mat, const glm::mat4 &trans,
                float dist = 0.0f)
      : mesh(m), material(mat), transform(trans), distanceToCamera(dist) {}
};

struct ParticleRenderCommand {
  int amount;
  int textureWidth;
  std::shared_ptr<Texture> mappingTexture;
  std::shared_ptr<Material> material;
  std::shared_ptr<Mesh> customMesh;
  glm::mat4 transform;

  glm::vec3 velocity;
  glm::vec3 baseColor;
  glm::vec2 size;
  float radius;
  float angle;
  bool center;
};

#endif // RENDER_COMMAND_HPP