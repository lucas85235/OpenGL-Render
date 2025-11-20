# 🚀 Model Loading - Guia Rápido

## 📦 Arquivos do Projeto

```
seu_projeto/
├── main.cpp                    # Aplicação principal
├── shader.hpp                  # Sistema de shaders
├── model.hpp                   # Carregamento de modelos (Assimp)
├── mesh.hpp                    # Renderização de malhas
├── framebuffer.hpp            # Sistema de framebuffers
├── procedural_model.hpp       # Modelos geométricos procedurais
├── stb_image.h                # Carregamento de texturas
├── compile.sh                 # Script de compilação
├── models/                    # Seus modelos 3D
│   └── backpack/
│       ├── backpack.obj
│       └── textures/
└── README_MODEL_LOADING.md    # Documentação completa
```

## ⚡ Quick Start (3 passos)

### 1. Instalar Dependências

```bash
# Ubuntu/Debian
sudo apt-get install libglew-dev libglfw3-dev libassimp-dev libglm-dev

# Fedora
sudo dnf install glew-devel glfw-devel assimp-devel glm-devel

# Arch
sudo pacman -S glew glfw-x11 assimp glm
```

### 2. Baixar stb_image.h

```bash
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

### 3. Compilar e Executar

```bash
chmod +x compile.sh
./compile.sh
```

## 🎯 Opções de Uso

### Opção A: Testar SEM Baixar Modelos

Use modelos procedurais! Adicione no `main.cpp`:

```cpp
#include "procedural_model.hpp"

// Ao invés de carregar modelo:
ProceduralModel sphere;
sphere.CreateSphere(1.0f);

// No loop de renderização:
sphere.Draw();
```

**Formas disponíveis:**
- `CreateCube()` - Cubo com iluminação
- `CreateSphere()` - Esfera suave
- `CreatePlane()` - Plano (para chão)
- `CreateCylinder()` - Cilindro

### Opção B: Carregar Modelos Reais

**1. Baixar um modelo:**
- [LearnOpenGL Backpack](https://learnopengl.com/img/model/backpack.zip)
- [Sketchfab](https://sketchfab.com/feed) (filtrar "downloadable")
- [Free3D](https://free3d.com/)

**2. Colocar na pasta:**
```
models/
└── backpack/
    ├── backpack.obj
    └── diffuse.jpg
```

**3. Carregar no código:**
```cpp
auto model = std::make_unique<Model>("models/backpack/backpack.obj");
model->Draw(shader.GetProgramID());
```

## 🎮 Controles

| Tecla | Ação |
|-------|------|
| W/A/S/D | Mover câmera |
| Mouse | Olhar ao redor |
| Scroll | Zoom |
| Space | Subir |
| Shift | Descer |
| ESC | Sair |

## 📝 Exemplo Completo Mínimo

```cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "shader.hpp"
#include "model.hpp"

int main() {
    // 1. Inicializar OpenGL/GLFW/GLEW
    glfwInit();
    // ... (configuração da janela)
    
    // 2. Compilar shader
    Shader shader;
    shader.CompileFromSource(
        ShaderSource::ModelVertexShader,
        ShaderSource::ModelFragmentShader
    );
    
    // 3. Carregar modelo
    Model model("models/seu_modelo.obj");
    
    // 4. Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        shader.Use();
        // Configurar matrizes...
        
        model.Draw(shader.GetProgramID());
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}
```

## 🔧 Personalizações Comuns

### Ajustar Tamanho do Modelo

```cpp
glm::mat4 model = glm::mat4(1.0f);
model = glm::scale(model, glm::vec3(0.1f));  // 10% do tamanho
```

### Múltiplos Modelos

```cpp
std::vector<std::unique_ptr<Model>> models;
models.push_back(std::make_unique<Model>("model1.obj"));
models.push_back(std::make_unique<Model>("model2.obj"));

