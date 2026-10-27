---
name: prd
description: Gera PRD estruturado (features + tasks numeradas) no CLAUDE.md + progress.txt vazio, pronto para consumir via half loop.
---

# Skill: Gerar PRD do projeto

Use quando o usuário pedir para planejar features do projeto OpenGL-Render em formato consumível pelo half loop (`scripts/half/h.sh` ou `h.bat`).

## O que fazer

1. Conversar com o usuário para entender o objetivo do PRD: que features adicionar ao renderer atual (model viewer 3D com RHI OpenGL+Vulkan, ImGui editor, Assimp).
2. Escrever as features na seção **# Roadmap** do `CLAUDE.md` (criar a seção se não existir; manter o resto do arquivo intacto — stack, build, estrutura).
3. Cada feature tem:
   - Título curto
   - **Contexto** — 1–3 linhas justificando
   - **Tasks** numeradas `- [ ] X.Y: descrição curta acionável`
4. Criar/reescrever `progress.txt` com uma única linha: `No tasks completed yet. Start from Task 1.1.`

## Regras das tasks

- Cada task deve ser compilável isoladamente — não quebrar o build entre tasks.
- Tasks pequenas: 1 arquivo novo OU 1 classe OU 1 integração. Não empacotar feature inteira em 1 task.
- Numeração: `feature.task` (ex.: `1.1`, `1.2`, `2.1`).
- Seguir stack e estrutura existentes: RHI em `src/rhi/`, renderer em `src/renderer/`, editor em `src/editor/`, scene em `src/scene/`, shaders em `src/shaders/` no formato `.shader` unificado.
- Evitar introduzir libs novas sem justificativa — stb_image já baixado pelo CMake, resto via system/vcpkg.

## Output

- `CLAUDE.md` com `# Roadmap` preenchido (seções existentes preservadas).
- `progress.txt` reescrito com o marcador inicial.
- Mostrar ao usuário o comando do half loop: `./scripts/half/h.sh` (Linux/git bash) ou `scripts\half\h.bat` (cmd).
