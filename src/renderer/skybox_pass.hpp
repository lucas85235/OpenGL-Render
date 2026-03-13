#ifndef SKYBOX_PASS_HPP
#define SKYBOX_PASS_HPP

#include "../rhi/rhi_device.h"
#include "../rhi/shader_preprocessor.hpp"
#include "shader.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>
#include <sstream>

/**
 * @brief RHI-based skybox rendering pass
 *
 * Renders a cubemap environment as an infinite background. Uses depth test
 * with LEQUAL comparison and disables depth write so it doesn't occlude
 * geometry. The view matrix translation is removed in the shader to keep the
 * skybox fixed.
 */
class SkyboxPass {
private:
  RHI::IDevice *device = nullptr;

  RHI::BufferHandle vertexBuffer;
  RHI::VertexArrayHandle vao;
  RHI::PipelineHandle pipeline;
  RHI::SamplerHandle sampler;
  RHI::TextureHandle environmentMap;

  std::unique_ptr<Shader> shader;
  bool initialized = false;

  static constexpr float cubeVertices[] = {
      // Back face (-Z)
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      // Front face (+Z)
      -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      // Left face (-X)
      -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
      1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      // Right face (+X)
      1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      // Bottom face (-Y)
      -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
      // Top face (+Y)
      -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f};

public:
  SkyboxPass() = default;

  ~SkyboxPass() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderBasePath) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    device = dev;

    // Create vertex buffer with cube geometry
    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = sizeof(cubeVertices);
    vbDesc.data = cubeVertices;
    vertexBuffer = device->CreateBuffer(vbDesc);

    if (!RHI::IsValid(vertexBuffer)) {
      std::cerr << "[SkyboxPass] Failed to create vertex buffer" << std::endl;
      return false;
    }

    // Create vertex layout (position only)
    RHI::VertexLayout layout;
    layout.stride = sizeof(float) * 3;
    layout.attributes = {{0, RHI::VertexAttributeType::Float3, 0, false}};

    vao = device->CreateVertexArray(vertexBuffer, RHI::BufferHandle{0}, layout);
    if (!RHI::IsValid(vao)) {
      std::cerr << "[SkyboxPass] Failed to create VAO" << std::endl;
      return false;
    }

    // Create sampler for cubemap
    RHI::SamplerDescriptor samplerDesc;
    samplerDesc.wrapS = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.wrapT = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.wrapR = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.minFilter = RHI::TextureFilterMode::Linear;
    samplerDesc.magFilter = RHI::TextureFilterMode::Linear;
    sampler = device->CreateSampler(samplerDesc);

    // Load shader
    shader = std::make_unique<Shader>(device);

    if (device->GetAPI() == RHI::API::Vulkan) {
      // Vulkan: Load pre-compiled SPIR-V
      if (!shader->CompileFromSPIRV(shaderBasePath + "/skybox.vert.spv",
                                    shaderBasePath + "/skybox.frag.spv")) {
        std::cerr << "[SkyboxPass] Failed to compile Vulkan shaders"
                  << std::endl;
        return false;
      }
    } else {
      // OpenGL: Runtime compile from .shader source
      std::string shaderPath = shaderBasePath + "/skybox.shader";
      std::ifstream file(shaderPath);
      if (!file.is_open()) {
        std::cerr << "[SkyboxPass] Failed to open: " << shaderPath << std::endl;
        return false;
      }
      std::stringstream buffer;
      buffer << file.rdbuf();
      std::string source = buffer.str();
      file.close();

      RHI::ShaderStageSource stages =
          RHI::ShaderPreprocessor::Process(source, RHI::API::OpenGL, 450);

      if (stages.vertex.empty() || stages.fragment.empty()) {
        std::cerr << "[SkyboxPass] Failed to preprocess shader" << std::endl;
        return false;
      }

      if (!shader->CompileFromSource(stages.vertex.c_str(),
                                     stages.fragment.c_str())) {
        std::cerr << "[SkyboxPass] Failed to compile OpenGL shaders"
                  << std::endl;
        return false;
      }
    }

    // Create pipeline with skybox-specific states
    RHI::PipelineDescriptor pipelineDesc;
    pipelineDesc.topology = RHI::PrimitiveTopology::TriangleList;

    // Depth: test enabled with LEQUAL, write disabled
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = false;
    pipelineDesc.depthStencil.depthCompareOp = RHI::CompareOp::LessOrEqual;

    // Cull front faces (we're inside the cube looking out)
    pipelineDesc.rasterizer.cullMode = RHI::CullMode::Front;
    pipelineDesc.rasterizer.frontFace = RHI::FrontFace::CounterClockwise;

    pipeline =
        device->CreatePipeline(pipelineDesc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[SkyboxPass] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    return true;
  }

  void SetEnvironmentMap(RHI::TextureHandle cubemap) {
    environmentMap = cubemap;
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
    if (RHI::IsValid(vertexBuffer)) {
      device->DestroyBuffer(vertexBuffer);
      vertexBuffer = RHI::BufferHandle{0};
    }
    if (RHI::IsValid(sampler)) {
      device->DestroySampler(sampler);
      sampler = RHI::SamplerHandle{0};
    }

    shader.reset();
    initialized = false;
    std::cout << "[SkyboxPass] Shutdown complete" << std::endl;
  }

  bool IsInitialized() const { return initialized; }

  // Prevent copy
  SkyboxPass(const SkyboxPass &) = delete;
  SkyboxPass &operator=(const SkyboxPass &) = delete;

  // Allow move
  SkyboxPass(SkyboxPass &&) noexcept = default;
  SkyboxPass &operator=(SkyboxPass &&) noexcept = default;
};

#endif // SKYBOX_PASS_HPP
