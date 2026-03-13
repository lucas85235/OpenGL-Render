#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>

#include "../rhi/rhi_device.h"
#include "lighting.hpp"
#include "model.hpp"
#include "render_command.hpp"
#include "render_context.hpp"
#include "shader.hpp"

struct SceneData {
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
  glm::vec3 cameraPos;
  glm::vec3 lightPos;
  glm::vec3 lightColor;
};

// Backward compatibility alias
using PointLightData = PointLight;

class Renderer {
private:
  std::vector<RenderCommand> opaqueQueue;
  std::vector<RenderCommand> transparentQueue;
  std::vector<ParticleRenderCommand> particleQueue;
  SceneData sceneData;
  Shader *activeShader = nullptr;
  Shader *activeParticleShader = nullptr;
  RHI::IDevice *device = nullptr;

  RHI::BufferHandle screenQuadVB;
  RHI::VertexArrayHandle screenQuadVAO;
  RHI::PipelineHandle mainPipeline;
  RHI::PipelineHandle particlePipeline;

  DirectionalLight sunLight;
  std::vector<PointLightData> pointLights;

  RHI::TextureHandle iblIrradiance;
  RHI::TextureHandle iblPrefilter;
  RHI::TextureHandle iblBrdf;
  bool useIBL = false;

  void initRenderData() {
    if (!device)
      return;

    struct QuadVertex {
      float x, y, u, v;
    };
    std::vector<QuadVertex> quadVertices = {
        {-1.0f, 1.0f, 0.0f, 1.0f}, {-1.0f, -1.0f, 0.0f, 0.0f},
        {1.0f, -1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f, 1.0f},
        {1.0f, -1.0f, 1.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f}};

    RHI::BufferDescriptor vbDesc;
    vbDesc.type = RHI::BufferType::Vertex;
    vbDesc.usage = RHI::BufferUsage::Static;
    vbDesc.size = quadVertices.size() * sizeof(QuadVertex);
    vbDesc.data = quadVertices.data();
    screenQuadVB = device->CreateBuffer(vbDesc);

    RHI::VertexLayout layout;
    layout.stride = sizeof(QuadVertex);
    layout.attributes = {
        {0, RHI::VertexAttributeType::Float2, offsetof(QuadVertex, x), false},
        {1, RHI::VertexAttributeType::Float2, offsetof(QuadVertex, u), false}};
    screenQuadVAO =
        device->CreateVertexArray(screenQuadVB, RHI::BufferHandle{0}, layout);
  }

public:
  Renderer() = default;

  ~Renderer() {
    if (device) {
      if (RHI::IsValid(mainPipeline))
        device->DestroyPipeline(mainPipeline);
      if (RHI::IsValid(particlePipeline))
        device->DestroyPipeline(particlePipeline);
      if (RHI::IsValid(screenQuadVAO))
        device->DestroyVertexArray(screenQuadVAO);
      if (RHI::IsValid(screenQuadVB))
        device->DestroyBuffer(screenQuadVB);
    }
  }

