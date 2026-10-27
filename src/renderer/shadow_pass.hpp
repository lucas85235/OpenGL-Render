#ifndef SHADOW_PASS_HPP
#define SHADOW_PASS_HPP

#include <cstdint>
#include <entt/entt.hpp>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <sstream>

#include "../rhi/rhi_device.h"
#include "../rhi/shader_preprocessor.hpp"
#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "mesh.hpp"
#include "model.hpp"
#include "shader.hpp"

/**
 * @brief Directional-light shadow map pass.
 *
 * Owns a 2048x2048 Depth32F texture + depth-only FBO, derives a light-space
 * matrix from the scene's directional light, and renders scene geometry into
 * the depth map using shadow_depth_opengl.shader.
 *
 * Opens its own render pass — caller must NOT be inside an active render pass
 * when invoking Execute(). OpenGL only for now.
 */
class ShadowPass {
public:
  static constexpr uint32_t kShadowMapSize = 2048u;

  ShadowPass() = default;
  ~ShadowPass() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    if (dev->GetAPI() != RHI::API::OpenGL) {
      std::cerr << "[ShadowPass] Skipping: API is not OpenGL" << std::endl;
      return false;
    }

    device = dev;

    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = RHI::TextureFormat::Depth32F;
    texDesc.width = kShadowMapSize;
    texDesc.height = kShadowMapSize;
    texDesc.mipLevels = 1;
    depthTexture = device->CreateTexture(texDesc);
    if (!RHI::IsValid(depthTexture)) {
      std::cerr << "[ShadowPass] Failed to create depth texture" << std::endl;
      return false;
    }

    RHI::FramebufferDescriptor fbDesc;
    fbDesc.width = kShadowMapSize;
    fbDesc.height = kShadowMapSize;
    fbDesc.hasDepth = true;
    RHI::RenderTargetAttachment depthAtt;
    depthAtt.texture = depthTexture;
    fbDesc.depthAttachment = depthAtt;
    // No color attachments — RHI will emit glDrawBuffer(GL_NONE).

    framebuffer = device->CreateFramebuffer(fbDesc);
    if (!RHI::IsValid(framebuffer)) {
      std::cerr << "[ShadowPass] Failed to create framebuffer" << std::endl;
      device->DestroyTexture(depthTexture);
      depthTexture = RHI::TextureHandle{0};
      return false;
    }

