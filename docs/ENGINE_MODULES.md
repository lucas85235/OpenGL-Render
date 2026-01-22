# Mini Motor Gráfico - Engine Modules

Este documento descreve os módulos principais do Mini Motor Gráfico implementados no projeto.

## 📦 Módulos Disponíveis

| Módulo | Arquivo | Descrição |
|--------|---------|-----------|
| Camera | `src/core/camera.hpp` | Sistema de câmera FPS/Orbit |
| Lighting | `src/renderer/lighting.hpp` | Luzes direcionais, pontuais e spot |
| Post-Processing | `src/renderer/post_process.hpp` | 7 efeitos de pós-processamento |
| Primitives | `src/renderer/primitives.hpp` | 8 geometrias procedurais |
| Scene Serialization | `src/scene/scene_serializer.hpp` | Save/Load de cenas |
| Shader Hot-Reload | `src/renderer/shader_watcher.hpp` | Recompilação automática |
| Debug UI | `src/core/debug_ui.hpp` | Stats e console de debug |

---

## 🎥 Camera System

**Arquivo:** `src/core/camera.hpp`

### Modos
- **FPS Mode**: Movimento livre com WASD
- **Orbit Mode**: Rotação ao redor de um ponto

### Controles Padrão
| Tecla | Ação |
|-------|------|
| W/A/S/D | Movimento |
| Q/E | Descer/Subir |
| Mouse Direito + Arrastar | Look around |
| Scroll | Zoom |
| P | Toggle perspectiva/ortográfica |

### Uso
```cpp
#include "core/camera.hpp"

Camera camera(CameraMode::FPS);
camera.SetPosition({0, 5, 10});
camera.ProcessKeyboard(Camera::FORWARD, deltaTime);
camera.ProcessMouse(xOffset, yOffset);

glm::mat4 view = camera.GetViewMatrix();
glm::mat4 proj = camera.GetProjectionMatrix(aspectRatio);
```

---

## 💡 Lighting System

**Arquivo:** `src/renderer/lighting.hpp`

### Tipos de Luz
- `DirectionalLight` - Luz global (sol)
- `PointLight` - Luz pontual com atenuação
- `SpotLight` - Luz spot com cone interno/externo

### LightManager
```cpp
#include "renderer/lighting.hpp"

LightManager lights;
lights.SetSunLight({glm::vec3(-1, -1, 0), glm::vec3(1), 2.0f});
lights.AddPointLight({position, color, intensity, radius});
lights.ApplyToShader(device, shader);
```

---

## 🎨 Post-Processing

**Arquivo:** `src/renderer/post_process.hpp`  
**Shader:** `src/shaders/post_process_opengl.shader`

### Efeitos Disponíveis
1. Grayscale
2. Sepia
3. Invert
4. Vignette
5. Chromatic Aberration
6. Sharpen
7. Edge Detection

### Uso
```cpp
#include "renderer/post_process.hpp"

PostProcessStack stack;
stack.AddEffect<VignetteEffect>();
stack.AddEffect<SepiaEffect>();
stack.CycleEffect(); // Alterna entre efeitos
stack.ApplyToShader(shader);
```

---

## 🔷 Primitives

**Arquivo:** `src/renderer/primitives.hpp`

### Geometrias Disponíveis
| Função | Parâmetros |
|--------|------------|
| `CreateQuad` | width, height |
| `CreatePlane` | size, subdivisions |
| `CreateCube` | size |
| `CreateSphere` | radius, sectors, stacks |
| `CreateCylinder` | radius, height, sectors |
| `CreateCone` | radius, height, sectors |
| `CreateTorus` | majorR, minorR, segments |
| `CreateCapsule` | radius, height, sectors, stacks |

### Uso
```cpp
#include "renderer/primitives.hpp"

auto sphere = Primitives::CreateSphere(device, 1.0f, 36, 18);
auto cube = Primitives::CreateCube(device, 2.0f);
auto torus = Primitives::CreateTorus(device, 1.0f, 0.3f);
```

---

## 💾 Scene Serialization

**Arquivo:** `src/scene/scene_serializer.hpp`

### Funcionalidades
- Salvar cenas em formato texto legível
- Carregar cenas de arquivo
- Preserva transforms de entidades

### Uso
```cpp
#include "scene/scene_serializer.hpp"

// Salvar
SceneSerializer::SaveToFile(scene, "scenes/level1.scene");

// Carregar
SceneSerializer::LoadFromFile(scene, "scenes/level1.scene");
```

### Formato do Arquivo
```
scene {
  entity "Player" {
    transform {
      position 0 1 0
      rotation 0 90 0
      scale 1 1 1
    }
  }
}
```

---

## 🔄 Shader Hot-Reload

**Arquivo:** `src/renderer/shader_watcher.hpp`

### Funcionalidades
- Monitora arquivos de shader automaticamente
- Detecta mudanças e recompila
- Callback opcional para notificação

### Uso
```cpp
#include "renderer/shader_watcher.hpp"

ShaderWatcher watcher(shaderManager, fileSystem, RHI::API::OpenGL);
watcher.WatchShader("pbr", "shaders/pbr_opengl.shader");

watcher.SetOnReloadCallback([](const std::string& name) {
    std::cout << "Reloaded: " << name << std::endl;
});

// No loop de update
watcher.Update(deltaTime);
```

---

## 🐞 Debug UI

**Arquivo:** `src/core/debug_ui.hpp`

### Funcionalidades
- Frame stats (FPS, timing)
- Console commands
- Debug settings (pause, wireframe, timescale)

### Teclas de Atalho
| Tecla | Ação |
|-------|------|
| F1 | Mostrar stats |
| F2 | Pause/Resume |

### Console Commands
- `help` - Lista comandos
- `stats` - Mostra estatísticas
- `wireframe` - Toggle wireframe
- `pause` - Toggle pause
- `timescale <value>` - Define time scale
- `clear` - Limpa console

### Uso
```cpp
#include "core/debug_ui.hpp"

DebugUI debug;
debug.BeginFrame();
debug.BeginUpdate(); /* update */ debug.EndUpdate();
debug.BeginRender(); /* render */ debug.EndRender();
debug.EndFrame(deltaTime);

debug.PrintStats(); // Mostra no console
std::string stats = debug.GetStatsString(); // Retorna string
```

---

## 🚀 Quick Start

```cpp
#include "core/camera.hpp"
#include "core/debug_ui.hpp"
#include "renderer/lighting.hpp"
#include "renderer/primitives.hpp"
#include "renderer/post_process.hpp"
#include "scene/scene_serializer.hpp"
#include "renderer/shader_watcher.hpp"

// Criar câmera
Camera camera(CameraMode::FPS);

// Criar geometria
auto sphere = Primitives::CreateSphere(device, 1.0f);

// Setup luzes
LightManager lights;
lights.SetSunLight(sunLight);

// Setup debug
DebugUI debug;

// Game loop
while (running) {
    debug.BeginFrame();
    
    camera.ProcessKeyboard(direction, dt);
    lights.ApplyToShader(device, shader);
    
    debug.EndFrame(dt);
}
```
