#include "src/core/window.hpp"
#include "src/rhi/opengl_device.hpp"
#include <glm/glm.hpp>
#include <memory>

namespace RHI {

std::unique_ptr<IDevice> DeviceFactory::Create(API api) {
  switch (api) {
  case API::OpenGL:
    return std::make_unique<OpenGLDevice>();

  case API::Vulkan:
    // return std::make_unique<VulkanDevice>();
    std::cerr << "[RHI] Vulkan não implementado ainda!" << std::endl;
    return nullptr;

  case API::DirectX12:
    // return std::make_unique<D3D12Device>();
    std::cerr << "[RHI] DirectX12 não implementado ainda!" << std::endl;
    return nullptr;

  case API::Metal:
    // return std::make_unique<MetalDevice>();
    std::cerr << "[RHI] Metal não implementado ainda!" << std::endl;
    return nullptr;

  default:
    return nullptr;
  }
}
} // namespace RHI

using namespace RHI;

struct Vertex {
  glm::vec3 position;
  glm::vec3 color;
};

std::vector<Vertex> triangleVertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}}, // Vermelho
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}},  // Verde
    {{0.0f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}}    // Azul
};

std::vector<uint32_t> triangleIndices = {0, 1, 2};

const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 color;

void main() {
    gl_Position = vec4(aPos, 1.0);
    color = aColor;
}
)";

const char *fragmentShaderSource = R"(
#version 330 core
in vec3 color;
out vec4 FragColor;

void main() {
    FragColor = vec4(color, 1.0);
}
)";

class App {
private:
  std::unique_ptr<Window> window;
  std::unique_ptr<IDevice> device;
  std::unique_ptr<FramebufferHandle> framebuffer;

  std::string title;
  int width;
  int height;

  bool Init() {
    if (!window->Init())
      return false;

    window->SetResizeCallback([this](int w, int h) {
      device->ResizeFramebuffer(*framebuffer, window->GetWidth(),
                                window->GetHeight());
    });

    // Create Graphics Context
    device = DeviceFactory::Create(API::OpenGL);
    if (!device) {
      std::cerr << "Falha ao criar device!" << std::endl;
      return false;
    }

    // Initialize Graphics
    if (!device->Initialize()) {
      std::cerr << "Falha ao inicializar device!" << std::endl;
      return false;
    }

    return true;
  }

public:
  App(const std::string &title, int width, int height)
      : title(title), width(width), height(height) {
    window = std::make_unique<Window>(width, height, this->title);
  }

  ~App() {
    device->Shutdown();
    std::cout << "Recursos liberados com sucesso!" << std::endl;
  }

  void Run() {
    if (!Init())
      return;

    // Create Buffers
    BufferDescriptor vbDesc;
    vbDesc.type = BufferType::Vertex;
    vbDesc.usage = BufferUsage::Static;
    vbDesc.size = triangleVertices.size() * sizeof(Vertex);
    vbDesc.data = triangleVertices.data();
    BufferHandle vertexBuffer = device->CreateBuffer(vbDesc);

    BufferDescriptor ibDesc;
    ibDesc.type = BufferType::Index;
    ibDesc.usage = BufferUsage::Static;
    ibDesc.size = triangleIndices.size() * sizeof(uint32_t);
    ibDesc.data = triangleIndices.data();
    BufferHandle indexBuffer = device->CreateBuffer(ibDesc);

    // Create Vertex Layout
    VertexLayout layout;
    layout.stride = sizeof(Vertex);
    layout.attributes = {
        {0, VertexAttributeType::Float3,
         offsetof(Vertex, position)},                             // Position
        {1, VertexAttributeType::Float3, offsetof(Vertex, color)} // Color
    };

    // Create Vertex Array
    VertexArrayHandle vao =
        device->CreateVertexArray(vertexBuffer, indexBuffer, layout);

    // Create Shader
    std::vector<ShaderDescriptor> shaderStages = {
        {ShaderStage::Vertex, vertexShaderSource},
        {ShaderStage::Fragment, fragmentShaderSource}};

    ShaderHandle shader = device->CreateShader(shaderStages);
    if (!IsValid(shader)) {
      std::cerr << "Falha ao criar shader!" << std::endl;
      return;
    }

    // Create Pipeline
    PipelineDescriptor pipelineDesc;
    pipelineDesc.rasterizer.cullMode = CullMode::Back;
    pipelineDesc.rasterizer.frontFace = FrontFace::CounterClockwise;
    pipelineDesc.depthStencil.depthTestEnable = true;
    pipelineDesc.depthStencil.depthWriteEnable = true;
    pipelineDesc.blend.blendEnable = false;
    pipelineDesc.topology = PrimitiveTopology::TriangleList;
    PipelineHandle pipeline =
        device->CreatePipeline(pipelineDesc, shader, layout);

    // Note: Off-screen framebuffer removed for this example
    // Rendering directly to default framebuffer (screen)

    // Main Loop
    while (!window->ShouldClose()) {
      // Render to default framebuffer (screen)
      device->BindFramebuffer(FramebufferHandle{0});
      device->SetClearColor({0.1f, 0.1f, 0.15f, 1.0f});
      device->Clear(true, true, false);

      // Bind pipeline and VAO
      device->BindPipeline(pipeline);
      device->BindVertexArray(vao);

      // Draw
      DrawIndexedCommand drawCmd;
      drawCmd.indexCount = triangleIndices.size();
      drawCmd.instanceCount = 1;
      device->DrawIndexed(drawCmd);

      // Swap buffers and poll events
      window->OnUpdate();
    }
  }
};

int main() {
  App app("OpenGL Render", 1280, 720);
  app.Run();
  return 0;
}
