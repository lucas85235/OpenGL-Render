# 🎨 Sistema de Materiais e Texturas Customizados

## 📋 Visão Geral

Sistema completo e modular para gerenciar texturas e materiais customizados em OpenGL com suporte a:
- ✅ PBR (Physically Based Rendering)
- ✅ Phong/Blinn-Phong tradicional
- ✅ Normal mapping
- ✅ Texturas múltiplas por tipo
- ✅ Cache de texturas
- ✅ Materiais procedurais
- ✅ Materiais emissivos

## 🗂️ Estrutura de Arquivos

```
src/renderer/
├── texture.hpp           # Sistema de texturas e cache
├── material.hpp          # Sistema de materiais
├── mesh_updated.hpp      # Mesh com suporte a materiais
├── custom_shaders.hpp    # Shaders PBR avançados
└── stb_image.h          # Carregador de imagens
```

## 🔧 Instalação

### 1. Adicionar os arquivos ao projeto

Copie os arquivos para `src/renderer/`:
- `texture.hpp`
- `material.hpp`
- `mesh_updated.hpp`
- `custom_shaders.hpp`

### 2. Atualizar includes no main.cpp

```cpp
// Remova:
// #include "mesh.hpp"

// Adicione:
#include "texture.hpp"
#include "material.hpp"
#include "mesh_updated.hpp"
#include "custom_shaders.hpp"
```

## 📝 Uso Básico

### Exemplo 1: Material com Texturas

```cpp
// Criar material
auto material = std::make_shared<Material>("MyMaterial");

// Carregar texturas
material->LoadTexture("textures/diffuse.png", TextureType::DIFFUSE);
material->LoadTexture("textures/normal.png", TextureType::NORMAL);
material->LoadTexture("textures/roughness.png", TextureType::ROUGHNESS);

// Configurar propriedades
material->SetMetallic(0.2f);
material->SetRoughness(0.5f);

// Aplicar em mesh
mesh.SetMaterial(material);
```

### Exemplo 2: Material Procedural

```cpp
// Usar materiais pré-definidos
auto goldMaterial = std::make_shared<Material>(
    MaterialLibrary::CreateGold()
);

mesh.SetMaterial(goldMaterial);
```

### Exemplo 3: Shader Customizado

```cpp
// Compilar shader PBR
Shader pbrShader;
pbrShader.CompileFromSource(
    CustomShaders::AdvancedVertexShader,
    CustomShaders::CustomMaterialFragmentShader
);

// No loop de renderização
pbrShader.Use();

// Configurar flags de textura
pbrShader.SetBool("hasTextureDiffuse", 
    material->HasTextureType(TextureType::DIFFUSE));
pbrShader.SetBool("hasTextureNormal", 
    material->HasTextureType(TextureType::NORMAL));
// ... outras flags

// Desenhar
model->Draw(pbrShader.GetProgramID());
```

## 🎯 Tipos de Texturas Suportados

| Tipo | Uso | Shader Uniform |
|------|-----|---------------|
| `DIFFUSE` | Cor base | `texture_diffuse1` |
| `SPECULAR` | Reflexo especular | `texture_specular1` |
| `NORMAL` | Detalhes de superfície | `texture_normal1` |
| `HEIGHT` | Parallax/displacement | `texture_height1` |
| `METALLIC` | Metálico (PBR) | `texture_metallic1` |
| `ROUGHNESS` | Rugosidade (PBR) | `texture_roughness1` |
| `AO` | Ambient Occlusion | `texture_ao1` |
| `EMISSION` | Brilho próprio | `texture_emission1` |

## 🎨 Materiais Pré-definidos

```cpp
// Metais
MaterialLibrary::CreateGold();
MaterialLibrary::CreateSilver();
MaterialLibrary::CreateCopper();

// Não-metais
MaterialLibrary::CreatePlastic();
MaterialLibrary::CreateRubber();

// Especiais
MaterialLibrary::CreateEmissive(glm::vec3(1.0f, 0.0f, 0.0f), 2.0f);
MaterialLibrary::CreatePhong(glm::vec3(0.8f, 0.2f, 0.2f));
```

