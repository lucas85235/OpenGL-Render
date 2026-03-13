#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "window.hpp"

#include "../editor/console_panel.hpp"
#include "../editor/editor_ui.hpp"
#include "../editor/example_panel.hpp"
#include "../editor/viewport_panel.hpp"
#include "../renderer/framebuffer.hpp"
#include "../renderer/model_factory.hpp"
#include "../renderer/pbr_utils.hpp"
#include "../renderer/render_context.hpp"
#include "../renderer/render_graph/pbr_pass_node.hpp"
#include "../renderer/render_graph/render_graph.hpp"
#include "../renderer/render_graph/skybox_pass_node.hpp"
#include "../scene/components.hpp"
#include "../scene/particle_system.hpp"
#include "camera.hpp"
#include "vfs/native_file_system.hpp"

#include "../scene/systems/particle_system_runner.hpp"
#include "../scene/systems/render_system.hpp"

// Native Scripts Definitions
class RotatorScript : public ScriptableEntity {
public:
  glm::vec3 rotationSpeed = glm::vec3(0.0f);

  void OnCreate() override {
    // Find existing RotatorScriptComponent if added for configuration (legacy
    // fallback/config pass) or just initialize. For now we configure manually
    // via instance after binding or via components. If we want to read config
    // from an old component:
    if (HasComponent<RotatorScriptComponent>()) {
      rotationSpeed = GetComponent<RotatorScriptComponent>().rotationSpeed;
    }
  }

  void OnUpdate(float dt) override {
    auto &transform = GetComponent<TransformComponent>();
    transform.Rotation += rotationSpeed * dt;
  }
};

class FloaterScript : public ScriptableEntity {
public:
  float startY = 0.0f;
  float time = 0.0f;
  float amplitude = 1.0f;
  float frequency = 1.0f;
  bool initialized = false;

  void OnCreate() override {
    if (HasComponent<FloaterScriptComponent>()) {
      auto comp = GetComponent<FloaterScriptComponent>();
      amplitude = comp.amplitude;
      frequency = comp.frequency;
    }
  }

  void OnUpdate(float dt) override {
    auto &transform = GetComponent<TransformComponent>();
    if (!initialized) {
      startY = transform.Position.y;
      initialized = true;
    }
    time += dt;
    transform.Position.y = startY + std::sin(time * frequency) * amplitude;
  }
};

class Application {
private:
  std::unique_ptr<Window> window;
  std::unique_ptr<IFileSystem> fileSystem;

  // RHI Device
  std::unique_ptr<RenderContext> renderContext;

  // Core Systems
  Renderer renderer;

  // Scene framebuffer (3D content renders here, displayed in viewport panel)
  std::unique_ptr<FrameBuffer> sceneFB;

  std::unique_ptr<RenderGraph> renderGraph;

  // Environment
  PBRUtils::EnvironmentMap envMap;

  // Scene Data
  std::unique_ptr<Scene> activeScene;
  std::vector<std::shared_ptr<Material>> materials;

  // Game State
  Camera camera{CameraMode::FPS};

  // Input Control
  bool mKeyPressed = false;
  int currentMatIndex = 0;
  Entity playerEntity;

  // Editor UI
  EditorUI editorUI;
  ViewportPanel *viewportPanel = nullptr;

public:
  Application(const std::string &title, int width, int height) {
    window = std::make_unique<Window>(width, height, title);
  }

  ~Application() { editorUI.Shutdown(); }

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

    auto rhiDevice = renderContext->GetDevice();
    if (!rhiDevice->BeginFrame())
      return;

    // Render 3D scene into the scene framebuffer
    RenderScene();

    // Render editor UI (which displays the scene via ViewportPanel)
    editorUI.BeginFrame();
    editorUI.Update(deltaTime);
    editorUI.Render();
    editorUI.EndFrame();

    rhiDevice->EndFrame();
    window->OnUpdate();
  }

  bool ShouldClose() const { return window->ShouldClose(); }

