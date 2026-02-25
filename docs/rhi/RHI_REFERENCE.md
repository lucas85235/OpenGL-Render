# RHI - Referência Rápida e Roadmap

## 🚀 Quick Start

### 1. Inicializar
```cpp
auto device = RHI::CreateDevice(RHI::API::OpenGL, window);
device->Initialize();
```

### 2. Create Shader
```cpp
auto shader = device->CreateShader({
    {RHI::ShaderStage::Vertex, vertexSource},
    {RHI::ShaderStage::Fragment, fragmentSource}
});
```

### 3. Create Geometry
```cpp
auto vbo = device->CreateBuffer({RHI::BufferType::Vertex, RHI::BufferUsage::Static, vboSize, vboData});
auto ibo = device->CreateBuffer({RHI::BufferType::Index, RHI::BufferUsage::Static, iboSize, iboData});
auto vao = device->CreateVertexArray(vbo, ibo, layout);
```

### 4. Create Pipeline
```cpp
auto pipeline = device->CreatePipeline(pipelineDesc, shader, layout);
```

### 5. Render
```cpp
device->BeginFrame();
device->BindPipeline(pipeline);
device->BindVertexArray(vao);
device->DrawIndexed(indexCount);
device->EndFrame();
```

---

## 📚 Documentação Completa

| Documento | Conteúdo | Para Quem |
|---|---|---|
| [RHI_SYSTEM.md](RHI_SYSTEM.md) | Guia completo, tipos, interfaces, exemplos básicos | Novos desenvolvedores, referência |
| [RHI_ADVANCED.md](RHI_ADVANCED.md) | Padrões arquiteturais, otimizações, casos avançados | Desenvolvedores experientes |
| [RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md) | Debugging, FAQ, soluções comuns | Quando algo não funciona |
| **Este arquivo** | Referência rápida, índice, roadmap | Busca rápida |

---

## 🔍 Índice por Tarefa

### Renderização Básica

