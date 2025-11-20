#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>

#include "src/renderer/shader.hpp"
#include "src/renderer/model.hpp"
#include "src/renderer/framebuffer.hpp"

// Configurações
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

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (fb) {
        fb->Resize(width, height);
    }
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
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
    glfwWindowHint(GLFW_SAMPLES, 4); // Anti-aliasing

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, 
                                          "Model Loading - OpenGL", NULL, NULL);
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

    // Configurar OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);

    // Compilar shaders
    Shader modelShader;
    if (!modelShader.CompileFromSource(ShaderSource::ModelVertexShader,
                                        ShaderSource::ModelFragmentShader)) {
        std::cerr << "Falha ao compilar shader do modelo!" << std::endl;
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

    // Carregar modelo
    std::cout << "\nCarregando modelo..." << std::endl;
    std::cout << "IMPORTANTE: Coloque seu modelo .obj na pasta 'models/'" << std::endl;
    std::cout << "Exemplo: models/backpack/backpack.obj" << std::endl;

    std::unique_ptr<Model> model;
    try {
        model = std::make_unique<Model>("models/car/Intergalactic_Spaceship-(Wavefront).obj");
    } catch (const std::exception& e) {
        std::cerr << "Erro ao carregar modelo: " << e.what() << std::endl;
        std::cout << "\nNão foi possível carregar o modelo." << std::endl;
        std::cout << "Baixe um modelo .obj e coloque em 'models/'" << std::endl;
        std::cout << "Sugestões de modelos gratuitos:" << std::endl;
        std::cout << "- https://sketchfab.com/feed (filtrar por downloadable)" << std::endl;
        std::cout << "- https://free3d.com/" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Criar quad para renderizar a textura do framebuffer
    float quadVertices[] = {
        // Posições  // TexCoords
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
    std::cout << "ESC - Sair" << std::endl;
    std::cout << "==================\n" << std::endl;

    // Inicializar framebuffer
    fb = std::make_unique<FrameBuffer>(SCR_WIDTH, SCR_HEIGHT);
    if (!fb->Init()) {
        std::cerr << "Falha ao inicializar framebuffer!" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        // Frame timing
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // ==========================================
        // PASSO 1: Renderizar modelo no framebuffer
        // ==========================================
        fb->Bind();
        glEnable(GL_DEPTH_TEST);

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Ativar shader do modelo
        modelShader.Use();

        // Matrizes de view e projeção
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);

        modelShader.SetMat4("view", glm::value_ptr(view));
        modelShader.SetMat4("projection", glm::value_ptr(projection));

        // Matriz de modelo (escala e posição)
        glm::mat4 modelMatrix = glm::mat4(0.5f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
        modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * 0.3f, 
                                  glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.SetMat4("model", glm::value_ptr(modelMatrix));

        // Configurar iluminação
        modelShader.SetVec3("lightPos", 1.2f, 1.0f, 2.0f);
        modelShader.SetVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        modelShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        modelShader.SetVec3("objectColor", 0.2f, 1.0f, 0.2f);

        // Desenhar modelo NO FRAMEBUFFER
        model->Draw(modelShader.GetProgramID());

        // ==========================================
        // PASSO 2: Renderizar framebuffer na tela
        // ==========================================
        fb->Unbind();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Ativar shader da tela
        screenShader.Use();
        screenShader.SetInt("screenTexture", 0);

        // Desenhar quad com a textura do framebuffer
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, fb->GetTexture());
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    std::cout << "Encerrando aplicação..." << std::endl;
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    
    glfwTerminate();
    return 0;
}
