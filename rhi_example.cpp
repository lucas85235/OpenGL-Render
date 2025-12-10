#include "src/core/window.hpp"
#include "src/rhi/opengl_device.hpp"
#include "src/rhi/vulkan_device.hpp"
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <sstream>

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

namespace {

std::string LoadShaderFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cerr << "[RHI] Failed to load shader: " << path << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  return buffer.str();
}

const char *cubeVertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aTexCoord;

out vec3 vertexColor;
out vec2 texCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vertexColor = aColor;
    texCoord = aTexCoord;
}
)";

const char *cubeFragmentShader = R"(
#version 330 core
in vec3 vertexColor;
in vec2 texCoord;
out vec4 FragColor;

uniform float time;

void main() {
    vec3 color = vertexColor;
    color.r *= 0.5 + 0.5 * sin(time * 2.0);
    color.g *= 0.5 + 0.5 * sin(time * 2.0 + 2.094);
    color.b *= 0.5 + 0.5 * sin(time * 2.0 + 4.189);
    FragColor = vec4(color, 1.0);
}
)";

struct CubeVertex {
  float x, y, z;
  float r, g, b;
  float u, v;
};

} // namespace

class App {
public:
  void Run() {
    using namespace RHI;

    bool useVulkan = false;
    auto window =
        std::make_unique<Window>(1280, 720, "RHI Demo - Rotating Cube");
    if (!window->Init())
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

    // Cube vertices with position, color, and texcoords
    std::vector<CubeVertex> cubeVertices = {
        // Front face (Z+)
        {-0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f},
        {0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f},
        {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        // Back face (Z-)
        {0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f},
        {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f},
        {-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},
        {0.5f, 0.5f, -0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f},
        // Top face (Y+)
        {-0.5f, 0.5f, 0.5f, 1.0f, 0.5f, 0.0f, 0.0f, 0.0f},
        {0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.5f, 1.0f, 0.0f},
        {0.5f, 0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 1.0f},
        {-0.5f, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f},
        // Bottom face (Y-)
        {-0.5f, -0.5f, -0.5f, 0.0f, 0.5f, 1.0f, 0.0f, 0.0f},
        {0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.5f, 1.0f, 0.0f},
        {0.5f, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 1.0f},
        {-0.5f, -0.5f, 0.5f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f},
        // Right face (X+)
        {0.5f, -0.5f, 0.5f, 0.8f, 0.2f, 0.2f, 0.0f, 0.0f},
        {0.5f, -0.5f, -0.5f, 0.2f, 0.8f, 0.2f, 1.0f, 0.0f},
        {0.5f, 0.5f, -0.5f, 0.2f, 0.2f, 0.8f, 1.0f, 1.0f},
        {0.5f, 0.5f, 0.5f, 0.8f, 0.8f, 0.2f, 0.0f, 1.0f},
        // Left face (X-)
        {-0.5f, -0.5f, -0.5f, 0.2f, 0.8f, 0.8f, 0.0f, 0.0f},
        {-0.5f, -0.5f, 0.5f, 0.8f, 0.2f, 0.8f, 1.0f, 0.0f},
        {-0.5f, 0.5f, 0.5f, 0.8f, 0.8f, 0.8f, 1.0f, 1.0f},
        {-0.5f, 0.5f, -0.5f, 0.4f, 0.4f, 0.4f, 0.0f, 1.0f},
    };

    std::vector<uint32_t> cubeIndices = {
        0,  1,  2,  2,  3,  0,  // Front
        4,  5,  6,  6,  7,  4,  // Back
        8,  9,  10, 10, 11, 8,  // Top
        12, 13, 14, 14, 15, 12, // Bottom
        16, 17, 18, 18, 19, 16, // Right
        20, 21, 22, 22, 23, 20  // Left
    };

    // Create RHI resources
    BufferDescriptor vbDesc;
    vbDesc.type = BufferType::Vertex;
    vbDesc.usage = BufferUsage::Static;
    vbDesc.size = cubeVertices.size() * sizeof(CubeVertex);
    vbDesc.data = cubeVertices.data();
    auto vertexBuffer = device->CreateBuffer(vbDesc);

    BufferDescriptor ibDesc;
    ibDesc.type = BufferType::Index;
    ibDesc.usage = BufferUsage::Static;
    ibDesc.size = cubeIndices.size() * sizeof(uint32_t);
    ibDesc.data = cubeIndices.data();
    auto indexBuffer = device->CreateBuffer(ibDesc);

    std::vector<ShaderDescriptor> shaderStages = {
        {ShaderStage::Vertex, cubeVertexShader},
        {ShaderStage::Fragment, cubeFragmentShader}};
    auto shader = device->CreateShader(shaderStages);
    if (!IsValid(shader)) {
      std::cerr << "[RHI] Failed to create shader" << std::endl;
      return;
    }

    VertexLayout layout;
    layout.stride = sizeof(CubeVertex);
    layout.attributes = {
        {0, VertexAttributeType::Float3, offsetof(CubeVertex, x), false},
        {1, VertexAttributeType::Float3, offsetof(CubeVertex, r), false},
        {2, VertexAttributeType::Float2, offsetof(CubeVertex, u), false}};

    PipelineDescriptor pipelineDesc;
    pipelineDesc.topology = PrimitiveTopology::TriangleList;
    pipelineDesc.rasterizer.cullMode = CullMode::Back;
    pipelineDesc.rasterizer.frontFace = FrontFace::CounterClockwise;
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.depthStencil.depthCompareOp = CompareOp::Less;
    auto pipeline = device->CreatePipeline(pipelineDesc, shader, layout);

    auto vao = device->CreateVertexArray(vertexBuffer, indexBuffer, layout);

    std::cout << "[RHI] Setup complete. Entering render loop..." << std::endl;
    std::cout << "[RHI] Controls: WASD to move camera, ESC to exit"
              << std::endl;

    glm::vec3 cameraPos(0.0f, 1.0f, 3.0f);
    float lastTime = 0.0f;

    while (!window->ShouldClose()) {
      float currentTime = static_cast<float>(glfwGetTime());
      float deltaTime = currentTime - lastTime;
      lastTime = currentTime;

      // Camera controls
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
      device->SetViewport({0.0f, 0.0f, width, height, 0.0f, 1.0f});

      device->BindPipeline(pipeline);
      device->BindVertexArray(vao);

      // View and projection matrices
      glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f),
                                   glm::vec3(0.0f, 1.0f, 0.0f));
      glm::mat4 proj =
          glm::perspective(glm::radians(45.0f),
                           static_cast<float>(width) / height, 0.1f, 100.0f);

      // Rotating model matrix
      glm::mat4 model = glm::mat4(1.0f);
      model =
          glm::rotate(model, currentTime * 0.5f, glm::vec3(0.0f, 1.0f, 0.0f));
      model =
          glm::rotate(model, currentTime * 0.3f, glm::vec3(1.0f, 0.0f, 0.0f));

      device->SetUniformMatrix4(shader, "model", &model[0][0]);
      device->SetUniformMatrix4(shader, "view", &view[0][0]);
      device->SetUniformMatrix4(shader, "projection", &proj[0][0]);
      device->SetUniform(shader, "time", currentTime);

      DrawIndexedCommand drawCmd;
      drawCmd.indexCount = cubeIndices.size();
      drawCmd.instanceCount = 1;
      drawCmd.indexType = IndexType::UInt32;
      device->DrawIndexed(drawCmd);

      device->EndFrame();
      window->OnUpdate();
    }

    device->WaitIdle();
    device->DestroyVertexArray(vao);
    device->DestroyPipeline(pipeline);
    device->DestroyShader(shader);
    device->DestroyBuffer(indexBuffer);
    device->DestroyBuffer(vertexBuffer);
    device->Shutdown();
    std::cout << "[RHI] Resources released successfully" << std::endl;
  }
};

int main() {
  App app;
  app.Run();
  return 0;
}