## ⚙️ Configuração de Texturas

```cpp
TextureParams params;
params.wrapS = TextureWrap::REPEAT;              // Repetir em S
params.wrapT = TextureWrap::CLAMP_TO_EDGE;      // Clamp em T
params.minFilter = TextureFilter::LINEAR_MIPMAP_LINEAR; // Filtro
params.magFilter = TextureFilter::LINEAR;        // Magnificação
params.generateMipmap = true;                    // Gerar mipmaps
params.flipVertically = true;                    // Flip Y

material->LoadTexture("texture.png", TextureType::DIFFUSE, params);
```

## 🔄 Cache de Texturas

O sistema automaticamente cacheia texturas para evitar carregamentos duplicados:

```cpp
auto& manager = TextureManager::GetInstance();

// Primeira carga: lê do disco
auto tex1 = manager.LoadTexture("brick.jpg", TextureType::DIFFUSE);

// Segunda carga: retorna do cache
auto tex2 = manager.LoadTexture("brick.jpg", TextureType::DIFFUSE);

// Verificar cache
manager.PrintCacheInfo();

// Limpar cache (se necessário)
manager.ClearCache();
```

## 🎮 Integração Completa no main.cpp

```cpp
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.hpp"
#include "texture.hpp"
#include "material.hpp"
#include "mesh_updated.hpp"
#include "custom_shaders.hpp"
#include "model.hpp"

int main() {
    // ... inicialização GLFW/GLEW ...
    
    // Compilar shader customizado
    Shader pbrShader;
    if (!pbrShader.CompileFromSource(
            CustomShaders::AdvancedVertexShader,
            CustomShaders::CustomMaterialFragmentShader)) {
        std::cerr << "Falha ao compilar shader PBR!" << std::endl;
        return -1;
    }
    
    // Carregar modelo (model.hpp já atualizado)
    std::unique_ptr<Model> model;
    try {
        model = std::make_unique<Model>("models/gun/gun.obj");
    } catch (const std::exception& e) {
        std::cerr << "Erro: " << e.what() << std::endl;
        return -1;
    }
    
    // Opcional: Substituir material do modelo
    // auto customMat = std::make_shared<Material>(MaterialLibrary::CreateGold());
    // model->GetMesh(0).SetMaterial(customMat);
    
    // Loop de renderização
    while (!glfwWindowShouldClose(window)) {
        // ... clear, input, etc ...
        
        pbrShader.Use();
        
        // Matrizes
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                                (float)WIDTH / HEIGHT, 0.1f, 100.0f);
        
        pbrShader.SetMat4("model", glm::value_ptr(model));
        pbrShader.SetMat4("view", glm::value_ptr(view));
        pbrShader.SetMat4("projection", glm::value_ptr(projection));
        
        // Iluminação
        pbrShader.SetVec3("lightPos", 5.0f, 5.0f, 5.0f);
        pbrShader.SetVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
        pbrShader.SetVec3("lightColor", 1.0f, 1.0f, 1.0f);
        
        // Configurar flags de textura (uma vez por frame)
        // Nota: Pode ser otimizado fazendo isso apenas quando muda de material
        auto mat = model->GetMesh(0).GetMaterial();
        pbrShader.SetBool("hasTextureDiffuse", mat->HasTextureType(TextureType::DIFFUSE));
        pbrShader.SetBool("hasTextureNormal", mat->HasTextureType(TextureType::NORMAL));
        pbrShader.SetBool("hasTextureMetallic", mat->HasTextureType(TextureType::METALLIC));
        pbrShader.SetBool("hasTextureRoughness", mat->HasTextureType(TextureType::ROUGHNESS));
        pbrShader.SetBool("hasTextureAO", mat->HasTextureType(TextureType::AO));
        pbrShader.SetBool("hasTextureEmission", mat->HasTextureType(TextureType::EMISSION));
        
        // Desenhar
        model->Draw(pbrShader.GetProgramID());
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}
```

## 🛠️ Atualizar Model.hpp

Para que o sistema funcione com modelos carregados, você precisa atualizar `model.hpp` para usar `Material` ao invés do sistema antigo:

