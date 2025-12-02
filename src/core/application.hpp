#ifndef APPLICATION_HPP
#define APPLICATION_HPP

#include <memory>
#include <vector>
#include <iostream>

#include "window.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/framebuffer.hpp"
#include "../renderer/custom_shaders.hpp"
#include "../renderer/pbr_utils.hpp"
#include "../renderer/model_factory.hpp"
#include "../scene/scene.hpp"
#include "../scene/components.hpp"

class Application {
private:
    std::unique_ptr<Window> window;
    
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
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 6.0f);
    
    // Input Control
    bool mKeyPressed = false;
    int currentMatIndex = 0;
    std::shared_ptr<Entity> playerEntity; // Referência para input

public:
    Application(const std::string& title, int width, int height) {
        window = std::make_unique<Window>(width, height, title);
    }

    void Run() {
        if (!Init()) return;
        
        LoadContent();
        
        // Loop Principal
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
        // 1. Iniciar Janela
        if (!window->Init()) return false;

        // Configurar Callback de Resize
        window->SetResizeCallback([this](int w, int h) {
            if (this->fb) this->fb->Resize(w, h);
        });

        // 2. Compilar Shaders
        pbrShader = std::make_unique<Shader>();
        if (!pbrShader->CompileFromSource(CustomShaders::AdvancedVertexShader, 
                                          CustomShaders::CustomMaterialFragmentShader)) return false;

        screenShader = std::make_unique<Shader>();
        if (!screenShader->CompileFromSource(ShaderSource::ScreenVertexShader, 
                                             ShaderSource::ScreenFragmentShader)) return false;

        skyboxShader = std::make_unique<Shader>();
        if (!skyboxShader->CompileFromSource(CustomShaders::SkyboxVertexShader, 
                                             CustomShaders::SkyboxFragmentShader)) return false;

        // 3. Setup Renderer
        renderer.Init(pbrShader.get(), skyboxShader.get());
        
        // 4. Setup Framebuffer
        fb = std::make_unique<FrameBuffer>(window->GetWidth(), window->GetHeight());
        fb->Init();

        return true;
    }

    void LoadContent() {
        activeScene = std::make_unique<Scene>();

        // Materiais
        auto gold = std::make_shared<Material>(MaterialLibrary::CreateGold());
        auto silver = std::make_shared<Material>(MaterialLibrary::CreateSilver());
        auto plastic = std::make_shared<Material>(MaterialLibrary::CreatePlastic());
        auto rubber = std::make_shared<Material>(MaterialLibrary::CreateRubber());
        auto copper = std::make_shared<Material>(MaterialLibrary::CreateCopper());
        
        materials = { gold, silver, plastic, rubber, copper };

        // --- CARREGAR MODELO ---
        playerEntity = activeScene->CreateEntity("Helmet");
        try {
            auto model = std::make_shared<Model>("models/DamagedHelmet/DamagedHelmet.glb");
            
            // Salvar material original
            if(model->GetMeshCount() > 0) 
                materials.push_back(model->GetMesh(0).GetMaterial());

            auto renderComp = playerEntity->AddComponent<MeshRenderer>(model);
            renderComp->SetMaterial(materials[0]); // Começa com Ouro
        } catch(...) { std::cerr << "Erro carregando modelo" << std::endl; }

        playerEntity->AddComponent<RotatorScript>(glm::vec3(0, 30, 0));
        playerEntity->transform.Position = glm::vec3(0, 0.5f, 0);
        playerEntity->transform.Rotation = glm::vec3(90, 0, 0);

        // --- CARREGAR CHÃO ---
        auto floor = activeScene->CreateEntity("Floor");
        auto floorMesh = std::make_shared<Mesh>(ModelFactory::CreatePlaneMesh(1.0f));
        auto floorRend = floor->AddComponent<SimpleMeshRenderer>(floorMesh);
        floorRend->SetMaterial(copper);
        floor->transform.Scale = glm::vec3(10.0f);
        floor->transform.Position = glm::vec3(0, -1.0f, 0);

        // --- ILUMINAÇÃO & IBL ---
        // Tente carregar o HDR, se falhar não quebra o app
        envMap.LoadFromHDR("models/golden_gate_hills_4k.hdr");
        if (envMap.envCubemap) {
            renderer.SetIBLMaps(envMap.GetIrradianceMapID(), envMap.GetPrefilterMapID(), envMap.brdfLUTTexture);
        }

        // Luzes
        auto sun = activeScene->CreateEntity("Sun");
        sun->AddComponent<DirectionalLightComponent>(glm::vec3(1.0f, 0.9f, 0.8f), 2.0f);
        
        auto redLight = activeScene->CreateEntity("RedLight");
        redLight->AddComponent<PointLightComponent>(glm::vec3(1,0,0), 30.0f, 10.0f);
        redLight->transform.Position = glm::vec3(-2, 1, -2);
        
        auto blueLight = activeScene->CreateEntity("BlueLight");
        blueLight->AddComponent<PointLightComponent>(glm::vec3(0,0.5f,1), 30.0f, 10.0f);
        blueLight->transform.Position = glm::vec3(2, 1, 0);
        blueLight->AddComponent<FloaterScript>(1.0f, 2.0f);

        activeScene->OnStart();
        std::cout << "Cena carregada!" << std::endl;
    }

    void ProcessInput(float dt) {
        if (window->IsKeyPressed(GLFW_KEY_ESCAPE))
            window->Close();

        // Controle Câmera
        float speed = 2.5f * dt;
        if (window->IsKeyPressed(GLFW_KEY_W)) cameraPos.z -= speed;
        if (window->IsKeyPressed(GLFW_KEY_S)) cameraPos.z += speed;
        if (window->IsKeyPressed(GLFW_KEY_A)) cameraPos.x -= speed;
        if (window->IsKeyPressed(GLFW_KEY_D)) cameraPos.x += speed;

        // Troca de Material (Toggle)
        bool mPressed = window->IsKeyPressed(GLFW_KEY_M);
        if (mPressed && !mKeyPressed) {
            currentMatIndex = (currentMatIndex + 1) % materials.size();
            
            if(playerEntity) {
                if(auto rend = playerEntity->GetComponent<MeshRenderer>()) {
                    rend->SetMaterial(materials[currentMatIndex]);
                    std::cout << "Material: " << materials[currentMatIndex]->GetName() << std::endl;
                }
            }
        }
        mKeyPressed = mPressed;
    }

    void Update(float dt) {
        if (activeScene) activeScene->OnUpdate(dt);
    }

    void Render() {
        // 1. Geometry Pass (Framebuffer)
        fb->Bind();
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), window->GetAspect(), 0.1f, 100.0f);

        renderer.BeginScene(view, proj, cameraPos);
        if (activeScene) activeScene->OnRender(renderer);
        renderer.EndScene();

        renderer.DrawSkybox(envMap.envCubemap, view, proj);

        // 2. Post-Process (Screen)
        fb->Unbind();
        renderer.DrawScreenQuad(*screenShader, fb->GetTexture());
    }
};

#endif