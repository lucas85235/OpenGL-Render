#ifndef HDR_TO_CUBEMAP_HPP
#define HDR_TO_CUBEMAP_HPP

#include "../rhi/rhi_device.h"
#include "shader.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <memory>

/**
 * @brief Converts an equirectangular HDR texture to a cubemap via
 * render-to-texture
 *
 * Uses a cube geometry and samples the 2D equirectangular map using spherical
 * coordinates to output each cubemap face.
 */
class HdrToCubemapConverter {
private:
  RHI::IDevice *device = nullptr;

  RHI::BufferHandle cubeVB;
  RHI::VertexArrayHandle cubeVAO;
  RHI::PipelineHandle pipeline;
  RHI::SamplerHandle sampler;
  std::unique_ptr<Shader> shader;

  bool initialized = false;

  static constexpr float cubeVertices[] = {
      // Back face
      -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f,
      // Front face
      -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
      1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f,
      // Left face
      -1.0f, 1.0f, 1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,
      -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f,
      // Right face
      1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f, 1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
      // Bottom face
      -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
      1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f, -1.0f,
      // Top face
      -1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, -1.0f, 1.0f, 1.0f, -1.0f,
      -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f};

  glm::mat4 captureProjection;
  glm::mat4 captureViews[6];

  void SetupCaptureMatrices() {
    captureProjection =
        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    captureViews[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, -1.0f, 0.0f));
    captureViews[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f),
                                  glm::vec3(0.0f, -1.0f, 0.0f));
    captureViews[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
                                  glm::vec3(0.0f, 0.0f, 1.0f));
    captureViews[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
                                  glm::vec3(0.0f, 0.0f, -1.0f));
    captureViews[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
                                  glm::vec3(0.0f, -1.0f, 0.0f));
    captureViews[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
                                  glm::vec3(0.0f, -1.0f, 0.0f));
  }

