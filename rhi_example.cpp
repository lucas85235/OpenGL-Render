#include "src/core/window.hpp"
#include "src/renderer/model.hpp"
#include "src/renderer/renderer.hpp"
#include "src/rhi/opengl_device.hpp"
#include "src/rhi/vulkan_device.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>

namespace RHI {
std::unique_ptr<IDevice> DeviceFactory::Create(API api) {
  switch (api) {
  case API::OpenGL:
    return std::make_unique<OpenGLDevice>();
  case API::Vulkan:
    return std::make_unique<VulkanDevice>();
  default:
    return nullptr;
  }
}
} // namespace RHI

class RHIExampleApp {
private:
  std::unique_ptr<Window> window;
  std::unique_ptr<RHI::IDevice> device;

  Renderer renderer;
  std::unique_ptr<Shader> shader;
  std::shared_ptr<Model> model;

  glm::vec3 cameraPos = glm::vec3(0.0f, 1.0f, 3.0f);
  float lastTime = 0.0f;

public:
  RHIExampleApp(const std::string &title, int width, int height) {
    window = std::make_unique<Window>(width, height, title);
  }

  bool Init() {
    bool useVulkan = true;

    if (!window->Init(!useVulkan))
      return false;

    device = RHI::DeviceFactory::Create(useVulkan ? RHI::API::Vulkan
                                                  : RHI::API::OpenGL);
    if (!device)
      return false;

    if (useVulkan) {
      auto *vkDevice = dynamic_cast<RHI::VulkanDevice *>(device.get());
      if (vkDevice)
        vkDevice->SetWindow(window->GetNativeWindow());
    }

    if (!device->Initialize())
      return false;

    auto info = device->GetDeviceInfo();
    std::cout << "[RHI] Renderer: " << info.rendererName << std::endl;
    std::cout << "[RHI] Version: " << info.apiVersion << std::endl;

    // Create shader
    shader = std::make_unique<Shader>(device.get());

    const char *vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 fragColor;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    fragColor = aNormal * 0.5 + 0.5;
}
)";

    const char *fragmentShader = R"(
#version 330 core
in vec3 fragColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(fragColor, 1.0);
}
)";

    if (!shader->CompileFromSource(vertexShader, fragmentShader)) {
      std::cerr << "[App] Failed to compile shader" << std::endl;
      return false;
    }

    // Initialize renderer with device and shader
    renderer.Init(device.get(), shader.get());

    // Initialize texture manager for model loading
    TextureManager::GetInstance().SetDevice(device.get());

    // Load DamagedHelmet model
    model = std::make_shared<Model>(device.get(),
                                    "models/DamagedHelmet/DamagedHelmet.glb");

    std::cout << "[App] Initialization complete!" << std::endl;
    return true;
  }

  void Run() {
    if (!Init())
      return;

    std::cout << "[App] Entering render loop..." << std::endl;
    std::cout << "[App] Controls: WASD to move camera, ESC to exit"
              << std::endl;

    while (!window->ShouldClose()) {
      float currentTime = static_cast<float>(glfwGetTime());
      float deltaTime = currentTime - lastTime;
      lastTime = currentTime;

      ProcessInput(deltaTime);
      Update(deltaTime);
      Render();

      window->OnUpdate();
    }

    Shutdown();
  }

private:
  void ProcessInput(float dt) {
    float speed = 2.5f * dt;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_W) == GLFW_PRESS)
      cameraPos.z -= speed;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_S) == GLFW_PRESS)
      cameraPos.z += speed;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_A) == GLFW_PRESS)
      cameraPos.x -= speed;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_D) == GLFW_PRESS)
      cameraPos.x += speed;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_SPACE) == GLFW_PRESS)
      cameraPos.y += speed;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_LEFT_SHIFT) ==
        GLFW_PRESS)
      cameraPos.y -= speed;
    if (glfwGetKey(window->GetNativeWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
      window->Close();
  }

  void Update(float dt) {
    // Game logic updates here
  }

  void Render() {
    if (!device->BeginFrame())
      return;

    // Clear screen
    device->BindFramebuffer(RHI::FramebufferHandle{0});
    device->SetClearColor({0.05f, 0.05f, 0.08f, 1.0f});
    device->Clear(true, true, false);

    int width, height;
    glfwGetFramebufferSize(window->GetNativeWindow(), &width, &height);
    device->SetViewport({0.0f, 0.0f, static_cast<float>(width),
                         static_cast<float>(height), 0.0f, 1.0f});

    // Setup view and projection
    glm::mat4 view =
        glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(
        glm::radians(45.0f), static_cast<float>(width) / height, 0.1f, 100.0f);

    // Begin scene with renderer
    renderer.BeginScene(view, proj, cameraPos);

    // Submit rotating model
    float time = static_cast<float>(glfwGetTime());
    glm::mat4 transform =
        glm::rotate(glm::mat4(1.0f), time * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
    transform =
        glm::rotate(transform, time * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

    renderer.Submit(model, transform);

    // End scene - this renders all submitted meshes
    renderer.EndScene();

    device->EndFrame();
  }

  void Shutdown() {
    device->WaitIdle();
    model.reset();
    shader.reset();
    device->Shutdown();
    std::cout << "[App] Shutdown complete" << std::endl;
  }
};

int main() {
  RHIExampleApp app("RHI Demo - Model Loading", 1280, 720);
  app.Run();
  return 0;
}
