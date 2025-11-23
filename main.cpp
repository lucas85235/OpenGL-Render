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
#include "src/renderer/texture.hpp" // Para limpar cache ao sair, se quiser

// Configurações
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Câmera
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f); // Afastei um pouco a câmera
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Globais
std::unique_ptr<FrameBuffer> fb;
bool wireframe = false;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (fb) fb->Resize(width, height);
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    
    // Toggle wireframe (P)
    static bool pPressed = false;
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !pPressed) {
        wireframe = !wireframe;
        glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
        pPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) {
        pPressed = false;
    }
}

int main() {
    // 1. Inicialização (GLFW/GLEW)
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Advanced Renderer", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_CULL_FACE); // Opcional: Performance

    // 2. Shaders
    Shader modelShader;
    if (!modelShader.CompileFromSource(ShaderSource::ModelVertexShader, ShaderSource::ModelFragmentShader)) return -1;

    Shader screenShader;
    if (!screenShader.CompileFromSource(ShaderSource::ScreenVertexShader, ShaderSource::ScreenFragmentShader)) return -1;

    // 3. Carregar Modelo
    std::unique_ptr<Model> model;
    try {
        // Tente carregar seu modelo aqui
        model = std::make_unique<Model>("models/others/texture_cube.fbx");
    } catch (...) {
        std::cerr << "Erro fatal carregando modelo." << std::endl;
        return -1;
    }

    // 4. Configuração do Quad (Tela)
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

    fb = std::make_unique<FrameBuffer>(SCR_WIDTH, SCR_HEIGHT);
    fb->Init();

    // 5. Loop Principal
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // --- Pass 1: Renderizar no Framebuffer ---
        fb->Bind();
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        modelShader.Use();

        // Transforms
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        modelShader.SetMat4("projection", glm::value_ptr(projection));
        modelShader.SetMat4("view", glm::value_ptr(view));

        // Iluminação Global (O material cuida da reação à luz, mas a luz precisa existir)
        modelShader.SetVec3("lightPos", 2.0f, 4.0f, 3.0f);
        modelShader.SetVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        modelShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);

        // Renderizar Modelo
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, glm::vec3(0.0f, -0.5f, 0.0f));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f));
        modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
        modelShader.SetMat4("model", glm::value_ptr(modelMatrix));

        // O draw agora chama material->Apply() internamente
        model->Draw(modelShader.GetProgramID());

        // --- Pass 2: Renderizar na Tela ---
        fb->Unbind();
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader.Use();
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, fb->GetTexture());
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    TextureManager::GetInstance().ClearCache();
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glfwTerminate();
    return 0;
}
