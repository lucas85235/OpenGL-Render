# OpenGL-Render

Model viewer / rendering engine. RHI abstraction over OpenGL + Vulkan. ImGui editor (inspector, scene hierarchy). Assimp model loading. Unified `.shader` format compiled to SPIR-V via Python preprocessor.

## Stack atual
- C++17, CMake + Ninja
- Windowing: GLFW 3.3+
- GL loader: GLEW
- Vulkan (RHI paralelo ao OpenGL)
- GLM (math)
- Assimp (model loading)
- Dear ImGui (docking branch, vendored em `src/vendor/imgui`)
- stb_image (baixado automaticamente pelo CMake)
- SPIRV-Cross (opcional, `-DUSE_SPIRV_CROSS=ON`)

## Build

Detalhes completos + troubleshooting em [BUILD.md](BUILD.md).

### Linux
```bash
./build.sh
```
Requer: `build-essential cmake ninja-build python3 libgl1-mesa-dev libglew-dev libglfw3-dev libglm-dev libassimp-dev glslang-tools libvulkan-dev`.

### Windows
```cmd
setx VCPKG_ROOT "C:\Users\lucas\vcpkg"
:: novo terminal
build.bat
```

Requer:
- Visual Studio 2019/2022 com workload "Desktop development with C++" (traz MSVC + Ninja)
- Vulkan SDK (`%VULKAN_SDK%` setado pelo installer)
- Python 3
- vcpkg clonado em `C:\Users\lucas\vcpkg` com `glew glfw3 glm assimp` instalados no triplet `x64-windows`

`build.bat` auto-detecta VS via `vswhere` (filtra range `[16.0, 18.0)` pra pular previews quebrados), chama `vcvars64.bat`, aplica toolchain do vcpkg, preserva `VCPKG_ROOT` do usuário (vcvars64 sobrescreve).

## Estrutura
- `main.cpp` — entry point
- `src/rhi/` — abstração de Render Hardware Interface (opengl/, vulkan/)
- `src/renderer/` — mesh, texture, model, model_factory, pbr_utils, render_context
- `src/scene/` — cena + hierarquia
- `src/editor/` — inspector_panel, hierarchy panel
- `src/core/` — VFS, componentes
- `src/shaders/` — `.shader` unificado (vertex + fragment em um arquivo)
- `src/vendor/` — imgui, stb_image
- `tools/compile_shaders.py` — preprocessa `.shader` → SPIR-V
- `models/` — assets (.obj, .gltf, texturas)

## Ajustes cross-platform no CMakeLists.txt
- `find_package(PkgConfig QUIET)` — PkgConfig era REQUIRED, não existe no Windows por padrão
- `add_compile_definitions(_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS)` em MSVC — MSVC não define `M_PI` em `<cmath>`
- Target moderno `assimp::assimp` (não `assimp`) — vcpkg MSVC usa nome versionado `assimp-vc145-mt.lib`

---

# Roadmap

Features incrementais em cima do renderer atual. Consumido por `/half` (skill em `.claude/skills/half.md`) via `scripts/half/h.sh` ou `scripts/half/h.bat`. Cada task compila isolada. Marcar `- [x]` ao completar + registrar em `progress.txt`.

## Feature 1 — Viewport Camera Controls

### Contexto
Câmera orbital/flythrough pro viewport. `src/core/camera.hpp` já existe — expandir.

### Tasks
- [x] 1.1: Adicionar modo orbit em `Camera` (alvo + raio + yaw + pitch), método `setOrbit(target, radius, yaw, pitch)`
- [x] 1.2: Adicionar modo fly (WASD + QE up/down + mouse look com RMB)
- [x] 1.3: ViewportPanel captura input do mouse/teclado só quando viewport tem foco, encaminha pra `Camera`
- [x] 1.4: Scroll wheel zoom (orbit: ajusta raio; fly: ajusta velocidade)
- [x] 1.5: Tecla F faz `focusOnSelected()` — centraliza orbit no bbox da entidade selecionada

## Feature 2 — Grid + Axis Gizmo no Viewport

### Contexto
Grid infinito no plano XZ + gizmo XYZ no canto inferior esquerdo, estilo Blender.

### Tasks
- [x] 2.1: Criar shader `grid.shader` (vertex fullscreen quad, fragment calcula linhas com fade por distância)
- [x] 2.2: `RenderContext` desenha grid após skybox e antes de objetos opacos (ou transparente depois de tudo, avaliar)
- [x] 2.3: Toggle visibilidade do grid via menu View na editor_ui
- [x] 2.4: Axis gizmo (3 linhas XYZ coloridas) renderizado em overlay no canto do viewport, mesma orientação da câmera

## Feature 3 — Entity Picking

### Contexto
Click na viewport seleciona entidade. Usar color-picking (render id por entidade em framebuffer offscreen).