private:
  float lastFrame = 0.0f;

  bool InitInternal() {
    RHI::API api = RHI::API::OpenGL;

    bool createGLContext = (api == RHI::API::OpenGL);
    if (!window->Init(createGLContext))
      return false;

    ConsolePanel::LogInfo("Window initialized (%dx%d)", window->GetWidth(),
                          window->GetHeight());

    fileSystem = std::make_unique<NativeFileSystem>();

    renderContext =
        std::make_unique<RenderContext>(api, window->GetNativeWindow());
    if (!renderContext->Initialize(fileSystem.get())) {
      ConsolePanel::LogError("Failed to initialize RenderContext");
      return false;
    }
    ConsolePanel::LogInfo("RenderContext initialized (API: %s)",
                          api == RHI::API::OpenGL ? "OpenGL" : "Vulkan");

    auto rhiDevice = renderContext->GetDevice();

    // Create scene framebuffer
    sceneFB = std::make_unique<FrameBuffer>(rhiDevice, window->GetWidth(),
                                            window->GetHeight());
    if (!sceneFB->Init()) {
      ConsolePanel::LogError("Failed to create scene framebuffer");
      return false;
    }
    ConsolePanel::LogInfo("Scene framebuffer created (%dx%d)",
                          window->GetWidth(), window->GetHeight());

    // Mouse: only process when right-clicking
    window->SetMouseMoveCallback([this](double xOffset, double yOffset) {
      if (window->IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT)) {
        camera.ProcessMouse(static_cast<float>(xOffset),
                            static_cast<float>(yOffset));
      }
    });

    window->SetScrollCallback([this](double yOffset) {
      camera.ProcessScroll(static_cast<float>(yOffset));
    });

    // Load shaders
    auto shaderMgr = renderContext->GetShaderManager();
    std::shared_ptr<Shader> pbrShaderLoaded;

    if (api == RHI::API::Vulkan) {
      pbrShaderLoaded = shaderMgr->LoadShader(
          "pbr", "shaders/pbr_vulkan.vert.spv", "shaders/pbr_vulkan.frag.spv");
    } else {
      pbrShaderLoaded =
          shaderMgr->LoadUnifiedShader("pbr", "shaders/pbr_opengl.shader");
    }

    if (!pbrShaderLoaded) {
      ConsolePanel::LogError("Failed to load PBR shader");
      return false;
    }
    ConsolePanel::LogInfo("PBR shader loaded");

    std::shared_ptr<Shader> particleShaderLoaded;
    if (api == RHI::API::OpenGL) {
      particleShaderLoaded = shaderMgr->LoadUnifiedShader(
          "particle", "models/particle_opengl.shader");
    }

    if (particleShaderLoaded) {
      ConsolePanel::LogInfo("Particle shader loaded");
    } else {
      ConsolePanel::LogWarn(
          "Failed to load particle shader, particles may not appear.");
    }

    renderer.Init(renderContext.get(), pbrShaderLoaded.get(),
                  particleShaderLoaded.get());

    // Setup Environment Map
    envMap.Initialize(rhiDevice, fileSystem.get());

    renderGraph = std::make_unique<RenderGraph>(renderContext.get());
    renderGraph->AddPass(std::make_unique<SkyboxPassNode>(&envMap));
    renderGraph->AddPass(std::make_unique<PBRPassNode>(&renderer));

    // Initialize Editor UI
    if (!editorUI.Init(window->GetNativeWindow(), rhiDevice)) {
      ConsolePanel::LogError("Failed to initialize EditorUI");
      return false;
    }

    // Register panels
    viewportPanel = editorUI.AddPanel<ViewportPanel>();
    viewportPanel->SetFrameBuffer(sceneFB.get());
    viewportPanel->SetResizeCallback([this](int w, int h) {
      ConsolePanel::LogInfo("Viewport resized to %dx%d", w, h);
    });

    editorUI.AddPanel<ConsolePanel>();
    editorUI.AddPanel<ExamplePanel>();

    ConsolePanel::LogInfo("Editor UI initialized with %d panels", 3);
    ConsolePanel::LogInfo("Engine ready");

    return true;
  }

  void LoadContent() {
    activeScene = std::make_unique<Scene>();
    activeScene->AddSystem(std::make_shared<RenderSystem>());
    activeScene->AddSystem(std::make_shared<ParticleSystemRunner>());

    auto gold = std::make_shared<Material>(MaterialLibrary::CreateGold());
    auto silver = std::make_shared<Material>(MaterialLibrary::CreateSilver());
    auto plastic = std::make_shared<Material>(MaterialLibrary::CreatePlastic());
    auto rubber = std::make_shared<Material>(MaterialLibrary::CreateRubber());
    auto copper = std::make_shared<Material>(MaterialLibrary::CreateCopper());

    materials = {gold, silver, plastic, rubber, copper};

    playerEntity = activeScene->CreateEntity("Helmet");
    try {
      auto model = std::make_shared<Model>(
          renderContext->GetDevice(), renderContext->GetTextureManager(),
          "models/DamagedHelmet/DamagedHelmet.glb");
      if (model->GetMeshCount() > 0)
        materials.push_back(model->GetMesh(0).GetMaterial());

      auto &renderComp =
          playerEntity.AddComponent<MeshRendererComponent>(model);
      renderComp.materialOverride = materials[5];
      ConsolePanel::LogInfo("Model loaded: DamagedHelmet (%zu meshes)",
                            model->GetMeshCount());
    } catch (...) {
      ConsolePanel::LogError("Failed to load model");
    }

    playerEntity.AddComponent<RotatorScriptComponent>(glm::vec3(0, 30, 0));
    playerEntity.AddComponent<NativeScriptComponent>().Bind<RotatorScript>();
    playerEntity.GetTransform().Position = glm::vec3(0, 0.5f, 0);
    playerEntity.GetTransform().Rotation = glm::vec3(0, 0, 0);

    auto floor = activeScene->CreateEntity("Floor");
    auto floorMesh = std::make_shared<Mesh>(
        ModelFactory::CreatePlane(renderContext->GetDevice(), 1.0f));
    auto &floorRend =
        floor.AddComponent<SimpleMeshRendererComponent>(floorMesh);
    floorRend.mesh->SetMaterial(copper);
    floor.GetTransform().Scale = glm::vec3(10.0f);
    floor.GetTransform().Position = glm::vec3(0, -1.0f, 0);

    envMap.LoadFromHDR("models/golden_gate_hills_4k.hdr");
    if (envMap.IsValid()) {
      renderer.SetIBLMaps(envMap.GetIrradianceMap(), envMap.GetPrefilterMap(),
                          envMap.GetBrdfLUT());
      ConsolePanel::LogInfo("IBL environment loaded");
    }

    auto sun = activeScene->CreateEntity("Sun");
    sun.AddComponent<DirectionalLightComponent>(glm::vec3(1.0f, 0.9f, 0.8f),
                                                2.0f);

    auto redLight = activeScene->CreateEntity("RedLight");
    redLight.AddComponent<PointLightComponent>(glm::vec3(1, 0, 0), 30.0f,
                                               10.0f);
    redLight.GetTransform().Position = glm::vec3(-2, 1, -2);

    auto blueLight = activeScene->CreateEntity("BlueLight");
    blueLight.AddComponent<PointLightComponent>(glm::vec3(0, 0.5f, 1), 30.0f,
                                                10.0f);
    blueLight.GetTransform().Position = glm::vec3(2, 1, 0);
    blueLight.AddComponent<FloaterScriptComponent>(1.0f, 2.0f);
    blueLight.AddComponent<NativeScriptComponent>().Bind<FloaterScript>();

    auto particleSystem = activeScene->CreateEntity("Particles");
    ParticleSystemParams pParams;
    pParams.amount = 5000;
    pParams.baseColor = glm::vec3(1.0f, 0.7f, 0.2f);
    pParams.radius = 3.0f;
    pParams.size = glm::vec2(0.05f);
    pParams.positionFunction = [](float t) -> glm::vec4 {
      float angle = t * 3.14159f * 2.0f * 20.0f;
      float y = (t - 0.5f) * 2.0f;
      float r = sqrt(1.0f - y * y);
      return glm::vec4(r * cos(angle), y, r * sin(angle), t);
    };
    particleSystem.AddComponent<ParticleSystemComponent>(pParams);
    particleSystem.GetTransform().Position = glm::vec3(0, 2.0f, -2.0f);

    activeScene->OnStart();
    ConsolePanel::LogInfo("Scene loaded (%zu entities)",
                          activeScene->GetEntityCount());
  }

  void ProcessInput(float dt) {
    if (window->IsKeyPressed(GLFW_KEY_ESCAPE))
      window->Close();

    if (window->IsKeyPressed(GLFW_KEY_W))
      camera.ProcessKeyboard(Camera::FORWARD, dt);
    if (window->IsKeyPressed(GLFW_KEY_S))
      camera.ProcessKeyboard(Camera::BACKWARD, dt);
    if (window->IsKeyPressed(GLFW_KEY_A))
      camera.ProcessKeyboard(Camera::LEFT, dt);
    if (window->IsKeyPressed(GLFW_KEY_D))
      camera.ProcessKeyboard(Camera::RIGHT, dt);
    if (window->IsKeyPressed(GLFW_KEY_E))
      camera.ProcessKeyboard(Camera::UP, dt);
    if (window->IsKeyPressed(GLFW_KEY_Q))
      camera.ProcessKeyboard(Camera::DOWN, dt);

    static bool pKeyPressed = false;
    bool pPressed = window->IsKeyPressed(GLFW_KEY_P);
    if (pPressed && !pKeyPressed) {
      camera.ToggleProjection();
    }
    pKeyPressed = pPressed;

    bool mPressed = window->IsKeyPressed(GLFW_KEY_M);
    if (mPressed && !mKeyPressed) {
      currentMatIndex = (currentMatIndex + 1) % materials.size();
      if (playerEntity.HasComponent<MeshRendererComponent>()) {
        auto &rend = playerEntity.GetComponent<MeshRendererComponent>();
        rend.materialOverride = materials[currentMatIndex];
      }
    }
    mKeyPressed = mPressed;
  }

  void Update(float dt) {
    if (activeScene)
      activeScene->OnUpdate(dt);
  }

  void RenderScene() {
    auto rhiDevice = renderContext->GetDevice();

    // Render to scene framebuffer
    if (sceneFB && sceneFB->IsInitialized()) {
      float vpAspect = 1.0f;
      if (viewportPanel && viewportPanel->GetViewportHeight() > 0) {
        vpAspect = static_cast<float>(viewportPanel->GetViewportWidth()) /
                   static_cast<float>(viewportPanel->GetViewportHeight());
      } else {
        vpAspect = window->GetAspect();
      }

      glm::mat4 view = camera.GetViewMatrix();
      glm::mat4 proj = camera.GetProjectionMatrix(vpAspect);

      RenderPassData passData;
      passData.view = view;
      passData.projection = proj;
      passData.cameraPos = camera.GetPosition();
      passData.windowWidth = static_cast<uint32_t>(sceneFB->GetWidth());
      passData.windowHeight = static_cast<uint32_t>(sceneFB->GetHeight());

      RHI::CommandListHandle cmdHandle = rhiDevice->CreateCommandList();
      RHI::ICommandList *cmdList = rhiDevice->GetCommandList(cmdHandle);

      if (cmdList) {
        cmdList->Begin();

        RHI::RenderPassDescriptor passDesc;
        passDesc.framebuffer = sceneFB->GetHandle();
        passDesc.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        passDesc.clearDepth = 1.0f;
        passDesc.clearColorBuffer = true;
        passDesc.clearDepthBuffer = true;
        passDesc.clearStencilBuffer = false;

        cmdList->BeginRenderPass(passDesc);
        cmdList->SetViewport({0.0f, 0.0f, sceneFB->GetWidth(),
                              sceneFB->GetHeight(), 0.0f, 1.0f});
        cmdList->SetScissor({0, 0, static_cast<uint32_t>(sceneFB->GetWidth()),
                             static_cast<uint32_t>(sceneFB->GetHeight())});

        passData.cmdList = cmdList;

        renderGraph->Execute(passData, activeScene.get());

        cmdList->EndRenderPass();
        cmdList->End();
        rhiDevice->SubmitCommandList(cmdHandle);
      }

      rhiDevice->DestroyCommandList(cmdHandle);
    }
  }
};

#endif