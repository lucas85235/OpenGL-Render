#define STB_IMAGE_IMPLEMENTATION

// 1. Standard Includes
#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

// 2. GL Includes
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// 3. Project Includes
#include "src/renderer/shader.hpp"
#include "src/renderer/model.hpp"
#include "src/renderer/custom_shaders.hpp"
#include "src/renderer/renderer.hpp"
#include "src/renderer/framebuffer.hpp"
#include "src/renderer/texture.hpp"
#include "src/renderer/material.hpp"
#include "src/renderer/model_factory.hpp"
#include "src/scene/scene.hpp"
#include "src/scene/components.hpp"
#include "src/renderer/pbr_utils.hpp"

// Configurações da Janela
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const char* APP_TITLE = "Engine: Render Pass & Framebuffer";
    
std::unique_ptr<FrameBuffer> fb;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (fb) {
        fb->Resize(width, height);
    }
}

// ==========================================
// CLASSE PRINCIPAL DA APLICAÇÃO
// ==========================================
class DemoApp {
private:
    // Janela e Contexto
    GLFWwindow* window = nullptr;

    // Sistemas Core
    Renderer renderer;
    
    // Shaders
    std::unique_ptr<Shader> pbrShader;
    std::unique_ptr<Shader> screenShader;
    std::unique_ptr<Shader> skyboxShader;

    // Skybox
    PBRUtils::EnvironmentMap envMap;

    // Assets da Cena
    std::unique_ptr<Scene> activeScene;
    std::shared_ptr<Entity> playerShipEntity;
    std::vector<std::shared_ptr<Material>> availableMaterials;

    // Estado da Aplicação (Câmera e Inputs)
    glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 6.0f);
    int currentMaterialIndex = 0;
    bool mKeyPressed = false;

public:
    DemoApp() {}

    ~DemoApp() {
        glfwTerminate();
    }

    void Run() {
        if (!Init()) return;
        LoadScene();
        // LoadAssets();
        MainLoop();
    }