- **Renderizar um triângulo** → [Exemplo 1 em RHI_SYSTEM.md](RHI_SYSTEM.md#exemplo-1-renderizar-um-triângulo)
- **Renderizar modelo 3D** → [Guia de Uso em RHI_SYSTEM.md](RHI_SYSTEM.md#guia-de-uso)
- **Usar uniform buffers** → [Exemplo 2 em RHI_SYSTEM.md](RHI_SYSTEM.md#exemplo-2-usar-uniform-buffer)

### Texturas

- **Carregar textura 2D** → [Criar Texturas em RHI_SYSTEM.md](RHI_SYSTEM.md#4-criar-texturas)
- **Cubemaps** → [FAQ em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#q-por-que-meu-cubemap-está-invertido)
- **Formatação (sRGB, linear)** → [Cores em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#cores-estranhas)

### Shaders

- **Compilar offline em SPIR-V** → [Compilação em RHI_ADVANCED.md](RHI_ADVANCED.md#compilação-offline-de-spir-v)
- **Usar SPIR-V em runtime** → [FAQ em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#q-como-compilar-shaders-offline)
- **Hot-reloading** → [Hot-Reloading em RHI_ADVANCED.md](RHI_ADVANCED.md#hot-reloading-de-shaders-development-only)

### Otimização

- **Batch rendering** → [Batch Rendering em RHI_ADVANCED.md](RHI_ADVANCED.md#1-batch-rendering)
- **Instanced rendering** → [Instanced Rendering em RHI_ADVANCED.md](RHI_ADVANCED.md#2-instanced-rendering)
- **LOD system** → [LOD System em RHI_ADVANCED.md](RHI_ADVANCED.md#3-lod-level-of-detail-system)

### Advanced Rendering

- **Deferred rendering** → [Deferred Rendering em RHI_ADVANCED.md](RHI_ADVANCED.md#1-deferred-rendering)
- **Shadow mapping** → [Shadow Mapping em RHI_ADVANCED.md](RHI_ADVANCED.md#2-shadow-mapping)
- **Post-processing** → [Post-Processing em RHI_ADVANCED.md](RHI_ADVANCED.md#3-post-processing)
- **G-Buffer** → [Deferred Rendering em RHI_ADVANCED.md](RHI_ADVANCED.md#1-deferred-rendering)

### Debugging

- **Blank screen** → [Blank Screen em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#blank-screen--nada-renderiza)
- **Texturas invertidas** → [Texturas em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#texturas-loucas-invertidas-distorcidas)
- **Shader errors** → [Shader Errors em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#shader-compilation-errors)
- **Performance** → [Performance em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#performance)
- **Memory leaks** → [Memory Leaks em RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md#memoryresource-leaks)

### Arquitetura e Padrões

| Padrão | Arquivo | Descrição |
|---|---|---|
| Resource Manager | RHI_ADVANCED.md | Cachear e gerenciar recursos |
| Render Pass | RHI_ADVANCED.md | Estruturar renderização em passes |
| Descriptor Sets | RHI_ADVANCED.md | Agrupar recursos relacionados |
| Material System | RHI_ADVANCED.md | Gerenciar materiais |

---

## ⚡ Cheat Sheet - Handlers

Todos os handles são `uint64_t`, não ponteiros:

```cpp
RHI::BufferHandle buffer = device->CreateBuffer(...);
if (buffer == 0) { /* erro */ }
device->UpdateBuffer(buffer, data, size);
device->DestroyBuffer(buffer);
// NÃO: delete buffer;  ❌
```

---

## 📊 Tipos Mais Usados

### Buffers
```cpp
device->CreateBuffer({
    .type = RHI::BufferType::Vertex,           // Vertex, Index, Uniform
    .usage = RHI::BufferUsage::Static,         // Static, Dynamic, Stream
    .size = 1024,
    .data = ptr
});
```

### Texturas
```cpp
device->CreateTexture({
    .type = RHI::TextureType::Texture2D,       // 2D, 3D, Cube
    .format = RHI::TextureFormat::SRGB8_Alpha8,// Cores: sRGB, dados: RGB
    .width = 1024,
    .height = 1024,
    .generateMipmaps = true
});
```

### Samplers
```cpp
device->CreateSampler({
    .wrapS = RHI::TextureWrapMode::Repeat,
    .wrapT = RHI::TextureWrapMode::Repeat,
    .minFilter = RHI::TextureFilterMode::LinearMipmapLinear,
    .magFilter = RHI::TextureFilterMode::Linear,
    .anisotropy = 4.0f
});
```

### Pipeline
```cpp
device->CreatePipeline({
    .topology = RHI::PrimitiveTopology::TriangleList,
    .rasterizer = {
        .cullMode = RHI::CullMode::Back,
        .frontFace = RHI::FrontFace::CounterClockwise
    },
    .depthStencil = {
        .depthTest = true,
        .depthCompare = RHI::CompareOp::Less
    },
    .blend = {
        .enabled = false  // true para alpha blending
    }
}, shader, layout);
```

---

## 🔄 Coordenadas (OpenGL vs Vulkan)

| Aspecto | OpenGL | Vulkan | Solução |
|---|---|---|---|
| Y-Axis | Y-up | Y-down | Flip Y na matrix |
| Z-Range | -1 a +1 | 0 a 1 | `glClipControl` (GL4.5+) |
| Screen Origin | Bottom-left | Top-left | Trata `SetViewport` |

**Quick fix:**
```cpp
auto proj = glm::perspective(...);
if (device->GetAPI() == RHI::API::Vulkan) {
    proj[1][1] *= -1.0f;  // Flip Y
}
```

---

## 📋 Checklist de Deploy

Antes de passar pra produção:

- [ ] Validar todos os descriptores ([Validator em RHI_ADVANCED.md](RHI_ADVANCED.md#validação-robusta))
- [ ] Compilar shaders offline em SPIR-V
- [ ] Remover debug logging
- [ ] Testar em ambas APIs (OpenGL e Vulkan)
- [ ] Verificar memory leaks em debugger
- [ ] Profile com GPU queries (timestamps)
- [ ] Batch renderização (< 1000 draw calls/frame)
- [ ] Mipmaps em todas as texturas grandes
- [ ] Proper viewport e scissors setup
- [ ] Error handling robusto

---

## 🛣️ Roadmap (Em Desenvolvimento)

### Planned Features

- [ ] DirectX 12 Backend
- [ ] Metal Backend (macOS/iOS)
- [ ] Ray Tracing Support
- [ ] Async Compute Shaders
- [ ] Dynamic SkyBox/IBL
- [ ] Meshlet Rendering
- [ ] Streaming Resources
- [ ] Render Graph API
- [ ] GPU Profiler Integration
- [ ] Shader Validation Tool

### API Stability

- [x] Core interfaces (`IDevice`, handles)
- [x] Basic resource creation (buffers, textures)
- [x] Rendering (pipelines, draw calls)
- [x] Shader system (SPIR-V, cross-compilation)
- [x] OpenGL implementation (core feature complete)
- [x] Vulkan implementation (core feature complete)
- [ ] Additional features (see above)

### Known Limitations

1. **Multi-GPU**: Não suportado (single GPU por device)
2. **Compute**: Básico, sem shared memory unificada
3. **Async**: OpenGL tem limitações de threading
4. **Validation**: Use external tools (RenderDoc, NVIDIA NSight)

---

## 🔗 Links Importantes

### Documentação Interna
- [RHI_SYSTEM.md](RHI_SYSTEM.md) - Referência completa
- [RHI_ADVANCED.md](RHI_ADVANCED.md) - Padrões e otimizações
- [RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md) - Debugging e FAQ
- [ENGINE_MODULES.md](ENGINE_MODULES.md) - Visão geral do engine
- [GETTING_STARTED.md](GETTING_STARTED.md) - Setup inicial

### Especificações Externas
- [Vulkan Specification](https://www.khronos.org/registry/vulkan/)
- [OpenGL Documentation](https://www.opengl.org/documentation/)
- [SPIR-V Specification](https://www.khronos.org/registry/spir-v/)
- [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross)
- [glslang Validator](https://github.com/KhronosGroup/glslang)

### Tools
- [RenderDoc](https://renderdoc.org/) - GPU debugging
- [NVIDIA NSight](https://developer.nvidia.com/nsight-graphics) - Profiling
- [VulkanMemoryAllocator](https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator)
- [Khronos Validation Layers](https://github.com/KhronosGroup/Vulkan-ValidationLayers)

---

## 🎯 Decisão: Qual Backend Usar?

```
┌─ Prototipagem rápida?
├─ SIM → OpenGL (mais fácil, iteração rápida)
├─ NÃO ⤵
│
├─ Target é Web?
├─ SIM → OpenGL + Emscripten
├─ NÃO ⤵
│
├─ Performance crítica?
├─ SIM → Vulkan (especialmente muitos objetos/shaders)
├─ NÃO ⤵
│
├─ Multi-threading importante?
├─ SIM → Vulkan
├─ NÃO ⤵
│
└─ macOS exclusive?
  └─ SIM → Metal (future) | OpenGL (fallback)
  └─ NÃO → Considerar ambos, RHI abstrai
```

---

## 💡 Pro Tips

1. **Sempre use SPIR-V para portabilidade**
   ```bash
   glslangValidator -V shader.glsl -o shader.spv
   ```

2. **Batch resources juntas**
   ```cpp
   // ✅ Uma lista de descriptores, uma chamada
   std::vector<RHI::ShaderDescriptor> stages = {...};
   device->CreateShader(stages);
   ```

3. **Debug em OpenGL primeiro, validate em Vulkan**
   - OpenGL dá bons error messages
   - Vulkan é mais estrito

4. **Use resource manager pattern**
   - Evita alocações/dealocações por frame
   - Caching automático

5. **Profile GPU, não apenas CPU**
   ```cpp
   // GPU queries, timestamps
   device->BeginTimestamp(...);
   // ... render ...
   device->EndTimestamp(...);
   ```

---

## 📞 Support

Para questões detalhadas:

1. Busque no [RHI_TROUBLESHOOTING.md](RHI_TROUBLESHOOTING.md) (FAQ)
2. Veja exemplos em [RHI_SYSTEM.md](RHI_SYSTEM.md) (Exemplos Práticos)
3. Consulte padrões em [RHI_ADVANCED.md](RHI_ADVANCED.md)
4. Leia source em `/src/rhi/`
5. Use debugger (RenderDoc para GPU)

---

**Last Updated**: 2026-02-25  
**RHI Version**: v1.0 (Stable)  
**Supported APIs**: OpenGL 3.3+, Vulkan 1.0+
