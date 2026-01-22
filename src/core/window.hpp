#ifndef WINDOW_HPP
#define WINDOW_HPP

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <functional>
#include <iostream>
#include <string>

class Window {
private:
  GLFWwindow *handle;
  int width;
  int height;
  std::string title;
  bool hasGLContext = false;

  // Mouse state
  double lastMouseX = 0.0;
  double lastMouseY = 0.0;
  double mouseX = 0.0;
  double mouseY = 0.0;
  bool firstMouse = true;

  // Callbacks
  std::function<void(int, int)> resizeCallback;
  std::function<void(double, double)> mouseMoveCallback;
  std::function<void(double)> scrollCallback;

  static void FramebufferSizeCallback(GLFWwindow *window, int w, int h) {
    Window *win = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (win) {
      if (win->hasGLContext) {
        glViewport(0, 0, w, h);
      }
      win->width = w;
      win->height = h;
      if (win->resizeCallback)
        win->resizeCallback(w, h);
    }
  }

  static void MouseCallback(GLFWwindow *window, double xpos, double ypos) {
    Window *win = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (win) {
      if (win->firstMouse) {
        win->lastMouseX = xpos;
        win->lastMouseY = ypos;
        win->firstMouse = false;
      }
      win->mouseX = xpos;
      win->mouseY = ypos;

      double xOffset = xpos - win->lastMouseX;
      double yOffset = win->lastMouseY - ypos; // Inverted: y goes bottom to top

      win->lastMouseX = xpos;
      win->lastMouseY = ypos;

      if (win->mouseMoveCallback)
        win->mouseMoveCallback(xOffset, yOffset);
    }
  }

  static void ScrollCallback(GLFWwindow *window, double xoffset,
                             double yoffset) {
    Window *win = static_cast<Window *>(glfwGetWindowUserPointer(window));
    if (win && win->scrollCallback) {
      win->scrollCallback(yoffset);
    }
  }

public:
  Window(int w, int h, const std::string &t)
      : handle(nullptr), width(w), height(h), title(t) {}

  ~Window() {
    if (handle) {
      glfwDestroyWindow(handle);
    }
    glfwTerminate();
  }

  bool Init(bool createGLContext = true) {
    if (!glfwInit()) {
      std::cerr << "Falha ao iniciar GLFW" << std::endl;
      return false;
    }

    if (createGLContext) {
      glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
      glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
      glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    } else {
      glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    handle = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
    if (!handle) {
      std::cerr << "Falha ao criar janela GLFW" << std::endl;
      glfwTerminate();
      return false;
    }

    // Ponteiro para "this" para usar nos callbacks
    glfwSetWindowUserPointer(handle, this);
    glfwSetFramebufferSizeCallback(handle, FramebufferSizeCallback);
    glfwSetCursorPosCallback(handle, MouseCallback);
    glfwSetScrollCallback(handle, ScrollCallback);

    if (createGLContext) {
      glfwMakeContextCurrent(handle);

      // GLEW (precisa de um contexto ativo antes)
      glewExperimental = GL_TRUE;
      if (glewInit() != GLEW_OK) {
        std::cerr << "Falha ao iniciar GLEW" << std::endl;
        return false;
      }
      hasGLContext = true; // Set flag if GL context is created
    }

    return true;
  }

  void OnUpdate() {
    if (hasGLContext) { // Only swap buffers if a GL context exists
      glfwSwapBuffers(handle);
    }
    glfwPollEvents();
  }

  bool ShouldClose() const { return glfwWindowShouldClose(handle); }

  void SetResizeCallback(const std::function<void(int, int)> &callback) {
    resizeCallback = callback;
  }

  void
  SetMouseMoveCallback(const std::function<void(double, double)> &callback) {
    mouseMoveCallback = callback;
  }

  void SetScrollCallback(const std::function<void(double)> &callback) {
    scrollCallback = callback;
  }

  void SetCursorMode(bool captured) {
    glfwSetInputMode(handle, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (!captured) {
      firstMouse = true; // Reset on release
    }
  }

  bool IsCursorCaptured() const {
    return glfwGetInputMode(handle, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;
  }

  void GetMousePosition(double &x, double &y) const {
    x = mouseX;
    y = mouseY;
  }

  bool IsMouseButtonPressed(int button) const {
    return glfwGetMouseButton(handle, button) == GLFW_PRESS;
  }

  // Input Helpers
  bool IsKeyPressed(int key) const {
    return glfwGetKey(handle, key) == GLFW_PRESS;
  }

  bool IsKeyReleased(int key) const {
    return glfwGetKey(handle, key) == GLFW_RELEASE;
  }

  void Close() { glfwSetWindowShouldClose(handle, true); }

  // Getters
  GLFWwindow *GetNativeWindow() const { return handle; }
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  float GetAspect() const { return (float)width / (float)height; }
};

#endif // WINDOW_HPP
