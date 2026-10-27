#ifndef BLOOM_PASS_HPP
#define BLOOM_PASS_HPP

#include "../rhi/rhi_device.h"
#include "../rhi/shader_preprocessor.hpp"
#include "shader.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

/**
 * @brief HDR bloom pass — 5-level mip chain with 13-tap downsample and
 *        3x3 tent upsample, additively composited back into the scene FBO.
 *
 * Runs AFTER the scene render pass has ended — opens its own render passes
 * on each mip FBO and a final pass on the scene FBO. Caller must NOT be
 * inside an active render pass when invoking Execute(). OpenGL only for now
 * (matches PostProcess/Grid/Shadow convention).
 */
class BloomPass {
public:
  static constexpr uint32_t kMipCount = 5;

  BloomPass() = default;
  ~BloomPass() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    if (dev->GetAPI() != RHI::API::OpenGL) {
      std::cerr << "[BloomPass] Skipping: API is not OpenGL" << std::endl;
      return false;
    }

    device = dev;

    // Dummy VBO — fullscreen triangle pulls vertex data from gl_VertexID,
    // but RHI::CreateVertexArray requires a valid buffer handle.
    float dummy[3] = {0.0f, 0.0f, 0.0f};
    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = sizeof(dummy);
    vbDesc.data = dummy;
    dummyVertexBuffer = device->CreateBuffer(vbDesc);
    if (!RHI::IsValid(dummyVertexBuffer)) {
      std::cerr << "[BloomPass] Failed to create dummy VBO" << std::endl;
      return false;
    }

    RHI::VertexLayout layout;
    layout.stride = sizeof(float);
    layout.attributes = {};

    vao = device->CreateVertexArray(dummyVertexBuffer, RHI::BufferHandle{0},
                                    layout);
    if (!RHI::IsValid(vao)) {
      std::cerr << "[BloomPass] Failed to create VAO" << std::endl;
      return false;
    }

