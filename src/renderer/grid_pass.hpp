#ifndef GRID_PASS_HPP
#define GRID_PASS_HPP

#include "../rhi/rhi_device.h"
#include "../rhi/shader_preprocessor.hpp"
#include "shader.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>
#include <sstream>

/**
 * @brief Infinite XZ ground-plane grid overlay.
 *
 * Fullscreen triangle (gl_VertexID, no VBO data). Fragment unprojects ray,
 * intersects y=0 plane, computes line coverage at two scales with axis tint
 * and distance fade. Writes gl_FragDepth so occlusion works with opaque
 * geometry. Render AFTER opaque pass so alpha blend uses final color.
 * OpenGL only for now (uses gl_VertexID-driven fullscreen triangle).
 */
class GridPass {
private:
  RHI::IDevice *device = nullptr;

  RHI::BufferHandle dummyVertexBuffer;
  RHI::VertexArrayHandle vao;
  RHI::PipelineHandle pipeline;

  std::unique_ptr<Shader> shader;
  bool initialized = false;

public:
  GridPass() = default;

  ~GridPass() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    // Grid shader is OpenGL-only (gl_VertexID-driven fullscreen tri).
    if (dev->GetAPI() != RHI::API::OpenGL) {
      std::cerr << "[GridPass] Skipping: API is not OpenGL" << std::endl;
      return false;
    }

    device = dev;

    // Dummy VBO: RHI CreateVertexArray requires a valid vertex buffer, but
    // the shader pulls vertex data from gl_VertexID, so content is irrelevant.
    float dummy[3] = {0.0f, 0.0f, 0.0f};
    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = sizeof(dummy);
    vbDesc.data = dummy;
    dummyVertexBuffer = device->CreateBuffer(vbDesc);
    if (!RHI::IsValid(dummyVertexBuffer)) {
      std::cerr << "[GridPass] Failed to create dummy VBO" << std::endl;
      return false;
    }

    RHI::VertexLayout layout;
    layout.stride = sizeof(float);
    layout.attributes = {}; // no attributes — gl_VertexID is enough

    vao = device->CreateVertexArray(dummyVertexBuffer, RHI::BufferHandle{0},
                                    layout);
    if (!RHI::IsValid(vao)) {
      std::cerr << "[GridPass] Failed to create VAO" << std::endl;
      return false;
    }

    shader = std::make_unique<Shader>(device);
    std::string shaderPath = shaderBasePath + "/grid_opengl.shader";
    std::ifstream file(shaderPath);
    if (!file.is_open()) {
      std::cerr << "[GridPass] Failed to open: " << shaderPath << std::endl;
      return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);

    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[GridPass] Failed to preprocess shader" << std::endl;
      return false;
    }

    if (!shader->CompileFromSource(stages.vertex.c_str(),
                                   stages.fragment.c_str())) {
      std::cerr << "[GridPass] Failed to compile shader" << std::endl;
      return false;
    }

    RHI::PipelineDescriptor pipelineDesc;
    pipelineDesc.topology = RHI::PrimitiveTopology::TriangleList;

    // Depth: test ON, write ON (we emit gl_FragDepth for correct occlusion).
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = RHI::CompareOp::LessOrEqual;

    // No culling — fullscreen triangle.
    pipelineDesc.rasterizer.cullMode = RHI::CullMode::None;

    // Alpha blend so lines fade into the background / opaque scene.
    pipelineDesc.blend.blendEnable = true;
    pipelineDesc.blend.srcColorFactor = RHI::BlendFactor::SrcAlpha;
    pipelineDesc.blend.dstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha;
    pipelineDesc.blend.colorOp = RHI::BlendOp::Add;
    pipelineDesc.blend.srcAlphaFactor = RHI::BlendFactor::One;
    pipelineDesc.blend.dstAlphaFactor = RHI::BlendFactor::OneMinusSrcAlpha;
    pipelineDesc.blend.alphaOp = RHI::BlendOp::Add;

    pipeline =
        device->CreatePipeline(pipelineDesc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[GridPass] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  void Execute(RHI::ICommandList *cmdList, const glm::mat4 &view,
               const glm::mat4 &projection) {
    if (!initialized || !cmdList)
      return;

    cmdList->BindPipeline(pipeline);
    cmdList->BindVertexArray(vao);

    shader->BindCommandList(cmdList);
    shader->SetMat4("u_view", &view[0][0]);
    shader->SetMat4("u_projection", &projection[0][0]);

    RHI::DrawCommand cmd;
    cmd.vertexCount = 3;
    cmd.instanceCount = 1;
    cmdList->Draw(cmd);
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

  GridPass(const GridPass &) = delete;
  GridPass &operator=(const GridPass &) = delete;
  GridPass(GridPass &&) noexcept = default;
  GridPass &operator=(GridPass &&) noexcept = default;
};

#endif // GRID_PASS_HPP