public:
  HdrToCubemapConverter() = default;

  ~HdrToCubemapConverter() { Shutdown(); }

  bool Initialize(RHI::IDevice *dev, const std::string &shaderPath) {
    if (initialized)
      return true;
    if (!dev)
      return false;

    device = dev;
    std::cout << "[HdrToCubemap] Initializing converter..." << std::endl;

    SetupCaptureMatrices();

    // Create cube vertex buffer
    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = sizeof(cubeVertices);
    vbDesc.data = cubeVertices;
    cubeVB = device->CreateBuffer(vbDesc);

    if (!RHI::IsValid(cubeVB)) {
      std::cerr << "[HdrToCubemap] Failed to create vertex buffer" << std::endl;
      return false;
    }

    RHI::VertexLayout layout;
    layout.stride = sizeof(float) * 3;
    layout.attributes = {{0, RHI::VertexAttributeType::Float3, 0, false}};

    cubeVAO = device->CreateVertexArray(cubeVB, RHI::BufferHandle{0}, layout);
    if (!RHI::IsValid(cubeVAO)) {
      std::cerr << "[HdrToCubemap] Failed to create VAO" << std::endl;
      return false;
    }

    // Create sampler for HDR texture
    RHI::SamplerDescriptor samplerDesc;
    samplerDesc.wrapS = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.wrapT = RHI::TextureWrapMode::ClampToEdge;
    samplerDesc.minFilter = RHI::TextureFilterMode::Linear;
    samplerDesc.magFilter = RHI::TextureFilterMode::Linear;
    sampler = device->CreateSampler(samplerDesc);

    // Load shader
    shader = std::make_unique<Shader>(device);
    if (!shader->CompileFromFile(shaderPath + "/equirect_to_cubemap.vert",
                                 shaderPath + "/equirect_to_cubemap.frag")) {
      std::cerr << "[HdrToCubemap] Failed to compile shaders" << std::endl;
      return false;
    }

    // Create pipeline
    RHI::PipelineDescriptor pipelineDesc;
    pipelineDesc.topology = RHI::PrimitiveTopology::TriangleList;
    pipelineDesc.depthStencil.depthTestEnable = false;
    pipelineDesc.depthStencil.depthWriteEnable = false;
    pipelineDesc.rasterizer.cullMode = RHI::CullMode::None;

    pipeline =
        device->CreatePipeline(pipelineDesc, shader->GetHandle(), layout);
    if (!RHI::IsValid(pipeline)) {
      std::cerr << "[HdrToCubemap] Failed to create pipeline" << std::endl;
      return false;
    }

    initialized = true;
    std::cout << "[HdrToCubemap] Converter initialized successfully"
              << std::endl;
    return true;
  }

  /**
   * @brief Converts an equirectangular HDR texture to a cubemap
   * @param hdrTexture The 2D HDR texture to convert
   * @param cubemap Pre-created cubemap texture to write to
   * @param faceSize Size of each cubemap face
   * @return true if successful
   */
  bool Convert(RHI::TextureHandle hdrTexture, RHI::TextureHandle cubemap,
               uint32_t faceSize) {
    if (!initialized || !device) {
      std::cerr << "[HdrToCubemap] Converter not initialized" << std::endl;
      return false;
    }

    if (!RHI::IsValid(hdrTexture) || !RHI::IsValid(cubemap)) {
      std::cerr << "[HdrToCubemap] Invalid texture handles" << std::endl;
      return false;
    }

    std::cout << "[HdrToCubemap] Converting HDR to cubemap (" << faceSize << "x"
              << faceSize << ")..." << std::endl;

    RHI::CommandListHandle cmdHandle = device->CreateCommandList();
    RHI::ICommandList *cmdList = device->GetCommandList(cmdHandle);
    if (!cmdList)
      return false;

    cmdList->Begin();
    std::vector<RHI::FramebufferHandle> tempFBs;

    for (int face = 0; face < 6; ++face) {
      RHI::FramebufferDescriptor fbDesc;
      fbDesc.width = faceSize;
      fbDesc.height = faceSize;
      fbDesc.hasDepth = false;

      RHI::TextureDescriptor faceTexDesc;
      faceTexDesc.type = RHI::TextureType::Texture2D;
      faceTexDesc.format = RHI::TextureFormat::RGBA16F;
      faceTexDesc.width = faceSize;
      faceTexDesc.height = faceSize;
      faceTexDesc.mipLevels = 1;

      RHI::TextureHandle faceTex = device->CreateTexture(faceTexDesc);
      if (!RHI::IsValid(faceTex)) {
        std::cerr << "[HdrToCubemap] Failed to create face texture " << face
                  << std::endl;
        continue;
      }

      RHI::RenderTargetAttachment colorAttachment;
      colorAttachment.texture = faceTex;
      colorAttachment.mipLevel = 0;
      fbDesc.colorAttachments.push_back(colorAttachment);

      RHI::FramebufferHandle fb = device->CreateFramebuffer(fbDesc);
      if (!RHI::IsValid(fb)) {
        device->DestroyTexture(faceTex);
        std::cerr << "[HdrToCubemap] Failed to create framebuffer for face "
                  << face << std::endl;
        continue;
      }
      tempFBs.push_back(fb);

      RHI::RenderPassDescriptor passDesc;
      passDesc.framebuffer = fb;
      passDesc.clearColorBuffer = true;
      passDesc.clearDepthBuffer = false;
      passDesc.clearStencilBuffer = false;
      passDesc.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

      cmdList->BeginRenderPass(passDesc);
      cmdList->SetViewport({0, 0, static_cast<int>(faceSize),
                            static_cast<int>(faceSize), 0.0f, 1.0f});

      cmdList->BindPipeline(pipeline);
      cmdList->BindVertexArray(cubeVAO);

      cmdList->BindTexture(0, hdrTexture);
      cmdList->BindSampler(0, sampler);

      shader->BindCommandList(cmdList);
      shader->SetInt("equirectangularMap", 0);
      shader->SetMat4("projection", glm::value_ptr(captureProjection));
      shader->SetMat4("view", glm::value_ptr(captureViews[face]));

      RHI::DrawCommand cmd;
      cmd.vertexCount = 36;
      cmd.instanceCount = 1;
      cmdList->Draw(cmd);

      shader->BindCommandList(nullptr);
      cmdList->EndRenderPass();

      std::cout << "[HdrToCubemap] Rendered face " << face << std::endl;
    }

    cmdList->End();
    device->SubmitCommandList(cmdHandle);
    device->DestroyCommandList(cmdHandle);

    for (auto fb : tempFBs) {
      device->DestroyFramebuffer(fb);
    }

    device->GenerateMipmaps(cubemap);

    std::cout << "[HdrToCubemap] Conversion complete" << std::endl;
    return true;
  }

  void Shutdown() {
    if (!device)
      return;

    if (RHI::IsValid(pipeline)) {
      device->DestroyPipeline(pipeline);
      pipeline = RHI::PipelineHandle{0};
    }
    if (RHI::IsValid(cubeVAO)) {
      device->DestroyVertexArray(cubeVAO);
      cubeVAO = RHI::VertexArrayHandle{0};
    }
    if (RHI::IsValid(cubeVB)) {
      device->DestroyBuffer(cubeVB);
      cubeVB = RHI::BufferHandle{0};
    }
    if (RHI::IsValid(sampler)) {
      device->DestroySampler(sampler);
      sampler = RHI::SamplerHandle{0};
    }

    shader.reset();
    initialized = false;
  }

  bool IsInitialized() const { return initialized; }
};

#endif // HDR_TO_CUBEMAP_HPP
