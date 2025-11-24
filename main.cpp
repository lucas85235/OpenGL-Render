#define STB_IMAGE_IMPLEMENTATION
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>
#include <vector>
#include <cmath>

// Includes do sistema
#include "src/renderer/shader.hpp"
#include "src/renderer/model.hpp"
#include "src/renderer/custom_shaders.hpp"
#include "src/renderer/renderer.hpp"
#include "src/renderer/framebuffer.hpp"
#include "src/renderer/texture.hpp"
#include "src/renderer/material.hpp"
#include "src/renderer/model_factory.hpp"

// Configurações
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Globais
Renderer renderer;
std::unique_ptr<FrameBuffer> fb;
glm::vec3 cameraPos = glm::vec3(0.0f, 2.0f, 6.0f);

// Controle de Input/Materiais
std::vector<std::shared_ptr<Material>> materials;
int currentMaterialIndex = 0;
bool mKeyPressed = false;

// --- LOGIC ---

void processInput(GLFWwindow *window, Model* model) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        std::cout << "[INPUT] ESC pressionado. Encerrando." << std::endl;
        glfwSetWindowShouldClose(window, true);
    }
    
    // Troca de Material (M)
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
        mKeyPressed = true;
        currentMaterialIndex = (currentMaterialIndex + 1) % materials.size();
        
        // Atualiza material
        if(model) model->SetMaterialAll(materials[currentMaterialIndex]);
        
        std::cout << "[INPUT] Troca de Material -> " << materials[currentMaterialIndex]->GetName() << std::endl;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) mKeyPressed = false;
    
    // Controles de Câmera (Simples)
    float speed = 2.5f * 0.016f; // DeltaTime fixo aprox
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos.z -= speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos.z += speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos.x -= speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos.x += speed;
}

void setupMaterials(const std::unique_ptr<Model>& model) {
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateGold()));
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateSilver()));
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreatePlastic())); // Vermelho
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateRubber())); // Preto
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateEmissive(glm::vec3(0,1,1), 5.0f))); // Cyan Neon

    if (model->GetMeshCount() > 0) {
        materials.push_back(model->GetMesh(0).GetMaterial());
    }
}

int main() {
    // 1. Inicialização
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Engine: Render Pass & Framebuffer", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    // 2. Setup Renderer e Framebuffer
    Shader pbrShader;
    if (!pbrShader.CompileFromSource(CustomShaders::AdvancedVertexShader, 
                                     CustomShaders::CustomMaterialFragmentShader)) return -1;

    Shader screenShader;
    if (!screenShader.CompileFromSource(ShaderSource::ScreenVertexShader, 
                                        ShaderSource::ScreenFragmentShader)) return -1;

    renderer.Init(&pbrShader);
    
    fb = std::make_unique<FrameBuffer>(SCR_WIDTH, SCR_HEIGHT);
    fb->Init();

    // 3. Carregar Assets
    std::unique_ptr<Model> shipModel = std::make_unique<Model>("models/car/Intergalactic_Spaceship-(Wavefront).obj");
    Mesh floorMesh = ModelFactory::CreatePlaneMesh(1.0f);
    auto floorMat = std::make_shared<Material>(MaterialLibrary::CreateRubber());
    floorMat->SetAlbedo(glm::vec3(1.0f, 1.0f, 1.0f)); 
    floorMesh.SetMaterial(floorMat);

    setupMaterials(shipModel);
    if(shipModel) shipModel->SetMaterialAll(materials[0]);

    std::cout << "=== ENGINE PRONTA ===" << std::endl;
    std::cout << "[INPUT] WASD para mover camera, M para trocar material" << std::endl;

    // 4. Render Loop
    while (!glfwWindowShouldClose(window)) {
        processInput(window, shipModel.get());

        // --- PASS 1: Renderizar Cena para Framebuffer ---
        fb->Bind();
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Cor de fundo do espaço
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // A. Begin Scene
        glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0,0,0), glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/SCR_HEIGHT, 0.1f, 100.0f);
        renderer.BeginScene(view, proj, cameraPos);
        
        // Nave
        if (shipModel) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::rotate(model, (float)glfwGetTime() * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(0.5f));
            renderer.Submit(shipModel, model);
        }

        
        glm::mat4 floor = glm::mat4(1.0f);
        floor = glm::rotate(floor, (float)glfwGetTime() * 0.3f, glm::vec3(0.0f, 1.0f, 0.0f));
        floor = glm::scale(floor, glm::vec3(10.0f));
        renderer.SubmitMesh(floorMesh, floor); // Chão no centro

        // C. End Scene
        renderer.EndScene();
        
        // --- PASS 2: Renderizar Quad na Tela (Post-Process) ---
        fb->Unbind();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        renderer.DrawScreenQuad(screenShader, fb->GetTexture());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // TextureManager::GetInstance().ClearCache();
    glfwTerminate();
    return 0;
};
