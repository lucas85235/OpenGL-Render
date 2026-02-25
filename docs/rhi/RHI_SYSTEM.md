# RHI (Render Hardware Interface) - Documentação Completa

## 📋 Índice

1. [Visão Geral](#visão-geral)
2. [Arquitetura](#arquitetura)
3. [Tipos e Enumerações](#tipos-e-enumerações)
4. [Estruturas (Descriptores)](#estruturas-descriptores)
5. [Interface IDevice](#interface-idevice)
6. [Factory Pattern](#factory-pattern)
7. [Gerenciamento de Recursos](#gerenciamento-de-recursos)
8. [Sistema de Shaders](#sistema-de-shaders)
9. [Implementações](#implementações)
10. [Guia de Uso](#guia-de-uso)
11. [Exemplos Práticos](#exemplos-práticos)

## Visão Geral

O **RHI (Render Hardware Interface)** é uma camada de abstração gráfica que permite ao engine renderizar usando diferentes APIs gráficas (OpenGL, Vulkan, etc.) com uma interface única e consistente.

### Objetivos Principais

- **Abstração de API**: Mesmo código funciona com OpenGL e Vulkan
- **Portabilidade**: Trocar de API apenas alterando o inicializador
- **Unificação de Shaders**: Sistema baseado em SPIR-V como formato intermediário
- **Gerenciamento de Recursos**: Interface consistente para GPU resources

## Arquitetura

```
┌─────────────────────────────────────┐
│          Application Code           │
└────────────┬────────────────────────┘
             │
┌────────────▼────────────────────────┐
│     RHI Abstract Interface (IDevice) │
│  Buffers | Textures | Shaders etc   │
└────────────┬────────────────────────┘
             │
    ┌────────┴────────┐
    │                 │
┌───▼────────┐  ┌────▼──────────┐
│   OpenGL   │  │    Vulkan     │
│ Implementation│ Implementation │
└────────────┘  └──────────────┘
```

### Camadas do Sistema

1. **Interface Abstrata (`IDevice`)**: Define contrato que todas as implementações devem cumprir
2. **Factory (`CreateDevice`)**: Cria instância correta baseada na API selecionada
3. **Implementações**: OpenGL e Vulkan com suas especificidades
4. **Sistema de Shaders**: Cross-compiler e preprocessor para unificação

## Tipos e Enumerações

### Enumerações Principais

#### API Gráfica
```cpp
enum class API { OpenGL, Vulkan, DirectX12, Metal };
```

#### Buffers
```cpp
enum class BufferUsage { Static, Dynamic, Stream };
enum class BufferType { Vertex, Index, Uniform };
```

| BufferUsage | Descrição |
|---|---|
| `Static` | Dados imutáveis, enviados uma vez à GPU |
| `Dynamic` | Dados que mudam frequentemente |
| `Stream` | Dados que mudam a cada frame |

#### Topologia de Primitivos
```cpp
enum class PrimitiveTopology {
  TriangleList,      // Triângulos isolados
  TriangleStrip,     // Triângulos compartilham arestas
  LineList,          // Linhas isoladas
  LineStrip,         // Linhas contíguas
  PointList          // Pontos individuais
};
```

#### Texturas
```cpp
enum class TextureFormat {
  // Inteiros
  R8, RG8, RGB8, RGBA8,
  
  // Float 16-bit
  R16F, RG16F, RGB16F, RGBA16F,
  
  // Float 32-bit
  R32F, RG32F, RGB32F, RGBA32F,
  
  // Profundidade/Stencil
  Depth24Stencil8, Depth32F,
  
  // SRGB (para gamma correction)
  SRGB8, SRGB8_Alpha8
};

enum class TextureWrapMode {
  Repeat,           // Repete textura
  MirroredRepeat,   // Repete espelhado
  ClampToEdge,      // Estende borda
  ClampToBorder     // Cor de borda
};

enum class TextureFilterMode {
  Nearest,                  // Sem suavização
  Linear,                   // Suavização bilinear
  NearestMipmapNearest,
  NearestMipmapLinear,
  LinearMipmapNearest,
  LinearMipmapLinear
};

enum class TextureType { Texture2D, Texture3D, TextureCube };
```

#### Estado de Rasterização
```cpp
enum class CullMode { None, Front, Back, FrontAndBack };
enum class FrontFace { Clockwise, CounterClockwise };
enum class CompareOp {
  Never, Less, Equal, LessOrEqual,
  Greater, NotEqual, GreaterOrEqual, Always
};

enum class BlendFactor {
  Zero, One, SrcColor, OneMinusSrcColor,
  DstColor, OneMinusDstColor, SrcAlpha,
  OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha
};

enum class BlendOp { Add, Subtract, ReverseSubtract, Min, Max };
```

#### Shaders
```cpp
enum class ShaderStage {
  Vertex,     // Processamento de vértices
  Fragment,   // Processamento de fragmentos/pixels
  Geometry,   // Geração de primitivos
  Compute     // Shaders computacionais
};

enum class VertexAttributeType {
  Float, Float2, Float3, Float4,
  Int, Int2, Int3, Int4,
  UInt, UInt2, UInt3, UInt4
};

enum class IndexType { UInt16, UInt32 };
```

## Estruturas (Descriptores)

Descriptores são estruturas que definem parâmetros para criar recursos na GPU.

### Viewport e Scissor

```cpp
struct Viewport {
  float x = 0.0f;          // Posição X em pixels
  float y = 0.0f;          // Posição Y em pixels
  int width = 800;         // Largura
  int height = 600;        // Altura
  float minDepth = 0.0f;   // Profundidade mínima (NDC)
  float maxDepth = 1.0f;   // Profundidade máxima (NDC)
};

struct Scissor {
  int x = 0;
  int y = 0;
  uint32_t width = 800;
  uint32_t height = 600;
};
```

### Buffer

```cpp
struct BufferDescriptor {
  BufferType type;         // Vertex, Index ou Uniform
  BufferUsage usage;       // Static, Dynamic ou Stream
  uint32_t size;           // Tamanho em bytes
  const void *data = nullptr;  // Dados iniciais
};
```

**Exemplo:**
```cpp
BufferDescriptor vertexBufferDesc{
  .type = BufferType::Vertex,
  .usage = BufferUsage::Static,
  .size = vertices.size() * sizeof(Vertex),
  .data = vertices.data()
};

RHI::BufferHandle vbo = device->CreateBuffer(vertexBufferDesc);
```

### Textura

```cpp
struct TextureDescriptor {
  TextureType type = TextureType::Texture2D;
  TextureFormat format = TextureFormat::RGBA8;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t depth = 1;          // Para Texture3D
  uint32_t mipLevels = 1;
  bool generateMipmaps = false;
  const void *data = nullptr;  // Dados iniciais
};
```

**Exemplo:**
```cpp
TextureDescriptor texDesc{
  .type = TextureType::Texture2D,
  .format = TextureFormat::SRGB8_Alpha8,
  .width = 1024,
  .height = 1024,
  .generateMipmaps = true,
  .data = imageData
};

RHI::TextureHandle texture = device->CreateTexture(texDesc);
```

### Sampler

```cpp
struct SamplerDescriptor {
  TextureWrapMode wrapS = TextureWrapMode::Repeat;
  TextureWrapMode wrapT = TextureWrapMode::Repeat;
  TextureWrapMode wrapR = TextureWrapMode::Repeat;
  TextureFilterMode minFilter = TextureFilterMode::LinearMipmapLinear;
  TextureFilterMode magFilter = TextureFilterMode::Linear;
  float anisotropy = 1.0f;  // Anisotropic filtering
};
```

### Shader

```cpp
struct ShaderDescriptor {
  ShaderStage stage;                       // Vertex, Fragment, etc
  std::string source;                      // Código GLSL nativo
  std::vector<uint32_t> spirvBinary;      // Bytecode SPIR-V
  std::string entryPoint = "main";         // Função de entrada
  bool useSPIRV = false;                   // Se true, usa SPIR-V
};
```

### Rasterização e Blending

```cpp
struct RasterizerState {
  CullMode cullMode = CullMode::Back;
  FrontFace frontFace = FrontFace::CounterClockwise;
  bool depthBias = false;
  float depthBiasConstant = 0.0f;
  float depthBiasSlope = 0.0f;
  bool lineSmoothing = false;
  float lineWidth = 1.0f;
};

struct DepthStencilState {
  bool depthTest = true;
  bool depthWrite = true;
  CompareOp depthCompare = CompareOp::Less;
  bool stencilTest = false;
  // ... mais configurações
};

struct BlendState {
  bool enabled = false;
  BlendFactor srcFactor = BlendFactor::SrcAlpha;
  BlendFactor dstFactor = BlendFactor::OneMinusSrcAlpha;
  BlendOp operation = BlendOp::Add;
  BlendFactor srcAlphaFactor = BlendFactor::One;
  BlendFactor dstAlphaFactor = BlendFactor::Zero;
  BlendOp alphaOperation = BlendOp::Add;
};
```

### Vértices

```cpp
struct VertexAttribute {
  uint32_t location;           // Local (0, 1, 2, ...)
  VertexAttributeType type;    // Float3, Int2, etc
  uint32_t offset;             // Offset na estrutura de vértice
  bool normalized = false;
};

struct VertexLayout {
  uint32_t stride;             // Tamanho total de um vértice
  std::vector<VertexAttribute> attributes;
};
```

**Exemplo:**
```cpp
struct Vertex {
  glm::vec3 position;  // offset 0
  glm::vec3 normal;    // offset 12
  glm::vec2 texCoord;  // offset 24
};

RHI::VertexLayout layout{
  .stride = sizeof(Vertex),
  .attributes = {
    {0, RHI::VertexAttributeType::Float3, offsetof(Vertex, position)},
    {1, RHI::VertexAttributeType::Float3, offsetof(Vertex, normal)},
    {2, RHI::VertexAttributeType::Float2, offsetof(Vertex, texCoord)}
  }
};
```

### Pipeline

```cpp
struct PipelineDescriptor {
  PrimitiveTopology topology = PrimitiveTopology::TriangleList;
  RasterizerState rasterizer;
  DepthStencilState depthStencil;
  BlendState blend;
  uint32_t colorAttachmentCount = 1;
  std::vector<TextureFormat> colorFormats;
  TextureFormat depthFormat = TextureFormat::Depth32F;
};
```

### Framebuffer

```cpp
struct FramebufferDescriptor {
  uint32_t width;
  uint32_t height;
  uint32_t colorAttachmentCount = 1;
  std::vector<TextureFormat> colorFormats;
  TextureFormat depthFormat = TextureFormat::Depth32F;
  bool useStencil = false;
};
```

## Interface IDevice

`IDevice` é a interface abstrata principal que define todas as operações disponíveis.

### Ciclo de Vida

```cpp
// Inicializar
bool Initialize();

// Cada frame
bool BeginFrame();
// ... renderizar ...
void EndFrame();

// Finalizar
void Shutdown();
```

### Gerenciamento de Buffers

```cpp
// Criar buffer
BufferHandle CreateBuffer(const BufferDescriptor &desc);

// Atualizar dados
void UpdateBuffer(BufferHandle buffer, const void *data,
                  uint32_t size, uint32_t offset = 0);

// Destruir
void DestroyBuffer(BufferHandle buffer);
```

### Gerenciamento de Texturas

```cpp
// Criar textura
TextureHandle CreateTexture(const TextureDescriptor &desc);

// Atualizar dados
void UpdateTexture(TextureHandle texture, const void *data,
                   uint32_t mipLevel = 0);

// Para cubemaps
void UpdateTextureCubeFace(TextureHandle texture, CubemapFace face,
                           const void *data, uint32_t mipLevel = 0);

// Gerar mipmaps
void GenerateMipmaps(TextureHandle texture);

// Destruir
void DestroyTexture(TextureHandle texture);
```

### Gerenciamento de Samplers

```cpp
SamplerHandle CreateSampler(const SamplerDescriptor &desc);
void DestroySampler(SamplerHandle sampler);
```

### Gerenciamento de Shaders

```cpp
// Criar shader program
ShaderHandle CreateShader(const std::vector<ShaderDescriptor> &stages);

// Destruir
void DestroyShader(ShaderHandle shader);
```

### Gerenciamento de Pipelines

```cpp
PipelineHandle CreatePipeline(const PipelineDescriptor &desc,
                              ShaderHandle shader,
                              const VertexLayout &layout);
void DestroyPipeline(PipelineHandle pipeline);
```

### Vertex Arrays (VAO)

```cpp
VertexArrayHandle CreateVertexArray(BufferHandle vertexBuffer,
                                    BufferHandle indexBuffer,
                                    const VertexLayout &layout);
void DestroyVertexArray(VertexArrayHandle vao);
```

### Framebuffers

```cpp
FramebufferHandle CreateFramebuffer(const FramebufferDescriptor &desc);

void AttachTexture(FramebufferHandle framebuffer,
                   FramebufferAttachment attachment,
                   TextureHandle texture);

TextureHandle GetFramebufferTexture(FramebufferHandle framebuffer,
                                    FramebufferAttachment attachment);

void ResizeFramebuffer(FramebufferHandle framebuffer,
                       uint32_t width, uint32_t height);

void DestroyFramebuffer(FramebufferHandle framebuffer);
```

### Renderização

```cpp
// Estado
void SetViewport(const Viewport &viewport);
void SetScissor(const Scissor &scissor);
void SetClearColor(const ClearColor &color);
void SetClearDepth(float depth);

// Bind (vincular recursos)
void BindPipeline(PipelineHandle pipeline);
void BindVertexArray(VertexArrayHandle vao);
void BindFramebuffer(FramebufferHandle framebuffer);
void BindTexture(uint32_t slot, TextureHandle texture, SamplerHandle sampler);
void BindUniformBuffer(uint32_t slot, BufferHandle buffer);

// Clear (limpar)
void Clear(uint32_t flags);  // RHI_CLEAR_COLOR | RHI_CLEAR_DEPTH

// Draw calls
void DrawArrays(uint32_t count, uint32_t offset = 0);
void DrawIndexed(uint32_t indexCount, uint32_t offset = 0);
void DrawInstancedArrays(uint32_t count, uint32_t instanceCount);
void DrawInstancedIndexed(uint32_t indexCount, uint32_t instanceCount);

// Compute
void DispatchCompute(uint32_t groupsX, uint32_t groupsY, uint32_t groupsZ);
```

### Informações do Dispositivo

```cpp
API GetAPI() const;
DeviceInfo GetDeviceInfo() const;
```

## Factory Pattern

O padrão Factory centraliza a criação de dispositivos:

```cpp
// Em rhi_factory.h
std::unique_ptr<IDevice> CreateDevice(API api, GLFWwindow *window) {
  switch (api) {
  case API::OpenGL:
    return std::make_unique<OpenGLDevice>();
  case API::Vulkan:
    auto device = std::make_unique<VulkanDevice>();
    device->SetWindow(window);
    return device;
  default:
    return nullptr;
  }
}
```

**Uso:**
```cpp
auto device = RHI::CreateDevice(RHI::API::OpenGL, window);
if (!device->Initialize()) {
  // Error handling
}
```

## Gerenciamento de Recursos

### Handles

Todos os recursos são referenciados por handles (números inteiros de 64 bits):

```cpp
using BufferHandle = uint64_t;
using TextureHandle = uint64_t;
using ShaderHandle = uint64_t;
using PipelineHandle = uint64_t;
using VertexArrayHandle = uint64_t;
using FramebufferHandle = uint64_t;
using SamplerHandle = uint64_t;
```

### Implementação (OpenGL)

OpenGL usa `unordered_map<uint64_t, NativeHandle>` para rastrear recursos:

```cpp
class OpenGLDevice : public IDevice {
private:
  struct BufferObject {
    GLuint id;
    GLenum target;
    GLenum usage;
    uint32_t size;
  };
  
  std::unordered_map<uint64_t, BufferObject> buffers;
  
  // ... outras estruturas ...
};
```

## Sistema de Shaders

### Filosofia: SPIR-V como Intermediário

```
┌──────────────┐
│ GLSL Source  │
└──────┬───────┘
       │
  (compile offline com glslangValidator)
       │
       ▼
┌──────────────────┐
│  SPIR-V Binary   │
└──────┬───────────┘
       │
       ├─────────────────────┬──────────────────┐
       │                     │                  │
   (passthrough)      (transpile com SPIRV-Cross)
       │                     │
       ▼                     ▼
   Vulkan              OpenGL (GLSL)
```

### ShaderCrossCompiler

Transforma SPIR-V em shaders específicos da API:

```cpp
class ShaderCrossCompiler {
public:
  static CrossCompileResult Process(
    const std::vector<uint32_t> &spirvBinary,
    RenderAPI targetAPI,
    ShaderStageType stage,
    uint32_t glslVersion = 450
  );
};

struct CrossCompileResult {
  bool success;
  std::string errorMessage;
  std::string glslSource;           // Para OpenGL
  std::vector<uint32_t> spirvBinary; // Para Vulkan
  RenderAPI targetAPI;
};
```

**Exemplo:**
```cpp
// Carregar SPIR-V de arquivo
std::vector<uint32_t> spirvData = LoadSPIRV("shader.vert.spv");

// Compilar para a API alvo
auto result = RHI::ShaderCrossCompiler::Process(
  spirvData,
  RHI::RenderAPI::OpenGL,
  RHI::ShaderStageType::Vertex,
  450  // GLSL version
);

if (result.success) {
  // Usar result.glslSource ou result.spirvBinary
}
```

### ShaderPreprocessor

Processa shaders e extrai metadados:

```cpp
class ShaderPreprocessor {
public:
  static ShaderStageSource Process(
    const std::string &source,
    API targetAPI,
    uint32_t glslVersion = 450
  );
  
  static ShaderMetadata ExtractMetadata(const std::string &source);
};

struct ShaderStageSource {
  std::string vertex;
  std::string fragment;
  std::string geometry;
  std::string compute;
  std::string tessControl;
  std::string tessEval;
};

struct ShaderMetadata {
  ShaderType type;
  std::vector<ResourceBinding> bindings;
  std::string commonBlock;
  std::string resourcesBlock;
};
```

### Blocos de Shader

Shaders podem usar blocos especiais:

```glsl
#shader common
// Código compartilhado entre todos os estágios

#shader resources
// Declarações de bindings e recursos

#shader vertex
// Código do vertex shader
void main() { }

#shader fragment
// Código do fragment shader
void main() { }
```

### Coordenadas e Convenções

**⚠️ Atenção: Diferenças entre Vulkan e OpenGL**

#### Y-Axis (Sistema de Coordenadas)

- **Vulkan**: Y-down (0 no topo, 1 na base)
- **OpenGL**: Y-up (0 na base, 1 no topo)

**Solução**: Inverter Y na matriz de projeção (não no shader):

```cpp
glm::mat4 proj = glm::perspective(fov, aspect, near, far);
if (api == RHI::API::Vulkan) {
  proj[1][1] *= -1.0f;  // Flip Y
}
```

#### Depth Range (Z NDC)

- **Vulkan**: 0 a 1 (zero-to-one)
- **OpenGL**: -1 a 1 (negative-one-to-one)

**Solução para OpenGL 4.5+**: Usar `glClipControl`:

```cpp
glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
```

Isso faz OpenGL usar a mesma convenção que Vulkan.

## Implementações

### OpenGL (opengl_device.hpp/cpp)

**Características:**
- Usa GLEW para extensões
- Implementa handles via `unordered_map`
- Suporta OpenGL 3.3+ + GLSL
- Renderização com VAO/VBO tradicional

**Recursos Internos:**
```cpp
std::unordered_map<uint64_t, BufferObject> buffers;
std::unordered_map<uint64_t, TextureObject> textures;
std::unordered_map<uint64_t, SamplerObject> samplers;
std::unordered_map<uint64_t, ShaderObject> shaders;
std::unordered_map<uint64_t, PipelineObject> pipelines;
std::unordered_map<uint64_t, VertexArrayObject> vertexArrays;
std::unordered_map<uint64_t, FramebufferObject> framebuffers;
```

**Especificidades:**
- Skybox tem recursos dedicados (`skyboxShaderProgram`, `skyboxVAO`, etc)
- Métodos de conversão: `ToGLBufferTarget()`, `ToGLBufferUsage()`
- Gerenciamento de estado global do OpenGL

### Vulkan (vulkan_device.hpp/cpp)

**Características:**
- API moderna com sincronização explícita
- Command buffers e frame pipelining
- Suporte nativo a SPIR-V
- Ray tracing preparado

**Diferenças Principais:**
- Requer janela GLFW via `SetWindow()`
- Maior overhead de inicialização
- Melhor performance em larga escala
- Mais verbose (tradeof por controle fino)

## Guia de Uso

### 1. Inicializar o RHI

```cpp
#include "rhi/rhi_factory.h"

// Criar dispositivo
auto device = RHI::CreateDevice(RHI::API::OpenGL, glfwWindow);

// Inicializar
if (!device->Initialize()) {
    std::cerr << "Failed to initialize RHI device\n";
    return false;
}

// Obter informações (opcional)
auto info = device->GetDeviceInfo();
std::cout << "Device: " << info.name << "\n";
```

### 2. Criar Shaders

```cpp
// Método 1: GLSL direto (OpenGL)
RHI::ShaderDescriptor vertexShader{
    .stage = RHI::ShaderStage::Vertex,
    .source = R"(
        #version 330 core
        layout(location = 0) in vec3 position;
        void main() {
            gl_Position = vec4(position, 1.0);
        }
    )"
};

// Método 2: SPIR-V (ambas as APIs)
auto spirvData = LoadSPIRVFile("shader.vert.spv");
RHI::ShaderDescriptor vertexShaderSPIRV{
    .stage = RHI::ShaderStage::Vertex,
    .spirvBinary = spirvData,
    .useSPIRV = true
};

// Criar programa
RHI::ShaderHandle shader = device->CreateShader({
    vertexShader,
    fragmentShader
});
```

### 3. Criar Buffers e Geometria

```cpp
// Dados de vértices
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

std::vector<Vertex> vertices = { /* ... */ };
std::vector<uint32_t> indices = { /* ... */ };

// Criar VBO
RHI::BufferDescriptor vboDesc{
    .type = RHI::BufferType::Vertex,
    .usage = RHI::BufferUsage::Static,
    .size = vertices.size() * sizeof(Vertex),
    .data = vertices.data()
};
RHI::BufferHandle vbo = device->CreateBuffer(vboDesc);

// Criar IBO
RHI::BufferDescriptor iboDesc{
    .type = RHI::BufferType::Index,
    .usage = RHI::BufferUsage::Static,
    .size = indices.size() * sizeof(uint32_t),
    .data = indices.data()
};
RHI::BufferHandle ibo = device->CreateBuffer(iboDesc);

// Definir layout de vértice
RHI::VertexLayout layout{
    .stride = sizeof(Vertex),
    .attributes = {
        {0, RHI::VertexAttributeType::Float3, offsetof(Vertex, position)},
        {1, RHI::VertexAttributeType::Float3, offsetof(Vertex, normal)},
        {2, RHI::VertexAttributeType::Float2, offsetof(Vertex, texCoord)}
    }
};

// Criar VAO
RHI::VertexArrayHandle vao = device->CreateVertexArray(vbo, ibo, layout);
```

### 4. Criar Texturas

```cpp
// Carregar imagem (pseudo-código)
ImageData image = LoadImage("texture.png");

// Criar textura 2D
RHI::TextureDescriptor texDesc{
    .type = RHI::TextureType::Texture2D,
    .format = RHI::TextureFormat::SRGB8_Alpha8,
    .width = image.width,
    .height = image.height,
    .generateMipmaps = true,
    .data = image.data
};
RHI::TextureHandle texture = device->CreateTexture(texDesc);

// Criar sampler
RHI::SamplerDescriptor samplerDesc{
    .wrapS = RHI::TextureWrapMode::Repeat,
    .wrapT = RHI::TextureWrapMode::Repeat,
    .minFilter = RHI::TextureFilterMode::LinearMipmapLinear,
    .magFilter = RHI::TextureFilterMode::Linear,
    .anisotropy = 4.0f
};
RHI::SamplerHandle sampler = device->CreateSampler(samplerDesc);
```

### 5. Criar Pipeline

```cpp
RHI::PipelineDescriptor pipelineDesc{
    .topology = RHI::PrimitiveTopology::TriangleList,
    .rasterizer = {
        .cullMode = RHI::CullMode::Back,
        .frontFace = RHI::FrontFace::CounterClockwise
    },
    .depthStencil = {
        .depthTest = true,
        .depthWrite = true,
        .depthCompare = RHI::CompareOp::Less
    },
    .blend = {
        .enabled = true,
        .srcFactor = RHI::BlendFactor::SrcAlpha,
        .dstFactor = RHI::BlendFactor::OneMinusSrcAlpha
    }
};

RHI::PipelineHandle pipeline = device->CreatePipeline(
    pipelineDesc,
    shader,
    layout
);
```

### 6. Renderizar um Frame

```cpp
// Iniciar frame
if (!device->BeginFrame()) {
    return false;
}

// Configurar estado
device->SetViewport({0, 0, 800, 600, 0.0f, 1.0f});
device->SetClearColor({0.1f, 0.1f, 0.1f, 1.0f});
device->Clear(RHI_CLEAR_COLOR | RHI_CLEAR_DEPTH);

// Vincular recursos
device->BindPipeline(pipeline);
device->BindVertexArray(vao);
device->BindFramebuffer(framebuffer);  // ou nullptr para screen

device->BindTexture(0, texture, sampler);
device->BindUniformBuffer(0, uniformBuffer);

// Draw call
device->DrawIndexed(indexCount);

// Finalizar frame
device->EndFrame();
```

### 7. Limpeza

```cpp
device->DestroyVertexArray(vao);
device->DestroyBuffer(vbo);
device->DestroyBuffer(ibo);
device->DestroyTexture(texture);
device->DestroySampler(sampler);
device->DestroyShader(shader);
device->DestroyPipeline(pipeline);

device->Shutdown();
```

## Exemplos Práticos

### Exemplo 1: Renderizar um Triângulo

```cpp
// Setup
auto device = RHI::CreateDevice(RHI::API::OpenGL, window);
device->Initialize();

// Vertex data
struct Vertex { glm::vec3 pos; };
std::vector<Vertex> triangle = {
    {{-0.5f, -0.5f, 0.0f}},
    {{ 0.5f, -0.5f, 0.0f}},
    {{ 0.0f,  0.5f, 0.0f}}
};

// Create VBO
auto vbo = device->CreateBuffer({
    RHI::BufferType::Vertex,
    RHI::BufferUsage::Static,
    triangle.size() * sizeof(Vertex),
    triangle.data()
});

// Create shader
auto shader = device->CreateShader({
    {RHI::ShaderStage::Vertex, R"(
        #version 330 core
        layout(location = 0) in vec3 pos;
        void main() { gl_Position = vec4(pos, 1.0); }
    )"},
    {RHI::ShaderStage::Fragment, R"(
        #version 330 core
        out vec4 color;
        void main() { color = vec4(1.0, 0.0, 0.0, 1.0); }
    )"}
});

// Create layout and VAO
RHI::VertexLayout layout{
    sizeof(Vertex),
    {{0, RHI::VertexAttributeType::Float3, 0}}
};
auto vao = device->CreateVertexArray(vbo, nullptr, layout);

// Create pipeline
auto pipeline = device->CreatePipeline({}, shader, layout);

// Render loop
while (running) {
    device->BeginFrame();
    device->SetClearColor({0, 0, 0, 1});
    device->Clear(RHI_CLEAR_COLOR);
    device->BindPipeline(pipeline);
    device->BindVertexArray(vao);
    device->DrawArrays(3);
    device->EndFrame();
}

// Cleanup
device->DestroyVertexArray(vao);
device->DestroyBuffer(vbo);
device->DestroyShader(shader);
device->DestroyPipeline(pipeline);
device->Shutdown();
```

### Exemplo 2: Usar Uniform Buffer

```cpp
// Criar uniform buffer
struct Uniforms {
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};

Uniforms uniforms;
auto uniformBuffer = device->CreateBuffer({
    RHI::BufferType::Uniform,
    RHI::BufferUsage::Dynamic,
    sizeof(Uniforms),
    &uniforms
});

// Em cada frame
uniforms.model = glm::translate(glm::identity<glm::mat4>(), {0, 0, -3});
uniforms.view = glm::lookAt(
    glm::vec3(0, 2, 2),
    glm::vec3(0, 0, 0),
    glm::vec3(0, 1, 0)
);
uniforms.projection = glm::perspective(
    glm::radians(45.0f),
    800.0f / 600.0f,
    0.1f,
    100.0f
);

// Flip Y for Vulkan
if (device->GetAPI() == RHI::API::Vulkan) {
    uniforms.projection[1][1] *= -1.0f;
}

device->UpdateBuffer(uniformBuffer, &uniforms, sizeof(Uniforms));
device->BindUniformBuffer(0, uniformBuffer);
```

### Exemplo 3: Post-processing com Framebuffer

```cpp
// Criar framebuffer e textura de cor
RHI::TextureDescriptor colorTexDesc{
    .type = RHI::TextureType::Texture2D,
    .format = RHI::TextureFormat::RGBA16F,
    .width = 800,
    .height = 600
};
auto colorTex = device->CreateTexture(colorTexDesc);

RHI::FramebufferDescriptor fbDesc{
    .width = 800,
    .height = 600,
    .colorAttachmentCount = 1,
    .colorFormats = {RHI::TextureFormat::RGBA16F}
};
auto fb = device->CreateFramebuffer(fbDesc);
device->AttachTexture(fb, RHI::FramebufferAttachment::Color0, colorTex);

// Renderizar cena no framebuffer
device->BindFramebuffer(fb);
device->Clear(RHI_CLEAR_COLOR);
// ... render scene ...

// Post-processing: renderizar quad com textura
device->BindFramebuffer(nullptr);  // Back to screen
device->BindTexture(0, colorTex, sampler);
// ... render screen quad ...
```

## Boas Práticas

1. **Sempre verificar retornos**: `Initialize()`, `CreateDevice()` podem falhar

2. **Gerenciar handles corretamente**: Destruir recursos quando não precisar mais

3. **Usar SPIR-V para portabilidade**: Compile shaders offline, use SPIR-V em runtime

4. **Considerar coordenadas**: Aplicar transformações para compatibilizar APIs

5. **Batch draw calls**: Uma chamada com `DrawInstancedIndexed` é melhor que muitas `DrawIndexed`

6. **Reutilizar pipelines**: Criar pipeline uma vez, usar múltiplas vezes

7. **Buffer mapping**: Usar `Dynamic` para dados que mudam, `Static` para imutáveis

8. **Textura format selection**:
   - `SRGB8` para cores (automático gamma correction)
   - `RGB32F` para normal maps e dados técnicos
   - `RGBA16F` para post-processing intermediate textures
   - `Depth32F` para depth buffers

## Referências

- [Vulkan Specification](https://www.khronos.org/registry/vulkan/)
- [OpenGL Documentation](https://www.opengl.org/documentation/)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [glslang Validator](https://github.com/KhronosGroup/glslang)
