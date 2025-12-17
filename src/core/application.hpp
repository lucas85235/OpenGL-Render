#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "../renderer/framebuffer.hpp"
#include "../renderer/model_factory.hpp"
#include "../renderer/pbr_utils.hpp"
#include "../renderer/render_context.hpp"
#include "../renderer/render_graph/pbr_pass_node.hpp"
#include "../renderer/render_graph/render_graph.hpp"
#include "../renderer/render_graph/skybox_pass_node.hpp"
#include "../scene/components.hpp"
#include "filesystem.hpp"
#include "vfs/native_file_system.hpp"
#include "window.hpp"

class Application {
private:
  std::unique_ptr<Window> window;
  std::unique_ptr<IFileSystem> fileSystem;

  // RHI Device
  std::unique_ptr<RenderContext> renderContext;

  // Core Systems
  Renderer renderer;
  std::unique_ptr<FrameBuffer> fb;

  // Shaders
  // Shader pointers are now retrieved from Manager or passed to passes
  // std::unique_ptr<Shader> pbrShader; // Removed member
  std::unique_ptr<Shader> screenShader;

  // Skybox
  std::unique_ptr<RenderGraph> renderGraph;

  // Environment
  PBRUtils::EnvironmentMap envMap;

  // Scene Data
  std::unique_ptr<Scene> activeScene;
  std::vector<std::shared_ptr<Material>> materials;

  // Game State
  glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);

  // Input Control
  bool mKeyPressed = false;
  int currentMatIndex = 0;
  std::shared_ptr<Entity> playerEntity;

  // FPS Counter
  float fpsTimer = 0.0f;
  int frameCount = 0;
  float currentFPS = 0.0f;

public:
  Application(const std::string &title, int width, int height) {
    window = std::make_unique<Window>(width, height, title);
  }

  bool Initialize() {
    if (!InitInternal())
      return false;

    LoadContent();
    return true;
  }

  void Tick() {
    float currentFrame = static_cast<float>(glfwGetTime());
    float deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    ProcessInput(deltaTime);
    Update(deltaTime);
    Render();

    window->OnUpdate();
  }

  bool ShouldClose() const { return window->ShouldClose(); }

