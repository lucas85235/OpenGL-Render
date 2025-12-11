#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <memory>
#include <vector>

#include "../rhi/rhi_device.h"
#include "model.hpp"
#include "render_command.hpp"
#include "shader.hpp"

struct SceneData {
  glm::mat4 viewMatrix;
  glm::mat4 projectionMatrix;
  glm::vec3 cameraPos;
  glm::vec3 lightPos;
  glm::vec3 lightColor;
};

struct DirectionalLight {
  glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
  glm::vec3 color = glm::vec3(1.0f);
  float intensity = 1.0f;
};

struct PointLightData {
  glm::vec3 position;
  glm::vec3 color;
  float intensity;
  float radius;
};

class Renderer {
private:
  std::vector<RenderCommand> opaqueQueue;
  std::vector<RenderCommand> transparentQueue;
  SceneData sceneData;
  Shader *activeShader = nullptr;
  RHI::IDevice *device = nullptr;

  RHI::BufferHandle screenQuadVB;
  RHI::VertexArrayHandle screenQuadVAO;
  RHI::PipelineHandle mainPipeline;

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
      if (RHI::IsValid(screenQuadVAO))
        device->DestroyVertexArray(screenQuadVAO);
      if (RHI::IsValid(screenQuadVB))
        device->DestroyBuffer(screenQuadVB);
    }
  }

  void Init(RHI::IDevice *rhiDevice, Shader *defaultShader) {
    device = rhiDevice;
    activeShader = defaultShader;
    if (device) {
      device->SetClearColor({0.0f, 0.0f, 0.0f, 1.0f});

      // Create main pipeline for mesh rendering
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
      pipelineDesc.rasterizer.frontFace = RHI::FrontFace::CounterClockwise;

      mainPipeline = device->CreatePipeline(
          pipelineDesc, activeShader->GetHandle(), meshLayout);
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
    sceneData.projectionMatrix = proj;
    sceneData.cameraPos = camPos;
    sceneData.lightPos = glm::vec3(2.0f, 4.0f, 3.0f);
    sceneData.lightColor = glm::vec3(1.0f);
    opaqueQueue.clear();
    transparentQueue.clear();
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

  void EndScene() {
    if (!device || !activeShader)
      return;

    std::sort(opaqueQueue.begin(), opaqueQueue.end(),
              [](const RenderCommand &a, const RenderCommand &b) {
                return a.distanceToCamera < b.distanceToCamera;
              });

    RHI::ShaderHandle shader = activeShader->GetHandle();

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
      device->BindTexture(10, iblIrradiance);
      device->BindTexture(11, iblPrefilter);
      device->BindTexture(12, iblBrdf);
      activeShader->SetInt("irradianceMap", 10);
      activeShader->SetInt("prefilterMap", 11);
      activeShader->SetInt("brdfLUT", 12);
    }

    // Bind pipeline before rendering
    if (RHI::IsValid(mainPipeline)) {
      device->BindPipeline(mainPipeline);
    }

    for (const auto &cmd : opaqueQueue) {
      RenderMesh(cmd);
    }
  }

  void DrawScreenQuad(Shader &screenShader, RHI::TextureHandle texture) {
    if (!device || !RHI::IsValid(screenQuadVAO))
      return;

    screenShader.SetInt("screenTexture", 0);
    device->BindTexture(0, texture);
    device->BindVertexArray(screenQuadVAO);

    RHI::DrawCommand cmd;
    cmd.vertexCount = 6;
    cmd.instanceCount = 1;
    device->Draw(cmd);
  }

  void DrawScreenQuad() {
    if (!device || !RHI::IsValid(screenQuadVAO))
      return;

    device->BindVertexArray(screenQuadVAO);
    RHI::DrawCommand cmd;
    cmd.vertexCount = 6;
    cmd.instanceCount = 1;
    device->Draw(cmd);
  }

private:
  void RenderMesh(const RenderCommand &cmd) {
    if (!device || !activeShader)
      return;

    RHI::ShaderHandle shader = activeShader->GetHandle();

    if (cmd.material) {
      cmd.material->Apply(device, shader);
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

    device->BindVertexArray(cmd.mesh->GetVAO());
    RHI::DrawIndexedCommand drawCmd;
    drawCmd.indexCount = cmd.mesh->GetIndexCount();
    drawCmd.instanceCount = 1;
    drawCmd.indexType = RHI::IndexType::UInt32;
    device->DrawIndexed(drawCmd);
  }
};

#endif // RENDERER_HPP