### Tasks
- [x] 3.1: Adicionar pass de picking em `render_graph/` — render cada entidade com cor `uint32 → vec4` da entity id
- [x] 3.2: Capturar click no `ViewportPanel`, ler pixel do framebuffer de picking, resolver entity id
- [x] 3.3: Setar entidade selecionada no `HierarchyPanel`/`InspectorPanel` via evento/shared state
- [x] 3.4: Outline visual na entidade selecionada (stencil buffer ou pass extra com vertex displacement)

## Feature 4 — Transform Hierarchy (Parent/Child)

### Contexto
`Transform` atualmente é local. Adicionar suporte a parent → world matrix propagada.

### Tasks
- [x] 4.1: Adicionar campo `Entity parent` + `std::vector<Entity> children` no componente `Transform`
- [x] 4.2: Método `Transform::getWorldMatrix()` multiplica cadeia de parents
- [x] 4.3: `HierarchyPanel` renderiza árvore (drag-drop para reparenting)
- [x] 4.4: `Scene::destroyEntity` destrói filhos recursivamente
- [x] 4.5: Atualizar `SceneSerializer` para salvar/carregar relação parent/children

## Feature 5 — Shadow Mapping (Directional Light)

### Contexto
`lighting.hpp` já tem luzes. Adicionar shadow map pra luz direcional principal.

### Tasks
- [x] 5.1: Criar `ShadowPass` em `render_graph/` — renderiza cena de POV da luz direcional em depth texture 2048x2048
- [x] 5.2: Shader `shadow_depth.shader` (apenas vertex + empty fragment)
- [x] 5.3: Passar shadow map + matriz light-space pro `pbr_opengl`/`pbr_vulkan` via UBO
- [x] 5.4: Sample shadow map nos fragment shaders com PCF 3x3
- [x] 5.5: Toggle shadows e bias controls no InspectorPanel da luz direcional

## Feature 6 — Post-Process Pipeline Extensão

### Contexto
`post_process.hpp` + `post_process_opengl.shader` já existem. Adicionar bloom + tonemap configuráveis.

### Tasks
- [x] 6.1: Bloom pass — downsample em 5 níveis + upsample com filtro tent + blend
- [x] 6.2: Tonemap (ACES) + gamma correction no pass final
- [x] 6.3: Expor threshold/intensity/gamma via ImGui panel (criar `PostProcessPanel` em `src/editor/`)
- [x] 6.4: Versão Vulkan do pipeline (espelhar opengl em `post_process_vulkan.shader` se já não existir)

## Feature 7 — Stats Panel (FPS + Draw Calls)

### Contexto
Validar performance. Renderer precisa expor contadores.

### Tasks
- [x] 7.1: Adicionar contadores `frameCount`, `drawCalls`, `triangles`, `vertices` em `RenderContext` (resetados por frame)
- [ ] 7.2: Cada backend (opengl_device, vulkan_device) incrementa contadores em `draw()` / `drawIndexed()`
- [ ] 7.3: Criar `StatsPanel` em `src/editor/` — FPS médio (janela deslizante 60 frames), draw calls, triangles
- [ ] 7.4: Histograma do frame time com ImGui plot (últimos 120 frames)

---

# PRD Original (histórico)

Plano original era shoot'em up 2D com ECS. Projeto evoluiu para model viewer/renderer 3D. Seções abaixo mantidas como referência — **não refletem estado atual**.

## Visão Geral
Evolução de uma engine OpenGL existente (C++, GLFW + GLAD, sistema de partículas básico)
para uma engine 2D completa com ECS (entt), compute shaders para partículas e features
suficientes para rodar um jogo simples. O objetivo final é um shoot'em up 2D top-down
minimalista como jogo de demonstração da engine.

## Stack Técnica
- Linguagem: C++17 ou superior
- Windowing: GLFW + GLAD
- ECS: entt
- Compute Shaders: OpenGL 4.3+
- Build: CMake
- Assets: texturas PNG via stb_image

---

## Feature 1 — Compute Shader Particle System

### Contexto
O repositório já possui um sistema de partículas básico na CPU. Esta feature migra
a simulação para a GPU via compute shaders.

### Tasks
- 1.1: Criar Shader Storage Buffer Objects (SSBO) para posição, velocidade, cor e lifetime
       de cada partícula
- 1.2: Escrever compute shader `particles.comp` que atualiza posição, aplica gravidade
       configurável e decrementa lifetime por delta time
- 1.3: Escrever vertex + fragment shader para renderizar partículas como billboards ou
       point sprites
- 1.4: Criar classe `ParticleSystem` com métodos: `emit(count, config)`, `update(dt)`,
       `render()`; config expõe: posição inicial, spread, velocidade, cor inicial/final,
       lifetime, gravidade
- 1.5: Integrar `ParticleSystem` no game loop existente e validar que 100k partículas
       rodam a 60fps em hardware médio

---

## Feature 2 — Arquitetura ECS com entt