private:
  float lastFrame = 0.0f;

  bool InitInternal() {
    // Choose API
    RHI::API api = RHI::API::OpenGL;

    // OpenGL needs GL context, Vulkan needs GLFW_NO_API
    bool createGLContext = (api == RHI::API::OpenGL);
    if (!window->Init(createGLContext))
      return false;

    // Initialize FileSystem
    fileSystem = std::make_unique<NativeFileSystem>();

    // Create Render Context
    renderContext =
        std::make_unique<RenderContext>(api, window->GetNativeWindow());
    if (!renderContext->Initialize(fileSystem.get())) {
      std::cerr << "[App] Failed to initialize RenderContext!" << std::endl;
      return false;
    }

    // Note: rhiDevice member removed. Access via context.
    auto rhiDevice = renderContext->GetDevice();

    auto info = rhiDevice->GetDeviceInfo();
    std::cout << "[RHI] Renderer: " << info.rendererName << std::endl;
    std::cout << "[RHI] Version: " << info.apiVersion << std::endl;

    window->SetResizeCallback([this](int w, int h) {
      if (this->fb)
        this->fb->Resize(w, h);
    });

    // Compile Shaders
    // Compile Shaders via Manager
    auto shaderMgr = renderContext->GetShaderManager();
    std::shared_ptr<Shader> pbrShaderLoaded;

    if (api == RHI::API::Vulkan) {
      // Vulkan: Use SPIR-V shaders
      pbrShaderLoaded = shaderMgr->LoadShader(
          "pbr", FS::GetPath("shaders/unified/pbr.vert.spv"),
          FS::GetPath("shaders/unified/pbr.frag.spv"));
      std::cout << "[App] Using Vulkan SPIR-V shaders" << std::endl;
    } else {
      // OpenGL: Use GLSL shaders
      pbrShaderLoaded =
          shaderMgr->LoadShader("pbr", FS::GetPath("shaders/pbr.vert"),
                                FS::GetPath("shaders/pbr.frag"));
      std::cout << "[App] Using OpenGL GLSL shaders" << std::endl;
    }

    if (!pbrShaderLoaded) {
      std::cerr << "[App] Failed to load PBR shader" << std::endl;
      return false;
    }

    // screenShader = std::make_unique<Shader>(rhiDevice.get());
    // screenShader->CompileFromSPIRV(FS::GetPath("shaders/unified/screen.vert.spv"),
    //                                FS::GetPath("shaders/unified/screen.frag.spv"));

    // skyboxShader = std::make_unique<Shader>(rhiDevice.get());
    // skyboxShader->CompileFromSPIRV(FS::GetPath("shaders/unified/skybox.vert.spv"),
    //                                FS::GetPath("shaders/unified/skybox.frag.spv"));

    // Setup Renderer
    renderer.Init(renderContext.get(), pbrShaderLoaded.get());

    // Setup Framebuffer (skip for Vulkan - has its own swapchain)
    if (api == RHI::API::Vulkan) {
      fb = std::make_unique<FrameBuffer>(rhiDevice, window->GetWidth(),
                                         window->GetHeight());
      fb->Init();
    }

    // Setup Environment Map
    envMap.Initialize(rhiDevice, fileSystem.get());

    // Setup Render Graph
    renderGraph = std::make_unique<RenderGraph>(renderContext.get());

    // Add Skybox Pass
    // We pass envMap pointer so the pass can query textures
    renderGraph->AddPass(std::make_unique<SkyboxPassNode>(&envMap));

    // Add PBR Pass
    renderGraph->AddPass(std::make_unique<PBRPassNode>(&renderer));

    return true;
  }

  void LoadContent() {
    activeScene = std::make_unique<Scene>();

    // Materials
    auto gold = std::make_shared<Material>(MaterialLibrary::CreateGold());
    auto silver = std::make_shared<Material>(MaterialLibrary::CreateSilver());
    auto plastic = std::make_shared<Material>(MaterialLibrary::CreatePlastic());
    auto rubber = std::make_shared<Material>(MaterialLibrary::CreateRubber());
    auto copper = std::make_shared<Material>(MaterialLibrary::CreateCopper());

    materials = {gold, silver, plastic, rubber, copper};

    // Load Model
    playerEntity = activeScene->CreateEntity("Helmet");
    try {
      auto model = std::make_shared<Model>(
          renderContext->GetDevice(), renderContext->GetTextureManager(),
          "models/DamagedHelmet/DamagedHelmet.glb");
      // "models/car/Intergalactic_Spaceship-(Wavefront).obj");
      if (model->GetMeshCount() > 0)
        materials.push_back(model->GetMesh(0).GetMaterial());

      auto renderComp = playerEntity->AddComponent<MeshRenderer>(model);
      renderComp->SetMaterial(materials[5]);
    } catch (...) {
      std::cerr << "[App] Error loading model" << std::endl;
    }

    playerEntity->AddComponent<RotatorScript>(glm::vec3(0, 30, 0));
    playerEntity->transform.Position = glm::vec3(0, 0.5f, 0);
    playerEntity->transform.Rotation = glm::vec3(0, 0, 0);

    // Floor
    // Floor
    auto floor = activeScene->CreateEntity("Floor");
    auto floorMesh = std::make_shared<Mesh>(
        ModelFactory::CreatePlane(renderContext->GetDevice(), 1.0f));
    auto floorRend = floor->AddComponent<SimpleMeshRenderer>(floorMesh);
    floorRend->SetMaterial(copper);
    floor->transform.Scale = glm::vec3(10.0f);
    floor->transform.Position = glm::vec3(0, -1.0f, 0);

    // IBL
    envMap.LoadFromHDR("models/golden_gate_hills_4k.hdr");
    if (envMap.IsValid()) {
      renderer.SetIBLMaps(envMap.GetIrradianceMap(), envMap.GetPrefilterMap(),
                          envMap.GetBrdfLUT());
    }

    // Lights
    auto sun = activeScene->CreateEntity("Sun");
    sun->AddComponent<DirectionalLightComponent>(glm::vec3(1.0f, 0.9f, 0.8f),
                                                 2.0f);

    auto redLight = activeScene->CreateEntity("RedLight");
    redLight->AddComponent<PointLightComponent>(glm::vec3(1, 0, 0), 30.0f,
                                                10.0f);
    redLight->transform.Position = glm::vec3(-2, 1, -2);

    auto blueLight = activeScene->CreateEntity("BlueLight");
    blueLight->AddComponent<PointLightComponent>(glm::vec3(0, 0.5f, 1), 30.0f,
                                                 10.0f);
    blueLight->transform.Position = glm::vec3(2, 1, 0);
    blueLight->AddComponent<FloaterScript>(1.0f, 2.0f);

    activeScene->OnStart();
    std::cout << "[App] Scene loaded!" << std::endl;
  }

  void ProcessInput(float dt) {
    if (window->IsKeyPressed(GLFW_KEY_ESCAPE))
      window->Close();

    float speed = 25.0f * dt;
    if (window->IsKeyPressed(GLFW_KEY_W))
      cameraPos.z -= speed;
    if (window->IsKeyPressed(GLFW_KEY_S))
      cameraPos.z += speed;
    if (window->IsKeyPressed(GLFW_KEY_A))
      cameraPos.x -= speed;
    if (window->IsKeyPressed(GLFW_KEY_D))
      cameraPos.x += speed;

    bool mPressed = window->IsKeyPressed(GLFW_KEY_M);
    if (mPressed && !mKeyPressed) {
      currentMatIndex = (currentMatIndex + 1) % materials.size();

      if (playerEntity) {
        if (auto rend = playerEntity->GetComponent<MeshRenderer>()) {
          rend->SetMaterial(materials[currentMatIndex]);
          std::cout << "[App] Material: "
                    << materials[currentMatIndex]->GetName() << std::endl;
        }
      }
    }
    mKeyPressed = mPressed;
  }

  void Update(float dt) {
    if (activeScene)
      activeScene->OnUpdate(dt);

    frameCount++;
    fpsTimer += dt;
    if (fpsTimer >= 1.0f) {
      currentFPS = static_cast<float>(frameCount) / fpsTimer;
      std::cout << "[App] FPS: " << static_cast<int>(currentFPS) << std::endl;
      frameCount = 0;
      fpsTimer = 0.0f;
    }
  }

  void Render() {
    auto rhiDevice = renderContext->GetDevice();
    rhiDevice->BeginFrame();

    glm::mat4 view =
        glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), window->GetAspect(),
                                      0.1f, 100.0f);

    // Render Graph Execution
    RenderPassData passData;
    passData.view = view;
    passData.projection = proj;
    passData.cameraPos = cameraPos;
    passData.windowWidth = window->GetWidth();
    passData.windowHeight = window->GetHeight();

    renderGraph->Execute(passData, activeScene.get());

    rhiDevice->EndFrame();
  }
};

#endif