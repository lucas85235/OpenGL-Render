#ifndef PICKING_PASS_HPP
#define PICKING_PASS_HPP

#include <GL/glew.h>
#include <cstdint>
#include <entt/entt.hpp>
#include <fstream>
#include <glm/glm.hpp>
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
 * @brief Offscreen pass that renders each entity with a flat color derived
 * from its entt::entity id. Target is RGBA8; reading a pixel lets a click
 * handler recover the entity under the cursor.
 *
 * Owns its own framebuffer + depth so it can run independently of the main
 * scene render pass. OpenGL only for now (non-block uniforms).
 *
 * Task 3.1 scope: pass infrastructure + id encoding. Click capture / pixel
 * readback lands in task 3.2.
 */
class PickingPass {
public:
  PickingPass() = default;
  ~PickingPass() { Shutdown(); }

  static constexpr uint32_t kInvalidEntityId = 0xFFFFFFFFu;

  // uint32 id -> RGBA8 vec4 in [0,1]. Reserve 0xFFFFFFFF for "no hit"
  // (matches the cleared framebuffer if we clear to white, but we clear
  // to 0 so id=0 is the sentinel instead — see Execute()).
  static glm::vec4 EncodeEntityId(uint32_t id) {
    return glm::vec4(
        static_cast<float>((id >> 0) & 0xFFu) / 255.0f,
        static_cast<float>((id >> 8) & 0xFFu) / 255.0f,
        static_cast<float>((id >> 16) & 0xFFu) / 255.0f,
        static_cast<float>((id >> 24) & 0xFFu) / 255.0f);
  }

