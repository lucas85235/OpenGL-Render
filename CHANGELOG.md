# Changelog

Mudanças não commitadas desde o último commit `deac30c "Windows build"`.

## Infra de build Windows

- **[build.bat](build.bat)** — novo. Equivalente do `build.sh` pra Windows. Auto-detecta Visual Studio 2019/2022 via `vswhere.exe` (filtra range `[16.0, 18.0)` pra pular VS 18 BuildTools quebrado), roda `vcvars64.bat`, aplica toolchain vcpkg via `VCPKG_ROOT`, preserva VCPKG_ROOT do user (vcvars64 sobrescreve), usa `pushd` + `.\vswhere.exe` por causa de política de CWD do cmd moderno.
- **[BUILD.md](BUILD.md)** — novo. Requisitos Linux (apt packages) + Windows (VS 2022, Vulkan SDK, vcpkg com `glew glfw3 glm assimp` no triplet `x64-windows`). Troubleshooting: polyclipping SSL, M_PI MSVC, VS múltiplos, VCPKG_ROOT override, DLLs runtime.
- **[CMakeLists.txt](CMakeLists.txt)** — modificado:
  - `find_package(PkgConfig QUIET)` (era REQUIRED; PkgConfig é Linux-only)
  - Bloco MSVC adiciona `_USE_MATH_DEFINES NOMINMAX _CRT_SECURE_NO_WARNINGS` (M_PI não existe em `<cmath>` no MSVC)
  - Target `assimp::assimp` (não `assimp` plano; vcpkg MSVC usa nome versionado `assimp-vc145-mt.lib`)

## Infra de agente autônomo

- **[CLAUDE.md](CLAUDE.md)** — novo. Documentação do stack real (GLEW+Vulkan+ImGui+Assimp), comandos de build Linux/Windows, estrutura do projeto, seção `# Roadmap` com 7 features × 31 tasks, PRD 2D antigo preservado como histórico.
- **[progress.txt](progress.txt)** — novo. Log do half loop com uma linha por task concluída.
- **[.claude/skills/prd.md](.claude/skills/prd.md)** — skill que gera Roadmap estruturado (features + tasks numeradas) no CLAUDE.md.
- **[.claude/skills/half.md](.claude/skills/half.md)** — skill que executa **uma** task incompleta por invocação, compila, atualiza CLAUDE.md checkbox + progress.txt.
- **[scripts/half/h.sh](scripts/half/h.sh)** — runner bash do half loop (git bash / Linux / macOS). `--tool`, `--max`, MAX_ITERS via env.
- **[scripts/half/h.bat](scripts/half/h.bat)** — runner cmd do half loop (Windows).

## Feature 1 — Viewport Camera Controls

- **[src/core/camera.hpp](src/core/camera.hpp)** — `SetOrbit(target, radius, yaw, pitch)` explícito, `CameraMode::FLY` + `SetFly(pos, yaw, pitch)`, `ProcessScroll` ganhou branch FLY (zoom por `movementSpeed *= pow(1.1, yOffset)`, clamp [0.1, 200]).
- **[src/editor/viewport_panel.hpp](src/editor/viewport_panel.hpp)** — ganhou `Camera*` + `Scene*` + `HierarchyPanel*` setters; WASDQE via `ImGui::IsKeyDown`, RMB drag via `io.MouseDelta` quando focused, scroll wheel forwarded pra camera quando hovered, RMB click auto-foca, tecla F chama `FocusOnSelected()` (computa AABB mundo dos vértices do Mesh/Model do selected, centraliza orbit em `center + radius*2.5`).
- **[src/core/application.hpp](src/core/application.hpp)** — removido WASDQE de `ProcessInput`, removido `camera.ProcessMouse` do window mouse callback, removido `window->SetScrollCallback` (evita double-fire); wire-up de panels→camera/scene/hierarchy pós-Init.

## Feature 2 — Grid + Axis Gizmo

