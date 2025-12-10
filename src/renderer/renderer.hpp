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
#include "skybox_manager.hpp"

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
  Shader *activeShader;

  // RHI Device
  RHI::IDevice *device = nullptr;

  // Screen quad RHI resources
  RHI::BufferHandle screenQuadVB;
  RHI::VertexArrayHandle screenQuadVAO;
  RHI::PipelineHandle screenQuadPipeline;

  Shader *skyboxShader = nullptr;
  SkyboxManager skyboxManager;

  DirectionalLight sunLight;
  std::vector<PointLightData> pointLights;

  unsigned int iblIrradiance = 0;
  unsigned int iblPrefilter = 0;
  unsigned int iblBrdf = 0;
  bool useIBL = false;

  void initRenderData() {
    if (!device)
      return;

    struct QuadVertex {
      float x, y;
      float u, v;
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

    RHI::PipelineDescriptor pipelineDesc;
    pipelineDesc.topology = RHI::PrimitiveTopology::TriangleList;
    pipelineDesc.rasterizer.cullMode = RHI::CullMode::None;
    pipelineDesc.depthStencil.depthTestEnable = false;
    pipelineDesc.depthStencil.depthWriteEnable = false;

    if (!skyboxManager.Initialize()) {
      std::cerr << "[Renderer] Failed to initialize SkyboxManager" << std::endl;
    }
  }

public:
  Renderer() : activeShader(nullptr) {}

  ~Renderer() {
    if (device) {
      if (RHI::IsValid(screenQuadVAO))
        device->DestroyVertexArray(screenQuadVAO);
      if (RHI::IsValid(screenQuadVB))
        device->DestroyBuffer(screenQuadVB);
    }
  }

  void Init(RHI::IDevice *rhiDevice, Shader *defaultShader,
            Shader *sbShader = nullptr) {
    device = rhiDevice;
    activeShader = defaultShader;
    skyboxShader = sbShader;

    if (device) {
      device->SetClearColor({0.0f, 0.0f, 0.0f, 1.0f});
    }

    initRenderData();
  }

  // Legacy init for backwards compatibility
  void Init(Shader *defaultShader, Shader *sbShader = nullptr) {
    activeShader = defaultShader;
    skyboxShader = sbShader;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Legacy OpenGL initialization
    float quadVertices[] = {-1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                            1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                            1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f};

    glGenVertexArrays(1, &legacyScreenQuadVAO);
    glGenBuffers(1, &legacyScreenQuadVBO);
    glBindVertexArray(legacyScreenQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, legacyScreenQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                          (void *)(2 * sizeof(float)));

    glBindVertexArray(0);

    if (!skyboxManager.Initialize()) {
      std::cerr << "[Renderer] Failed to initialize SkyboxManager" << std::endl;
    }
  }

  RHI::IDevice *GetDevice() const { return device; }

  void SetIBLMaps(unsigned int irradiance, unsigned int prefilter,
                  unsigned int brdf) {
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
    if (pointLights.size() < 4) {
      pointLights.push_back(light);
    }
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
    std::sort(opaqueQueue.begin(), opaqueQueue.end(),
              [](const RenderCommand &a, const RenderCommand &b) {
                return a.distanceToCamera < b.distanceToCamera;
              });

    activeShader->Use();
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

    activeShader->SetInt("numPointLights", (int)pointLights.size());

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

    if (useIBL) {
      activeShader->SetBool("useIBL", true);

      glActiveTexture(GL_TEXTURE5);
      glBindTexture(GL_TEXTURE_CUBE_MAP, iblIrradiance);
      activeShader->SetInt("irradianceMap", 10);

      glActiveTexture(GL_TEXTURE6);
      glBindTexture(GL_TEXTURE_CUBE_MAP, iblPrefilter);
      activeShader->SetInt("prefilterMap", 11);

      glActiveTexture(GL_TEXTURE7);
      glBindTexture(GL_TEXTURE_2D, iblBrdf);
      activeShader->SetInt("brdfLUT", 12);
    } else {
      activeShader->SetBool("useIBL", false);
    }

    for (const auto &cmd : opaqueQueue) {
      RenderMesh(cmd);
    }
  }

  void DrawSkybox(unsigned int cubemapID, const glm::mat4 &view,
                  const glm::mat4 &proj) {
    if (!skyboxShader || !skyboxManager.IsInitialized()) {
      return;
    }

    glDepthFunc(GL_LEQUAL);
    GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    skyboxShader->Use();
    skyboxShader->SetMat4("view", glm::value_ptr(view));
    skyboxShader->SetMat4("projection", glm::value_ptr(proj));
    skyboxShader->SetInt("skybox", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

    skyboxManager.Render();

    glDepthFunc(GL_LESS);
    if (cullFaceWasEnabled)
      glEnable(GL_CULL_FACE);
  }

  void SetSkyboxShader(Shader *s) { skyboxShader = s; }

  void DrawScreenQuad(Shader &screenShader, unsigned int textureID) {
    if (device && RHI::IsValid(screenQuadVAO)) {
      // RHI path
      screenShader.Use();
      screenShader.SetInt("screenTexture", 0);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureID);

      device->BindVertexArray(screenQuadVAO);

      RHI::DrawCommand cmd;
      cmd.vertexCount = 6;
      cmd.instanceCount = 1;
      cmd.firstVertex = 0;
      device->Draw(cmd);
    } else {
      // Legacy OpenGL path
      glDisable(GL_DEPTH_TEST);

      screenShader.Use();
      screenShader.SetInt("screenTexture", 0);

      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textureID);

      glBindVertexArray(legacyScreenQuadVAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);

      glEnable(GL_DEPTH_TEST);
    }
  }

  void DrawScreenQuad() {
    if (device && RHI::IsValid(screenQuadVAO)) {
      device->BindVertexArray(screenQuadVAO);
      RHI::DrawCommand cmd;
      cmd.vertexCount = 6;
      cmd.instanceCount = 1;
      device->Draw(cmd);
    } else {
      glDisable(GL_DEPTH_TEST);
      glBindVertexArray(legacyScreenQuadVAO);
      glDrawArrays(GL_TRIANGLES, 0, 6);
      glBindVertexArray(0);
      glEnable(GL_DEPTH_TEST);
    }
  }

  void DebugCubemap(unsigned int cubemapID, const char *name) {
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);

    GLint width, height, format;
    glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,
                             GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,
                             GL_TEXTURE_HEIGHT, &height);
    glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,
                             GL_TEXTURE_INTERNAL_FORMAT, &format);

    std::cout << "[DEBUG] " << name << ":" << std::endl;
    std::cout << "  - Size: " << width << "x" << height << std::endl;
    std::cout << "  - Format: 0x" << std::hex << format << std::dec
              << std::endl;

    for (int i = 0; i < 6; i++) {
      GLint faceWidth;
      glGetTexLevelParameteriv(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0,
                               GL_TEXTURE_WIDTH, &faceWidth);
      if (faceWidth == 0) {
        std::cerr << "  - ERROR: Face " << i << " was not created!"
                  << std::endl;
      }
    }

    GLint maxLevel;
    glGetTexParameteriv(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, &maxLevel);
    std::cout << "  - Max Mip Level: " << maxLevel << std::endl;

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
  }

private:
  // Legacy OpenGL resources (for backwards compatibility)
  unsigned int legacyScreenQuadVAO = 0;
  unsigned int legacyScreenQuadVBO = 0;

  void RenderMesh(const RenderCommand &cmd) {
    if (cmd.material) {
      cmd.material->Apply(activeShader->GetProgramID());

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

    glBindVertexArray(cmd.mesh->GetVAO());
    glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
  }
};

#endif // RENDERER_HPP