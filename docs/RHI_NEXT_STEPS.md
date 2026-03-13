# RHI: Próximos Passos de Melhoria

Com a fase principal de Refatoração de Command Buffers e eliminação do "Immediate Mode" finalizada, o RHI está preparado para evolução, multithreading avançado e otimizações pesadas de CPU.

Abaixo, detalhamos os próximos grandes passos de melhoria arquitetural mapeados para a engine gráfica.

---

## Fase 2: Multithreading Verdadeiro e Command Pools

### 1. Gravação Multithread (Parallel Command Encoding)
Agora que não estamos mais atrelados ao estado global instável do `IDevice`, podemos instanciar múltiplas conexões de submissão do frame em CPU.
- **Implementação**: Dividir as ramificações pesadas do grafo de renderização (culling espaciais, draws de geometria vasta ou múltiplas sub-renderizações de probes/cube maps) de forma que threads secundárias reservem e preencham preencham listas lógicas (`ICommandList`) separadamente, de modo assíncrono.
- **Sincronização**: Alimentar uma via única controlada atrelada ao Thread Principal da fila de apresentação (Queue). O `IDevice::SubmitCommandLists({cmd1, cmd2, cmd3})` enfileiraria o resultado agregado em um lote.

### 2. Command Allocators (Gerenciamento Cíclico de Memória)
- **O Problema**: Cada frame instanciar classes, vetores ou invocar `new std::function` degrada a performance da CPU.
- **Implementação**: O Motor gráfico passará a requisitar um Buffer Ring (Pool) por Thread associado a um CommandBuffer. Todos os dados graváveis efêmeros de um frame (ex: lambdas da OpenGL API e vetores auxiliares) viverão numa arena de memória estrita ("Linear Allocator"), sendo resetados no final do frame. Assim, varremos a fragmentação de heap para debaixo do tapete.

---

## Fase 3: Modernização de Dados de Shader

### 1. Uniform Buffers (UBOs) e Descriptor Sets
Atualmente, as atualizações via `SetUniform` na interface geram uma enxurrada de trafégo síncrono da CPU por atributo alterado do material.
- **Implementação**: Refatorar o Material e o Shader para preencher localmente (memcpy) **Constant Buffers (CBV)** e apenas transferir esse bloco atômico no começo de um Draw para o Pipeline global. Em OpenGL usa-se Uniform Buffer Objects (UBO); No Vulkan faremos Binding de **Descriptor Sets**.
- **Benefício**: Performance severamente aprimorada. Em Vulkan nativo não usar descritores unificados esgota recursos e eleva overhead das extensões Push Constants que devem ser preservadas apenas em casos estritos.

### 2. Separação de RenderPass Nativo no Vulkan (Multirendering Native)
- **Implementação**: Uma continuação mais aderente à natureza do subsistema gráfico render passes. Invés de dependermos do render pass global fixo do SwapChain no `vulkan_device.cpp`, integraremos de verdade o fluxo Vulkan que converte perfeitamente o novo `RenderPassDescriptor` do `ICommandList` em instâncias de `VkRenderPass` temporárias guardadas num cache por hash de anexos e tipos.
- **Benefício**: Suporte massivo a pós-processamento assíncrono real sem gambiarras do RenderGraph atual.

---

## Fase 4: Profiling Eficiente e State Baking (PSO)

### 1. GPU Queries Integradas
- **Implementação**: Extender o `ICommandList` expondo marcadores como `BeginTimestamp(id)` e `EndTimestamp(id)`.
- **Benefício**: Conseguir atrelar estatísticas reais (ms de render de hardware puro) no ImGui de Devs e emular o rastreio fino dos gargalos. Permitirá visualizar quanto tempo a GPU gasta executando OpaqueGeometry x Lighting x UI.

### 2. Objetos de Estado Unificados (State Block / Pre-Baked PSOs)
- **Implementação**: Aglutinar configurações isoladas do `Rasterizer`, `Depth/Stencil`, e flags de `Blend` do frame anterior e encapsular eles perfeitamente amarrados da forma como o Vulkan prefere: os `VkPipeline`.
- **Benefício**: Ao remover estado dinâmico (com exceção de Viewport/Scissor que são razoáveis), criamos e trocamos toda a alma do render com apenas um ponteiro global na GPU. Reduções extremas de validação de API por draw call.
