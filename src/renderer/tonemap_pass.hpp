#ifndef TONEMAP_PASS_HPP
#define TONEMAP_PASS_HPP

#include "../rhi/rhi_device.h"
#include "../rhi/shader_preprocessor.hpp"
#include "shader.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

/**
 * @brief Final-pass HDR -> LDR tonemap (ACES Narkowicz) + gamma correction.
 *
 * Runs AFTER the scene + bloom composite have populated sceneFB. Samples
 * sceneFB color and writes the tonemapped/gamma-corrected result into a
 * caller-supplied output framebuffer. OpenGL only (gl_VertexID fullscreen
 * tri, same pattern as bloom/grid).
 */
class TonemapPass {
public:
  TonemapPass() = default;
  ~TonemapPass() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    if (dev->GetAPI() != RHI::API::OpenGL) {
      std::cerr << "[TonemapPass] Skipping: API is not OpenGL" << std::endl;
      return false;
    }

    device = dev;

    float dummy[3] = {0.0f, 0.0f, 0.0f};
    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = sizeof(dummy);
    vbDesc.data = dummy;
    dummyVertexBuffer = device->CreateBuffer(vbDesc);
    if (!RHI::IsValid(dummyVertexBuffer)) {
      std::cerr << "[TonemapPass] Failed to create dummy VBO" << std::endl;
      return false;
    }

    RHI::VertexLayout layout;
    layout.stride = sizeof(float);
    layout.attributes = {};

    vao = device->CreateVertexArray(dummyVertexBuffer, RHI::BufferHandle{0},
                                    layout);
    if (!RHI::IsValid(vao)) {
      std::cerr << "[TonemapPass] Failed to create VAO" << std::endl;
      return false;
    }

    shader = std::make_unique<Shader>(device);
    std::string shaderPath = shaderBasePath + "/tonemap_opengl.shader";
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
      std::cerr << "[TonemapPass] Failed to open: " << shaderPath << std::endl;
      return false;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();
    file.close();

    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);
    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[TonemapPass] Failed to preprocess shader" << std::endl;
      return false;
    }
    if (!shader->CompileFromSource(stages.vertex.c_str(),
                                   stages.fragment.c_str())) {
      std::cerr << "[TonemapPass] Failed to compile shader" << std::endl;
      return false;
    }

    RHI::PipelineDescriptor desc;
    desc.topology = RHI::PrimitiveTopology::TriangleList;
    desc.depthStencil.depthTestEnable = false;
    desc.depthStencil.depthWriteEnable = false;
    desc.rasterizer.cullMode = RHI::CullMode::None;
    desc.blend.blendEnable = false;
    pipeline = device->CreatePipeline(desc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[TonemapPass] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  // Reads sceneColor (HDR), applies ACES + gamma, writes to targetFB.
  // Caller must NOT be inside an active render pass on targetFB.
  void Execute(RHI::ICommandList *cmdList, RHI::TextureHandle sceneColor,
               RHI::FramebufferHandle targetFB, uint32_t width,
               uint32_t height) {
    if (!initialized || !cmdList || width == 0 || height == 0)
      return;
    if (!RHI::IsValid(sceneColor) || !RHI::IsValid(targetFB))
      return;

    RHI::RenderPassDescriptor pass;
    pass.framebuffer = targetFB;
    pass.clearColorBuffer = true;
    pass.clearDepthBuffer = false;
    pass.clearStencilBuffer = false;
    cmdList->BeginRenderPass(pass);
    cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(width),
                          static_cast<int>(height), 0.0f, 1.0f});
    cmdList->SetScissor({0, 0, width, height});
    cmdList->BindPipeline(pipeline);
    cmdList->BindVertexArray(vao);
    cmdList->BindTexture(0, sceneColor);

    shader->BindCommandList(cmdList);
    shader->SetInt("u_source", 0);
    shader->SetFloat("u_exposure", exposure);
    shader->SetFloat("u_gamma", gamma);

    RHI::DrawCommand d;
    d.vertexCount = 3;
    d.instanceCount = 1;
    cmdList->Draw(d);
    cmdList->EndRenderPass();
    shader->BindCommandList(nullptr);
  }

  void Shutdown() {
    if (!device)
      return;
    if (RHI::IsValid(pipeline)) {
      device->DestroyPipeline(pipeline);
      pipeline = RHI::PipelineHandle{0};
    }
    if (RHI::IsValid(vao)) {
      device->DestroyVertexArray(vao);
      vao = RHI::VertexArrayHandle{0};
    }
    if (RHI::IsValid(dummyVertexBuffer)) {
      device->DestroyBuffer(dummyVertexBuffer);
      dummyVertexBuffer = RHI::BufferHandle{0};
    }
    shader.reset();
    initialized = false;
  }

  bool IsInitialized() const { return initialized; }

  float GetExposure() const { return exposure; }
  void SetExposure(float v) { exposure = v; }
  float GetGamma() const { return gamma; }
  void SetGamma(float v) { gamma = v; }

  TonemapPass(const TonemapPass &) = delete;
  TonemapPass &operator=(const TonemapPass &) = delete;

private:
  RHI::IDevice *device = nullptr;
  RHI::BufferHandle dummyVertexBuffer;
  RHI::VertexArrayHandle vao;
  RHI::PipelineHandle pipeline;
  std::unique_ptr<Shader> shader;
  bool initialized = false;

  float exposure = 1.0f;
  float gamma = 2.2f;
};

#endif // TONEMAP_PASS_HPP
