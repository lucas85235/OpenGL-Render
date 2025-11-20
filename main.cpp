#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <memory>

#include "shader.hpp"
#include "model.hpp"

// Configurações
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;

// Câmera
glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, -3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;

// Timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    return;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraPos += cameraFront * static_cast<float>(yoffset) * 0.5f;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // float cameraSpeed = 2.5f * deltaTime;
    // if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    //     cameraPos += cameraSpeed * cameraFront;
    // if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    //     cameraPos -= cameraSpeed * cameraFront;
    // if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    //     cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    // if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    //     cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    // if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    //     cameraPos += cameraSpeed * cameraUp;
    // if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    //     cameraPos -= cameraSpeed * cameraUp;
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
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Capturar o mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

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
    
    // Tente carregar um modelo (ajuste o caminho conforme necessário)
    std::unique_ptr<Model> model;
    try {
        // Tente carregar backpack (modelo comum nos tutoriais)
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
    std::cout << "WASD - Mover câmera" << std::endl;
    std::cout << "Mouse - Olhar ao redor" << std::endl;
    std::cout << "Scroll - Zoom in/out" << std::endl;
    std::cout << "Space - Subir" << std::endl;
    std::cout << "Shift - Descer" << std::endl;
    std::cout << "ESC - Sair" << std::endl;
    std::cout << "==================\n" << std::endl;

    // Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        // Frame timing
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Input
        processInput(window);

        // Renderizar
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
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

        // Desenhar modelo
        model->Draw(modelShader.GetProgramID());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Limpeza
    std::cout << "Encerrando aplicação..." << std::endl;
    glfwTerminate();
    return 0;
}