for(auto& m : models) {
    // Ajustar posição de cada um...
    m->Draw(shader);
}
```

### Mudar Iluminação

```cpp
shader.SetVec3("lightPos", 5.0f, 10.0f, 5.0f);
shader.SetVec3("lightColor", 1.0f, 0.9f, 0.8f);  // Luz amarelada
```

### Remover Rotação Automática

```cpp
// Comente esta linha:
// modelMatrix = glm::rotate(modelMatrix, time, glm::vec3(0,1,0));
```

## 🐛 Problemas Comuns

### "Cannot find -lassimp"
```bash
sudo apt-get install libassimp-dev
```

### "stb_image.h: No such file"
```bash
wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
```

### Modelo não aparece
- Verifique caminho do arquivo
- Ajuste escala: `glm::scale(model, glm::vec3(0.1f))`
- Afaste câmera: `cameraPos.z = 10.0f`

### Modelo aparece preto
- Aumente luz ambiente no shader
- Verifique se normais foram carregadas
- Ajuste posição da luz

### FPS baixo
- Reduza polígonos do modelo
- Ative face culling: `glEnable(GL_CULL_FACE)`
- Use modelos LOD (Level of Detail)

## 📊 Comparação: Procedural vs Assimp

| Feature | Procedural | Assimp |
|---------|-----------|--------|
| Velocidade | ⚡ Instantâneo | 🐌 Carregamento |
| Qualidade | ⭐⭐ Simples | ⭐⭐⭐⭐⭐ Realista |
| Texturas | ❌ Não | ✅ Sim |
| Complexidade | 🟢 Fácil | 🟡 Médio |
| Tamanho | Pequeno | Grande |
| Animações | ❌ | ✅ |

**Recomendação:**
- **Aprendendo?** Use procedural primeiro
- **Projeto real?** Use Assimp

## 🎨 Galeria de Modelos Gratuitos

### Sites Confiáveis

1. **Sketchfab** - https://sketchfab.com
   - Filtros: Downloadable + Free
   - Formatos: OBJ, FBX, GLTF

2. **Free3D** - https://free3d.com
   - Boa variedade
   - Principalmente OBJ

3. **CGTrader** - https://cgtrader.com/free-3d-models
   - Qualidade profissional
   - Alguns gratuitos

4. **TurboSquid** - https://turbosquid.com/Search/3D-Models/free
   - Modelos verificados
   - Boa qualidade

### Formatos Recomendados

✅ **OBJ** - Simples, universal, boa compatibilidade  
✅ **FBX** - Bom para animações  
✅ **GLTF** - Padrão moderno, eficiente  
⚠️ **Blend** - Requer Assimp recente  
❌ **STL** - Sem cores/texturas

## 📚 Próximos Passos

### Nível 1: Básico
- [x] Carregar modelos OBJ
- [ ] Adicionar múltiplas luzes
- [ ] Implementar câmera suave
- [ ] Criar cena com vários objetos

### Nível 2: Intermediário
- [ ] Normal mapping
- [ ] Shadow mapping
- [ ] Skybox
- [ ] Particle system

### Nível 3: Avançado
- [ ] Animações esqueléticas
- [ ] PBR (Physically Based Rendering)
- [ ] Deferred shading
- [ ] IBL (Image-Based Lighting)

## 💡 Dicas Pro

1. **Organize seus assets:**
   ```
   models/
   ├── characters/
   ├── props/
   ├── environments/
   └── effects/
   ```

2. **Use convenções de nomenclatura:**
   ```
   model_name.obj
   model_name_diffuse.png
   model_name_normal.png
   model_name_specular.png
   ```

3. **Otimize antes de importar:**
   - Reduza polígonos desnecessários
   - Use texturas power-of-two (256, 512, 1024)
   - Combine meshes quando possível

4. **Debug visual:**
   ```cpp
   // Desenhar normais
   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Wireframe
   ```

## 🤝 Contribuindo

Encontrou um bug? Tem uma sugestão?
- Abra uma issue
- Envie um pull request
- Compartilhe seus modelos!

## 📖 Recursos Adicionais

- [LearnOpenGL Tutorial](https://learnopengl.com/Model-Loading/Assimp)
- [Assimp Docs](https://assimp-docs.readthedocs.io/)
- [OpenGL Wiki](https://www.khronos.org/opengl/wiki/)
- [Blender (criar modelos)](https://www.blender.org/)

---

**Dúvidas?** Revise o README_MODEL_LOADING.md para informações detalhadas!

**Bom desenvolvimento! 🚀**