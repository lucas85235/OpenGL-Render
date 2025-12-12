#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "../renderer/framebuffer.hpp"
#include "../renderer/model_factory.hpp"
#include "../renderer/pbr_utils.hpp"
#include "../renderer/renderer.hpp"
#include "../rhi/rhi_device.h"
#include "../rhi/rhi_factory.h"
#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "filesystem.hpp"
#include "window.hpp"

class Application {
private:
  std::unique_ptr<Window> window;

  // RHI Device
  std::unique_ptr<RHI::IDevice> rhiDevice;

  // Core Systems
  Renderer renderer;
  std::unique_ptr<FrameBuffer> fb;

  // Shaders
  std::unique_ptr<Shader> pbrShader;
  std::unique_ptr<Shader> screenShader;
  std::unique_ptr<Shader> skyboxShader;

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

  void Run() {
    if (!Init())
      return;

    LoadContent();

    float lastFrame = 0.0f;
    while (!window->ShouldClose()) {
      float currentFrame = static_cast<float>(glfwGetTime());
      float deltaTime = currentFrame - lastFrame;
      lastFrame = currentFrame;

      ProcessInput(deltaTime);
      Update(deltaTime);
      Render();

      window->OnUpdate();
    }
  }

private:
  bool Init() {
    // Choose API
    RHI::API api = RHI::API::Vulkan;

    // OpenGL needs GL context, Vulkan needs GLFW_NO_API
    bool createGLContext = (api == RHI::API::OpenGL);
    if (!window->Init(createGLContext))
      return false;

    // Create RHI Device
    rhiDevice = RHI::CreateDevice(api, window->GetNativeWindow());
    if (!rhiDevice) {
      std::cerr << "[App] Failed to create RHI device!" << std::endl;
      return false;
    }

    // Initialize TextureManager with device
    TextureManager::GetInstance().SetDevice(rhiDevice.get());

    auto info = rhiDevice->GetDeviceInfo();
    std::cout << "[RHI] Renderer: " << info.rendererName << std::endl;
    std::cout << "[RHI] Version: " << info.apiVersion << std::endl;

    window->SetResizeCallback([this](int w, int h) {
      if (this->fb)
        this->fb->Resize(w, h);
    });

    // Compile Shaders
    pbrShader = std::make_unique<Shader>(rhiDevice.get());

    if (api == RHI::API::Vulkan) {
      // Vulkan: Use SPIR-V shaders
      if (!pbrShader->CompileFromSPIRV(
              FS::GetPath("shaders/unified/pbr.vert.spv"),
              FS::GetPath("shaders/unified/pbr.frag.spv"))) {
        std::cerr << "[App] Failed to compile Vulkan SPIR-V shaders"
                  << std::endl;
        return false;
      }
      std::cout << "[App] Using Vulkan SPIR-V shaders" << std::endl;
    } else {
      // OpenGL: Use GLSL shaders
      if (!pbrShader->CompileFromFile(FS::GetPath("shaders/pbr.vert"),
                                      FS::GetPath("shaders/pbr.frag"))) {
        std::cerr << "[App] Failed to compile OpenGL GLSL shaders" << std::endl;
        return false;
      }
      std::cout << "[App] Using OpenGL GLSL shaders" << std::endl;
    }

    // screenShader = std::make_unique<Shader>(rhiDevice.get());
    // screenShader->CompileFromSPIRV(FS::GetPath("shaders/unified/screen.vert.spv"),
    //                                FS::GetPath("shaders/unified/screen.frag.spv"));

    // skyboxShader = std::make_unique<Shader>(rhiDevice.get());
    // skyboxShader->CompileFromSPIRV(FS::GetPath("shaders/unified/skybox.vert.spv"),
    //                                FS::GetPath("shaders/unified/skybox.frag.spv"));

    // Setup Renderer
    renderer.Init(rhiDevice.get(), pbrShader.get());

    // Setup Framebuffer (skip for Vulkan - has its own swapchain)
    if (api == RHI::API::Vulkan) {
      fb = std::make_unique<FrameBuffer>(rhiDevice.get(), window->GetWidth(),
                                         window->GetHeight());
      fb->Init();
    }

    // Setup Environment Map
    envMap.SetDevice(rhiDevice.get());

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
          rhiDevice.get(), "models/DamagedHelmet/DamagedHelmet.glb");
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
    auto floor = activeScene->CreateEntity("Floor");
    auto floorMesh = std::make_shared<Mesh>(
        ModelFactory::CreatePlane(rhiDevice.get(), 1.0f));
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
    rhiDevice->BeginFrame();

    glm::mat4 view =
        glm::lookAt(cameraPos, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), window->GetAspect(),
                                      0.1f, 100.0f);

    renderer.BeginScene(view, proj, cameraPos);
    if (activeScene)
      activeScene->OnRender(renderer);
    renderer.EndScene();

    rhiDevice->EndFrame();
  }
};

#endif