---
name: half
description: Executa uma iteração do half loop — ler CLAUDE.md + progress.txt, avançar a próxima task incompleta, compilar, atualizar progress.txt.
---

# Skill: Half Loop Iteration

Ativado quando o usuário (ou `scripts/half/h.*`) chama `/half`. Uma iteração = uma task. **Nunca** fazer mais de uma task por invocação.

## Passos por iteração

1. **Ler** `CLAUDE.md` inteiro.
2. **Ler** `progress.txt` para saber o que já foi feito.
3. **Identificar** a próxima task `- [ ] X.Y` no CLAUDE.md `# Roadmap` que ainda não está em `progress.txt` como `[DONE]` ou `[SKIPPED]`.
4. **Executar** apenas essa task. Escopo restrito — nada além do que a task pede.
5. **Compilar** com `build.bat` (Windows) ou `./build.sh` (Linux). Se falhar:
   - Tentar corrigir. Máximo 5 tentativas.
   - Se não conseguir em 5 tentativas, marcar a task como `[SKIPPED]` em `progress.txt` com motivo.
6. **Atualizar** `CLAUDE.md`: trocar `- [ ] X.Y: ...` por `- [x] X.Y: ...` na task completada.
7. **Atualizar** `progress.txt`: adicionar linha `[DONE] X.Y <descrição curta> — <observação se relevante>` ou `[SKIPPED] X.Y <motivo>`.
8. Se **todas** as tasks do Roadmap estão `[x]` ou `[SKIPPED]`, escrever na **última linha** de `progress.txt`: `ALL TASKS COMPLETE`. O loop para.

## Regras duras

- **Nunca** quebrar o build entre tasks. Se a task atual depende de algo ainda não feito, pular (marcar SKIPPED com motivo) em vez de mocar.
- **Nunca** refatorar código de tasks anteriores sem necessidade técnica direta (se fizer, registrar motivo em progress.txt).
- **Nunca** adicionar libs novas além das já usadas (GLEW, GLFW, Vulkan, GLM, Assimp, ImGui, stb_image) sem justificativa em progress.txt.
- **Nunca** editar CLAUDE.md fora da seção `# Roadmap` e do checkbox da task corrente.
- Seguir estrutura existente: RHI em `src/rhi/`, renderer em `src/renderer/`, editor em `src/editor/`, scene em `src/scene/`, shaders `.shader` em `src/shaders/`.
- Após terminar, **parar**. Não começar próxima task — o wrapper (`h.sh`/`h.bat`) reinvoca.

## Formato de entrada típico

Quando chamado por `h.sh`/`h.bat`, o prompt inclui CLAUDE.md + progress.txt via concatenação. Skill apenas executa passos acima sem perguntar nada ao usuário.
