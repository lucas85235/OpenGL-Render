#ifndef MESH_HPP
#define MESH_HPP

#include "../rhi/rhi_device.h"
#include "material.hpp"
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <memory>
#include <vector>


struct Vertex {
  glm::vec3 Position;
  glm::vec3 Normal;
  glm::vec2 TexCoords;
  glm::vec3 Tangent;
  glm::vec3 Bitangent;
};

class Mesh {
private:
  // RHI resources
  RHI::IDevice *device = nullptr;
  RHI::BufferHandle rhiVBO;
  RHI::BufferHandle rhiEBO;
  RHI::VertexArrayHandle rhiVAO;
  bool useRHI = false;

  // Legacy OpenGL resources
  unsigned int VAO = 0, VBO = 0, EBO = 0;

  std::shared_ptr<Material> material;

  void setupMeshRHI() {
    if (!device)
      return;

    // Create vertex buffer
    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = vertices.size() * sizeof(Vertex);
    vbDesc.data = vertices.data();
    rhiVBO = device->CreateBuffer(vbDesc);

    // Create index buffer
    RHI::BufferDescriptor ibDesc;
    ibDesc.type = RHI::BufferType::Index;
    ibDesc.usage = RHI::BufferUsage::Static;
    ibDesc.size = indices.size() * sizeof(unsigned int);
    ibDesc.data = indices.data();
    rhiEBO = device->CreateBuffer(ibDesc);

    // Create vertex layout matching Vertex struct
    RHI::VertexLayout layout;
    layout.stride = sizeof(Vertex);
    layout.attributes = {
        {0, RHI::VertexAttributeType::Float3, offsetof(Vertex, Position),
         false},
        {1, RHI::VertexAttributeType::Float3, offsetof(Vertex, Normal), false},
        {2, RHI::VertexAttributeType::Float2, offsetof(Vertex, TexCoords),
         false},
        {3, RHI::VertexAttributeType::Float3, offsetof(Vertex, Tangent), false},
        {4, RHI::VertexAttributeType::Float3, offsetof(Vertex, Bitangent),
         false}};

    rhiVAO = device->CreateVertexArray(rhiVBO, rhiEBO, layout);
    useRHI = true;
  }

  void setupMeshGL() {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 &indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void *)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, TexCoords));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Tangent));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void *)offsetof(Vertex, Bitangent));

    glBindVertexArray(0);
  }

public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  // RHI constructor
  Mesh(RHI::IDevice *rhiDevice, std::vector<Vertex> verts,
       std::vector<unsigned int> inds, std::shared_ptr<Material> mat = nullptr)
      : device(rhiDevice), vertices(std::move(verts)), indices(std::move(inds)),
        material(mat) {

    if (!material) {
      material = std::make_shared<Material>("Default");
    }

    if (device) {
      setupMeshRHI();
    } else {
      setupMeshGL();
    }
  }

  // Legacy OpenGL constructor
  Mesh(std::vector<Vertex> verts, std::vector<unsigned int> inds,
       std::shared_ptr<Material> mat = nullptr)
      : vertices(std::move(verts)), indices(std::move(inds)), material(mat) {

    if (!material) {
      material = std::make_shared<Material>("Default");
    }

    setupMeshGL();
  }

  ~Mesh() {
    if (useRHI && device) {
      if (RHI::IsValid(rhiVAO))
        device->DestroyVertexArray(rhiVAO);
      if (RHI::IsValid(rhiEBO))
        device->DestroyBuffer(rhiEBO);
      if (RHI::IsValid(rhiVBO))
        device->DestroyBuffer(rhiVBO);
    } else {
      if (VAO)
        glDeleteVertexArrays(1, &VAO);
      if (VBO)
        glDeleteBuffers(1, &VBO);
      if (EBO)
        glDeleteBuffers(1, &EBO);
    }
  }

  void Draw(unsigned int shaderProgram) {
    if (material) {
      material->Apply(shaderProgram);
    }

    if (useRHI && device) {
      device->BindVertexArray(rhiVAO);
      RHI::DrawIndexedCommand cmd;
      cmd.indexCount = indices.size();
      cmd.instanceCount = 1;
      cmd.indexType = RHI::IndexType::UInt32;
      device->DrawIndexed(cmd);
    } else {
      glBindVertexArray(VAO);
      glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
      glBindVertexArray(0);
    }

    glActiveTexture(GL_TEXTURE0);
  }

  // RHI-based draw (for use with Renderer that has IDevice)
  void Draw(RHI::IDevice *dev, RHI::ShaderHandle shader) {
    if (!dev)
      return;

    // Bind VAO and draw
    if (useRHI) {
      dev->BindVertexArray(rhiVAO);
    } else {
      glBindVertexArray(VAO);
    }

    RHI::DrawIndexedCommand cmd;
    cmd.indexCount = indices.size();
    cmd.instanceCount = 1;
    cmd.indexType = RHI::IndexType::UInt32;
    dev->DrawIndexed(cmd);
  }

  unsigned int GetVAO() const { return VAO; }
  RHI::VertexArrayHandle GetRHIVAO() const { return rhiVAO; }
  unsigned int GetIndexCount() const { return indices.size(); }
  bool IsUsingRHI() const { return useRHI; }

  void SetMaterial(std::shared_ptr<Material> mat) { material = mat; }

  std::shared_ptr<Material> GetMaterial() const { return material; }

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  Mesh(Mesh &&other) noexcept
      : device(other.device), rhiVBO(other.rhiVBO), rhiEBO(other.rhiEBO),
        rhiVAO(other.rhiVAO), useRHI(other.useRHI), VAO(other.VAO),
        VBO(other.VBO), EBO(other.EBO), vertices(std::move(other.vertices)),
        indices(std::move(other.indices)), material(std::move(other.material)) {
    other.device = nullptr;
    other.rhiVBO = RHI::BufferHandle{0};
    other.rhiEBO = RHI::BufferHandle{0};
    other.rhiVAO = RHI::VertexArrayHandle{0};
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.useRHI = false;
  }

  Mesh &operator=(Mesh &&other) noexcept {
    if (this != &other) {
      // Cleanup current resources
      if (useRHI && device) {
        if (RHI::IsValid(rhiVAO))
          device->DestroyVertexArray(rhiVAO);
        if (RHI::IsValid(rhiEBO))
          device->DestroyBuffer(rhiEBO);
        if (RHI::IsValid(rhiVBO))
          device->DestroyBuffer(rhiVBO);
      } else {
        if (VAO)
          glDeleteVertexArrays(1, &VAO);
        if (VBO)
          glDeleteBuffers(1, &VBO);
        if (EBO)
          glDeleteBuffers(1, &EBO);
      }

      device = other.device;
      rhiVBO = other.rhiVBO;
      rhiEBO = other.rhiEBO;
      rhiVAO = other.rhiVAO;
      useRHI = other.useRHI;
      VAO = other.VAO;
      VBO = other.VBO;
      EBO = other.EBO;
      vertices = std::move(other.vertices);
      indices = std::move(other.indices);
      material = std::move(other.material);

      other.device = nullptr;
      other.rhiVBO = RHI::BufferHandle{0};
      other.rhiEBO = RHI::BufferHandle{0};
      other.rhiVAO = RHI::VertexArrayHandle{0};
      other.VAO = 0;
      other.VBO = 0;
      other.EBO = 0;
      other.useRHI = false;
    }
    return *this;
  }
};

#endif // MESH_HPP