  void Init(RenderContext *context, Shader *defaultShader,
            Shader *particleShader = nullptr) {
    device = context->GetDevice();
    activeShader = defaultShader;
    activeParticleShader = particleShader;
    if (device) { // Create main pipeline for mesh rendering
      RHI::VertexLayout meshLayout;
      meshLayout.stride = sizeof(Vertex);
      meshLayout.attributes = {{0, RHI::VertexAttributeType::Float3,
                                offsetof(Vertex, Position), false},
                               {1, RHI::VertexAttributeType::Float3,
                                offsetof(Vertex, Normal), false},
                               {2, RHI::VertexAttributeType::Float2,
                                offsetof(Vertex, TexCoords), false}};

      RHI::PipelineDescriptor pipelineDesc;
      pipelineDesc.topology = RHI::PrimitiveTopology::TriangleList;
      pipelineDesc.depthStencil.depthTestEnable = true;
      pipelineDesc.depthStencil.depthWriteEnable = true;
      pipelineDesc.depthStencil.depthCompareOp = RHI::CompareOp::Less;
      pipelineDesc.rasterizer.cullMode = RHI::CullMode::Back;

      // Vulkan Y-flip inverts apparent winding order, so use Clockwise for
      // Vulkan
      if (device->GetAPI() == RHI::API::Vulkan) {
        pipelineDesc.rasterizer.frontFace = RHI::FrontFace::Clockwise;
      } else {
        pipelineDesc.rasterizer.frontFace = RHI::FrontFace::CounterClockwise;
      }

      mainPipeline = device->CreatePipeline(
          pipelineDesc, activeShader->GetHandle(), meshLayout);

      if (activeParticleShader) {
        RHI::PipelineDescriptor particleDesc = pipelineDesc;
        particleDesc.blend.blendEnable = true;
        particleDesc.blend.srcColorFactor = RHI::BlendFactor::SrcAlpha;
        particleDesc.blend.dstColorFactor = RHI::BlendFactor::OneMinusSrcAlpha;
        particleDesc.blend.colorOp = RHI::BlendOp::Add;
        particleDesc.blend.srcAlphaFactor = RHI::BlendFactor::One;
        particleDesc.blend.dstAlphaFactor = RHI::BlendFactor::Zero;
        particleDesc.blend.alphaOp = RHI::BlendOp::Add;
        particleDesc.depthStencil.depthWriteEnable =
            false; // Usually particles don't write depth
        particleDesc.rasterizer.cullMode =
            RHI::CullMode::None; // Particles are often billlboards

        particlePipeline = device->CreatePipeline(
            particleDesc, activeParticleShader->GetHandle(), meshLayout);
      }
    }
    initRenderData();
  }

  RHI::IDevice *GetDevice() const { return device; }

  void SetIBLMaps(RHI::TextureHandle irradiance, RHI::TextureHandle prefilter,
                  RHI::TextureHandle brdf) {
    iblIrradiance = irradiance;
    iblPrefilter = prefilter;
    iblBrdf = brdf;
    useIBL = true;
  }

  void BeginScene(const glm::mat4 &view, const glm::mat4 &proj,
                  const glm::vec3 &camPos) {
    sceneData.viewMatrix = view;
    sceneData.cameraPos = camPos;
    sceneData.lightPos = glm::vec3(2.0f, 4.0f, 3.0f);
    sceneData.lightColor = glm::vec3(1.0f);

    // Flip Y-axis for Vulkan (Vulkan has Y-down, OpenGL has Y-up)
    // This allows the same projection matrix to work on both APIs
    glm::mat4 projAdjusted = proj;
    if (device && device->GetAPI() == RHI::API::Vulkan) {
      projAdjusted[1][1] *= -1.0f;
    }
    sceneData.projectionMatrix = projAdjusted;

    opaqueQueue.clear();
    transparentQueue.clear();
    particleQueue.clear();
    pointLights.clear();
  }

  void SubmitDirectionalLight(const DirectionalLight &light) {
    sunLight = light;
  }

  void SubmitPointLight(const PointLightData &light) {
    if (pointLights.size() < 4)
      pointLights.push_back(light);
  }

  void Submit(const std::shared_ptr<Model> &model, const glm::mat4 &transform) {
    for (size_t i = 0; i < model->GetMeshCount(); i++) {
      const Mesh &meshRef = model->GetMesh(i);
      Mesh *meshPtr = const_cast<Mesh *>(&meshRef);
      Material *matPtr = meshPtr->GetMaterial().get();
      float dist = glm::length(sceneData.cameraPos - glm::vec3(transform[3]));
      opaqueQueue.emplace_back(meshPtr, matPtr, transform, dist);
    }
  }

  void SubmitMesh(const Mesh &mesh, const glm::mat4 &transform) {
    Mesh *meshPtr = const_cast<Mesh *>(&mesh);
    Material *matPtr = meshPtr->GetMaterial().get();
    float dist = glm::length(sceneData.cameraPos - glm::vec3(transform[3]));
    opaqueQueue.emplace_back(meshPtr, matPtr, transform, dist);
  }

  void SubmitParticles(const ParticleRenderCommand &cmd) {
    particleQueue.push_back(cmd);
  }