private:
    // 1. Inicialização do Sistema (GLFW, GLEW, Framebuffer)
    bool Init() {
        if (!glfwInit()) {
            std::cerr << "Falha ao iniciar GLFW" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, APP_TITLE, NULL, NULL);
        if (!window) {
            std::cerr << "Falha ao criar janela GLFW" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Falha ao iniciar GLEW" << std::endl;
            return false;
        }

        // Setup Shaders
        pbrShader = std::make_unique<Shader>();
        if (!pbrShader->CompileFromSource(CustomShaders::AdvancedVertexShader, 
                                          CustomShaders::CustomMaterialFragmentShader)) return false;

        screenShader = std::make_unique<Shader>();
        if (!screenShader->CompileFromSource(ShaderSource::ScreenVertexShader, 
                                             ShaderSource::ScreenFragmentShader)) return false;

        skyboxShader = std::make_unique<Shader>();
        if (!skyboxShader->CompileFromSource(CustomShaders::SkyboxVertexShader, 
                                             CustomShaders::SkyboxFragmentShader)) return false;

        // Setup Renderer e Framebuffer
        renderer.Init(pbrShader.get());
        renderer.SetSkyboxShader(skyboxShader.get());

        fb = std::make_unique<FrameBuffer>(SCR_WIDTH, SCR_HEIGHT);
        fb->Init();

        std::cout << "=== ENGINE INICIALIZADA ===" << std::endl;
        std::cout << "[INPUT] WASD: Mover Câmera | M: Trocar Material | ESC: Sair" << std::endl;
        
        return true;
    }

    void LoadScene() {
        activeScene = std::make_unique<Scene>();

        auto goldMat = std::make_shared<Material>(MaterialLibrary::CreateGold());
        auto silverMat = std::make_shared<Material>(MaterialLibrary::CreateSilver());
        auto plasticMat = std::make_shared<Material>(MaterialLibrary::CreatePlastic());
        auto rubberMat = std::make_shared<Material>(MaterialLibrary::CreateRubber());
        auto neonMat = std::make_shared<Material>(MaterialLibrary::CreateEmissive(glm::vec3(0,1,1), 5.0f));

        availableMaterials = { goldMat, silverMat, plasticMat, rubberMat, neonMat };

        // 1. Criar Materiais (Poderia ser um AssetManager, mas vamos deixar aqui por enquanto)
        // auto goldMat = std::make_shared<Material>(MaterialLibrary::CreateGold());

        auto floorMat = std::make_shared<Material>(MaterialLibrary::CreatePhong(glm::vec3(0.1f, 0.1f, 0.1f)));

        // 2. Criar Entidade NAVE
        // playerShipEntity = activeScene->CreateEntity("PlayerShip");
        
        // // Adiciona componente de renderização (carrega o modelo)
        // auto model = std::make_shared<Model>("models/car/Intergalactic_Spaceship-(Wavefront).obj");
        // if (model->GetMeshCount() > 0) {
        //     availableMaterials.push_back(model->GetMesh(0).GetMaterial());
        // }

        // auto renderComp = playerShipEntity->AddComponent<MeshRenderer>(model);
        // renderComp->SetMaterial(availableMaterials[0]);

        // // Adiciona comportamento (Script)
        // playerShipEntity->AddComponent<RotatorScript>(glm::vec3(0.0f, 30.0f, 0.0f)); // Gira 30 graus/s no Y
        // playerShipEntity->transform.Scale = glm::vec3(0.5f);
        // playerShipEntity->transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);

        // // 3. Criar Entidade CHÃO
        // auto floorEntity = activeScene->CreateEntity("Floor");
        // auto floorMesh = std::make_shared<Mesh>(ModelFactory::CreatePlaneMesh(1.0f));
        // auto floorRender = floorEntity->AddComponent<SimpleMeshRenderer>(floorMesh);
        // floorRender->SetMaterial(floorMat);
        // floorEntity->transform.Scale = glm::vec3(20.0f);
        // floorEntity->transform.Position = glm::vec3(0.0f, -1.0f, 0.0f);
        
        auto sphereEntity = activeScene->CreateEntity("Sphere");
        auto spherMesh = std::make_shared<Mesh>(ModelFactory::CreateSphere(1.0f));
        sphereEntity->transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);
        auto redMeshComp = sphereEntity->AddComponent<SimpleMeshRenderer>(spherMesh);
        auto metalicMaterial = std::make_shared<Material>();
        metalicMaterial->SetAlbedo(glm::vec3(1.0f, 0.0f, 0.0f));
        metalicMaterial->SetMetallic(1.0f);
        metalicMaterial->SetRoughness(0.0f);
        redMeshComp->SetMaterial(metalicMaterial);

        // --- ILUMINAÇÃO ---

        envMap.LoadFromHDR("models/golden_gate_hills_8k.hdr");

        // 1. SOL (Directional Light)
        auto sunEntity = activeScene->CreateEntity("Sun");
        sunEntity->AddComponent<DirectionalLightComponent>(glm::vec3(1.0f, 0.95f, 0.8f), 2.0f); // Cor meio alaranjada, forte
        sunEntity->transform.Position = glm::vec3(5.0f, 10.0f, 5.0f); // Sol vem dessa posição apontando pro centro

        // 2. LUZ VERMELHA (Point Light 1)
        auto redLight = activeScene->CreateEntity("RedLight");
        // redLight->AddComponent<PointLightComponent>(glm::vec3(1.0f, 0.0f, 0.0f), 30.0f, 10.0f);
        redLight->transform.Position = glm::vec3(-2.0f, 1.0f, -2.0f);
        
        // Pequeno cubo para visualizar onde a luz está (Debug)
        // auto debugMeshRed = std::make_shared<Mesh>(ModelFactory::CreateSphere(0.1f));
        // auto redMeshComp = redLight->AddComponent<SimpleMeshRenderer>(debugMeshRed);
        // redMeshComp->SetMaterial(std::make_shared<Material>(MaterialLibrary::CreateEmissive(glm::vec3(1,0,0), 10.0f)));

        // 3. LUZ AZUL (Point Light 2) - Animada!
        auto blueLight = activeScene->CreateEntity("BlueLight");
        // blueLight->AddComponent<PointLightComponent>(glm::vec3(0.0f, 0.5f, 1.0f), 30.0f, 10.0f);
        blueLight->transform.Position = glm::vec3(2.0f, 1.0f, 0.0f);
        
        // Script para fazer a luz orbitar
        blueLight->AddComponent<RotatorScript>(glm::vec3(0, 50, 0)); // Isso gira o transform
        // Precisaríamos de um script "Orbiter" real para mover a posição, 
        // mas se a luz for filha de um objeto rotativo funcionaria. 
        // Como não temos hierarquia pai-filho ainda, vamos deixar estática ou usar Floater.
        blueLight->AddComponent<FloaterScript>(1.0f, 2.0f); // Luz subindo e descendo

        // Inicia todos os scripts
        activeScene->OnStart();
    }

    // 3. Processamento de Input e Lógica
    void Update(float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
            mKeyPressed = true;
            currentMaterialIndex = (currentMaterialIndex + 1) % availableMaterials.size();

            if(playerShipEntity) {
                auto rendererComponent = playerShipEntity->GetComponent<MeshRenderer>();
                if (rendererComponent) {
                    rendererComponent->SetMaterial(availableMaterials[currentMaterialIndex]);
                    std::cout << "[MATERIAL] Trocado para: " 
                              << availableMaterials[currentMaterialIndex]->GetName() << std::endl;
                }
            }
        }
        if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) mKeyPressed = false;
        
        // Câmera
        float speed = 2.5f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos.z -= speed;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos.z += speed;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos.x -= speed;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos.x += speed;
    
        if (activeScene) activeScene->OnUpdate(deltaTime);
    }

    // 4. Renderização (Pass 1 + Pass 2)
    void Render(float time) {
        // --- PASS 1: Scene -> Framebuffer ---
        fb->Bind();
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f); // Espaço escuro
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 100.0f);

        renderer.BeginScene(view, proj, cameraPos);

        if (activeScene) 
            activeScene->OnRender(renderer);

        renderer.EndScene();
        renderer.DrawSkybox(envMap.envCubemap, view, proj);

        fb->Unbind();
        renderer.DrawScreenQuad(*screenShader, fb->GetTexture());
    }

    // 5. Loop Principal
    void MainLoop() {
        float lastFrame = 0.0f;

        while (!glfwWindowShouldClose(window)) {
            float currentFrame = static_cast<float>(glfwGetTime());
            float deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;

            Update(deltaTime);
            Render(currentFrame);

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }
};

// ==========================================
// ENTRY POINT
// ==========================================
int main() {
    DemoApp app;
    app.Run();
    return 0;
}