    shader = std::make_unique<Shader>(device);
    std::string shaderPath = shaderBasePath + "/bloom_opengl.shader";
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
      std::cerr << "[BloomPass] Failed to open: " << shaderPath << std::endl;
      return false;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();
    file.close();

    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);
    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[BloomPass] Failed to preprocess shader" << std::endl;
      return false;
    }
    if (!shader->CompileFromSource(stages.vertex.c_str(),
                                   stages.fragment.c_str())) {
      std::cerr << "[BloomPass] Failed to compile shader" << std::endl;
      return false;
    }

    // Non-blended pipeline — prefilter + downsample write-replace.
    RHI::PipelineDescriptor writeDesc;
    writeDesc.topology = RHI::PrimitiveTopology::TriangleList;
    writeDesc.depthStencil.depthTestEnable = false;
    writeDesc.depthStencil.depthWriteEnable = false;
    writeDesc.rasterizer.cullMode = RHI::CullMode::None;
    writeDesc.blend.blendEnable = false;
    pipelineWrite =
        device->CreatePipeline(writeDesc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipelineWrite)) {
      std::cerr << "[BloomPass] Failed to create write pipeline" << std::endl;
      return false;
    }

    // Additive pipeline — upsample blends smaller level onto next-bigger,
    // and final composite adds bloom into scene color.
    RHI::PipelineDescriptor addDesc = writeDesc;
    addDesc.blend.blendEnable = true;
    addDesc.blend.srcColorFactor = RHI::BlendFactor::One;
    addDesc.blend.dstColorFactor = RHI::BlendFactor::One;
    addDesc.blend.colorOp = RHI::BlendOp::Add;
    addDesc.blend.srcAlphaFactor = RHI::BlendFactor::One;
    addDesc.blend.dstAlphaFactor = RHI::BlendFactor::One;
    addDesc.blend.alphaOp = RHI::BlendOp::Add;
    pipelineAdd = device->CreatePipeline(addDesc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipelineAdd)) {
      std::cerr << "[BloomPass] Failed to create additive pipeline"
                << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  // Executes the full bloom chain: prefilter + downsample + upsample, then
  // additively composites mip[0] into the scene framebuffer. Caller must have
  // already ended the scene render pass on cmdList before calling this.
  void Execute(RHI::ICommandList *cmdList, RHI::TextureHandle sceneColor,
               RHI::FramebufferHandle sceneFB, uint32_t sceneWidth,
               uint32_t sceneHeight) {
    if (!initialized || !cmdList || sceneWidth == 0 || sceneHeight == 0)
      return;
    if (!RHI::IsValid(sceneColor) || !RHI::IsValid(sceneFB))
      return;

    EnsureChain(sceneWidth, sceneHeight);
    if (mips[0].width == 0)
      return;

    shader->BindCommandList(cmdList);
    shader->SetFloat("u_intensity", 1.0f);
    shader->SetFloat("u_filterRadius", filterRadius);

    // ---- Prefilter: sceneColor -> mip[0]
    {
      RHI::RenderPassDescriptor pass;
      pass.framebuffer = mips[0].fbo;
      pass.clearColorBuffer = true;
      pass.clearDepthBuffer = false;
      pass.clearStencilBuffer = false;
      cmdList->BeginRenderPass(pass);
      cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(mips[0].width),
                            static_cast<int>(mips[0].height), 0.0f, 1.0f});
      cmdList->SetScissor({0, 0, mips[0].width, mips[0].height});
      cmdList->BindPipeline(pipelineWrite);
      cmdList->BindVertexArray(vao);
      cmdList->BindTexture(0, sceneColor);

      shader->SetInt("u_mode", 0);
      shader->SetInt("u_source", 0);
      shader->SetFloat("u_threshold", threshold);
      shader->SetFloat("u_knee", knee);
      shader->SetVec2("u_texelSize", 1.0f / static_cast<float>(sceneWidth),
                      1.0f / static_cast<float>(sceneHeight));

      RHI::DrawCommand d;
      d.vertexCount = 3;
      d.instanceCount = 1;
      cmdList->Draw(d);
      cmdList->EndRenderPass();
    }

    // ---- Downsample chain: mip[i-1] -> mip[i]
    for (uint32_t i = 1; i < kMipCount; ++i) {
      RHI::RenderPassDescriptor pass;
      pass.framebuffer = mips[i].fbo;
      pass.clearColorBuffer = true;
      pass.clearDepthBuffer = false;
      pass.clearStencilBuffer = false;
      cmdList->BeginRenderPass(pass);
      cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(mips[i].width),
                            static_cast<int>(mips[i].height), 0.0f, 1.0f});
      cmdList->SetScissor({0, 0, mips[i].width, mips[i].height});
      cmdList->BindPipeline(pipelineWrite);
      cmdList->BindVertexArray(vao);
      cmdList->BindTexture(0, mips[i - 1].color);

      shader->SetInt("u_mode", 1);
      shader->SetInt("u_source", 0);
      shader->SetVec2("u_texelSize",
                      1.0f / static_cast<float>(mips[i - 1].width),
                      1.0f / static_cast<float>(mips[i - 1].height));

      RHI::DrawCommand d;
      d.vertexCount = 3;
      d.instanceCount = 1;
      cmdList->Draw(d);
      cmdList->EndRenderPass();
    }

    // ---- Upsample chain: mip[i+1] -> mip[i] with additive blend onto the
    //      already-downsampled mip[i] contents.
    for (int i = static_cast<int>(kMipCount) - 2; i >= 0; --i) {
      RHI::RenderPassDescriptor pass;
      pass.framebuffer = mips[i].fbo;
      pass.clearColorBuffer = false;
      pass.clearDepthBuffer = false;
      pass.clearStencilBuffer = false;
      cmdList->BeginRenderPass(pass);
      cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(mips[i].width),
                            static_cast<int>(mips[i].height), 0.0f, 1.0f});
      cmdList->SetScissor({0, 0, mips[i].width, mips[i].height});
      cmdList->BindPipeline(pipelineAdd);
      cmdList->BindVertexArray(vao);
      cmdList->BindTexture(0, mips[i + 1].color);

      shader->SetInt("u_mode", 2);
      shader->SetInt("u_source", 0);
      shader->SetFloat("u_intensity", 1.0f);
      shader->SetFloat("u_filterRadius", filterRadius);
      shader->SetVec2("u_texelSize",
                      1.0f / static_cast<float>(mips[i + 1].width),
                      1.0f / static_cast<float>(mips[i + 1].height));

      RHI::DrawCommand d;
      d.vertexCount = 3;
      d.instanceCount = 1;
      cmdList->Draw(d);
      cmdList->EndRenderPass();
    }

    // ---- Composite: additively blend mip[0] into scene framebuffer.
    {
      RHI::RenderPassDescriptor pass;
      pass.framebuffer = sceneFB;
      pass.clearColorBuffer = false;
      pass.clearDepthBuffer = false;
      pass.clearStencilBuffer = false;
      cmdList->BeginRenderPass(pass);
      cmdList->SetViewport({0.0f, 0.0f, static_cast<int>(sceneWidth),
                            static_cast<int>(sceneHeight), 0.0f, 1.0f});
      cmdList->SetScissor({0, 0, sceneWidth, sceneHeight});
      cmdList->BindPipeline(pipelineAdd);
      cmdList->BindVertexArray(vao);
      cmdList->BindTexture(0, mips[0].color);

      shader->SetInt("u_mode", 2);
      shader->SetInt("u_source", 0);
      shader->SetFloat("u_intensity", intensity);
      shader->SetFloat("u_filterRadius", filterRadius);
      shader->SetVec2("u_texelSize", 1.0f / static_cast<float>(mips[0].width),
                      1.0f / static_cast<float>(mips[0].height));

      RHI::DrawCommand d;
      d.vertexCount = 3;
      d.instanceCount = 1;
      cmdList->Draw(d);
      cmdList->EndRenderPass();
    }

    shader->BindCommandList(nullptr);
  }

  void Shutdown() {
    if (!device)
      return;
    DestroyChain();
    if (RHI::IsValid(pipelineAdd)) {
      device->DestroyPipeline(pipelineAdd);
      pipelineAdd = RHI::PipelineHandle{0};
    }
    if (RHI::IsValid(pipelineWrite)) {
      device->DestroyPipeline(pipelineWrite);
      pipelineWrite = RHI::PipelineHandle{0};
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

  float GetThreshold() const { return threshold; }
  void SetThreshold(float v) { threshold = v; }
  float GetKnee() const { return knee; }
  void SetKnee(float v) { knee = v; }
  float GetIntensity() const { return intensity; }
  void SetIntensity(float v) { intensity = v; }
  float GetFilterRadius() const { return filterRadius; }
  void SetFilterRadius(float v) { filterRadius = v; }

  BloomPass(const BloomPass &) = delete;
  BloomPass &operator=(const BloomPass &) = delete;

private:
  struct MipTarget {
    RHI::TextureHandle color;
    RHI::FramebufferHandle fbo;
    uint32_t width = 0;
    uint32_t height = 0;
  };

  void EnsureChain(uint32_t sceneWidth, uint32_t sceneHeight) {
    // mip[0] is half-res of scene; each subsequent level halves again.
    uint32_t targetW0 = std::max<uint32_t>(1, sceneWidth / 2);
    uint32_t targetH0 = std::max<uint32_t>(1, sceneHeight / 2);
    if (mips[0].width == targetW0 && mips[0].height == targetH0 &&
        RHI::IsValid(mips[0].fbo))
      return;

    DestroyChain();

    uint32_t w = targetW0;
    uint32_t h = targetH0;
    for (uint32_t i = 0; i < kMipCount; ++i) {
      RHI::TextureDescriptor texDesc;
      texDesc.type = RHI::TextureType::Texture2D;
      texDesc.format = RHI::TextureFormat::RGBA16F;
      texDesc.width = w;
      texDesc.height = h;
      texDesc.mipLevels = 1;
      mips[i].color = device->CreateTexture(texDesc);

      RHI::FramebufferDescriptor fbDesc;
      fbDesc.width = w;
      fbDesc.height = h;
      RHI::RenderTargetAttachment att;
      att.texture = mips[i].color;
      fbDesc.colorAttachments.push_back(att);
      fbDesc.hasDepth = false;
      mips[i].fbo = device->CreateFramebuffer(fbDesc);
      mips[i].width = w;
      mips[i].height = h;

      w = std::max<uint32_t>(1, w / 2);
      h = std::max<uint32_t>(1, h / 2);
    }
  }

  void DestroyChain() {
    if (!device)
      return;
    for (auto &m : mips) {
      if (RHI::IsValid(m.fbo)) {
        device->DestroyFramebuffer(m.fbo);
        m.fbo = RHI::FramebufferHandle{0};
      }
      if (RHI::IsValid(m.color)) {
        device->DestroyTexture(m.color);
        m.color = RHI::TextureHandle{0};
      }
      m.width = 0;
      m.height = 0;
    }
  }

  RHI::IDevice *device = nullptr;
  RHI::BufferHandle dummyVertexBuffer;
  RHI::VertexArrayHandle vao;
  RHI::PipelineHandle pipelineWrite;
  RHI::PipelineHandle pipelineAdd;
  std::unique_ptr<Shader> shader;
  std::array<MipTarget, kMipCount> mips;
  bool initialized = false;

  float threshold = 1.0f;
  float knee = 0.5f;
  float intensity = 0.5f;
  float filterRadius = 1.0f;
};

#endif // BLOOM_PASS_HPP
