# 🎨 Model Loading com OpenGL + Assimp

Sistema completo para carregar e renderizar modelos 3D (.obj, .fbx, .gltf, etc.) usando OpenGL e Assimp.

## 📁 Estrutura do Projeto

```
.
├── main.cpp              # Aplicação com câmera livre
├── shader.hpp            # Gerenciador de shaders
├── model.hpp             # Classe Model (carrega modelos)
├── mesh.hpp              # Classe Mesh (renderiza geometria)
├── stb_image.h          # Biblioteca para carregar texturas
├── models/              # Pasta para seus modelos 3D
│   └── backpack/
│       ├── backpack.obj
│       └── diffuse.jpg
└── README_MODEL_LOADING.md
```

## 🔧 Dependências

### Bibliotecas Necessárias

1. **OpenGL** - API gráfica
2. **GLEW** - Extensões OpenGL
3. **GLFW** - Gerenciamento de janelas
4. **Assimp** - Carregamento de modelos 3D
5. **GLM** - Matemática para gráficos 3D
6. **stb_image.h** - Carregamento de texturas (header-only)

### Instalação no Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install libglew-dev libglfw3-dev libassimp-dev libglm-dev
```

### Instalação no Fedora

```bash
sudo dnf install glew-devel glfw-devel assimp-devel glm-devel
```

### Instalação no Arch Linux

```bash
sudo pacman -S glew glfw-x11 assimp glm
```

### stb_image.h (Header-only)

Baixe de: https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

```bash
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

Coloque no diretório do projeto.

## 📥 Baixando Modelos 3D

### Sites Recomendados (Modelos Gratuitos)

1. **Sketchfab** - https://sketchfab.com/feed
   - Filtrar por "Downloadable" e "Free"
   - Formatos: .obj, .fbx, .gltf

2. **Free3D** - https://free3d.com/
   - Muitos modelos gratuitos
   - Formato .obj é o mais compatível

3. **TurboSquid Free** - https://www.turbosquid.com/Search/3D-Models/free
   - Modelos free de boa qualidade

4. **CGTrader** - https://www.cgtrader.com/free-3d-models
   - Boa variedade de modelos gratuitos

### Modelo de Exemplo (Backpack)

O tutorial LearnOpenGL usa este modelo:
- Download: https://learnopengl.com/img/model/backpack.zip
- Extrair para: `models/backpack/`

### Estrutura Esperada

```
models/
└── nome_do_modelo/
    ├── modelo.obj          # Arquivo principal
    ├── textura.jpg/png     # Texturas
    └── normal.jpg (opcional)
```

## 🛠️ Compilação

### Script de Compilação (compile.sh)

```bash
#!/bin/bash
clear
echo "Compilando Model Loading..."

g++ main.cpp \
    -o model_viewer \
    -std=c++17 \
    -lGL -lGLEW -lglfw -lassimp \
    -I/usr/include \
    -L/usr/lib

if [ $? -eq 0 ]; then
    echo "✓ Compilação bem-sucedida!"
    echo "Executando..."
    ./model_viewer
else
    echo "✗ Erro na compilação!"
fi
```

Torne executável:
```bash
chmod +x compile.sh
./compile.sh
```

### Compilação Manual

```bash
g++ main.cpp -o model_viewer -std=c++17 -lGL -lGLEW -lglfw -lassimp
./model_viewer
```

### Possíveis Erros de Compilação

**Erro: "assimp/Importer.hpp: No such file"**
```bash
# Instale assimp
sudo apt-get install libassimp-dev
```

**Erro: "glm/glm.hpp: No such file"**
```bash
# Instale GLM
sudo apt-get install libglm-dev
```

**Erro: "undefined reference to assimp"**
```bash
# Certifique-se de ter -lassimp no comando
g++ ... -lassimp
```

## 🎮 Controles

| Tecla | Ação |
|-------|------|
| **W** | Mover para frente |
| **S** | Mover para trás |
| **A** | Mover para esquerda |
| **D** | Mover para direita |
| **Space** | Subir |
| **Shift** | Descer |
| **Mouse** | Olhar ao redor |
| **Scroll** | Zoom in/out |
| **ESC** | Sair |

## 📝 Arquitetura das Classes

### `Mesh` (mesh.hpp)

Representa uma única malha 3D com seus vértices, índices e texturas.

```cpp
struct Vertex {
    glm::vec3 Position;  // Posição do vértice
    glm::vec3 Normal;    // Normal para iluminação
    glm::vec2 TexCoords; // Coordenadas de textura
};

class Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    
    void Draw(unsigned int shaderProgram);
};
```

### `Model` (model.hpp)

Carrega e gerencia um modelo 3D completo (pode ter múltiplos meshes).

