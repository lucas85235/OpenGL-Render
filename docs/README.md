# Projeto Framebuffer OpenGL

Exemplo organizado de uso de framebuffers com OpenGL 3.3+ no Linux.

## Estrutura do Projeto

```
.
├── main.cpp           # Arquivo principal com loop de renderização
├── framebuffer.hpp    # Classe para gerenciar framebuffers
├── shader.hpp         # Classe para gerenciar shaders e código dos shaders
├── run.sh            # Script de compilação e execução
└── README.md         # Este arquivo
```

## Arquitetura

### `framebuffer.hpp`
Classe `FrameBuffer` que encapsula:
- Criação e gerenciamento de framebuffer objects
- Textura de cor (color attachment)
- Renderbuffer para depth/stencil
- Métodos para bind/unbind
- Redimensionamento dinâmico
- Limpeza automática de recursos (RAII)

**Métodos principais:**
- `Init()` - Inicializa o framebuffer
- `Bind()` - Ativa o framebuffer para renderização
- `Unbind()` - Volta para o framebuffer padrão
- `Resize(w, h)` - Redimensiona o framebuffer
- `GetTexture()` - Retorna a textura renderizada

### `shader.hpp`
Classe `Shader` que encapsula:
- Compilação de vertex e fragment shaders
- Linking do programa
- Métodos utilitários para uniforms
- Gerenciamento de recursos OpenGL
- Namespace `ShaderSource` com código dos shaders

**Métodos principais:**
- `CompileFromSource()` - Compila shaders a partir de strings
- `CompileFromFile()` - Compila shaders a partir de arquivos
- `Use()` - Ativa o shader
- `SetMat4()`, `SetInt()`, `SetFloat()`, etc. - Define uniforms

**Shaders incluídos:**
- `CubeVertexShader` / `CubeFragmentShader` - Renderiza o cubo
- `ScreenVertexShader` / `ScreenFragmentShader` - Pós-processamento (inversão de cores)
- `ScreenFragmentShaderGrayscale` - Efeito grayscale
- `ScreenFragmentShaderBlur` - Efeito de blur

### `main.cpp`
Arquivo principal que:
- Inicializa GLFW e GLEW
- Cria janela
- Compila shaders usando a classe `Shader`
- Cria geometria (cubo e quad da tela)
- Inicializa framebuffer
- Loop de renderização em duas passadas:
  1. Renderiza cena para o framebuffer
  2. Renderiza textura do framebuffer na tela com efeito

## Dependências

- OpenGL 3.3+
- GLEW (OpenGL Extension Wrangler)
- GLFW3 (Window management)

### Instalação no Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install libglew-dev libglfw3-dev
```

### Instalação no Fedora

```bash
sudo dnf install glew-devel glfw-devel
```

### Instalação no Arch Linux

```bash
sudo pacman -S glew glfw-x11
```

## Compilação e Execução

### Usando o script

```bash
chmod +x run.sh
./run.sh
```

### Manualmente

```bash
g++ main.cpp -o framebuffer -lGL -lGLEW -lglfw -lm
./framebuffer
```

## Como Funciona

O programa demonstra o conceito de **renderização offscreen** com framebuffers:

1. **Primeira passada (Offscreen)**:
   - A cena 3D (cubo rotativo) é renderizada para uma textura
   - Usa o framebuffer customizado
   - Depth testing habilitado

2. **Segunda passada (Tela)**:
   - A textura renderizada é aplicada em um quad que cobre toda a tela
   - Usa o framebuffer padrão (tela)
   - Efeito de pós-processamento aplicado (inversão de cores)

## Personalizações

### Trocar o efeito de pós-processamento

No `main.cpp`, troque o shader usado:

```cpp
// Em vez de:
screenShader.CompileFromSource(ShaderSource::ScreenVertexShader, 
                              ShaderSource::ScreenFragmentShader);

// Use:
screenShader.CompileFromSource(ShaderSource::ScreenVertexShader, 
                              ShaderSource::ScreenFragmentShaderGrayscale);
// ou
screenShader.CompileFromSource(ShaderSource::ScreenVertexShader, 
                              ShaderSource::ScreenFragmentShaderBlur);
```

### Adicionar novo efeito

1. Adicione o código do shader em `shader.hpp` no namespace `ShaderSource`
2. Compile usando o novo shader no `main.cpp`

### Criar shader a partir de arquivos

```cpp
Shader myShader;
myShader.CompileFromFile("vertex.glsl", "fragment.glsl");
```

## Controles

- **ESC** - Fechar aplicação
- A janela pode ser redimensionada (o framebuffer se ajusta automaticamente)

## Conceitos Demonstrados

- ✅ Framebuffer Objects (FBO)
- ✅ Render to Texture
- ✅ Pós-processamento
- ✅ RAII (Resource Acquisition Is Initialization)
- ✅ Arquitetura orientada a objetos
- ✅ Separação de responsabilidades
- ✅ Move semantics (C++11)

## Possíveis Melhorias

- [ ] Sistema de câmera
- [ ] Múltiplos framebuffers
- [ ] Shadow mapping
- [ ] Bloom effect
- [ ] HDR rendering
- [ ] Sistema de materiais
- [ ] Carregar modelos 3D

## Referências

- [LearnOpenGL - Framebuffers](https://learnopengl.com/Advanced-OpenGL/Framebuffers)
- [OpenGL Wiki - Framebuffer Object](https://www.khronos.org/opengl/wiki/Framebuffer_Object)

## Licença

Código de exemplo educacional - use livremente!