```cpp
// No processMesh(), ao invés de retornar Mesh com vector<Texture>:
Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
    // ... processar vértices e índices ...
    
    // Criar material
    auto material = std::make_shared<Material>("Mesh Material");
    
    // Carregar texturas do material
    if(mesh->mMaterialIndex >= 0) {
        aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];
        loadMaterialTextures(mat, material, scene);
    }
    
    return Mesh(vertices, indices, material);
}

void loadMaterialTextures(aiMaterial *mat, std::shared_ptr<Material> material, 
                          const aiScene* scene) {
    // Carregar diffuse
    for(unsigned int i = 0; i < mat->GetTextureCount(aiTextureType_DIFFUSE); i++) {
        aiString str;
        mat->GetTexture(aiTextureType_DIFFUSE, i, &str);
        material->LoadTexture(directory + "/" + str.C_Str(), TextureType::DIFFUSE);
    }
    
    // Repetir para outros tipos...
}
```

## 🎨 Exemplos Práticos

### Material Metálico com Texturas

```cpp
auto metalMaterial = std::make_shared<Material>("Metal");
metalMaterial->LoadTexture("metal_diffuse.png", TextureType::DIFFUSE);
metalMaterial->LoadTexture("metal_normal.png", TextureType::NORMAL);
metalMaterial->LoadTexture("metal_metallic.png", TextureType::METALLIC);
metalMaterial->LoadTexture("metal_roughness.png", TextureType::ROUGHNESS);
metalMaterial->SetMetallic(1.0f);
```

### Material Emissivo (Neon)

```cpp
auto neonMaterial = std::make_shared<Material>("Neon");
neonMaterial->SetAlbedo(glm::vec3(0.0f, 1.0f, 1.0f)); // Ciano
neonMaterial->SetEmission(glm::vec3(0.0f, 1.0f, 1.0f));
neonMaterial->SetEmissionStrength(5.0f);
neonMaterial->SetMetallic(0.0f);
neonMaterial->SetRoughness(0.1f);
```

### Trocar Material em Runtime

```cpp
std::vector<std::shared_ptr<Material>> materials = {
    std::make_shared<Material>(MaterialLibrary::CreateGold()),
    std::make_shared<Material>(MaterialLibrary::CreateSilver()),
    std::make_shared<Material>(MaterialLibrary::CreateCopper())
};

int currentMaterial = 0;

// No processInput():
if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
    currentMaterial = (currentMaterial + 1) % materials.size();
    mesh.SetMaterial(materials[currentMaterial]);
}
```

## 🐛 Troubleshooting

### Problema: Texturas não aparecem

**Solução:**
1. Verifique se as flags estão corretas no shader
2. Certifique-se que `hasTextureDiffuse` está true
3. Verifique o caminho do arquivo

### Problema: Normal mapping não funciona

**Solução:**
1. Certifique-se que o mesh tem tangentes e bitangentes
2. Use `AdvancedVertexShader` que calcula TBN matrix
3. Configure `hasTextureNormal = true`

### Problema: Materiais muito escuros/claros

**Solução:**
1. Ajuste `ambientStrength` no shader
2. Verifique posição da luz (`lightPos`)
3. Para PBR, ajuste `metallic` e `roughness`

## 📊 Performance

- **Cache de Texturas**: Texturas idênticas são compartilhadas automaticamente
- **Mipmaps**: Gerados automaticamente por padrão
- **SRGB**: Texturas de cor são carregadas em espaço sRGB para correção gamma automática

## 🚀 Próximos Passos

1. Implementar IBL (Image-Based Lighting)
2. Adicionar suporte para texturas de deslocamento (displacement)
3. Implementar parallax occlusion mapping
4. Adicionar suporte para texturas animadas
5. Criar editor visual de materiais

## 📚 Referências

- [LearnOpenGL PBR](https://learnopengl.com/PBR/Theory)
- [Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbr_epic_notes_v2.pdf)
- [Physically Based Rendering](https://pbr-book.org/)

---

**Dúvidas?** Revise os exemplos em `example_custom_materials.cpp`!