```cpp
class Model {
    std::vector<Mesh> meshes;
    
    void loadModel(std::string path);     // Usa Assimp
    void processNode(aiNode *node, ...);  // Hierarquia
    Mesh processMesh(aiMesh *mesh, ...);  // Converte mesh
    
public:
    Model(const std::string &path);
    void Draw(unsigned int shaderProgram);
};
```

## 🎨 Sistema de Shaders

### Vertex Shader

Processa cada vértice e prepara dados para o fragment shader:
- Transforma posições (model, view, projection)
- Calcula normais no espaço mundial
- Passa coordenadas de textura

### Fragment Shader

Calcula a cor final de cada pixel:
- **Ambient** - Iluminação ambiente
- **Diffuse** - Iluminação difusa (Lambert)
- **Specular** - Reflexos especulares (Phong)

## 🔍 Como Funciona (Pipeline)

```
1. Assimp carrega arquivo → aiScene
                ↓
2. Processa hierarquia de nós recursivamente
                ↓
3. Para cada aiMesh:
   - Extrai vértices (pos, normal, texcoord)
   - Extrai índices (faces)
   - Carrega texturas do material
                ↓
4. Cria objetos OpenGL (VAO, VBO, EBO)
                ↓
5. No loop de renderização:
   - Bind shader
   - Define uniforms (matrizes, luz)
   - Para cada mesh: Draw()
```

## 🎯 Customizações

### Ajustar Escala do Modelo

No `main.cpp`, linha ~180:

```cpp
modelMatrix = glm::scale(modelMatrix, glm::vec3(0.1f, 0.1f, 0.1f)); // Menor
// ou
modelMatrix = glm::scale(modelMatrix, glm::vec3(2.0f, 2.0f, 2.0f)); // Maior
```

### Mudar Posição da Luz

```cpp
modelShader.SetVec3("lightPos", 5.0f, 5.0f, 5.0f);
```

### Desabilitar Rotação Automática

Remova ou comente a linha:

```cpp
// modelMatrix = glm::rotate(modelMatrix, (float)glfwGetTime() * 0.3f, ...);
```

### Carregar Modelo Diferente

```cpp
model = std::make_unique<Model>("models/seu_modelo/modelo.obj");
```

### Adicionar Múltiplos Modelos

```cpp
std::vector<std::unique_ptr<Model>> models;
models.push_back(std::make_unique<Model>("models/model1.obj"));
models.push_back(std::make_unique<Model>("models/model2.obj"));

// No loop:
for(auto& model : models) {
    // Ajustar modelMatrix para cada um
    model->Draw(shader.GetProgramID());
}
```

## 🐛 Troubleshooting

### Problema: Modelo não aparece

**Soluções:**
1. Verifique se o caminho está correto
2. Ajuste a escala (modelo pode ser muito pequeno/grande)
3. Mova a câmera para trás: `cameraPos = glm::vec3(0, 0, 10)`
4. Verifique console para erros de carregamento

### Problema: Modelo aparece preto

**Soluções:**
1. Verifique se as normais foram carregadas
2. Ajuste posição da luz
3. Aumente `ambientStrength` no shader
4. Verifique se o modelo tem texturas

### Problema: Texturas não carregam

**Soluções:**
1. Verifique se stb_image.h está no projeto
2. Certifique-se que texturas estão na mesma pasta do .obj
3. Verifique caminhos relativos no arquivo .mtl
4. Use shader sem textura (`ModelFragmentShaderNoTexture`)

### Problema: FPS baixo

**Soluções:**
1. Reduza qualidade de texturas
2. Use menos polígonos
3. Implemente frustum culling
4. Ative face culling: `glEnable(GL_CULL_FACE)`

## 📊 Formatos Suportados

Assimp suporta 40+ formatos:

| Formato | Extensão | Recomendado |
|---------|----------|-------------|
| Wavefront | .obj | ✓ Ótimo para começar |
| Autodesk FBX | .fbx | ✓ Bom para animações |
| glTF 2.0 | .gltf/.glb | ✓ Padrão moderno |
| Collada | .dae | ✓ Boa compatibilidade |
| Blender | .blend | ⚠️ Requer versão recente |
| 3DS Max | .3ds | ✓ Comum |
| STL | .stl | △ Sem texturas/cores |

## 🚀 Próximos Passos

1. **Animações Esqueléticas** - Carregar e reproduzir animações
2. **Normal Mapping** - Mais detalhes sem mais polígonos
3. **LOD (Level of Detail)** - Otimização automática
4. **Instanced Rendering** - Renderizar muitos objetos
5. **Scene Graph** - Hierarquia de objetos
6. **PBR Materials** - Materiais fisicamente realistas

## 📚 Referências

- [LearnOpenGL Model Loading](https://learnopengl.com/Model-Loading/Assimp)
- [Assimp Documentation](https://assimp-docs.readthedocs.io/)
- [GLM Documentation](https://glm.g-truc.net/)
- [OpenGL Wiki](https://www.khronos.org/opengl/wiki/)

## 📄 Licença

Código educacional - use livremente!