- **[src/shaders/grid_opengl.shader](src/shaders/grid_opengl.shader)** — novo. Vertex emite fullscreen triangle via `gl_VertexID`, unprojeta NDC com `inverse(proj*view)`. Fragment raycasta plano y=0, grid 1u fino + 10u grosso via `fwidth`, axis tint X=vermelho/Z=azul, distance fade `smoothstep(20,120)`, `gl_FragDepth` correto.
- **[src/renderer/grid_pass.hpp](src/renderer/grid_pass.hpp)** — novo. Dummy VBO (RHI exige buffer válido), pipeline depth LEQUAL + write ON (por causa do `gl_FragDepth`), cull None, alpha blend. Early-out se API != OpenGL.
- **[src/renderer/render_graph/grid_pass_node.hpp](src/renderer/render_graph/grid_pass_node.hpp)** — novo. RenderPass node com `bool enabled` + `GetEnabledPtr()` pra toggle externo.
- **[src/editor/editor_ui.hpp](src/editor/editor_ui.hpp)** — `AddViewToggle(name, bool*)` genérico com `vector<pair<string, bool*>>` renderizado no menu View.
- **[src/editor/viewport_panel.hpp](src/editor/viewport_panel.hpp)** — `DrawAxisGizmo()` após `ImGui::Image`, ancorado no canto inferior esquerdo (margin 12, radius 36). `mat3(view)` transforma eixos mundo pra view-space, screen = `(v.x, -v.y) * radius`, sort por z pra depth ordering, eixos positivos com linha+círculo+label, negativos como círculo vazado alpha 0x80.

## Feature 3 — Entity Picking + Outline

