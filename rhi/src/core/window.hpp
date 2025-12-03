#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <iostream>
#include <functional>

class Window {
private:
    GLFWwindow* handle;
    int width;
    int height;
    std::string title;

    // Callback para notificar a Application sobre resize
    std::function<void(int, int)> resizeCallback;

    // Função estática para o GLFW chamar
    static void FramebufferSizeCallback(GLFWwindow* window, int w, int h) {
        glViewport(0, 0, w, h);
        
        // Recupera o ponteiro da nossa classe Window
        Window* win = static_cast<Window*>(glfwGetWindowUserPointer(window));
        if (win) {
            win->width = w;
            win->height = h;
            // Chama a função da Application se existir
            if (win->resizeCallback) win->resizeCallback(w, h);
        }
    }

public:
    Window(int w, int h, const std::string& t) 
        : handle(nullptr), width(w), height(h), title(t) {}

    ~Window() {
        if (handle) {
            glfwDestroyWindow(handle);
        }
        glfwTerminate();
    }

    bool Init() {
        if (!glfwInit()) {
            std::cerr << "Falha ao iniciar GLFW" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        handle = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
        if (!handle) {
            std::cerr << "Falha ao criar janela GLFW" << std::endl;
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(handle);
        
        // Ponteiro para "this" para usar nos callbacks
        glfwSetWindowUserPointer(handle, this);
        glfwSetFramebufferSizeCallback(handle, FramebufferSizeCallback);

        // GLEW (precisa de um contexto ativo antes)
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK) {
            std::cerr << "Falha ao iniciar GLEW" << std::endl;
            return false;
        }

        return true;
    }

    void OnUpdate() {
        glfwSwapBuffers(handle);
        glfwPollEvents();
    }

    bool ShouldClose() const {
        return glfwWindowShouldClose(handle);
    }

    void SetResizeCallback(const std::function<void(int, int)>& callback) {
        resizeCallback = callback;
    }

    // Input Helpers
    bool IsKeyPressed(int key) const {
        return glfwGetKey(handle, key) == GLFW_PRESS;
    }
    
    bool IsKeyReleased(int key) const {
        return glfwGetKey(handle, key) == GLFW_RELEASE;
    }

    void Close() {
        glfwSetWindowShouldClose(handle, true);
    }

    // Getters
    GLFWwindow* GetNativeWindow() const { return handle; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    float GetAspect() const { return (float)width / (float)height; }
};

#endif // WINDOW_HPP
