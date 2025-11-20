#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>

#include "shader.hpp"
#include "model.hpp"
#include "framebuffer.hpp"

// Configurações
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Câmera
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, -3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Framebuffer
std::unique_ptr<FrameBuffer> fb;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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

    // Capturar o mouse (opcional)
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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

    std::cout << "Shaders compilados com sucesso!" << std::endl;

    // Carregar modelo
    std::cout << "\nCarregando modelo..." << std::endl;
    std::cout << "IMPORTANTE: Coloque seu modelo .obj na pasta 'models/'" << std::endl;
    std::cout << "Exemplo: models/backpack/backpack.obj" << std::endl;

    std::unique_ptr<Model> model;
    try {
        model = std::make_unique<Model>("models/backpack/backpack.obj");
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

    std::cout << "\n=== CONTROLES ===" << std::endl;
    std::cout << "ESC - Sair" << std::endl;
    std::cout << "==================\n" << std::endl;

    fb = std::make_unique<FrameBuffer>();
    fb->Init();
    glEnable(GL_DEPTH_TEST);

    // Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        // Frame timing
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Bind framebuffer
        fb->Bind();
        glEnable(GL_DEPTH_TEST);

        // Renderizar
        // glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Ativar shader
        modelShader.Use();

        // Matrizes de view e projeção
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
                                                (float)SCR_WIDTH / (float)SCR_HEIGHT,
                                                0.1f, 100.0f);

        modelShader.SetMat4("view", glm::value_ptr(view));
        modelShader.SetMat4("projection", glm::value_ptr(projection));

        // Matriz de modelo (escala e posição)
        glm::mat4 modelMatrix = glm::mat4(0.1f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f, 1.0f, 1.0f));
        modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * 0.3f, 
                                  glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.SetMat4("model", glm::value_ptr(modelMatrix));

        // Configurar iluminação
        modelShader.SetVec3("lightPos", 1.2f, 1.0f, 2.0f);
        modelShader.SetVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        modelShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        modelShader.SetVec3("objectColor", 1.0f, 0.5f, 0.31f);

        // Bind framebuffer
        fb->Unbind();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);

        // Desenhar modelo
        model->Draw(modelShader.GetProgramID());

        glBindTexture(GL_TEXTURE_2D, fb->GetTexture());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    std::cout << "Encerrando aplicação..." << std::endl;
    glfwTerminate();
    return 0;
}
