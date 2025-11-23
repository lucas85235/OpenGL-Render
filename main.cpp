/*
 * EXEMPLO AVANÇADO - Sistema de Materiais Customizados
 * 
 * Este exemplo mostra como:
 * 1. Carregar modelos com materiais customizados
 * 2. Trocar materiais em runtime
 * 3. Usar materiais procedurais
 * 4. Aplicar texturas customizadas
 */

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>
#include <vector>

#include "src/renderer/shader.hpp"
#include "src/renderer/model.hpp"
#include "src/renderer/framebuffer.hpp"
#include "src/renderer/texture.hpp"
#include "src/renderer/material.hpp"
#include "src/renderer/custom_shaders.hpp"

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Câmera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Framebuffer
std::unique_ptr<FrameBuffer> fb;

// Controle de materiais
int currentMaterialIndex = 0;
std::vector<std::shared_ptr<Material>> materials;
bool keyPressed = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (fb) {
        fb->Resize(width, height);
    }
}

void processInput(GLFWwindow *window, Model* model) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Trocar material com tecla M
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !keyPressed) {
        keyPressed = true;
        currentMaterialIndex = (currentMaterialIndex + 1) % materials.size();
        
        // Aplicar novo material a todos os meshes
        model->SetMaterialAll(materials[currentMaterialIndex]);
        
        std::cout << "Material trocado para: " << materials[currentMaterialIndex]->GetName() << std::endl;
    }
    
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        keyPressed = false;
    }
}

int main() {
    // Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "Falha ao inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, 
                                          "Advanced Materials - OpenGL", NULL, NULL);
    if (!window) {
        std::cerr << "Falha ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // Inicializar GLEW
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Falha ao inicializar GLEW" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Compilar shader PBR customizado
    Shader pbrShader;
    if (!pbrShader.CompileFromSource(CustomShaders::AdvancedVertexShader,
                                     CustomShaders::CustomMaterialFragmentShader)) {
        std::cerr << "Falha ao compilar shader PBR!" << std::endl;
        glfwTerminate();
        return -1;
    }

    Shader screenShader;
    if (!screenShader.CompileFromSource(ShaderSource::ScreenVertexShader,
                                         ShaderSource::ScreenFragmentShader)) {
        std::cerr << "Falha ao compilar shader da tela!" << std::endl;
        glfwTerminate();
        return -1;
    }

    std::cout << "Shaders compilados com sucesso!" << std::endl;

    // Carregar modelo com materiais customizados
    std::cout << "\nCarregando modelo..." << std::endl;
    std::unique_ptr<Model> model;
    try {
        model = std::make_unique<Model>("models/car/Intergalactic_Spaceship-(Wavefront).obj");
    } catch (const std::exception& e) {
        std::cerr << "Erro ao carregar modelo: " << e.what() << std::endl;
        glfwTerminate();
        return -1;
    }

    // Criar biblioteca de materiais para trocar
    std::cout << "\n=== CRIANDO MATERIAIS ===" << std::endl;
    
    // Material 1: Ouro
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateGold()));
    std::cout << "Material 1: Ouro" << std::endl;
    
    // Material 2: Prata
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateSilver()));
    std::cout << "Material 2: Prata" << std::endl;
    
    // Material 3: Cobre
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateCopper()));
    std::cout << "Material 3: Cobre" << std::endl;
    
    // Material 4: Plástico vermelho
    auto plasticMaterial = std::make_shared<Material>(MaterialLibrary::CreatePlastic());
    plasticMaterial->SetAlbedo(glm::vec3(1.0f, 0.0f, 0.0f));
    plasticMaterial->SetName("Plástico Vermelho");
    materials.push_back(plasticMaterial);
    std::cout << "Material 4: Plástico Vermelho" << std::endl;
    
    // Material 5: Borracha preta
    materials.push_back(std::make_shared<Material>(MaterialLibrary::CreateRubber()));
    std::cout << "Material 5: Borracha" << std::endl;
    
    // Material 6: Emissivo verde
    auto emissiveMaterial = std::make_shared<Material>(
        MaterialLibrary::CreateEmissive(glm::vec3(0.0f, 1.0f, 0.0f), 3.0f)
    );
    emissiveMaterial->SetName("Neon Verde");
    materials.push_back(emissiveMaterial);
    std::cout << "Material 6: Neon Verde" << std::endl;
    
    // Material 7: Original do modelo (com texturas)
    if (model->GetMeshCount() > 0) {
        auto originalMaterial = model->GetMesh(0).GetMaterial();
        if (originalMaterial) {
            originalMaterial->SetName("Original (Texturas)");
            materials.push_back(originalMaterial);
            std::cout << "Material 7: Original (Texturas)" << std::endl;
        }
    }

    // Criar quad para framebuffer
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "\n=== CONTROLES ===" << std::endl;
    std::cout << "M   - Trocar material" << std::endl;
    std::cout << "ESC - Sair" << std::endl;
    std::cout << "==================\n" << std::endl;

    // Inicializar framebuffer
    fb = std::make_unique<FrameBuffer>(SCR_WIDTH, SCR_HEIGHT);
    if (!fb->Init()) {
        std::cerr << "Falha ao inicializar framebuffer!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Aplicar primeiro material
    model->SetMaterialAll(materials[currentMaterialIndex]);
    std::cout << "Material inicial: " << materials[currentMaterialIndex]->GetName() << std::endl;

    // Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window, model.get());

        // PASSO 1: Renderizar no framebuffer
        fb->Bind();
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        pbrShader.Use();

        // Matrizes
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);
        pbrShader.SetMat4("view", glm::value_ptr(view));
        pbrShader.SetMat4("projection", glm::value_ptr(projection));

        glm::mat4 modelMatrix = glm::mat4(0.4f);
        modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * 0.3f, 
                                  glm::vec3(0.0f, 1.0f, 0.0f));
        pbrShader.SetMat4("model", glm::value_ptr(modelMatrix));

        // Iluminação
        pbrShader.SetVec3("lightPos", 3.0f, 3.0f, 3.0f);
        pbrShader.SetVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        pbrShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);

        // Configurar flags de textura
        if (model->GetMeshCount() > 0) {
            auto mat = model->GetMesh(0).GetMaterial();
            if (mat) {
                pbrShader.SetBool("hasTextureDiffuse", mat->HasTextureType(TextureType::DIFFUSE));
                pbrShader.SetBool("hasTextureNormal", mat->HasTextureType(TextureType::NORMAL));
                pbrShader.SetBool("hasTextureMetallic", mat->HasTextureType(TextureType::METALLIC));
                pbrShader.SetBool("hasTextureRoughness", mat->HasTextureType(TextureType::ROUGHNESS));
                pbrShader.SetBool("hasTextureAO", mat->HasTextureType(TextureType::AO));
                pbrShader.SetBool("hasTextureEmission", mat->HasTextureType(TextureType::EMISSION));
            }
        }

        model->Draw(pbrShader.GetProgramID());

        // PASSO 2: Renderizar na tela
        fb->Unbind();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.Use();
        screenShader.SetInt("screenTexture", 0);
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, fb->GetTexture());
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    std::cout << "Encerrando aplicação..." << std::endl;
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    
    glfwTerminate();
    return 0;
}