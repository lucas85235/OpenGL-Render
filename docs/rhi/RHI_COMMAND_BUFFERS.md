# RHI: Command Buffers e Render Passes

Esta documentação detalha a arquitetura de **Command Buffers** (Listas de Comandos) e **Render Passes** adicionada ao projeto para substituir o antigo modelo de renderização "Immediate Mode" (Modo Imediato) legado.

## Motivação
Historicamente, o RHI (`Render Hardware Interface`) expunha métodos diretos na classe `IDevice` como `SetViewport`, `Clear`, `BindPipeline`, e `Draw`. Isso forçava a CPU a executar todos os comandos gráficos de forma síncrona, estrita e direta. Consequências dessa abordagem:
1. Impedia a geração de comandos em múltiplas threads, pois o estado era global ao `IDevice`.
2. Não refletia a natureza moderna de APIs como Vulkan, DirectX 12 e Metal (que se baseiam em Recording & Queue Submission).
3. Dificultava a otimização de trocas de estado (State Tracking).

## Arquitetura Atual

A renderização foi refatorada em duas novas primitivas principais: **ICommandList** e **RenderPassDescriptor**.

### ICommandList
A interface `ICommandList` representa um buffer de comandos gravado pela CPU que só surtirá efeito na GPU posteriomente.

- **Gravação Diferida**: Em vez de invocar chamadas diretas da API gráfica (OpenGL ou Vulkan) no momento do fluxo do código, o `ICommandList` armazena as requisições. No backend do OpenGL (`OpenGLCommandList`) e Vulkan (`VulkanCommandList`), essas requisições são postas numa fila de callbacks e avaliadas apenas em lote quando `Execute()` ou `SubmitCommandList` são acionados.
- **Isolamento de Estado**: Visto que falhas e redundâncias acontecem quando o estado gráfico é adulterado invisivelmente, todo controle (bindings de Shaders, Texturas, Viewport, etc) opera estritamente através do ponteiro referenciado por esse objeto.

#### Exemplo prático
```cpp
RHI::CommandListHandle cmdHandle = device->CreateCommandList();
RHI::ICommandList *cmdList = device->GetCommandList(cmdHandle);

cmdList->Begin(); // Abre a gravação

// Inicia um Render Pass estabelecendo em quais Framebuffers será desenhado.
RHI::RenderPassDescriptor passDesc;
passDesc.framebuffer = sceneFB->GetHandle();
passDesc.clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
passDesc.clearColorBuffer = true;
passDesc.clearDepthBuffer = true;
cmdList->BeginRenderPass(passDesc);

cmdList->SetViewport({0, 0, width, height, 0.0f, 1.0f});
cmdList->SetScissor({0, 0, width, height});

// Aplica configuração de shader, pipeline e desenha.
cmdList->BindPipeline(pipeline);
cmdList->BindVertexArray(vao);
cmdList->DrawIndexed(drawCmd);

cmdList->EndRenderPass(); // Encerra manipulação de imagem.
cmdList->End(); // Encerra gravação.

// Entrega a lista inteira à GPU de uma só vez.
device->SubmitCommandList(cmdHandle);
device->DestroyCommandList(cmdHandle);
```

### RenderPassDescriptor
O `RenderPassDescriptor` descreve como os Framebuffers são carregados antes do início do desenho e salvos (store) após o término. Substituto para as chamadas explícitas e globais de `BindFramebuffer` atrelado com limpezas manuais como `Clear`.

- **Operação Limpeza (Load / Clear)**: O framework utiliza as flags (`clearColorBuffer`, `clearDepth`, `clearColor`) para instruir à GPU caso ela deva descartar a informação antiga do pixel (comum em color) ou reaproveitar da renderização anterior. Isso se alinha assintoticamente com as diretrizes do `VkRenderPass` `VK_ATTACHMENT_LOAD_OP_CLEAR` em sistemas Vulkan integrados e reduz transferências lentas de memória.

## Integração com a abstração (Engine)
A engine principal concentra e submete suas gravações de loop em `Application::RenderScene`. Durante essa cena, um ciclo solitário da `ICommandList` principal é gerado e encadeado ao struct `RenderPassData`, que propaga este Command List entre diversos render nodes (`PBRPassNode`, `SkyboxPassNode`, e passagens de pós-processamento no `RenderGraph`). Estruturas mais primitivas do ECS (Model, Material, Texture, Camera, Lights) não acessam mais o `IDevice`, apenas dependendo do parâmetro `ICommandList*` para expressar as transferências e draw calls gráficas ativas.
