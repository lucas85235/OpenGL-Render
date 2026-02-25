# RHI - Guia Avançado e Padrões

## 📚 Conteúdo

1. [Padrões de Arquitetura](#padrões-de-arquitetura)
2. [Gerenciamento Avançado de Shaders](#gerenciamento-avançado-de-shaders)
3. [Otimizações de Performance](#otimizações-de-performance)
4. [Casos de Uso Específicos](#casos-de-uso-específicos)
5. [Tratamento de Erros](#tratamento-de-erros)
6. [Debugging e Profiling](#debugging-e-profiling)
7. [Migração entre APIs](#migração-entre-apis)

## Padrões de Arquitetura

### 1. Resource Manager Pattern

Centralizar criação e destruição de recursos:

```cpp
class ResourceManager {
private:
    std::unique_ptr<RHI::IDevice> device;
    std::unordered_map<std::string, RHI::TextureHandle> textures;
    std::unordered_map<std::string, RHI::ShaderHandle> shaders;
    std::unordered_map<std::string, RHI::BufferHandle> buffers;

public:
    ResourceManager(RHI::IDevice* dev) : device(dev) {}

    // Carregar e cachear textura
    RHI::TextureHandle LoadTexture(const std::string& path) {
        auto it = textures.find(path);
        if (it != textures.end()) {
            return it->second;
        }

        ImageData image = LoadImageFile(path);
        auto handle = device->CreateTexture({
            RHI::TextureType::Texture2D,
            RHI::TextureFormat::SRGB8_Alpha8,
            image.width,
            image.height,
            1,
            1,
            true,  // generateMipmaps
            image.data
        });

        textures[path] = handle;
        return handle;
    }

    // Carregar e cachear shader
    RHI::ShaderHandle LoadShader(const std::string& vertPath,
                                  const std::string& fragPath) {
        std::string key = vertPath + "|" + fragPath;
        auto it = shaders.find(key);
        if (it != shaders.end()) {
            return it->second;
        }

        auto vertCode = ReadFile(vertPath);
        auto fragCode = ReadFile(fragPath);

        auto handle = device->CreateShader({
            {RHI::ShaderStage::Vertex, vertCode},
            {RHI::ShaderStage::Fragment, fragCode}
        });

        shaders[key] = handle;
        return handle;
    }

    ~ResourceManager() {
        for (auto& [name, handle] : textures)
            device->DestroyTexture(handle);
        for (auto& [name, handle] : shaders)
            device->DestroyShader(handle);
        for (auto& [name, handle] : buffers)
            device->DestroyBuffer(handle);
    }
};
```

### 2. Render Pass Pattern

Estruturar renderização em passes bem definidos:

```cpp
class RenderPass {
public:
    virtual ~RenderPass() = default;
    virtual void Setup(RHI::IDevice* device) = 0;
    virtual void Execute(RHI::IDevice* device, const RenderContext& ctx) = 0;
    virtual void Cleanup(RHI::IDevice* device) = 0;
};

// Exemplo: Depth Pass
class DepthPass : public RenderPass {
private:
    RHI::PipelineHandle pipeline;
    RHI::ShaderHandle shader;
    RHI::FramebufferHandle depthFramebuffer;

public:
    void Setup(RHI::IDevice* device) override {
        // Compilar shader de profundidade
        shader = device->CreateShader({
            {RHI::ShaderStage::Vertex, R"(
                #version 330 core
                layout(location = 0) in vec3 position;
                uniform mat4 mvp;
                void main() {
                    gl_Position = mvp * vec4(position, 1.0);
                }
            )"},
            {RHI::ShaderStage::Fragment, R"(
                #version 330 core
                void main() {
                    gl_FragDepth = gl_FragCoord.z;
                }
            )"}
        });

        // Criar framebuffer de profundidade
        auto depthTex = device->CreateTexture({
            RHI::TextureType::Texture2D,
            RHI::TextureFormat::Depth32F,
            1024, 1024
        });

        depthFramebuffer = device->CreateFramebuffer({
            1024, 1024,
            0,  // sem color attachments
            {},
            RHI::TextureFormat::Depth32F
        });

        device->AttachTexture(depthFramebuffer, 
                            RHI::FramebufferAttachment::Depth, 
                            depthTex);

        // Criar pipeline
        RHI::VertexLayout layout{
            sizeof(glm::vec3),
            {{0, RHI::VertexAttributeType::Float3, 0}}
        };

        pipeline = device->CreatePipeline({
            RHI::PrimitiveTopology::TriangleList,
            {RHI::CullMode::Back},
            {true, true, RHI::CompareOp::Less}
        }, shader, layout);
    }

    void Execute(RHI::IDevice* device, const RenderContext& ctx) override {
        device->BindFramebuffer(depthFramebuffer);
        device->Clear(RHI_CLEAR_DEPTH);
        device->BindPipeline(pipeline);

        for (const auto& mesh : ctx.meshes) {
            device->BindVertexArray(mesh.vao);
            device->DrawIndexed(mesh.indexCount);
        }
    }

    void Cleanup(RHI::IDevice* device) override {
        device->DestroyPipeline(pipeline);
        device->DestroyShader(shader);
        device->DestroyFramebuffer(depthFramebuffer);
    }
};

// Forward pass
class ForwardPass : public RenderPass {
    // ... similar structure ...
};

// Composição
class Renderer {
private:
    std::vector<std::unique_ptr<RenderPass>> passes;

public:
    void AddPass(std::unique_ptr<RenderPass> pass) {
        passes.push_back(std::move(pass));
    }

    void Render(RHI::IDevice* device, const RenderContext& ctx) {
        for (auto& pass : passes) {
            pass->Execute(device, ctx);
        }
    }
};
```

### 3. Descriptor Set Pattern (Unificado)

Agrupar recursos relacionados:

```cpp
struct MaterialDescriptorSet {
    RHI::TextureHandle albedo;
    RHI::TextureHandle normal;
    RHI::TextureHandle roughness;
    RHI::SamplerHandle sampler;
    RHI::BufferHandle uniformBuffer;
};

class MaterialSystem {
private:
    std::unordered_map<std::string, MaterialDescriptorSet> materials;
    RHI::IDevice* device;

public:
    MaterialSystem(RHI::IDevice* dev) : device(dev) {}

    void CreateMaterial(const std::string& name,
                       const std::string& albedoPath,
                       const std::string& normalPath,
                       const std::string& roughnessPath) {
        MaterialDescriptorSet set{
            device->CreateTexture(LoadTextureDesc(albedoPath)),
            device->CreateTexture(LoadTextureDesc(normalPath)),
            device->CreateTexture(LoadTextureDesc(roughnessPath)),
            device->CreateSampler({
                RHI::TextureWrapMode::Repeat,
                RHI::TextureWrapMode::Repeat,
                RHI::TextureWrapMode::Repeat,
                RHI::TextureFilterMode::LinearMipmapLinear,
                RHI::TextureFilterMode::Linear,
                4.0f  // Anisotropy
            }),
            device->CreateBuffer({
                RHI::BufferType::Uniform,
                RHI::BufferUsage::Dynamic,
                sizeof(MaterialUniforms)
            })
        };

        materials[name] = set;
    }

    void BindMaterial(const std::string& name) {
        auto& set = materials[name];
        device->BindTexture(0, set.albedo, set.sampler);
        device->BindTexture(1, set.normal, set.sampler);
        device->BindTexture(2, set.roughness, set.sampler);
        device->BindUniformBuffer(0, set.uniformBuffer);
    }
};
```

## Gerenciamento Avançado de Shaders

### Compilação Offline de SPIR-V

Compilar shaders em tempo de build, não em runtime:

```bash
# build_shaders.sh
#!/bin/bash

GLSLANG="glslangValidator"
SHADER_DIR="shaders"
OUTPUT_DIR="build/shaders"

mkdir -p $OUTPUT_DIR

# Compilar todos os .vert para .vert.spv
for shader in $SHADER_DIR/*.vert; do
    base=$(basename "$shader" .vert)
    $GLSLANG -V $shader -o $OUTPUT_DIR/$base.vert.spv
done

# Compilar todos os .frag para .frag.spv
for shader in $SHADER_DIR/*.frag; do
    base=$(basename "$shader" .frag)
    $GLSLANG -V $shader -o $OUTPUT_DIR/$base.frag.spv
done
```

### Pipeline de Shader Unificado

```cpp
class ShaderPipeline {
private:
    std::vector<uint32_t> spirvData;
    RHI::API targetAPI;

public:
    static ShaderPipeline LoadFromFile(const std::string& spirvPath) {
        ShaderPipeline pipeline;
        pipeline.spirvData = LoadSPIRVFile(spirvPath);
        return pipeline;
    }

    RHI::ShaderHandle CompileForAPI(RHI::IDevice* device,
                                     RHI::ShaderStage stage) {
        auto api = device->GetAPI();

        if (api == RHI::API::OpenGL) {
            // Transpile SPIR-V to GLSL
            auto result = RHI::ShaderCrossCompiler::Process(
                spirvData,
                RHI::RenderAPI::OpenGL,
                ToShaderStageType(stage),
                450  // GLSL version
            );

            if (!result.success) {
                std::cerr << "Shader compilation failed: " 
                         << result.errorMessage << "\n";
                return nullptr;
            }

            return device->CreateShader({
                {stage, result.glslSource}
            });
        } else if (api == RHI::API::Vulkan) {
            // Use SPIR-V directly
            return device->CreateShader({
                {stage, "", spirvData, "main", true}
            });
        }

        return nullptr;
    }
};
```

### Hot-Reloading de Shaders (Development Only)

```cpp
#ifdef DEBUG_SHADER_RELOAD
class ShaderWatcher {
private:
    std::string watchPath;
    std::filesystem::file_time_type lastModTime;
    std::function<void(const std::string&)> onReload;

public:
    ShaderWatcher(const std::string& path, 
                 std::function<void(const std::string&)> callback)
        : watchPath(path), onReload(callback) {
        lastModTime = std::filesystem::last_write_time(path);
    }

    void Update() {
        auto currentTime = std::filesystem::last_write_time(watchPath);
        if (currentTime > lastModTime) {
            lastModTime = currentTime;
            onReload(watchPath);
            std::cout << "Shader reloaded: " << watchPath << "\n";
        }
    }
};

// Uso
ShaderWatcher watcher("shaders/pbr.frag", [](const std::string& path) {
    // Recompilar e reatualizar shader
    auto code = ReadFile(path);
    // ... recompile and update pipeline ...
});

// Em cada frame
watcher.Update();
#endif
```

## Otimizações de Performance

### 1. Batch Rendering

```cpp
struct DrawBatch {
    RHI::PipelineHandle pipeline;
    RHI::VertexArrayHandle vao;
    uint32_t startIndex;
    uint32_t indexCount;
    glm::mat4 transform;
};

class BatchRenderer {
private:
    std::vector<DrawBatch> batches;
    std::unordered_map<RHI::PipelineHandle, std::vector<DrawBatch>> 
        sortedBatches;

public:
    void AddBatch(const DrawBatch& batch) {
        batches.push_back(batch);
    }

    void Render(RHI::IDevice* device) {
        // Sort batches by pipeline to minimize state changes
        sortedBatches.clear();
        for (auto& batch : batches) {
            sortedBatches[batch.pipeline].push_back(batch);
        }

        for (auto& [pipeline, pipelineBatches] : sortedBatches) {
            device->BindPipeline(pipeline);

            for (auto& batch : pipelineBatches) {
                device->BindVertexArray(batch.vao);
                // Update transform uniform
                device->DrawIndexed(batch.indexCount, batch.startIndex);
            }
        }

        batches.clear();
    }
};
```

### 2. Instanced Rendering

```cpp
struct InstanceData {
    glm::mat4 transform;
    glm::vec4 color;
};

class InstancedBatch {
private:
    RHI::BufferHandle instanceBuffer;
    size_t maxInstances;
    std::vector<InstanceData> instances;
    RHI::IDevice* device;

public:
    InstancedBatch(RHI::IDevice* dev, size_t maxInst)
        : device(dev), maxInstances(maxInst) {
        
        instanceBuffer = device->CreateBuffer({
            RHI::BufferType::Vertex,
            RHI::BufferUsage::Dynamic,
            maxInstances * sizeof(InstanceData)
        });
    }

    void AddInstance(const InstanceData& data) {
        if (instances.size() < maxInstances) {
            instances.push_back(data);
        }
    }

    void Submit(RHI::VertexArrayHandle vao, uint32_t indexCount) {
        if (instances.empty()) return;

        device->UpdateBuffer(instanceBuffer, instances.data(),
                           instances.size() * sizeof(InstanceData));
        device->BindVertexArray(vao);
        device->DrawInstancedIndexed(indexCount, instances.size());

        instances.clear();
    }
};
```

### 3. LOD (Level of Detail) System

```cpp
struct LODMesh {
    RHI::VertexArrayHandle vao;
    uint32_t indexCount;
    float visibilityDistance;
};

class LODGroup {
private:
    std::vector<LODMesh> lods;  // 0 = highest detail

public:
    void AddLOD(const LODMesh& mesh) {
        lods.push_back(mesh);
    }

    RHI::VertexArrayHandle SelectLOD(float distance) const {
        for (const auto& lod : lods) {
            if (distance < lod.visibilityDistance) {
                return lod.vao;
            }
        }
        return lods.back().vao;  // Lowest detail
    }
};
```

## Casos de Uso Específicos

### 1. Deferred Rendering

```cpp
class GBufferPass : public RenderPass {
private:
    RHI::FramebufferHandle gBuffer;
    RHI::TextureHandle positionMap;
    RHI::TextureHandle normalMap;
    RHI::TextureHandle albedoMap;
    RHI::PipelineHandle pipeline;

public:
    void Setup(RHI::IDevice* device) override {
        // Create G-buffer textures
        RHI::TextureDescriptor desc{
            RHI::TextureType::Texture2D,
            RHI::TextureFormat::RGBA16F,
            1024, 1024
        };

        positionMap = device->CreateTexture(desc);
        normalMap = device->CreateTexture(desc);
        albedoMap = device->CreateTexture(desc);

        // Create framebuffer with multiple attachments
        gBuffer = device->CreateFramebuffer({
            1024, 1024, 3,
            {RHI::TextureFormat::RGBA16F,
             RHI::TextureFormat::RGBA16F,
             RHI::TextureFormat::RGBA8}
        });

        device->AttachTexture(gBuffer, RHI::FramebufferAttachment::Color0, 
                            positionMap);
        device->AttachTexture(gBuffer, RHI::FramebufferAttachment::Color1, 
                            normalMap);
        device->AttachTexture(gBuffer, RHI::FramebufferAttachment::Color2, 
                            albedoMap);
    }

    void Execute(RHI::IDevice* device, const RenderContext& ctx) override {
        device->BindFramebuffer(gBuffer);
        device->Clear(RHI_CLEAR_COLOR | RHI_CLEAR_DEPTH);
        device->BindPipeline(pipeline);

        for (const auto& mesh : ctx.meshes) {
            device->BindVertexArray(mesh.vao);
            device->DrawIndexed(mesh.indexCount);
        }
    }
};

class LightingPass : public RenderPass {
    // Read from G-buffer and compute lighting
};
```

### 2. Shadow Mapping

```cpp
class ShadowMapPass {
private:
    RHI::FramebufferHandle shadowFB;
    RHI::TextureHandle shadowMap;
    RHI::ShaderHandle shadowShader;
    glm::mat4 lightViewProj;

public:
    void Setup(RHI::IDevice* device, uint32_t resolution = 2048) {
        // Create shadow map
        shadowMap = device->CreateTexture({
            RHI::TextureType::Texture2D,
            RHI::TextureFormat::Depth32F,
            resolution, resolution
        });

        shadowFB = device->CreateFramebuffer({resolution, resolution});
        device->AttachTexture(shadowFB, 
                            RHI::FramebufferAttachment::Depth,
                            shadowMap);

        // Simple shadow shader
        shadowShader = device->CreateShader({
            {RHI::ShaderStage::Vertex, R"(
                #version 330 core
                layout(location = 0) in vec3 position;
                uniform mat4 lightViewProj;
                void main() {
                    gl_Position = lightViewProj * vec4(position, 1.0);
                }
            )"},
            {RHI::ShaderStage::Fragment, R"(
                #version 330 core
                void main() { }
            )"}
        });
    }

    void Render(RHI::IDevice* device, 
               const RenderContext& ctx,
               const glm::vec3& lightPos,
               const glm::vec3& lightTarget) {
        // Setup light view matrix
        auto lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0, 1, 0));
        auto lightProj = glm::perspective(
            glm::radians(45.0f), 1.0f, 0.1f, 100.0f
        );
        lightViewProj = lightProj * lightView;

        device->BindFramebuffer(shadowFB);
        device->Clear(RHI_CLEAR_DEPTH);
        device->BindPipeline(shadowPipeline);

        // Update uniform
        device->UpdateBuffer(shadowUniformBuffer, &lightViewProj, 
                           sizeof(glm::mat4));

        for (const auto& mesh : ctx.meshes) {
            device->BindVertexArray(mesh.vao);
            device->DrawIndexed(mesh.indexCount);
        }
    }

    RHI::TextureHandle GetShadowMap() const { return shadowMap; }
};
```

### 3. Post-Processing

```cpp
class PostProcessEffect {
public:
    virtual ~PostProcessEffect() = default;
    virtual void Setup(RHI::IDevice* device) = 0;
    virtual RHI::TextureHandle Apply(RHI::IDevice* device,
                                     RHI::TextureHandle input) = 0;
};

class BloomEffect : public PostProcessEffect {
private:
    RHI::PipelineHandle downsamplePipeline;
    RHI::PipelineHandle upsamplePipeline;
    RHI::PipelineHandle compositePipeline;
    std::vector<RHI::FramebufferHandle> mipFramebuffers;
    std::vector<RHI::TextureHandle> mipTextures;

public:
    void Setup(RHI::IDevice* device) override {
        // Setup pipelines and framebuffers for bloom
    }

    RHI::TextureHandle Apply(RHI::IDevice* device,
                             RHI::TextureHandle input) override {
        // Downsampling
        device->BindFramebuffer(mipFramebuffers[0]);
        device->BindTexture(0, input, samplerPoint);
        device->BindPipeline(downsamplePipeline);
        device->DrawArrays(6);  // Screen quad

        // Upsampling and blurring
        for (size_t i = 1; i < mipFramebuffers.size(); ++i) {
            device->BindFramebuffer(mipFramebuffers[i]);
            device->BindTexture(0, mipTextures[i-1], samplerLinear);
            device->BindPipeline(upsamplePipeline);
            device->DrawArrays(6);
        }

        // Composite
        device->BindFramebuffer(nullptr);
        device->BindTexture(0, input, samplerLinear);
        device->BindTexture(1, mipTextures.back(), samplerLinear);
        device->BindPipeline(compositePipeline);
        device->DrawArrays(6);

        return input;  // Or intermediate texture
    }
};
```

## Tratamento de Erros

### Validação Robusta

```cpp
class RHIValidator {
public:
    static bool ValidateBufferDescriptor(const RHI::BufferDescriptor& desc) {
        if (desc.size == 0) {
            std::cerr << "Buffer size cannot be 0\n";
            return false;
        }
        if (desc.usage == RHI::BufferUsage::Static && !desc.data) {
            std::cerr << "Static buffer must have initial data\n";
            return false;
        }
        return true;
    }

    static bool ValidateTextureDescriptor(
        const RHI::TextureDescriptor& desc) {
        if (desc.width == 0 || desc.height == 0) {
            std::cerr << "Texture dimensions cannot be 0\n";
            return false;
        }
        if (desc.mipLevels == 0) {
            std::cerr << "Mip levels must be >= 1\n";
            return false;
        }
        return true;
    }

    static bool ValidateVertexLayout(const RHI::VertexLayout& layout) {
        if (layout.stride == 0) {
            std::cerr << "Vertex stride cannot be 0\n";
            return false;
        }
        if (layout.attributes.empty()) {
            std::cerr << "Vertex layout must have at least one attribute\n";
            return false;
        }

        // Verify no overlapping attributes
        std::vector<std::pair<uint32_t, uint32_t>> ranges;
        for (const auto& attr : layout.attributes) {
            ranges.push_back({attr.offset, attr.offset + GetAttributeSize(attr)});
        }

        for (size_t i = 0; i < ranges.size(); ++i) {
            for (size_t j = i + 1; j < ranges.size(); ++j) {
                if (RangesOverlap(ranges[i], ranges[j])) {
                    std::cerr << "Overlapping vertex attributes\n";
                    return false;
                }
            }
        }

        return true;
    }
};
```

### Wrapper com Verificação

```cpp
class SafeRHIDevice : public RHI::IDevice {
private:
    std::unique_ptr<RHI::IDevice> impl;

public:
    SafeRHIDevice(std::unique_ptr<RHI::IDevice> device)
        : impl(std::move(device)) {}

    RHI::BufferHandle CreateBuffer(const RHI::BufferDescriptor& desc) override {
        if (!RHIValidator::ValidateBufferDescriptor(desc)) {
            return nullptr;
        }
        return impl->CreateBuffer(desc);
    }

    RHI::TextureHandle CreateTexture(
        const RHI::TextureDescriptor& desc) override {
        if (!RHIValidator::ValidateTextureDescriptor(desc)) {
            return nullptr;
        }
        return impl->CreateTexture(desc);
    }

    // Delegate other methods...
};
```

## Debugging e Profiling

### Debug Output

```cpp
class RHIDebugger {
private:
    bool enableValidation = true;
    std::ofstream logFile;

public:
    void LogDrawCall(RHI::PipelineHandle pipeline,
                    RHI::VertexArrayHandle vao,
                    uint32_t indexCount) {
        if (enableValidation) {
            logFile << "DrawCall: pipeline=" << pipeline
                   << ", vao=" << vao
                   << ", indices=" << indexCount << "\n";
        }
    }

    void LogResourceCreation(const std::string& type,
                            uint64_t handle,
                            const std::string& name) {
        logFile << "[CREATE] " << type << " #" << handle
               << " (" << name << ")\n";
    }

    void LogResourceDestruction(const std::string& type, uint64_t handle) {
        logFile << "[DESTROY] " << type << " #" << handle << "\n";
    }
};
```

### Performance Monitoring

```cpp
class RenderMetrics {
public:
    struct FrameStats {
        uint32_t drawCalls = 0;
        uint32_t trianglesRendered = 0;
        double frameTime = 0.0;
        double gpuTime = 0.0;
    };

private:
    FrameStats currentFrame;
    std::vector<FrameStats> frameHistory;
    static constexpr size_t HISTORY_SIZE = 60;

public:
    void RegisterDrawCall(uint32_t indexCount) {
        currentFrame.drawCalls++;
        currentFrame.trianglesRendered += indexCount / 3;
    }

    void EndFrame(double cpuTime, double gpuTime) {
        currentFrame.frameTime = cpuTime;
        currentFrame.gpuTime = gpuTime;

        frameHistory.push_back(currentFrame);
        if (frameHistory.size() > HISTORY_SIZE) {
            frameHistory.erase(frameHistory.begin());
        }

        currentFrame = {};
    }

    double GetAverageFrameTime() const {
        double sum = 0;
        for (const auto& stats : frameHistory) {
            sum += stats.frameTime;
        }
        return frameHistory.empty() ? 0 : sum / frameHistory.size();
    }

    uint32_t GetAverageDrawCalls() const {
        uint32_t sum = 0;
        for (const auto& stats : frameHistory) {
            sum += stats.drawCalls;
        }
        return frameHistory.empty() ? 0 : sum / frameHistory.size();
    }
};
```

## Migração entre APIs

### Trocar de OpenGL para Vulkan

1. **Código já existente continua funcionando:**
```cpp
// Antes: Específico do OpenGL
auto device = RHI::CreateDevice(RHI::API::OpenGL, window);

// Depois: Apenas muda esta linha
auto device = RHI::CreateDevice(RHI::API::Vulkan, window);
```

2. **Se usar SPIR-V (recomendado):**
```cpp
// Compilar shaders offline uma vez:
glslangValidator -V shader.vert -o shader.vert.spv
glslangValidator -V shader.frag -o shader.frag.spv

// Código fica agnóstico à API:
auto vertByteCode = LoadFile("shader.vert.spv");
auto fragByteCode = LoadFile("shader.frag.spv");

auto shader = device->CreateShader({
    {RHI::ShaderStage::Vertex, "", vertByteCode, "main", true},
    {RHI::ShaderStage::Fragment, "", fragByteCode, "main", true}
});
```

3. **Ajustar Coordenadas:**
```cpp
// Após criar device
auto api = device->GetAPI();

glm::mat4 projection = glm::perspective(...);
if (api == RHI::API::Vulkan) {
    projection[1][1] *= -1.0f;  // Flip Y
}

// Para Vulkan sem glClipControl
// Depth já é 0-1 por padrão
```

4. **Testar ambos em paralelo:**
```cpp
#ifdef DUAL_API_TESTING
    auto deviceGL = RHI::CreateDevice(RHI::API::OpenGL, window);
    auto deviceVK = RHI::CreateDevice(RHI::API::Vulkan, window);

    // Renderizar mesmo conteúdo com ambas
    if (testMode == TestMode::OpenGL) {
        device = deviceGL.get();
    } else {
        device = deviceVK.get();
    }
#endif
```

---

## Referências Adicionais

- [SPIR-V Specification](https://www.khronos.org/registry/spir-v/)
- [SPIRV-Cross Documentation](https://github.com/KhronosGroup/SPIRV-Cross/wiki)
- [Vulkan Best Practices](https://developer.nvidia.com/blog/vulkan-best-practices/)
- [OpenGL Best Practices](https://www.khronos.org/opengl/wiki/OpenGL_Coding_Style)
