#ifndef MESH_HPP
#define MESH_HPP

#include "../rhi/rhi_device.h"
#include "material.hpp"
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
  RHI::IDevice *device = nullptr;
  RHI::BufferHandle vbo;
  RHI::BufferHandle ebo;
  RHI::VertexArrayHandle vao;
  std::shared_ptr<Material> material;

  void setupMesh() {
    if (!device)
      return;

    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = vertices.size() * sizeof(Vertex);
    vbDesc.data = vertices.data();
    vbo = device->CreateBuffer(vbDesc);

    RHI::BufferDescriptor ibDesc;
    ibDesc.type = RHI::BufferType::Index;
    ibDesc.usage = RHI::BufferUsage::Static;
    ibDesc.size = indices.size() * sizeof(unsigned int);
    ibDesc.data = indices.data();
    ebo = device->CreateBuffer(ibDesc);

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

    vao = device->CreateVertexArray(vbo, ebo, layout);
  }

public:
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  Mesh(RHI::IDevice *rhiDevice, std::vector<Vertex> verts,
       std::vector<unsigned int> inds, std::shared_ptr<Material> mat = nullptr)
      : device(rhiDevice), vertices(std::move(verts)), indices(std::move(inds)),
        material(mat) {
    if (!material) {
      material = std::make_shared<Material>("Default");
    }
    setupMesh();
  }

  ~Mesh() {
    if (device) {
      if (RHI::IsValid(vao))
        device->DestroyVertexArray(vao);
      if (RHI::IsValid(ebo))
        device->DestroyBuffer(ebo);
      if (RHI::IsValid(vbo))
        device->DestroyBuffer(vbo);
    }
  }

  void Draw(RHI::ICommandList *cmdList, RHI::ShaderHandle shader) {
    if (!cmdList)
      return;

    if (material) {
      material->Apply(cmdList, shader);
    }

    cmdList->BindVertexArray(vao);
    RHI::DrawIndexedCommand cmd;
    cmd.indexCount = static_cast<uint32_t>(indices.size());
    cmd.instanceCount = 1;
    cmd.indexType = RHI::IndexType::UInt32;
    cmdList->DrawIndexed(cmd);
  }

  RHI::VertexArrayHandle GetVAO() const { return vao; }
  uint32_t GetIndexCount() const {
    return static_cast<uint32_t>(indices.size());
  }
  void SetMaterial(std::shared_ptr<Material> mat) { material = mat; }
  std::shared_ptr<Material> GetMaterial() const { return material; }
  RHI::IDevice *GetDevice() const { return device; }

  Mesh(const Mesh &) = delete;
  Mesh &operator=(const Mesh &) = delete;

  Mesh(Mesh &&other) noexcept
      : device(other.device), vbo(other.vbo), ebo(other.ebo), vao(other.vao),
        vertices(std::move(other.vertices)), indices(std::move(other.indices)),
        material(std::move(other.material)) {
    other.device = nullptr;
    other.vbo = RHI::BufferHandle{0};
    other.ebo = RHI::BufferHandle{0};
    other.vao = RHI::VertexArrayHandle{0};
  }

  Mesh &operator=(Mesh &&other) noexcept {
    if (this != &other) {
      if (device) {
        if (RHI::IsValid(vao))
          device->DestroyVertexArray(vao);
        if (RHI::IsValid(ebo))
          device->DestroyBuffer(ebo);
        if (RHI::IsValid(vbo))
          device->DestroyBuffer(vbo);
      }

      device = other.device;
      vbo = other.vbo;
      ebo = other.ebo;
      vao = other.vao;
      vertices = std::move(other.vertices);
      indices = std::move(other.indices);
      material = std::move(other.material);

      other.device = nullptr;
      other.vbo = RHI::BufferHandle{0};
      other.ebo = RHI::BufferHandle{0};
      other.vao = RHI::VertexArrayHandle{0};
    }
    return *this;
  }
};

#endif // MESH_HPP