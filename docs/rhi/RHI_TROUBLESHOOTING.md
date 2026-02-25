# RHI - Troubleshooting e FAQ

## 📖 Conteúdo

1. [Problemas Comuns](#problemas-comuns)
2. [Erros de Compilação](#erros-de-compilação)
3. [Erros de Runtime](#erros-de-runtime)
4. [Problemas de Renderização](#problemas-de-renderização)
5. [Performance](#performance)
6. [FAQ](#faq)
7. [Checklist de Debug](#checklist-de-debug)

## Problemas Comuns

### Blank Screen / Nada Renderiza

**Sintomas:** Tela preta, sem erros aparentes

**Causas Possíveis:**

1. **Pipeline não foi bindado**
   ```cpp
   // ❌ Errado
   device->BindVertexArray(vao);
   device->DrawIndexed(count);

   // ✅ Correto
   device->BindPipeline(pipeline);
   device->BindVertexArray(vao);
   device->DrawIndexed(count);
   ```

2. **Viewport não configurada**
   ```cpp
   // ✅ Configure viewport
   device->SetViewport({
       0, 0,           // x, y
       width, height,  // largura, altura
       0.0f, 1.0f      // min/max depth
   });
   ```

3. **Buffer vazio ou com offset incorreto**
   ```cpp
   // ✅ Verifique
   assert(!vertices.empty());
   assert(vbo != nullptr);
   device->DrawIndexed(indexCount, 0);  // offset = 0
   ```

4. **Matriz de projeção invertida/inválida**
   ```cpp
   // ❌ Errado
   auto proj = glm::perspective(0.0f, aspect, near, far);  // fov = 0!

   // ✅ Correto
   auto proj = glm::perspective(glm::radians(45.0f), aspect, near, far);
   ```

5. **Culling desativado (tri-winding order)**
   ```cpp
   // Se CCW (Counter-Clockwise) esperado mas está CW (Clockwise)
   RHI::RasterizerState rasterizer{
       .cullMode = RHI::CullMode::None,  // Desativar para debug
       .frontFace = RHI::FrontFace::CounterClockwise
   };
   ```

**Debugging:**

```cpp
// Debug draw calls
class DebugRenderer {
public:
    void DrawBoundingBox(const glm::mat4& mvp) {
        // Draw axis lines para visualizar sistem de coordenadas
        device->BindPipeline(debugPipeline);
        device->DrawArrays(6);  // X, Y, Z axes
    }
};
```

---

### Texturas Loucas (Invertidas, Distorcidas)

**Sintoma:** Textura renderizada incorretamente

**Causas:**

1. **Y-invertido entre OpenGL e Vulkan**
   ```cpp
   // ✅ Solução
   if (device->GetAPI() == RHI::API::Vulkan) {
       texCoord.y = 1.0 - texCoord.y;
   }
   ```

2. **Formato de imagem inconsistente**
   ```cpp
   // ✅ Verificar
   if (image.format != TextureFormat::SRGB8_Alpha8) {
       ConvertFormat(image);  // Converter se necessário
   }
   ```

3. **Sampler settings incorretos**
   ```cpp
   // ❌ Errado - MipLevels configurado mas não gerado
   TextureDescriptor desc{
       .mipLevels = 4,
       .generateMipmaps = false,  // ⚠️ Inconsistente!
       .data = imageData
   };

   // ✅ Correto
   auto texture = device->CreateTexture(desc);
   device->GenerateMipmaps(texture);
   ```

4. **Wrap mode wrong**
   ```cpp
   // Se textura aparece repetida quando não deve:
   RHI::SamplerDescriptor sampler{
       .wrapS = RHI::TextureWrapMode::ClampToEdge,
       .wrapT = RHI::TextureWrapMode::ClampToEdge
   };
   ```

---

### Shader Compilation Errors

**❌ "Shader program linking failed"**

```cpp
// Causa comum: Vertex e Fragment incompatíveis
// Vertex output attributes devem existir no Fragment input

// ✅ Correto
// vertex.glsl
out vec3 vNormal;
out vec2 vTexCoord;

// fragment.glsl
in vec3 vNormal;
in vec2 vTexCoord;
```

**❌ "Uniform not found"**

```cpp
// ✅ Verificar se uniform existe no código compilado
// (dead code elimination pode remover uniformes não usados)

// ✅ Usar uniforms ou as serão otimizadas
uniform mat4 uModel;
void main() {
    gl_Position = uModel * vec4(position, 1.0);  // Use it!
}
```

---

## Erros de Compilação

### CMake/Build Errors

**❌ "undefined reference to RHI::`...`"**

```bash
# Problema: Não linkando contra RHI library

# ✅ Solução - CMakeLists.txt
target_link_libraries(your_app RHI)
```

**❌ "GLEW/Vulkan headers not found"**

```bash
# ✅ Instalar dependências
sudo apt install libglew-dev  # Ubuntu/Debian
brew install glew             # macOS

# Ou especificar em CMakeLists.txt
find_package(GLEW REQUIRED)
find_package(Vulkan REQUIRED)
target_link_libraries(your_app GLEW::GLEW Vulkan::Vulkan)
```

### C++ Build Errors

**❌ "undefined member in std::vector"**

```cpp
// ❌ Errado
std::vector<RHI::BufferHandle> handles;
// handles é uint64_t, não nullptr!
auto h = handles[0];

// ✅ Correto - Validar handle válido
class BufferManager {
    static constexpr RHI::BufferHandle INVALID_HANDLE = 0;
    
    bool IsValid(RHI::BufferHandle handle) const {
        return handle != INVALID_HANDLE;
    }
};
```

**❌ "No matching member function"**

```cpp
// ❌ Errado
device.CreateBuffer(desc);  // device é shared_ptr, use ->

// ✅ Correto
device->CreateBuffer(desc);
```

---

## Erros de Runtime

### Memory/Resource Leaks

**Sintoma:** Memory aumenta continuamente, ou segfault ao destruir

**Debugging:**

```cpp
class ResourceTracker {
private:
    std::unordered_map<uint64_t, std::string> allocations;

public:
    void TrackAllocation(uint64_t handle, const std::string& name) {
        allocations[handle] = name;
    }

    void TrackDeallocation(uint64_t handle) {
        auto it = allocations.find(handle);
        if (it == allocations.end()) {
            std::cerr << "Double-delete of " << handle << "\n";
        } else {
            allocations.erase(it);
        }
    }

    void PrintAllocations() const {
        for (const auto& [handle, name] : allocations) {
            std::cout << "Leaked: " << name << "\n";
        }
    }
};
```

**Checklist:**

- [ ] `CreateBuffer` pareado com `DestroyBuffer`
- [ ] `CreateTexture` pareado com `DestroyTexture`
- [ ] `CreateShader` pareado com `DestroyShader`
- [ ] `CreatePipeline` pareado com `DestroyPipeline`
- [ ] `CreateFramebuffer` pareado com `DestroyFramebuffer`
- [ ] `Shutdown()` chamado antes de destruir device

---

### Access Violations / Segmentation Faults

**Causa comum: Handle inválido**

```cpp
// ❌ Errado
RHI::BufferHandle buffer = nullptr;  // Handles são uint64_t, não ponteiros!
device->UpdateBuffer(buffer, data, size);  // Crash!

// ✅ Correto
RHI::BufferHandle buffer = device->CreateBuffer(desc);
if (buffer == 0) {
    std::cerr << "Failed to create buffer\n";
    return;
}
device->UpdateBuffer(buffer, data, size);
```

**Null pointer check:**

```cpp
// ✅ Siempre validar device
auto device = RHI::CreateDevice(RHI::API::OpenGL, window);
if (!device) {
    std::cerr << "Failed to create device\n";
    return false;
}

if (!device->Initialize()) {
    std::cerr << "Failed to initialize\n";
    return false;
}
```

---

## Problemas de Renderização

### Cores estranhas

**Causa: Mismatch ColorSpace (sRGB vs Linear)**

```cpp
// ❌ Errado - misturando linear e sRGB
TextureDescriptor desc{
    .format = RHI::TextureFormat::RGB8,     // sRGB implícito
    // ... mas shader trata como linear
};

// ✅ Correto - ser consistente
// Para cores: use SRGB8
TextureDescriptor colorDesc{
    .format = RHI::TextureFormat::SRGB8_Alpha8
};

// Para dados (normals, height maps): use RGB8
TextureDescriptor dataDesc{
    .format = RHI::TextureFormat::RGB8
};
```

### Banding Artifacts

**Sintoma:** Gradientes mostram bands coloridas em vez de smooth

```cpp
// ❌ Errado - precisão insuficiente
TextureDescriptor desc{
    .format = RHI::TextureFormat::RGB8  // 8 bits = 256 valores
};

// ✅ Correto para post-processing
TextureDescriptor desc{
    .format = RHI::TextureFormat::RGBA16F  // 10 bits efetivos
};
```

### Z-Fighting (Flickering depth)

**Sintoma:** Dois triângulos na mesma profundidade flickeiam

```cpp
// ❌ Culpável: near muito grande, far muito pequeno
auto proj = glm::perspective(fov, aspect, 100.0f, 100.1f);  // ⚠️!

// ✅ Correto: larger ratio
auto proj = glm::perspective(fov, aspect, 0.1f, 100.0f);

// ✅ Ou usar Reverse-Z (Vulkan-style)
auto proj = glm::perspective(fov, aspect, 100.0f, 0.1f);
device->SetDepthCompare(RHI::CompareOp::GreaterOrEqual);
```

### Black / Missing Objects

**Causa: Back-face culling**

```cpp
// Debug: desativar culling
RHI::RasterizerState debug{
    .cullMode = RHI::CullMode::None  // Ver todos os triângulos
};

// Se objetos aparecem: problema é winding order
// Corrija ao carregar modelo ou inverta frontFace
```

---

## Performance

### Baixa FPS (Low Frame Rate)

**Diagnóstico:**

```cpp
double frameStart = glfwGetTime();

device->BeginFrame();
// ... render ...
device->EndFrame();

double frameTime = glfwGetTime() - frameStart;
double fps = 1.0 / frameTime;
std::cout << "FPS: " << fps << " (" << frameTime * 1000.0 << "ms)\n";
```

**Common bottlenecks:**

1. **Too many draw calls**
   ```cpp
   // ❌ Errado
   for (auto& obj : objects) {
       device->BindPipeline(obj.pipeline);
       device->DrawIndexed(obj.count);  // 1000 draw calls!
   }

   // ✅ Correto - batch
   BatchRenderer batch;
   for (auto& obj : objects) {
       batch.Add(obj);
   }
   batch.Render();  // ~10 draw calls
   ```

2. **GPU stalls (syncing)**
   ```cpp
   // ❌ Errado
   device->UpdateBuffer(buf, data, size);
   device->BindUniformBuffer(0, buf);
   device->DrawIndexed(count);  // Waiting for update!

   // ✅ Correto - duplo buffer
   std::vector<BufferHandle> uniforms(3);  // Triple buffer
   int currentFrame = 0;

   // In render loop:
   device->UpdateBuffer(uniforms[currentFrame], data, size);
   currentFrame = (currentFrame + 1) % 3;
   ```

3. **Large allocations per frame**
   ```cpp
   // ❌ Errado
   for (int i = 0; i < 1000; i++) {
       auto buf = device->CreateBuffer(desc);  // Allocate!
       device->DestroyBuffer(buf);             // Free!
   }

   // ✅ Correto - pool/reuse
   class BufferPool {
       std::vector<BufferHandle> available;
       std::vector<BufferHandle> inUse;

       BufferHandle Acquire() {
           if (!available.empty()) {
               auto buf = available.back();
               available.pop_back();
               inUse.push_back(buf);
               return buf;
           }
           // Create new
       }

       void Release(BufferHandle buf) {
           inUse.erase(std::remove(inUse.begin(), inUse.end(), buf));
           available.push_back(buf);
       }
   };
   ```

### GPU Stalls

**Indicador:** 0% GPU utilization, 100% CPU wait

```cpp
// ❌ Errado - ReadBack síncrono
void ReadPixels(RHI::TextureHandle tex) {
    device->BindTexture(0, tex, sampler);
    std::vector<uint32_t> pixels(1024 * 1024);
    device->ReadTexture(tex, pixels.data());  // GPU espera! ⚠️
}

// ✅ Correto - Async readback
class AsyncReadback {
private:
    std::queue<ReadbackRequest> pending;

public:
    void RequestReadback(TextureHandle tex) {
        pending.push({tex, frameIndex});
    }

    bool TryRetrieveResults(std::vector<uint32_t>& out) {
        if (pending.front().frameIndex + 2 < currentFrame) {
            // Safe to read (2 frames past)
            out = pending.front().data;
            pending.pop();
            return true;
        }
        return false;
    }
};
```

---

## FAQ

### Q: Qual API usar - OpenGL ou Vulkan?

**A:** Depende do caso:

| Aspecto | OpenGL | Vulkan |
|---|---|---|
| Facilidade | Mais fácil | Mais complexo |
| Performance | Bom | Excelente (larga escala) |
| Portabilidade | MacOS, Linux, Web | Não Web |
| Manutenção | Legacy | Futuro |
| Threads | Limited | Excelente |

**Recomendação:**
- Prototipagem: **OpenGL**
- Produção Desktop: **Vulkan**
- Mobile: Ambos (via RHI)
- Web: OpenGL via Emscripten

---

### Q: Como compilar shaders offline?

**A:**

```bash
# Instalar glslangValidator
sudo apt install glslang-tools  # Linux
brew install glslang            # macOS

# Compilar para SPIR-V
glslangValidator -V shader.vert -o shader.vert.spv
glslangValidator -V shader.frag -o shader.frag.spv

# Em CMakeLists.txt
add_custom_command(OUTPUT shader.vert.spv
    COMMAND glslangValidator -V ${CMAKE_SOURCE_DIR}/shader.vert
                             -o ${CMAKE_BINARY_DIR}/shader.vert.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/shader.vert
)
add_custom_target(CompileShaders ALL DEPENDS shader.vert.spv)
add_dependencies(your_app CompileShaders)
```

---

### Q: SPIR-V é realmente necessário?

**A:** Não, mas recomendado:

```cpp
// Opção 1: GLSL direto (OpenGL only)
auto shader = device->CreateShader({
    {RHI::ShaderStage::Vertex, glslSource}
});

// Opção 2: SPIR-V (ambas APIs)
auto shader = device->CreateShader({
    {RHI::ShaderStage::Vertex, "", spirvBinary, "main", true}
});

// SPIR-V é melhor para: portabilidade, caching compilações
```

---

### Q: Como fazer multi-threading com RHI?

**A:** OpenGL tem limitações:

```cpp
// ❌ Errado - OpenGL não é thread-safe
std::thread t([device]() {
    device->CreateBuffer(...);  // CRASH
});

// ✅ Correto - thread-pool CPU-side
struct CmdBuffer {
    std::vector<RenderCommand> commands;
};

// Thread worker
void BuildCommandBuffer(CmdBuffer& cmd, const Scene& scene) {
    // Apenas construir commands, não submeter
}

// Main thread
void SubmitCommands(const CmdBuffer& cmd) {
    for (const auto& cmd : cmd.commands) {
        ExecuteCommand(device, cmd);
    }
}
```

Vulkan é melhor para multi-threading.

---

### Q: Qual diferença prática entre Static/Dynamic/Stream?

**A:**

```cpp
// STATIC: Imutável, carregado uma vez
auto vbo = device->CreateBuffer({
    RHI::BufferUsage::Static,
    sizeof(geometryData),
    geometryData
});
// UpdateBuffer() não deve ser usado

// DYNAMIC: Muda algumas vezes por frame
auto uniformBuf = device->CreateBuffer({
    RHI::BufferUsage::Dynamic,
    sizeof(Uniforms)
});
// Em cada frame:
device->UpdateBuffer(uniformBuf, &uniforms, sizeof(Uniforms));

// STREAM: Muda a cada frame (pode ser "re-alocado")
auto particleBuf = device->CreateBuffer({
    RHI::BufferUsage::Stream,
    maxParticles * sizeof(Particle)
});
// Cada frame: UpdateBuffer() (GPU pode otimizar)
```

---

### Q: Por que meu cubemap está invertido?

**A:** Convenção de faces OpenGL vs Vulkan

```cpp
// OpenGL: +X, -X, +Y, -Y, +Z, -Z (order específica)
// Vulkan: pode ser diferente

// ✅ Verificar ordem ao carregar
enum class CubemapFace {
    PositiveX,  // +X (right)
    NegativeX,  // -X (left)
    PositiveY,  // +Y (top)
    NegativeY,  // -Y (bottom)
    PositiveZ,  // +Z (front)
    NegativeZ   // -Z (back)
};

// Carregar na ordem certa
device->UpdateTextureCubeFace(cubemap, RHI::CubemapFace::PositiveX, rightData);
device->UpdateTextureCubeFace(cubemap, RHI::CubemapFace::NegativeX, leftData);
// ...
```

---

### Q: Como debug OpenGL errors?

**A:**

```cpp
#include <GL/glew.h>

void CheckGLError(const char* label) {
    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::cerr << label << ": GL Error " << err << "\n";
        err = glGetError();
    }
}

// Usar em pontos críticos
device->CreateBuffer(...);
CheckGLError("CreateBuffer");
```

---

## Checklist de Debug

Ao enfrentar problemas:

- [ ] Device foi inicializado? `device->Initialize()`
- [ ] Viewport está configurado?
- [ ] Pipeline está bindado antes de draw call?
- [ ] Vertex array está bindado?
- [ ] Shaders compilaram sem erro?
- [ ] Texturas estão bindadas corretamente?
- [ ] Uniform buffers estão updatados?
- [ ] Scissor test não está cortando área (ou desativar)
- [ ] Clear color não é a mesma cor que renderizando (para debug)
- [ ] NDC (Normalized Device Coordinates) dentro de [-1, 1]? (OpenGL)
- [ ] Z no shape está entre near/far?
- [ ] Frontface culling OK? (CW vs CCW)
- [ ] Se em Vulkan: glClipControl não existe, projeto ajustado?
- [ ] Resources sendo destruídos nessa ordem correta?
- [ ] Device em estado válido antes de EndFrame?

---

## Contate

Para questões não cobertas, veja:
- [RHI_SYSTEM.md](RHI_SYSTEM.md) - Documentação completa
- [RHI_ADVANCED.md](RHI_ADVANCED.md) - Padrões avançados
- Source code em `/src/rhi/`