    shader = std::make_unique<Shader>(device);
    std::string shaderPath = shaderBasePath + "/shadow_depth_opengl.shader";
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
      std::cerr << "[ShadowPass] Failed to open: " << shaderPath << std::endl;
      return false;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();
    file.close();

    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);
    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[ShadowPass] Failed to preprocess shader" << std::endl;
      return false;
    }
    if (!shader->CompileFromSource(stages.vertex.c_str(),
                                   stages.fragment.c_str())) {
      std::cerr << "[ShadowPass] Failed to compile shader" << std::endl;
      return false;
    }

    // Layout matches Mesh VAO (5 attributes); shader only reads location 0.
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
    desc.depthStencil.depthTestEnable = true;
    desc.depthStencil.depthWriteEnable = true;
    desc.depthStencil.depthCompareOp = RHI::CompareOp::Less;
    // Front-face cull reduces self-shadow acne on closed meshes (back faces
    // cast; front faces don't). Fine for the default scene; could expose
    // later if floor/plane geometry needs a different choice.
    desc.rasterizer.cullMode = RHI::CullMode::Front;
    desc.rasterizer.frontFace = RHI::FrontFace::CounterClockwise;
    desc.blend.blendEnable = false;

    pipeline = device->CreatePipeline(desc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[ShadowPass] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  // Builds an orthographic light-space matrix around the world origin for the
  // given directional light direction. Tuned for the viewer's default scene
  // (floor at y=-1, scale 10); callers needing tighter fit can adjust bounds
  // after 5.3/5.4 wire sampling.
  void UpdateLightSpaceMatrix(const glm::vec3 &lightDirection) {
    glm::vec3 dir = glm::length(lightDirection) > 0.0f
                        ? glm::normalize(lightDirection)
                        : glm::vec3(0.0f, -1.0f, 0.0f);

    constexpr float kOrthoHalf = 20.0f;
    constexpr float kNear = 0.1f;
    constexpr float kFar = 80.0f;
    constexpr float kLightDistance = 30.0f;

    glm::vec3 eye = -dir * kLightDistance;
    glm::vec3 up = std::abs(dir.y) > 0.99f ? glm::vec3(0.0f, 0.0f, 1.0f)
                                           : glm::vec3(0.0f, 1.0f, 0.0f);
    glm::mat4 view = glm::lookAt(eye, glm::vec3(0.0f), up);
    glm::mat4 proj = glm::ortho(-kOrthoHalf, kOrthoHalf, -kOrthoHalf,
                                kOrthoHalf, kNear, kFar);
    lightSpaceMatrix = proj * view;
  }

  // Renders every entity with Transform + (MeshRenderer | SimpleMeshRenderer)
  // into the shadow map. Opens its own render pass — caller must NOT be
  // inside an active render pass.
  void Execute(RHI::ICommandList *cmdList, Scene *scene) {
    if (!initialized || !cmdList || !scene)
      return;

    RHI::RenderPassDescriptor pass;
    pass.framebuffer = framebuffer;
    pass.clearDepth = 1.0f;
    pass.clearColorBuffer = false;
    pass.clearDepthBuffer = true;
    pass.clearStencilBuffer = false;

    cmdList->BeginRenderPass(pass);
    cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(kShadowMapSize),
                          static_cast<int>(kShadowMapSize), 0.0f, 1.0f});
    cmdList->SetScissor({0, 0, kShadowMapSize, kShadowMapSize});
    cmdList->BindPipeline(pipeline);

    shader->BindCommandList(cmdList);
    shader->SetMat4("u_light_space", &lightSpaceMatrix[0][0]);

    auto &registry = scene->GetRegistry();

    auto drawMesh = [&](const Mesh &mesh, const glm::mat4 &model) {
      shader->SetMat4("u_model", &model[0][0]);
      cmdList->BindVertexArray(mesh.GetVAO());
      RHI::DrawIndexedCommand cmd;
      cmd.indexCount = mesh.GetIndexCount();
      cmd.instanceCount = 1;
      cmd.indexType = RHI::IndexType::UInt32;
      cmdList->DrawIndexed(cmd);
    };

    auto modelView = registry.view<TransformComponent, MeshRendererComponent>();
    for (auto entity : modelView) {
      auto &tr = modelView.get<TransformComponent>(entity);
      auto &mr = modelView.get<MeshRendererComponent>(entity);
      if (!mr.model)
        continue;
      glm::mat4 modelMat = tr.GetMatrix();
      for (size_t i = 0; i < mr.model->GetMeshCount(); ++i) {
        drawMesh(mr.model->GetMesh(i), modelMat);
      }
    }

    auto simpleView =
        registry.view<TransformComponent, SimpleMeshRendererComponent>();
    for (auto entity : simpleView) {
      auto &tr = simpleView.get<TransformComponent>(entity);
      auto &sr = simpleView.get<SimpleMeshRendererComponent>(entity);
      if (!sr.mesh)
        continue;
      glm::mat4 modelMat = tr.GetMatrix();
      drawMesh(*sr.mesh, modelMat);
    }

    shader->BindCommandList(nullptr);
    cmdList->EndRenderPass();
  }

  void Shutdown() {
    if (!device)
      return;
    if (RHI::IsValid(pipeline)) {
      device->DestroyPipeline(pipeline);
      pipeline = RHI::PipelineHandle{0};
    }
    if (RHI::IsValid(framebuffer)) {
      device->DestroyFramebuffer(framebuffer);
      framebuffer = RHI::FramebufferHandle{0};
    }
    if (RHI::IsValid(depthTexture)) {
      device->DestroyTexture(depthTexture);
      depthTexture = RHI::TextureHandle{0};
    }
    shader.reset();
    initialized = false;
  }

  bool IsInitialized() const { return initialized; }
  RHI::FramebufferHandle GetFramebuffer() const { return framebuffer; }
  RHI::TextureHandle GetDepthTexture() const { return depthTexture; }
  const glm::mat4 &GetLightSpaceMatrix() const { return lightSpaceMatrix; }
  uint32_t GetSize() const { return kShadowMapSize; }

  ShadowPass(const ShadowPass &) = delete;
  ShadowPass &operator=(const ShadowPass &) = delete;

private:
  RHI::IDevice *device = nullptr;
  RHI::FramebufferHandle framebuffer;
  RHI::TextureHandle depthTexture;
  RHI::PipelineHandle pipeline;
  std::unique_ptr<Shader> shader;
  glm::mat4 lightSpaceMatrix = glm::mat4(1.0f);
  bool initialized = false;
};

#endif // SHADOW_PASS_HPP
