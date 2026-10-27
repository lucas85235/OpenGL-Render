#ifndef OUTLINE_PASS_HPP
#define OUTLINE_PASS_HPP

#include "../rhi/rhi_device.h"
#include "../rhi/shader_preprocessor.hpp"
#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "shader.hpp"
#include <entt/entt.hpp>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <sstream>

/**
 * @brief Selected-entity outline via inflated shell.
 *
 * Vertex shader pushes each position along its normal by u_outline_width,
 * rendered with cull=Front and depth write OFF. The shell's back faces
 * only pass the depth test where they extend past the original mesh
 * silhouette against the background, producing a ring-shaped outline.
 *
 * Runs inside the main scene render pass (shares color + depth with PBR).
 * Caller sets selected entity before Execute; null entity = no-op.
 * OpenGL only (plain uniforms, no UBO wiring yet).
 */
class OutlinePass {
public:
  OutlinePass() = default;
  ~OutlinePass() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath) {
    if (initialized)
      return true;
    if (!dev)
      return false;
    if (dev->GetAPI() != RHI::API::OpenGL) {
      std::cerr << "[OutlinePass] Skipping: API is not OpenGL" << std::endl;
      return false;
    }

    device = dev;

    shader = std::make_unique<Shader>(device);
    std::string shaderPath = shaderBasePath + "/outline_opengl.shader";
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
      std::cerr << "[OutlinePass] Failed to open: " << shaderPath << std::endl;
      return false;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();
    file.close();

    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);
    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[OutlinePass] Failed to preprocess shader" << std::endl;
      return false;
    }
    if (!shader->CompileFromSource(stages.vertex.c_str(),
                                   stages.fragment.c_str())) {
      std::cerr << "[OutlinePass] Failed to compile shader" << std::endl;
      return false;
    }

    // Layout matches Mesh VAO (5 attributes); shader reads locations 0/1.
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

    RHI::PipelineDescriptor desc;
    desc.topology = RHI::PrimitiveTopology::TriangleList;
    // Depth test ON so the mesh's own front faces occlude the inflated
    // shell's back faces where they overlap — keeps the inside clean.
    // Depth write OFF so the outline doesn't overwrite mesh depth for
    // subsequent passes (grid, etc).
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = false;
    desc.depthStencil.depthCompareOp = RHI::CompareOp::LessOrEqual;
    // Cull FRONT: only the back faces of the inflated shell survive.
    desc.rasterizer.cullMode = RHI::CullMode::Front;
    desc.rasterizer.frontFace = RHI::FrontFace::CounterClockwise;
    desc.blend.blendEnable = true;
    desc.blend.srcColorFactor = RHI::BlendFactor::SrcAlpha;
    desc.blend.dstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha;
    desc.blend.colorOp = RHI::BlendOp::Add;
    desc.blend.srcAlphaFactor = RHI::BlendFactor::One;
    desc.blend.dstAlphaFactor = RHI::BlendFactor::OneMinusSrcAlpha;
    desc.blend.alphaOp = RHI::BlendOp::Add;

    pipeline = device->CreatePipeline(desc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[OutlinePass] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  void Execute(RHI::ICommandList *cmdList, Scene *scene, entt::entity selected,
               const glm::mat4 &view, const glm::mat4 &projection) {
    if (!initialized || !cmdList || !scene)
      return;
    if (selected == entt::null)
      return;

    auto &registry = scene->GetRegistry();
    if (!registry.valid(selected))
      return;
    if (!registry.any_of<TransformComponent>(selected))
      return;

    bool hasModel = registry.any_of<MeshRendererComponent>(selected);
    bool hasSimple = registry.any_of<SimpleMeshRendererComponent>(selected);
    if (!hasModel && !hasSimple)
      return;

    cmdList->BindPipeline(pipeline);
    shader->BindCommandList(cmdList);
    shader->SetMat4("u_view", &view[0][0]);
    shader->SetMat4("u_projection", &projection[0][0]);
    shader->SetFloat("u_outline_width", outlineWidth);
    float rgba[4] = {outlineColor.r, outlineColor.g, outlineColor.b,
                     outlineColor.a};
    cmdList->SetUniform(shader->GetHandle(), "u_outline_color", rgba, 4);

    glm::mat4 modelMat = registry.get<TransformComponent>(selected).GetMatrix();
    shader->SetMat4("u_model", &modelMat[0][0]);

    auto drawMesh = [&](const Mesh &mesh) {
      cmdList->BindVertexArray(mesh.GetVAO());
      RHI::DrawIndexedCommand cmd;
      cmd.indexCount = mesh.GetIndexCount();
      cmd.instanceCount = 1;
      cmd.indexType = RHI::IndexType::UInt32;
      cmdList->DrawIndexed(cmd);
    };

    if (hasModel) {
      auto &mr = registry.get<MeshRendererComponent>(selected);
      if (mr.model) {
        for (size_t i = 0; i < mr.model->GetMeshCount(); ++i) {
          drawMesh(mr.model->GetMesh(i));
        }
      }
    }
    if (hasSimple) {
      auto &sr = registry.get<SimpleMeshRendererComponent>(selected);
      if (sr.mesh)
        drawMesh(*sr.mesh);
    }
  }

  void Shutdown() {
    if (!device)
      return;
    if (RHI::IsValid(pipeline)) {
      device->DestroyPipeline(pipeline);
      pipeline = RHI::PipelineHandle{0};
    }
    shader.reset();
    initialized = false;
  }

  bool IsInitialized() const { return initialized; }
  void SetOutlineColor(const glm::vec4 &c) { outlineColor = c; }
  void SetOutlineWidth(float w) { outlineWidth = w; }
  float *GetOutlineWidthPtr() { return &outlineWidth; }
  glm::vec4 *GetOutlineColorPtr() { return &outlineColor; }

  OutlinePass(const OutlinePass &) = delete;
  OutlinePass &operator=(const OutlinePass &) = delete;

private:
  RHI::IDevice *device = nullptr;
  RHI::PipelineHandle pipeline;
  std::unique_ptr<Shader> shader;
  float outlineWidth = 0.04f;
  glm::vec4 outlineColor = glm::vec4(1.0f, 0.55f, 0.1f, 1.0f);
  bool initialized = false;
};

#endif // OUTLINE_PASS_HPP
