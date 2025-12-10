#include "src/core/window.hpp"
#include "src/renderer/model_factory.hpp"
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

class App {
public:
  void Run() {
    using namespace RHI;

    bool useVulkan = true;
    auto window =
        std::make_unique<Window>(1280, 720, "RHI Demo - Model Loading");

    if (!window->Init(!useVulkan))
      return;

    auto device = DeviceFactory::Create(useVulkan ? API::Vulkan : API::OpenGL);
    if (!device)
      return;

    if (useVulkan) {
      auto *vkDevice = dynamic_cast<VulkanDevice *>(device.get());
      if (vkDevice)
        vkDevice->SetWindow(window->GetNativeWindow());
    }

    if (!device->Initialize())
      return;

    auto info = device->GetDeviceInfo();
    std::cout << "[RHI] Renderer: " << info.rendererName << std::endl;
    std::cout << "[RHI] Version: " << info.apiVersion << std::endl;

    // Create a cube using ModelFactory
    Mesh cube = ModelFactory::CreateCube(device.get(), 1.0f);

    // Create shader (for OpenGL we use GLSL, Vulkan uses pre-compiled SPIR-V)
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

    std::vector<ShaderDescriptor> shaderStages = {
        {ShaderStage::Vertex, vertexShader},
        {ShaderStage::Fragment, fragmentShader}};
    auto shader = device->CreateShader(shaderStages);
    if (!IsValid(shader)) {
      std::cerr << "[RHI] Failed to create shader" << std::endl;
      return;
    }

    // Create pipeline
    VertexLayout layout;
    layout.stride = sizeof(Vertex);
    layout.attributes = {
        {0, VertexAttributeType::Float3, offsetof(Vertex, Position), false},
        {1, VertexAttributeType::Float3, offsetof(Vertex, Normal), false},
        {2, VertexAttributeType::Float2, offsetof(Vertex, TexCoords), false}};

    PipelineDescriptor pipelineDesc;
    pipelineDesc.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.rasterizer.cullMode = CullMode::Back;
    pipelineDesc.rasterizer.frontFace = FrontFace::CounterClockwise;
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = CompareOp::Less;
    auto pipeline = device->CreatePipeline(pipelineDesc, shader, layout);

    std::cout << "[RHI] Setup complete. Entering render loop..." << std::endl;
    std::cout << "[RHI] Controls: WASD to move camera, ESC to exit"
              << std::endl;

    glm::vec3 cameraPos(0.0f, 1.0f, 3.0f);
    float lastTime = 0.0f;

    while (!window->ShouldClose()) {
      float currentTime = static_cast<float>(glfwGetTime());
      float deltaTime = currentTime - lastTime;
      lastTime = currentTime;

      float speed = 2.0f * deltaTime;
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
        break;

      if (!device->BeginFrame())
        continue;

      device->BindFramebuffer(FramebufferHandle{0});
      device->SetClearColor({0.1f, 0.1f, 0.12f, 1.0f});
      device->Clear(true, true, false);

      int width, height;
      glfwGetFramebufferSize(window->GetNativeWindow(), &width, &height);
      device->SetViewport({0.0f, 0.0f, static_cast<float>(width),
                           static_cast<float>(height), 0.0f, 1.0f});

      device->BindPipeline(pipeline);
      device->BindVertexArray(cube.GetVAO());

      glm::mat4 view =
          glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
      glm::mat4 proj =
          glm::perspective(glm::radians(45.0f),
                           static_cast<float>(width) / height, 0.1f, 100.0f);
      glm::mat4 model = glm::rotate(glm::mat4(1.0f), currentTime * 0.5f,
                                    glm::vec3(0.0f, 1.0f, 0.0f));
      model =
          glm::rotate(model, currentTime * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

      device->SetUniformMatrix4(shader, "model", &model[0][0]);
      device->SetUniformMatrix4(shader, "view", &view[0][0]);
      device->SetUniformMatrix4(shader, "projection", &proj[0][0]);

      DrawIndexedCommand drawCmd;
      drawCmd.indexCount = cube.GetIndexCount();
      drawCmd.instanceCount = 1;
      drawCmd.indexType = IndexType::UInt32;
      device->DrawIndexed(drawCmd);

      device->EndFrame();
      window->OnUpdate();
    }

    device->WaitIdle();
    device->DestroyPipeline(pipeline);
    device->DestroyShader(shader);
    device->Shutdown();
    std::cout << "[RHI] Resources released successfully" << std::endl;
  }
};

int main() {
  App app;
  app.Run();
  return 0;
}
