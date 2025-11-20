#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cmath>
#include <memory>
#include "framebuffer.hpp"
#include "shader.hpp"

// Variável global para o framebuffer (para redimensionar)
std::unique_ptr<FrameBuffer> g_framebuffer;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    if (g_framebuffer) {
        g_framebuffer->Resize(width, height);
    }
}

void processInput(GLFWwindow* window) {
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
    
    GLFWwindow* window = glfwCreateWindow(800, 600, "Framebuffer OpenGL", NULL, NULL);
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
    
    // Compilar shaders usando a classe Shader
    Shader cubeShader;
    if (!cubeShader.CompileFromSource(ShaderSource::CubeVertexShader, 
                                       ShaderSource::CubeFragmentShader)) {
        std::cerr << "Falha ao compilar shader do cubo!" << std::endl;
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
    
    // Vértices do cubo
    float cubeVertices[] = {
        // Posições          // Texturas
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
        
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    
    // Quad da tela (cobre toda a tela)
    float quadVertices[] = {
        // Posições  // Texturas
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    
    // Setup VAO/VBO do cubo
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // Setup VAO/VBO do quad
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
    
    // Inicializar framebuffer
    g_framebuffer = std::make_unique<FrameBuffer>(800, 600);
    if (!g_framebuffer->Init()) {
        std::cerr << "Falha ao inicializar framebuffer!" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    std::cout << "Iniciando loop de renderização..." << std::endl;
    
    // Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        
        // ==========================================
        // PASSO 1: Renderizar cena no framebuffer
        // ==========================================
        g_framebuffer->Bind();
        
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        cubeShader.Use();
        
        // Matrizes de transformação
        float time = glfwGetTime();
        float model[16] = {
            static_cast<float>(cos(time)), 0, static_cast<float>(sin(time)), 0,
            0, 1, 0, 0,
            static_cast<float>(-sin(time)), 0, static_cast<float>(cos(time)), 0,
            0, 0, 0, 1
        };
        
        float view[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, -3, 1
        };
        
        float aspect = 800.0f / 600.0f;
        float fov = 45.0f * 3.14159f / 180.0f;
        float f = 1.0f / tan(fov / 2.0f);
        float projection[16] = {
            f/aspect, 0, 0, 0,
            0, f, 0, 0,
            0, 0, -1.002f, -1,
            0, 0, -0.2002f, 0
        };
        
        cubeShader.SetMat4("model", model);
        cubeShader.SetMat4("view", view);
        cubeShader.SetMat4("projection", projection);
        
        glBindVertexArray(cubeVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ==========================================
        // PASSO 2: Renderizar textura na tela
        // ==========================================
        g_framebuffer->Unbind();
        
        // Resetar viewport para o tamanho da janela
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        
        screenShader.Use();
        screenShader.SetInt("screenTexture", 0);
        
        glBindVertexArray(quadVAO);
        glBindTexture(GL_TEXTURE_2D, g_framebuffer->GetTexture());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Limpeza
    std::cout << "Encerrando aplicação..." << std::endl;
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteBuffers(1, &quadVBO);
    
    // Os shaders e framebuffer serão limpos automaticamente pelos destrutores
    g_framebuffer.reset();
    
    glfwTerminate();
    return 0;
}