  void EndScene(RHI::ICommandList *cmdList) {
    if (!device || !activeShader || !cmdList)
      return;

    std::sort(opaqueQueue.begin(), opaqueQueue.end(),
              [](const RenderCommand &a, const RenderCommand &b) {
                return a.distanceToCamera < b.distanceToCamera;
              });

    RHI::ShaderHandle shader = activeShader->GetHandle();

    activeShader->BindCommandList(cmdList);

    activeShader->SetMat4("view", glm::value_ptr(sceneData.viewMatrix));
    activeShader->SetMat4("projection",
                          glm::value_ptr(sceneData.projectionMatrix));
    activeShader->SetVec3("viewPos", sceneData.cameraPos.x,
                          sceneData.cameraPos.y, sceneData.cameraPos.z);
    activeShader->SetVec3("lightPos", sceneData.lightPos.x,
                          sceneData.lightPos.y, sceneData.lightPos.z);
    activeShader->SetVec3("lightColor", sceneData.lightColor.x,
                          sceneData.lightColor.y, sceneData.lightColor.z);

    activeShader->SetVec3("dirLight.direction", sunLight.direction.x,
                          sunLight.direction.y, sunLight.direction.z);
    activeShader->SetVec3("dirLight.color", sunLight.color.x, sunLight.color.y,
                          sunLight.color.z);
    activeShader->SetFloat("dirLight.intensity", sunLight.intensity);
    activeShader->SetInt("numPointLights",
                         static_cast<int>(pointLights.size()));

    for (size_t i = 0; i < pointLights.size(); i++) {
      std::string base = "pointLights[" + std::to_string(i) + "]";
      activeShader->SetVec3(base + ".position", pointLights[i].position.x,
                            pointLights[i].position.y,
                            pointLights[i].position.z);
      activeShader->SetVec3(base + ".color", pointLights[i].color.x,
                            pointLights[i].color.y, pointLights[i].color.z);
      activeShader->SetFloat(base + ".intensity", pointLights[i].intensity);
      activeShader->SetFloat(base + ".radius", pointLights[i].radius);
    }

    activeShader->SetBool("useIBL", useIBL);
    if (useIBL) {
      cmdList->BindTexture(10, iblIrradiance);
      cmdList->BindTexture(11, iblPrefilter);
      cmdList->BindTexture(12, iblBrdf);
      activeShader->SetInt("irradianceMap", 10);
      activeShader->SetInt("prefilterMap", 11);
      activeShader->SetInt("brdfLUT", 12);
    }

    // Bind pipeline before rendering
    if (RHI::IsValid(mainPipeline)) {
      cmdList->BindPipeline(mainPipeline);
    }

    for (const auto &cmd : opaqueQueue) {
      RenderMesh(cmdList, cmd);
    }

    activeShader->BindCommandList(nullptr);

    // Render Particles
    if (activeParticleShader && !particleQueue.empty()) {
      activeParticleShader->BindCommandList(cmdList);
      activeParticleShader->SetMat4("view",
                                    glm::value_ptr(sceneData.viewMatrix));
      activeParticleShader->SetMat4("projection",
                                    glm::value_ptr(sceneData.projectionMatrix));
      activeParticleShader->SetVec3("CameraPosition", sceneData.cameraPos.x,
                                    sceneData.cameraPos.y,
                                    sceneData.cameraPos.z);

      if (RHI::IsValid(particlePipeline)) {
        cmdList->BindPipeline(particlePipeline);
      }

      for (const auto &cmd : particleQueue) {
        RenderParticles(cmdList, cmd);
      }

      activeParticleShader->BindCommandList(nullptr);
    }
  }

  void DrawScreenQuad(RHI::ICommandList *cmdList, Shader &screenShader,
                      RHI::TextureHandle texture) {
    if (!cmdList || !RHI::IsValid(screenQuadVAO))
      return;

    screenShader.BindCommandList(cmdList);
    screenShader.SetInt("screenTexture", 0);
    cmdList->BindTexture(0, texture);
    cmdList->BindVertexArray(screenQuadVAO);

    RHI::DrawCommand cmd;
    cmd.vertexCount = 6;
    cmd.instanceCount = 1;
    cmdList->Draw(cmd);
    screenShader.BindCommandList(nullptr);
  }