  static uint32_t DecodeEntityId(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (static_cast<uint32_t>(r) << 0) |
           (static_cast<uint32_t>(g) << 8) |
           (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(a) << 24);
  }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath,
                  uint32_t width, uint32_t height) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    if (dev->GetAPI() != RHI::API::OpenGL) {
      std::cerr << "[PickingPass] Skipping: API is not OpenGL" << std::endl;
      return false;
    }

    device = dev;
    fbWidth = width;
    fbHeight = height;

    if (!CreateFramebuffer(fbWidth, fbHeight))
      return false;

    shader = std::make_unique<Shader>(device);
    std::string shaderPath = shaderBasePath + "/picking_opengl.shader";
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
      std::cerr << "[PickingPass] Failed to open: " << shaderPath << std::endl;
      return false;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();
    file.close();

    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);
    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[PickingPass] Failed to preprocess shader" << std::endl;
      return false;
    }
    if (!shader->CompileFromSource(stages.vertex.c_str(),
                                   stages.fragment.c_str())) {
      std::cerr << "[PickingPass] Failed to compile shader" << std::endl;
      return false;
    }

    // Pipeline layout must match Mesh VAO (5 attributes); shader only reads
    // location 0 but GL allows unused attributes bound on the VAO.
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
    desc.rasterizer.cullMode = RHI::CullMode::Back;
    desc.rasterizer.frontFace = RHI::FrontFace::CounterClockwise;
    desc.blend.blendEnable = false;

    pipeline = device->CreatePipeline(desc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[PickingPass] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  void Resize(uint32_t width, uint32_t height) {
    if (!initialized || width == 0 || height == 0)
      return;
    if (width == fbWidth && height == fbHeight)
      return;
    fbWidth = width;
    fbHeight = height;
    device->ResizeFramebuffer(framebuffer, fbWidth, fbHeight);
    colorTexture = device->GetFramebufferTexture(
        framebuffer, RHI::FramebufferAttachment::Color0);
  }

  // Opens its own render pass on the picking FBO and draws every entity that
  // has Transform + (MeshRenderer | SimpleMeshRenderer). Caller must NOT be
  // inside an active render pass when invoking this.
  void Execute(RHI::ICommandList *cmdList, Scene *scene, const glm::mat4 &view,
               const glm::mat4 &projection) {
    if (!initialized || !cmdList || !scene)
      return;

    RHI::RenderPassDescriptor pass;
    pass.framebuffer = framebuffer;
    // Clear to (0,0,0,0) so unrendered pixels decode to id 0. Real entities
    // should avoid relying on id 0 being pickable; callers can treat 0 as
    // "no hit" if the first created entity is guaranteed non-pickable, or
    // shift ids by +1 when encoding.
    pass.clearColor = {0.0f, 0.0f, 0.0f, 0.0f};
    pass.clearDepth = 1.0f;
    pass.clearColorBuffer = true;
    pass.clearDepthBuffer = true;
    pass.clearStencilBuffer = false;

    cmdList->BeginRenderPass(pass);
    cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(fbWidth),
                          static_cast<int>(fbHeight), 0.0f, 1.0f});
    cmdList->SetScissor({0, 0, fbWidth, fbHeight});
    cmdList->BindPipeline(pipeline);

    shader->BindCommandList(cmdList);
    shader->SetMat4("u_view", &view[0][0]);
    shader->SetMat4("u_projection", &projection[0][0]);

    auto &registry = scene->GetRegistry();

    auto drawMesh = [&](const Mesh &mesh, const glm::mat4 &model,
                        uint32_t entityId) {
      glm::vec4 color = EncodeEntityId(entityId);
      shader->SetMat4("u_model", &model[0][0]);
      float rgba[4] = {color.r, color.g, color.b, color.a};
      cmdList->SetUniform(shader->GetHandle(), "u_entity_color", rgba, 4);
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
      uint32_t id = static_cast<uint32_t>(entt::to_integral(entity));
      for (size_t i = 0; i < mr.model->GetMeshCount(); ++i) {
        drawMesh(mr.model->GetMesh(i), modelMat, id);
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
      uint32_t id = static_cast<uint32_t>(entt::to_integral(entity));
      drawMesh(*sr.mesh, modelMat, id);
    }

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
    if (RHI::IsValid(colorTexture)) {
      device->DestroyTexture(colorTexture);
      colorTexture = RHI::TextureHandle{0};
    }

    shader.reset();
    initialized = false;
  }

  bool IsInitialized() const { return initialized; }
  RHI::FramebufferHandle GetFramebuffer() const { return framebuffer; }
  RHI::TextureHandle GetColorTexture() const { return colorTexture; }
  uint32_t GetWidth() const { return fbWidth; }
  uint32_t GetHeight() const { return fbHeight; }

  // Reads a single RGBA8 pixel from the picking framebuffer and decodes the
  // entity id. Expects framebuffer-space coords (OpenGL origin: bottom-left).
  // Returns kInvalidEntityId if not initialized, out of range, or the pixel
  // was the clear value (0).
  uint32_t ReadPixelId(int x, int y) const {
    if (!initialized || !device)
      return kInvalidEntityId;
    if (x < 0 || y < 0 || x >= static_cast<int>(fbWidth) ||
        y >= static_cast<int>(fbHeight))
      return kInvalidEntityId;

    GLuint fbo =
        static_cast<GLuint>(device->GetNativeFramebufferID(framebuffer));
    if (fbo == 0)
      return kInvalidEntityId;

    GLint prevRead = 0;
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &prevRead);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);

    GLint prevReadBuffer = 0;
    glGetIntegerv(GL_READ_BUFFER, &prevReadBuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    GLint prevPackAlign = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &prevPackAlign);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    uint8_t rgba[4] = {0, 0, 0, 0};
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    glPixelStorei(GL_PACK_ALIGNMENT, prevPackAlign);
    glReadBuffer(static_cast<GLenum>(prevReadBuffer));
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(prevRead));

    uint32_t id = DecodeEntityId(rgba[0], rgba[1], rgba[2], rgba[3]);
    if (id == 0)
      return kInvalidEntityId;
    return id;
  }

  PickingPass(const PickingPass &) = delete;
  PickingPass &operator=(const PickingPass &) = delete;

private:
  bool CreateFramebuffer(uint32_t width, uint32_t height) {
    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = RHI::TextureFormat::RGBA8;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.mipLevels = 1;
    colorTexture = device->CreateTexture(texDesc);
    if (!RHI::IsValid(colorTexture)) {
      std::cerr << "[PickingPass] Failed to create color texture" << std::endl;
      return false;
    }

    RHI::FramebufferDescriptor fbDesc;
    fbDesc.width = width;
    fbDesc.height = height;
    RHI::RenderTargetAttachment colorAtt;
    colorAtt.texture = colorTexture;
    fbDesc.colorAttachments.push_back(colorAtt);
    fbDesc.hasDepth = true; // device auto-creates depth RBO

    framebuffer = device->CreateFramebuffer(fbDesc);
    if (!RHI::IsValid(framebuffer)) {
      std::cerr << "[PickingPass] Failed to create framebuffer" << std::endl;
      return false;
    }
    return true;
  }

  RHI::IDevice *device = nullptr;
  RHI::FramebufferHandle framebuffer;
  RHI::TextureHandle colorTexture;
  RHI::PipelineHandle pipeline;
  std::unique_ptr<Shader> shader;
  uint32_t fbWidth = 0;
  uint32_t fbHeight = 0;
  bool initialized = false;
};

#endif // PICKING_PASS_HPP
