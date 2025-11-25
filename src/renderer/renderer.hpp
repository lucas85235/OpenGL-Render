#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <vector>
#include <memory>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "render_command.hpp"
#include "shader.hpp"
#include "model.hpp"

// Dados globais da cena (Câmera, Luzes)
struct SceneData {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPos;
    // Futuramente: Adicionar array de luzes aqui
    glm::vec3 lightPos;
    glm::vec3 lightColor;
};

// ESTRUTURAS DE DADOS DE LUZ
struct DirectionalLight {
    glm::vec3 direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
};

struct PointLightData {
    glm::vec3 position;
    glm::vec3 color;
    float intensity;
    float radius;
};

class Renderer {
private:
    // Filas de renderização
    std::vector<RenderCommand> opaqueQueue;
    std::vector<RenderCommand> transparentQueue;

    SceneData sceneData;
    Shader* activeShader; // Shader padrão (PBR)

    // Recursos Internos do Renderer
    unsigned int screenQuadVAO = 0;
    unsigned int screenQuadVBO = 0;

    // FILAS DE LUZ
    DirectionalLight sunLight; // Apenas 1 sol por enquanto
    std::vector<PointLightData> pointLights;

    void initRenderData() {
        // Configuração do Quad de Tela Cheia (NDC: -1 a 1)
        float quadVertices[] = { 
            // posições       // texCoords
            -1.0f,  1.0f,     0.0f, 1.0f,
            -1.0f, -1.0f,     0.0f, 0.0f,
             1.0f, -1.0f,     1.0f, 0.0f,
            
            -1.0f,  1.0f,     0.0f, 1.0f,
             1.0f, -1.0f,     1.0f, 0.0f,
             1.0f,  1.0f,     1.0f, 1.0f
        };

        glGenVertexArrays(1, &screenQuadVAO);
        glGenBuffers(1, &screenQuadVBO);
        glBindVertexArray(screenQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, screenQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        
        glBindVertexArray(0);
    }

public:
    Renderer() : activeShader(nullptr) {}

    ~Renderer() {
        if (screenQuadVAO) glDeleteVertexArrays(1, &screenQuadVAO);
        if (screenQuadVBO) glDeleteBuffers(1, &screenQuadVBO);
    }

    void Init(Shader* defaultShader) {
        activeShader = defaultShader;
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        // glDisable(GL_CULL_FACE);
        initRenderData();
    }

    // 1. Início do Frame: Configura globais
    void BeginScene(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) {
        sceneData.viewMatrix = view;
        sceneData.projectionMatrix = proj;
        sceneData.cameraPos = camPos;
        
        // Setup temporário de luz (hardcoded por enquanto)
        sceneData.lightPos = glm::vec3(2.0f, 4.0f, 3.0f);
        sceneData.lightColor = glm::vec3(1.0f);

        // Limpar filas antigas
        opaqueQueue.clear();
        transparentQueue.clear();
        pointLights.clear();
    }

    // --- SUBMIT LIGHTS ---
    void SubmitDirectionalLight(const DirectionalLight& light) {
        sunLight = light;
    }

    void SubmitPointLight(const PointLightData& light) {
        if (pointLights.size() < 4) { // Limite do Shader (MAX_POINT_LIGHTS)
            pointLights.push_back(light);
        }
    }

    // 2. Submissão: Alguém pede para ser desenhado
    void Submit(const std::shared_ptr<Model>& model, const glm::mat4& transform) {
        // Itera sobre todos os meshes do modelo
        for(size_t i = 0; i < model->GetMeshCount(); i++) {
            // Precisamos do const_cast porque seus getters retornam const Mesh&
            // Idealmente, ajuste o Model para retornar ponteiros ou referências não-const se necessário
            const Mesh& meshRef = model->GetMesh(i);
            Mesh* meshPtr = const_cast<Mesh*>(&meshRef); 
            Material* matPtr = meshPtr->GetMaterial().get();

            // Calcular distância para ordenação (simples distância Euclidiana)
            float dist = glm::length(sceneData.cameraPos - glm::vec3(transform[3]));

            // Decidir fila (Opaco vs Transparente)
            // Por enquanto, tudo é opaco. Futuramente checar matPtr->IsTransparent()
            opaqueQueue.emplace_back(meshPtr, matPtr, transform, dist);
        }
    }

    // 2. Submissão: Alguém pede para ser desenhado
    void SubmitMesh(const Mesh& mesh, const glm::mat4& transform) {
        // Itera sobre todos os meshes do modelo

        // Precisamos do const_cast porque seus getters retornam const Mesh&
        // Idealmente, ajuste o Model para retornar ponteiros ou referências não-const se necessário
        // const Mesh& meshRef = *mesh;
        Mesh* meshPtr = const_cast<Mesh*>(&mesh); 
        Material* matPtr = meshPtr->GetMaterial().get();

        // Calcular distância para ordenação (simples distância Euclidiana)
        float dist = glm::length(sceneData.cameraPos - glm::vec3(transform[3]));

        // Decidir fila (Opaco vs Transparente)
        // Por enquanto, tudo é opaco. Futuramente checar matPtr->IsTransparent()
        opaqueQueue.emplace_back(meshPtr, matPtr, transform, dist);
    }

    // 3. Fim do Frame: Onde a mágica acontece
    void EndScene() {
        // PASSO A: Ordenação
        // Opacos: Frente -> Trás (Reduz Overdraw, ajuda o Z-Buffer)
        std::sort(opaqueQueue.begin(), opaqueQueue.end(), 
            [](const RenderCommand& a, const RenderCommand& b) {
                return a.distanceToCamera < b.distanceToCamera;
            });

        // PASSO B: Configuração Global do Shader
        activeShader->Use();
        activeShader->SetMat4("view", glm::value_ptr(sceneData.viewMatrix));
        activeShader->SetMat4("projection", glm::value_ptr(sceneData.projectionMatrix));
        activeShader->SetVec3("viewPos", sceneData.cameraPos.x, sceneData.cameraPos.y, sceneData.cameraPos.z);
        activeShader->SetVec3("lightPos", sceneData.lightPos.x, sceneData.lightPos.y, sceneData.lightPos.z);
        activeShader->SetVec3("lightColor", sceneData.lightColor.x, sceneData.lightColor.y, sceneData.lightColor.z);

        // ENVIO DE LUZES PARA O SHADER
        
        // 1. Sol
        activeShader->SetVec3("dirLight.direction", sunLight.direction.x, sunLight.direction.y, sunLight.direction.z);
        activeShader->SetVec3("dirLight.color", sunLight.color.x, sunLight.color.y, sunLight.color.z);
        activeShader->SetFloat("dirLight.intensity", sunLight.intensity);

        // 2. Point Lights
        activeShader->SetInt("numPointLights", (int)pointLights.size());
        
        for (size_t i = 0; i < pointLights.size(); i++) {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            activeShader->SetVec3(base + ".position", pointLights[i].position.x, pointLights[i].position.y, pointLights[i].position.z);
            activeShader->SetVec3(base + ".color", pointLights[i].color.x, pointLights[i].color.y, pointLights[i].color.z);
            activeShader->SetFloat(base + ".intensity", pointLights[i].intensity);
            activeShader->SetFloat(base + ".radius", pointLights[i].radius);
        }

        // PASSO C: Render Loop (Geometry Pass)
        for (const auto& cmd : opaqueQueue) {
            RenderMesh(cmd);
        }
    }

    // --- NOVO: Método para desenhar Pós-Processamento ---
    void DrawScreenQuad(Shader& screenShader, unsigned int textureID) {
        glDisable(GL_DEPTH_TEST); // Pós-processo não precisa de depth test
        
        screenShader.Use();
        screenShader.SetInt("screenTexture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glBindVertexArray(screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        glEnable(GL_DEPTH_TEST); // Restaura para o próximo frame
    }
    
    // Sobrecarga caso você queira desenhar sem setar textura (ex: shaders procedurais puros)
    void DrawScreenQuad() {
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

private:
    void RenderMesh(const RenderCommand& cmd) {
        // 1. Configurar Material
        if (cmd.material) {
            cmd.material->Apply(activeShader->GetProgramID());
            
            // Flags de textura (necessário manter compatibilidade com seu shader atual)
            activeShader->SetBool("hasTextureDiffuse", cmd.material->HasTextureType(TextureType::DIFFUSE));
            activeShader->SetBool("hasTextureNormal", cmd.material->HasTextureType(TextureType::NORMAL));
            activeShader->SetBool("hasTextureMetallic", cmd.material->HasTextureType(TextureType::METALLIC));
            activeShader->SetBool("hasTextureRoughness", cmd.material->HasTextureType(TextureType::ROUGHNESS));
            activeShader->SetBool("hasTextureAO", cmd.material->HasTextureType(TextureType::AO));
            activeShader->SetBool("hasTextureEmission", cmd.material->HasTextureType(TextureType::EMISSION));
        }

        // 2. Configurar Transform
        activeShader->SetMat4("model", glm::value_ptr(cmd.transform));

        // 3. Draw Call Crua (Acessando internos do Mesh)
        // Nota: Você precisará tornar Mesh "friend" de Renderer ou expor VAO/Indices
        // Por hora, vamos supor que vamos adicionar um método "BindAndDraw" no Mesh que não pede shader
        
        // Hack temporário: Chamar o Draw do mesh mas ignorar o shader program dentro dele se possível,
        // ou melhor, adicionar um método Render() limpo na classe Mesh.
        // Vamos assumir que você adicionou `void RenderPrimitive()` no Mesh.hpp
        
        glBindVertexArray(cmd.mesh->GetVAO()); // Você precisará criar esse getter
        glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, 0); // E esse getter
        glBindVertexArray(0);
    }
};

#endif // RENDERER_HPP