### Contexto
Refatorar a engine para usar entt como sistema de entidades e componentes.

### Tasks
- 2.1: Adicionar entt via CMake FetchContent
- 2.2: Definir componentes base:
       `Transform` (pos vec2, rotation float, scale vec2),
       `Sprite` (textureID, uvRect),
       `Velocity` (vec2),
       `Collider` (AABB),
       `ParticleEmitter` (referência ao ParticleSystem + config),
       `Health` (hp, maxHp),
       `Tag` (string ou enum: Player, Enemy, Bullet, Pickup)
- 2.3: Criar sistemas: `MovementSystem`, `RenderSystem`, `CollisionSystem`,
       `ParticleEmitterSystem`
- 2.4: Criar `World` wrapper sobre `entt::registry` com métodos `createEntity`,
       `destroyEntity`, `update(dt)`, `render()`
- 2.5: Migrar o código existente de partículas para usar ECS

---

## Feature 3 — Renderer 2D

### Tasks
- 3.1: Implementar sprite batching: agrupar draw calls por textura usando um único VAO
       com dados instanciados (model matrix, uvRect por instância)
- 3.2: Carregar texturas PNG com stb_image; criar `TextureCache` com lookup por path
- 3.3: Implementar câmera ortográfica 2D com posição, zoom e matriz de projeção
       atualizada por frame
- 3.4: Implementar z-ordering por layer (int) nos sprites para controle de profundidade
- 3.5: Adicionar suporte a spritesheet/atlas com uvRect configurável por sprite

---

## Feature 4 — Input System

### Tasks
- 4.1: Criar `InputManager` singleton com callbacks de GLFW registrados no init
- 4.2: Expor métodos: `isKeyDown(key)`, `isKeyJustPressed(key)`, `isKeyJustReleased(key)`,
       `getMousePos()`, `isMouseButtonDown(button)`
- 4.3: Limpar estado de "just pressed/released" ao fim de cada frame

---

## Feature 5 — Collision Detection (AABB)

### Tasks
- 5.1: `CollisionSystem` itera pares de entidades com `Collider` + `Tag` e detecta
       sobreposição AABB
- 5.2: Gerar eventos de colisão via entt dispatcher: `CollisionEvent{entityA, entityB}`
- 5.3: Filtrar colisões por layer mask (ex: bullets não colidem com bullets)

---

## Feature 6 — Scene Management

### Tasks
- 6.1: Criar interface `IScene` com métodos `onEnter()`, `onExit()`, `update(dt)`,
       `render()`
- 6.2: Criar `SceneManager` com `pushScene`, `popScene`, `replaceScene`
- 6.3: Implementar cenas: `MenuScene` (tela preta com texto "Press Enter"),
       `GameScene` (jogo em si), `GameOverScene`

---

## Feature 7 — Jogo de Demonstração: Mini Shoot'em Up Top-Down

### Contexto
Jogo minimalista para validar a engine. Naves geométricas (sem assets externos
obrigatórios — pode usar formas coloridas). O objetivo é mostrar que a engine
funciona de ponta a ponta.

### Tasks
- 7.1: `GameScene`: spawnar entidade Player com `Transform`, `Sprite`, `Velocity`,
       `Collider`, `Health(3)`, `Tag::Player`
- 7.2: Movimento do player via WASD/setas, limitar aos bounds da tela
- 7.3: Player atira bullets com barra de espaço (cooldown 0.2s); bullet tem
       `Velocity`, `Collider`, `Tag::Bullet`; destruir ao sair da tela
- 7.4: Spawnar enemies com padrão simples (descem da parte de cima da tela em
       velocidade constante); ao ser atingido por bullet, destruir enemy e spawnar
       partículas de explosão via `ParticleEmitter`
- 7.5: Colisão enemy→player reduz Health; ao chegar em 0, ir para `GameOverScene`
- 7.6: Score counter (int) incrementado por enemy destruído; exibir via texto
       simples (pode ser título da janela GLFW ou render de dígitos via sprites)
- 7.7: Aumentar dificuldade progressivamente: a cada 10 segundos, incrementar
       spawn rate e velocidade dos enemies

---

## Restrições e Regras para o Agente

- NÃO quebrar o build em nenhum momento entre tasks; cada task deve deixar o projeto
  compilável
- Usar CMake como build system; não adicionar dependências além de entt e stb_image
  sem justificativa explícita no progress.txt
- Manter um único arquivo `progress.txt` na raiz atualizado ao fim de cada task com:
  status (done/failed/skipped), observações e próxima task
- Se uma task falhar após 5 tentativas, marcar como `skipped` no progress.txt,
  registrar o motivo e avançar para a próxima
- Não refatorar código já validado de tasks anteriores salvo necessidade técnica
  direta; registrar no progress.txt se fizer isso e o motivo
- Testar compilação com `cmake --build build` após cada task