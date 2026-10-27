#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <iostream>
#include <memory>
#include <vector>

#include "window.hpp"

#include "../editor/console_panel.hpp"
#include "../editor/editor_ui.hpp"
#include "../editor/example_panel.hpp"
#include "../editor/hierarchy_panel.hpp"
#include "../editor/inspector_panel.hpp"
#include "../editor/post_process_panel.hpp"
#include "../editor/viewport_panel.hpp"
#include "../renderer/framebuffer.hpp"
#include "../renderer/model_factory.hpp"
#include "../renderer/pbr_utils.hpp"
#include "../renderer/render_context.hpp"
#include "../renderer/render_graph/bloom_pass_node.hpp"
#include "../renderer/render_graph/grid_pass_node.hpp"
#include "../renderer/render_graph/outline_pass_node.hpp"
#include "../renderer/render_graph/pbr_pass_node.hpp"
#include "../renderer/render_graph/picking_pass_node.hpp"
#include "../renderer/render_graph/render_graph.hpp"
#include "../renderer/render_graph/shadow_pass_node.hpp"
#include "../renderer/render_graph/skybox_pass_node.hpp"
#include "../renderer/render_graph/tonemap_pass_node.hpp"
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

  // Scene framebuffer (3D content renders here in HDR)
  std::unique_ptr<FrameBuffer> sceneFB;

  // Display framebuffer: TonemapPass output (ACES + gamma). ViewportPanel
  // samples this one so we don't dump raw HDR into the ImGui image.
  std::unique_ptr<FrameBuffer> displayFB;

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
  HierarchyPanel *hierarchyPanel = nullptr;
  InspectorPanel *inspectorPanel = nullptr;
  GridPassNode *gridPassNode = nullptr;
  PickingPassNode *pickingPassNode = nullptr;
  OutlinePassNode *outlinePassNode = nullptr;
  PBRPassNode *pbrPassNode = nullptr;
  // Shadow pass is NOT in the render graph — its Execute opens its own render
  // pass on a dedicated FBO, so it runs from RenderScene before the scene
  // pass begins. Kept as a direct owner here.
  std::unique_ptr<ShadowPassNode> shadowPassNode;
  // Bloom runs AFTER the scene render pass ends — it ping-pongs its own mip
  // FBOs and finally additively composites into sceneFB. Same ownership
  // pattern as shadow (outside the RenderGraph).
  std::unique_ptr<BloomPassNode> bloomPassNode;
  // Tonemap runs as the final pass after bloom — reads sceneFB (HDR),
  // writes to displayFB (LDR post ACES + gamma). Same ownership pattern as
  // shadow/bloom (outside the RenderGraph).
  std::unique_ptr<TonemapPassNode> tonemapPassNode;

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

    // If a click landed last frame, enable the picking pass for this frame's
    // scene render so the picking FBO gets populated before we read it.
    bool pickingActiveThisFrame = false;
    if (viewportPanel && viewportPanel->HasPendingPick() && pickingPassNode) {
      *pickingPassNode->GetEnabledPtr() = true;
      pickingActiveThisFrame = true;
    }

    auto rhiDevice = renderContext->GetDevice();
    if (!rhiDevice->BeginFrame())
      return;

    // Render 3D scene into the scene framebuffer
    RenderScene();

    if (pickingActiveThisFrame) {
      ResolvePick();
      *pickingPassNode->GetEnabledPtr() = false;
      viewportPanel->ClearPendingPick();
    }

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

    displayFB = std::make_unique<FrameBuffer>(rhiDevice, window->GetWidth(),
                                              window->GetHeight());
    if (!displayFB->Init()) {
      ConsolePanel::LogError("Failed to create display framebuffer");
      return false;
    }

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

    shadowPassNode = std::make_unique<ShadowPassNode>();
    if (!shadowPassNode->Initialize(renderContext.get())) {
      ConsolePanel::LogWarn("ShadowPass failed to initialize — shadows disabled");
    }

    bloomPassNode = std::make_unique<BloomPassNode>();
    if (!bloomPassNode->Initialize(renderContext.get())) {
      ConsolePanel::LogWarn("BloomPass failed to initialize — bloom disabled");
      bloomPassNode.reset();
    }

    tonemapPassNode = std::make_unique<TonemapPassNode>();
    if (!tonemapPassNode->Initialize(renderContext.get())) {
      ConsolePanel::LogWarn("TonemapPass failed to initialize — output will be raw HDR");
      tonemapPassNode.reset();
    }

    renderGraph = std::make_unique<RenderGraph>(renderContext.get());
    renderGraph->AddPass(std::make_unique<SkyboxPassNode>(&envMap));
    {
      auto pbrNode = std::make_unique<PBRPassNode>(&renderer);
      pbrPassNode = pbrNode.get();
      renderGraph->AddPass(std::move(pbrNode));
    }
    if (pbrPassNode && shadowPassNode) {
      pbrPassNode->SetShadowPass(shadowPassNode->GetPass());
      pbrPassNode->SetShadowEnabledPtr(shadowPassNode->GetEnabledPtr());
    }
    {
      auto outlineNode = std::make_unique<OutlinePassNode>();
      outlinePassNode = outlineNode.get();
      renderGraph->AddPass(std::move(outlineNode));
    }
    {
      auto gridNode = std::make_unique<GridPassNode>();
      gridPassNode = gridNode.get();
      renderGraph->AddPass(std::move(gridNode));
    }
    {
      // Picking pass is disabled by default — it targets its own FBO and
      // would corrupt the active scene render pass if run inline. Task 3.2
      // will invoke PickingPass::Execute directly on click.
      auto pickingNode = std::make_unique<PickingPassNode>();
      pickingPassNode = pickingNode.get();
      renderGraph->AddPass(std::move(pickingNode));
    }

    // Initialize Editor UI
    if (!editorUI.Init(window->GetNativeWindow(), rhiDevice)) {
      ConsolePanel::LogError("Failed to initialize EditorUI");
      return false;
    }

    // Register panels
    viewportPanel = editorUI.AddPanel<ViewportPanel>();
    viewportPanel->SetFrameBuffer(sceneFB.get());
    viewportPanel->SetDisplayFrameBuffer(displayFB.get());
    if (tonemapPassNode) {
      viewportPanel->SetDisplayEnabledPtr(tonemapPassNode->GetEnabledPtr());
    }
    viewportPanel->SetCamera(&camera);
    viewportPanel->SetResizeCallback([this](int w, int h) {
      if (displayFB)
        displayFB->Resize(w, h);
      ConsolePanel::LogInfo("Viewport resized to %dx%d", w, h);
    });

    editorUI.AddPanel<ConsolePanel>();
    editorUI.AddPanel<ExamplePanel>();
    hierarchyPanel = editorUI.AddPanel<HierarchyPanel>();
    inspectorPanel = editorUI.AddPanel<InspectorPanel>();
    {
      auto postProcessPanel = editorUI.AddPanel<PostProcessPanel>();
      if (bloomPassNode) {
        postProcessPanel->SetBloom(bloomPassNode->GetPass(),
                                   bloomPassNode->GetEnabledPtr());
      }
      if (tonemapPassNode) {
        postProcessPanel->SetTonemap(tonemapPassNode->GetPass(),
                                     tonemapPassNode->GetEnabledPtr());
      }
    }
    viewportPanel->SetHierarchyPanel(hierarchyPanel);
    if (outlinePassNode) {
      outlinePassNode->SetHierarchyPanel(hierarchyPanel);
    }

    if (gridPassNode) {
      editorUI.AddViewToggle("Grid", gridPassNode->GetEnabledPtr());
    }
    if (outlinePassNode) {
      editorUI.AddViewToggle("Outline", outlinePassNode->GetEnabledPtr());
    }
    if (shadowPassNode) {
      editorUI.AddViewToggle("Shadows", shadowPassNode->GetEnabledPtr());
    }
    if (bloomPassNode) {
      editorUI.AddViewToggle("Bloom", bloomPassNode->GetEnabledPtr());
    }
    if (tonemapPassNode) {
      editorUI.AddViewToggle("Tonemap", tonemapPassNode->GetEnabledPtr());
    }

    ConsolePanel::LogInfo("Editor UI initialized with %d panels", 5);
    ConsolePanel::LogInfo("Engine ready");

    return true;
  }

  void LoadContent() {
    activeScene = std::make_unique<Scene>();
    hierarchyPanel->SetContext(activeScene.get());
    inspectorPanel->SetContext(activeScene.get(), hierarchyPanel);
    viewportPanel->SetScene(activeScene.get());

    activeScene->AddSystem(std::make_shared<RenderSystem>());
    activeScene->AddSystem(std::make_shared<ParticleSystemRunner>());

    auto gold = std::make_shared<Material>(MaterialLibrary::CreateGold());
    auto silver = std::make_shared<Material>(MaterialLibrary::CreateSilver());
    auto plastic = std::make_shared<Material>(MaterialLibrary::CreatePlastic());
    auto rubber = std::make_shared<Material>(MaterialLibrary::CreateRubber());
    auto copper = std::make_shared<Material>(MaterialLibrary::CreateCopper());

    materials = {gold, silver, plastic, rubber, copper};

    materials = {gold, silver, plastic, rubber, copper};

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

    auto particleSystem = activeScene->CreateEntity("Particles");
    ParticleSystemParams pParams;
    pParams.amount = 100;
    pParams.baseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    pParams.radius = 2.0f;
    pParams.size = glm::vec2(0.2f);
    pParams.positionFunction = [](float t) -> glm::vec4 {
      float phi = t * 3.14159f * 2.0f;
      float cosTheta = 1.0f - 2.0f * t;
      float sinTheta = std::sqrt(1.0f - cosTheta * cosTheta);
      return glm::vec4(sinTheta * std::cos(phi), cosTheta,
                       sinTheta * std::sin(phi), 1.0f);
    };
    pParams.customMesh = std::make_shared<Mesh>(
        ModelFactory::CreateQuad(renderContext->GetDevice(), 1.0f));
    particleSystem.AddComponent<ParticleSystemComponent>(pParams);
    particleSystem.GetTransform().Position = glm::vec3(0, 1.0f, 0);

    activeScene->OnStart();
    ConsolePanel::LogInfo("Scene loaded (%zu entities)",
                          activeScene->GetEntityCount());
  }

  void ProcessInput(float dt) {
    if (window->IsKeyPressed(GLFW_KEY_ESCAPE))
      window->Close();

    static bool pKeyPressed = false;
    bool pPressed = window->IsKeyPressed(GLFW_KEY_P);
    if (pPressed && !pKeyPressed) {
      camera.ToggleProjection();
    }
    pKeyPressed = pPressed;
  }

  void Update(float dt) {
    if (activeScene)
      activeScene->OnUpdate(dt);
  }

  void ResolvePick() {
    if (!pickingPassNode || !activeScene || !viewportPanel)
      return;
    PickingPass *pass = pickingPassNode->GetPass();
    if (!pass || !pass->IsInitialized())
      return;

    uint32_t fbH = pass->GetHeight();
    int localY = viewportPanel->GetPickY();
    int glY = static_cast<int>(fbH) - 1 - localY;

    uint32_t id = pass->ReadPixelId(viewportPanel->GetPickX(), glY);
    if (id == PickingPass::kInvalidEntityId) {
      if (hierarchyPanel)
        hierarchyPanel->SetSelectedEntity(entt::null);
      ConsolePanel::LogInfo("Pick: no entity under cursor");
      return;
    }

    entt::entity picked = static_cast<entt::entity>(id);
    auto &reg = activeScene->GetRegistry();
    if (!reg.valid(picked)) {
      ConsolePanel::LogWarn("Pick: decoded id %u is not a valid entity", id);
      return;
    }

    if (hierarchyPanel)
      hierarchyPanel->SetSelectedEntity(picked);

    std::string tag = "Entity";
    if (reg.any_of<TagComponent>(picked))
      tag = reg.get<TagComponent>(picked).Tag;
    ConsolePanel::LogInfo("Pick: entity id=%u (\"%s\")", id, tag.c_str());
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

        passData.cmdList = cmdList;

        // Shadow pass runs first and opens/closes its own render pass on the
        // shadow FBO. Must happen BEFORE the scene pass begins so we don't
        // nest render passes.
        if (shadowPassNode) {
          shadowPassNode->Execute(renderContext.get(), passData,
                                  activeScene.get());
        }

        cmdList->BeginRenderPass(passDesc);
        cmdList->SetViewport({0.0f, 0.0f, sceneFB->GetWidth(),
                              sceneFB->GetHeight(), 0.0f, 1.0f});
        cmdList->SetScissor({0, 0, static_cast<uint32_t>(sceneFB->GetWidth()),
                             static_cast<uint32_t>(sceneFB->GetHeight())});

        renderGraph->Execute(passData, activeScene.get());

        cmdList->EndRenderPass();

        // Bloom runs after the scene pass ends: samples sceneFB color,
        // builds its mip chain, and additively composites back into sceneFB.
        if (bloomPassNode && bloomPassNode->IsEnabled()) {
          bloomPassNode->GetPass()->Execute(
              cmdList, sceneFB->GetTexture(), sceneFB->GetHandle(),
              static_cast<uint32_t>(sceneFB->GetWidth()),
              static_cast<uint32_t>(sceneFB->GetHeight()));
        }

        // Tonemap is the final pass: HDR sceneFB -> LDR displayFB via ACES
        // Narkowicz + gamma. Viewport samples displayFB. When disabled, fall
        // back to copying sceneFB straight through so viewport still shows
        // something (raw HDR, will look washed out).
        if (tonemapPassNode && tonemapPassNode->IsEnabled() && displayFB &&
            displayFB->IsInitialized()) {
          tonemapPassNode->GetPass()->Execute(
              cmdList, sceneFB->GetTexture(), displayFB->GetHandle(),
              static_cast<uint32_t>(displayFB->GetWidth()),
              static_cast<uint32_t>(displayFB->GetHeight()));
        }

        cmdList->End();
        rhiDevice->SubmitCommandList(cmdHandle);
      }

      rhiDevice->DestroyCommandList(cmdHandle);
    }
  }
};

#endif