- **[src/shaders/picking_opengl.shader](src/shaders/picking_opengl.shader)** — novo. Vertex padrão, fragment emite `u_entity_color` (entity id encodado via r/g/b/a).
- **[src/renderer/picking_pass.hpp](src/renderer/picking_pass.hpp)** — novo. FBO RGBA8+depth próprio, itera `<Transform, MeshRenderer>` + `<Transform, SimpleMeshRenderer>`, encoda `entt::to_integral(entity)` em vec4, bind VAO direto (bypass material). `ReadPixelId(x,y)` bind no FBO nativo + `glReadPixels` 1x1, decoda id.
- **[src/renderer/render_graph/picking_pass_node.hpp](src/renderer/render_graph/picking_pass_node.hpp)** — novo. Wrapper, `enabled=false` default (Execute é no-op no scene pass; 3.2 flip pra true só no frame do click).
- **[src/rhi/rhi_device.h](src/rhi/rhi_device.h)** + **[src/rhi/opengl/*](src/rhi/opengl)** / **[src/rhi/vulkan/*](src/rhi/vulkan)** — `GetNativeFramebufferID(FramebufferHandle)` (GL retorna `fbo.fbo`, Vulkan stub=0).
- **[src/shaders/outline_opengl.shader](src/shaders/outline_opengl.shader)** — novo. Vertex infla posição via `aNormal * u_outline_width`, fragment emite `u_outline_color`.
- **[src/renderer/outline_pass.hpp](src/renderer/outline_pass.hpp)** — novo. Pipeline cull=Front (só back faces do shell inflado), depth test LessOrEqual + write OFF (gera anel ao redor da silhueta), alpha blend.
- **[src/renderer/render_graph/outline_pass_node.hpp](src/renderer/render_graph/outline_pass_node.hpp)** — novo. `SetHierarchyPanel(*)` — lê entity selecionada via `GetSelectedEntity()`.
- **[src/editor/hierarchy_panel.hpp](src/editor/hierarchy_panel.hpp)** — `SetSelectedEntity(entt::entity)` público (inverso do getter).
- **[src/core/application.hpp](src/core/application.hpp)** — `ViewportPanel::HasPendingPick()` consultado no Tick, `pickingPass.enabled=true` antes do RenderScene, `ResolvePick` depois (converte y pra GL bottom-left, loga id+Tag no ConsolePanel, chama `hierarchy->SetSelectedEntity`, disable+Clear).

## Feature 4 — Transform Hierarchy

- **[src/scene/components.hpp](src/scene/components.hpp)** — `TransformComponent` ganhou `entt::entity Parent = entt::null` + `std::vector<entt::entity> Children`. `GetWorldMatrix(const entt::registry&)` recursivo (para se Parent=null/inválido, senão `parent.GetWorldMatrix(reg) * local`).
- **[src/editor/hierarchy_panel.hpp](src/editor/hierarchy_panel.hpp)** — árvore iterando roots, `DrawEntityNode` recursivo, drag source + drop target via `HIERARCHY_ENTITY` payload, `Reparent(child, newParent)` com cycle guard (walk parent chain, reject se encontrar child), `DetachFromParent` remove do Children do old parent. Filter propaga via `HasMatchingDescendant`. Delete simplificado pós-4.4 (chama `Scene::DestroyEntity`).
- **[src/scene/scene.hpp](src/scene/scene.hpp)** — `DestroyEntity` recursivo: snapshot Children, destroy cada um, detach do parent, registry.destroy. `#include <algorithm>`.
- **[src/scene/scene_serializer.hpp](src/scene/scene_serializer.hpp)** — reescrito. Save 2-pass (pass 1 mapeia handle→stableId, pass 2 escreve `id N` + `parent M` opcional). Load parse pra `vector<PendingEntity>`, pass 1 cria entidades, pass 2 resolve parents via idMap. Trocado `Transform` (inexistente) por `TransformComponent`. Migrado `reg.each(...)` (removido em entt novo) pra `reg.storage<entt::entity>()`.
- **[src/scene/scene.hpp](src/scene/scene.hpp)** — inclui `scene_serializer.hpp` no final (era header órfão, bugs compile-time passavam).

## Feature 5 — Shadow Mapping

- **[src/renderer/shadow_pass.hpp](src/renderer/shadow_pass.hpp)** — novo. FBO Depth32F 2048x2048 sem color attachment. `UpdateLightSpaceMatrix(dir)` = `ortho(-20..20, 0.1, 80) * lookAt(-dir*30, origin, up)` com up fallback se dir vertical. Execute bind shadow FBO, itera Transform+Mesh renderers, bind VAO direto + SetMat4 u_model + DrawIndexed.
- **[src/renderer/render_graph/shadow_pass_node.hpp](src/renderer/render_graph/shadow_pass_node.hpp)** — novo. Lê primeira DirectionalLightComponent (fallback `(-0.2,-1,-0.3)`), chama UpdateLightSpaceMatrix + Execute. View toggle "Shadows".
- **[src/rhi/opengl/opengl_device.cpp](src/rhi/opengl/opengl_device.cpp)** — `CreateFramebuffer` emite `glDrawBuffer/glReadBuffer(GL_NONE)` quando `colorAttachments` vazio (FBO depth-only ficaria incomplete).
- **[src/shaders/shadow_depth_opengl.shader](src/shaders/shadow_depth_opengl.shader)** — novo. Vertex `gl_Position = u_light_space * u_model * vec4(aPos,1)`, fragment vazio.
- **[src/renderer/renderer.hpp](src/renderer/renderer.hpp)** — `SetShadow(TextureHandle, mat4, bool, biasConstant, biasSlope)`. EndScene push uniforms slot 13, `u_lightSpace`, `u_shadowEnabled`, `u_shadowBiasConstant`, `u_shadowBiasSlope`.
- **[src/renderer/render_graph/pbr_pass_node.hpp](src/renderer/render_graph/pbr_pass_node.hpp)** — `SetShadowPass(*)` + `SetShadowEnabledPtr(*)`, propaga pro renderer antes de BeginScene.
- **[src/shaders/pbr_opengl.shader](src/shaders/pbr_opengl.shader)** — `u_lightSpace`, `shadowMap`, `u_shadowEnabled`, `u_shadowBiasConstant`, `u_shadowBiasSlope`. `SampleShadowPCF(N, L)` com perspective divide, bounds check, slope-scaled bias, loop 3x3 PCF. Directional light multiplica `CalcPBRLight` por shadow. **Removido** Reinhard tonemap + gamma inline (migrou pro TonemapPass em 6.2 pra evitar double-gamma).
- **[src/scene/components.hpp](src/scene/components.hpp)** — `DirectionalLightComponent` ganhou `castShadow=true`, `shadowBiasConstant=0.0005`, `shadowBiasSlope=0.005`.
- **[src/editor/inspector_panel.hpp](src/editor/inspector_panel.hpp)** — checkbox "Cast Shadow" + DragFloat Bias Constant/Slope (wrapped em BeginDisabled quando `!castShadow`).

## Feature 6 — Bloom + Tonemap

- **[src/shaders/bloom_opengl.shader](src/shaders/bloom_opengl.shader)** — novo. Fragment multiplexa 3 modos via `u_mode`: 0=prefilter soft-knee (Jimenez SIGGRAPH 2014), 1=downsample 13-tap, 2=upsample 3x3 tent.
- **[src/renderer/bloom_pass.hpp](src/renderer/bloom_pass.hpp)** — novo. Cadeia de 5 MipTargets RGBA16F (half do anterior), 2 pipelines (write sem blend pra prefilter/downsample; add com One/One pra upsample/composite). Fluxo: prefilter → loop down 1..4 → loop up 3..0 (clearColorBuffer=false) → composite mip[0] → sceneFB additive. Threshold=1.0, knee=0.5, intensity=0.5, filterRadius=1.0 defaults. `EnsureChain` recria on resize.
- **[src/renderer/render_graph/bloom_pass_node.hpp](src/renderer/render_graph/bloom_pass_node.hpp)** — novo. Execute do node é no-op; Application chama `GetPass()->Execute` direto após sceneFB EndRenderPass e antes de `cmdList->End()`.
- **[src/shaders/tonemap_opengl.shader](src/shaders/tonemap_opengl.shader)** — novo. ACES Narkowicz 2015 fit `(x*(ax+b)) / (x*(cx+d)+e)` clamp [0,1], depois `pow(1/u_gamma)`, default gamma 2.2 / exposure 1.0.
- **[src/renderer/tonemap_pass.hpp](src/renderer/tonemap_pass.hpp)** — novo. Pipeline depth-off + blend-off, abre pass próprio no target FBO, bind source + uniforms + draw 3 verts.
- **[src/renderer/render_graph/tonemap_pass_node.hpp](src/renderer/render_graph/tonemap_pass_node.hpp)** — novo. Wrapper no-op no graph.
- **[src/core/application.hpp](src/core/application.hpp)** — `displayFB` novo (2º RGBA16F, resize paralelo ao sceneFB) pra receber output LDR. ViewportPanel lê displayFB quando tonemap enabled, senão cai no sceneFB raw (debug/washed-out). Ordem final: scene pass → bloom → tonemap → Viewport mostra displayFB.
- **[src/editor/post_process_panel.hpp](src/editor/post_process_panel.hpp)** — novo. 2 CollapsingHeaders (Bloom/Tonemap). Bloom: Enabled + Threshold/Knee/Intensity/Filter Radius. Tonemap: Enabled + Exposure/Gamma. Checkboxes compartilham `bool*` com os MenuItem do View menu (sync automático).
- **[src/shaders/skybox.shader](src/shaders/skybox.shader)** — **removido** tonemap + gamma inline (mesmo motivo que pbr_opengl — double-gamma se mantido).
- **[src/shaders/post_process_vulkan.shader](src/shaders/post_process_vulkan.shader)** — novo. Espelho de `post_process_opengl` adaptado pra Vulkan: `@push_constant` em vez de uniforms, `@binding(0)` no sampler, `sample`→`s` (keyword reservada). Gera SPIRV ok; backend Vulkan ainda não consome (pipeline wiring fora do escopo).

## Feature 7 — Stats Panel (em andamento, 1/4 done)

- **[src/renderer/render_context.hpp](src/renderer/render_context.hpp)** — `struct FrameStats { uint64_t frameCount; uint32_t drawCalls, triangles, vertices; }`, membro privado, `GetFrameStats()`, `GetFrameStatsMutable()`, `BeginFrameStats()` zera draw/tri/vert + incrementa frameCount. `<cstdint>` incluído.
- Tasks 7.2 (backend increments), 7.3 (StatsPanel UI), 7.4 (frame time histogram) **pendentes**.

## Resumo

- **Arquivos novos:** 22 (build infra 2, agent infra 6, shaders 7, renderer passes 7, editor panel 1, doc 1)
- **Arquivos modificados:** 19 (CMakeLists + RHI device + camera + editor panels + shaders + scene + renderer + components + application)
- **Features completas:** 1, 2, 3, 4, 5, 6 (26 tasks)
- **Feature parcial:** 7 (1/4)
- **Total tasks done:** 27/31
