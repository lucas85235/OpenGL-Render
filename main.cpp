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

// Configurações da Janela
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
const char* APP_TITLE = "Engine: Render Pass & Framebuffer";

// ==========================================
// CLASSE PRINCIPAL DA APLICAÇÃO
// ==========================================
class DemoApp {
private:
    // Janela e Contexto
    GLFWwindow* window = nullptr;

    // Sistemas Core
    Renderer renderer;
    std::unique_ptr<FrameBuffer> fb;
    
    // Shaders
    std::unique_ptr<Shader> pbrShader;
    std::unique_ptr<Shader> screenShader;

    // Assets da Cena
    std::unique_ptr<Scene> activeScene;

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

        // Setup Renderer e Framebuffer
        renderer.Init(pbrShader.get());
        fb = std::make_unique<FrameBuffer>(SCR_WIDTH, SCR_HEIGHT);
        fb->Init();

        std::cout << "=== ENGINE INICIALIZADA ===" << std::endl;
        std::cout << "[INPUT] WASD: Mover Câmera | M: Trocar Material | ESC: Sair" << std::endl;
        
        return true;
    }

    void LoadScene() {
        activeScene = std::make_unique<Scene>();

        // 1. Criar Materiais (Poderia ser um AssetManager, mas vamos deixar aqui por enquanto)
        auto goldMat = std::make_shared<Material>(MaterialLibrary::CreateGold());
        auto floorMat = std::make_shared<Material>(MaterialLibrary::CreatePhong(glm::vec3(0.1f, 0.1f, 0.1f)));
        // floorMat->SetAlbedo(glm::vec3(0.5f));

        // 2. Criar Entidade NAVE
        auto shipEntity = activeScene->CreateEntity("PlayerShip");
        
        // Adiciona componente de renderização (carrega o modelo)
        auto model = std::make_shared<Model>("models/car/Intergalactic_Spaceship-(Wavefront).obj");
        auto renderComp = shipEntity->AddComponent<MeshRenderer>(model);
        renderComp->SetMaterial(goldMat);

        // Adiciona comportamento (Script)
        shipEntity->AddComponent<RotatorScript>(glm::vec3(0.0f, 30.0f, 0.0f)); // Gira 30 graus/s no Y
        
        // Ajusta posição inicial
        shipEntity->transform.Scale = glm::vec3(0.5f);
        shipEntity->transform.Position = glm::vec3(0.0f, 0.0f, 0.0f);

        // 3. Criar Entidade CHÃO
        auto floorEntity = activeScene->CreateEntity("Floor");
        
        auto floorMesh = std::make_shared<Mesh>(ModelFactory::CreatePlaneMesh(1.0f));
        auto floorRender = floorEntity->AddComponent<SimpleMeshRenderer>(floorMesh);
        floorRender->SetMaterial(floorMat);

        floorEntity->transform.Scale = glm::vec3(20.0f);
        floorEntity->transform.Position = glm::vec3(0.0f, -1.0f, 0.0f);
        
        // Inicia todos os scripts
        activeScene->OnStart();
    }

    // 3. Processamento de Input e Lógica
    void Update(float deltaTime) {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Troca de Material
        // if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
        //     mKeyPressed = true;
        //     currentMaterialIndex = (currentMaterialIndex + 1) % materials.size();
            
        //     if(shipModel) {
        //         shipModel->SetMaterialAll(materials[currentMaterialIndex]);
        //         std::cout << "[MATERIAL] " << materials[currentMaterialIndex]->GetName() << std::endl;
        //     }
        // }
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
        fb->Unbind();

        // --- PASS 2: Framebuffer -> Screen Quad ---
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
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