  void DrawScreenQuad(RHI::ICommandList *cmdList) {
    if (!cmdList || !RHI::IsValid(screenQuadVAO))
      return;

    cmdList->BindVertexArray(screenQuadVAO);
    RHI::DrawCommand cmd;
    cmd.vertexCount = 6;
    cmd.instanceCount = 1;
    cmdList->Draw(cmd);
  }

private:
  void RenderMesh(RHI::ICommandList *cmdList, const RenderCommand &cmd) {
    if (!cmdList || !activeShader)
      return;

    RHI::ShaderHandle shader = activeShader->GetHandle();

    if (cmd.material) {
      cmd.material->Apply(cmdList, shader);
      activeShader->SetBool("hasTextureDiffuse",
                            cmd.material->HasTextureType(TextureType::DIFFUSE));
      activeShader->SetBool("hasTextureNormal",
                            cmd.material->HasTextureType(TextureType::NORMAL));
      activeShader->SetBool("hasTextureMetallic", cmd.material->HasTextureType(
                                                      TextureType::METALLIC));
      activeShader->SetBool("hasTextureRoughness", cmd.material->HasTextureType(
                                                       TextureType::ROUGHNESS));
      activeShader->SetBool("hasTextureAO",
                            cmd.material->HasTextureType(TextureType::AO));
      activeShader->SetBool("hasTextureEmission", cmd.material->HasTextureType(
                                                      TextureType::EMISSION));
    }

    activeShader->SetMat4("model", glm::value_ptr(cmd.transform));

    cmdList->BindVertexArray(cmd.mesh->GetVAO());
    RHI::DrawIndexedCommand drawCmd;
    drawCmd.indexCount = cmd.mesh->GetIndexCount();
    drawCmd.instanceCount = 1;
    drawCmd.indexType = RHI::IndexType::UInt32;
    cmdList->DrawIndexed(drawCmd);
  }

  void RenderParticles(RHI::ICommandList *cmdList,
                       const ParticleRenderCommand &cmd) {
    if (!cmdList || !activeParticleShader)
      return;

    RHI::ShaderHandle shader = activeParticleShader->GetHandle();

    // Bind Mapping Texture
    if (cmd.mappingTexture) {
      cmdList->BindTexture(0, cmd.mappingTexture->GetHandle());
      activeParticleShader->SetInt("Tex", 0);
    }

    // Material logic (if it has textures or extra params)
    if (cmd.material) {
      if (cmd.material->GetTextureCount() > 0) {
        cmdList->BindTexture(1, cmd.material->GetTexture(0)->GetHandle());
        activeParticleShader->SetInt("Texture", 1);
      }
    }

    activeParticleShader->SetInt("TextureWidth", cmd.textureWidth);
    activeParticleShader->SetInt("Amount", cmd.amount);

    activeParticleShader->SetVec3("Velocity", cmd.velocity.x, cmd.velocity.y,
                                  cmd.velocity.z);
    activeParticleShader->SetVec3("Position", cmd.transform[3][0],
                                  cmd.transform[3][1], cmd.transform[3][2]);
    // The reference passes rotation from info - we can extract from transform
    // or send as param Let's rely on the vertex shader and pass model matrix
    // for base transfroms
    activeParticleShader->SetMat4("model", glm::value_ptr(cmd.transform));

    activeParticleShader->SetVec3("Color", cmd.baseColor.x, cmd.baseColor.y,
                                  cmd.baseColor.z);
    activeParticleShader->SetVec2("Size", cmd.size.x, cmd.size.y);
    activeParticleShader->SetFloat("Radius", cmd.radius);
    activeParticleShader->SetFloat("Angle", cmd.angle);
    activeParticleShader->SetBool("Center", cmd.center);

    // Time could be passed if needed

    // Select mesh (quad if null)
    if (cmd.customMesh) {
      cmdList->BindVertexArray(cmd.customMesh->GetVAO());
      RHI::DrawIndexedCommand drawCmd;
      drawCmd.indexCount = cmd.customMesh->GetIndexCount();
      drawCmd.instanceCount = cmd.amount;
      drawCmd.indexType = RHI::IndexType::UInt32;
      cmdList->DrawIndexed(drawCmd);
    } else {
      // Draw screen quad with instancing
      cmdList->BindVertexArray(screenQuadVAO);
      RHI::DrawCommand drawCmd;
      drawCmd.vertexCount = 6;
      drawCmd.instanceCount = cmd.amount;
      cmdList->Draw(drawCmd);
    }
  }
};

#endif // RENDERER_HPP