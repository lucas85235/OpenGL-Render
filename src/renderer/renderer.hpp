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
#include "skybox_manager.hpp"

// Dados globais da cena (Câmera, Luzes)
struct SceneData {
    glm::mat4 viewMatrix;
    glm::mat4 projectionMatrix;
    glm::vec3 cameraPos;
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
    Shader* activeShader;

    // Recursos Internos do Renderer
    unsigned int screenQuadVAO = 0;
    unsigned int screenQuadVBO = 0;

    Shader* skyboxShader = nullptr;
    SkyboxManager skyboxManager; // Gerenciador único do skybox

    // FILAS DE LUZ
    DirectionalLight sunLight;
    std::vector<PointLightData> pointLights;

    unsigned int iblIrradiance = 0;
    unsigned int iblPrefilter = 0;
    unsigned int iblBrdf = 0;
    bool useIBL = false;

    void initRenderData() {
        // Configuração do Quad de Tela Cheia
        float quadVertices[] = { 
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

        // Inicializar Skybox Manager
        if (!skyboxManager.Initialize()) {
            std::cerr << "Falha ao inicializar SkyboxManager no Renderer!" << std::endl;
        }
    }

public:
    Renderer() : activeShader(nullptr) {}

    ~Renderer() {
        if (screenQuadVAO) glDeleteVertexArrays(1, &screenQuadVAO);
        if (screenQuadVBO) glDeleteBuffers(1, &screenQuadVBO);
    }

    void Init(Shader* defaultShader, Shader* sbShader = nullptr) {
        activeShader = defaultShader;
        skyboxShader = sbShader;

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        initRenderData();
    }

    void SetIBLMaps(unsigned int irradiance, unsigned int prefilter, unsigned int brdf) {
        iblIrradiance = irradiance;
        iblPrefilter = prefilter;
        iblBrdf = brdf;
        useIBL = true;
    }

    void BeginScene(const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos) {
        sceneData.viewMatrix = view;
        sceneData.projectionMatrix = proj;
        sceneData.cameraPos = camPos;
        sceneData.lightPos = glm::vec3(2.0f, 4.0f, 3.0f);
        sceneData.lightColor = glm::vec3(1.0f);

        opaqueQueue.clear();
        transparentQueue.clear();
        pointLights.clear();
    }

    void SubmitDirectionalLight(const DirectionalLight& light) {
        sunLight = light;
    }

    void SubmitPointLight(const PointLightData& light) {
        if (pointLights.size() < 4) {
            pointLights.push_back(light);
        }
    }

    void Submit(const std::shared_ptr<Model>& model, const glm::mat4& transform) {
        for(size_t i = 0; i < model->GetMeshCount(); i++) {
            const Mesh& meshRef = model->GetMesh(i);
            Mesh* meshPtr = const_cast<Mesh*>(&meshRef); 
            Material* matPtr = meshPtr->GetMaterial().get();

            float dist = glm::length(sceneData.cameraPos - glm::vec3(transform[3]));
            opaqueQueue.emplace_back(meshPtr, matPtr, transform, dist);
        }
    }

    void SubmitMesh(const Mesh& mesh, const glm::mat4& transform) {
        Mesh* meshPtr = const_cast<Mesh*>(&mesh); 
        Material* matPtr = meshPtr->GetMaterial().get();

        float dist = glm::length(sceneData.cameraPos - glm::vec3(transform[3]));
        opaqueQueue.emplace_back(meshPtr, matPtr, transform, dist);
    }

    void EndScene() {
        // Ordenação
        std::sort(opaqueQueue.begin(), opaqueQueue.end(), 
            [](const RenderCommand& a, const RenderCommand& b) {
                return a.distanceToCamera < b.distanceToCamera;
            });

        // Configuração Global do Shader
        activeShader->Use();
        activeShader->SetMat4("view", glm::value_ptr(sceneData.viewMatrix));
        activeShader->SetMat4("projection", glm::value_ptr(sceneData.projectionMatrix));
        activeShader->SetVec3("viewPos", sceneData.cameraPos.x, sceneData.cameraPos.y, sceneData.cameraPos.z);
        activeShader->SetVec3("lightPos", sceneData.lightPos.x, sceneData.lightPos.y, sceneData.lightPos.z);
        activeShader->SetVec3("lightColor", sceneData.lightColor.x, sceneData.lightColor.y, sceneData.lightColor.z);

        // Envio de Luzes
        activeShader->SetVec3("dirLight.direction", sunLight.direction.x, sunLight.direction.y, sunLight.direction.z);
        activeShader->SetVec3("dirLight.color", sunLight.color.x, sunLight.color.y, sunLight.color.z);
        activeShader->SetFloat("dirLight.intensity", sunLight.intensity);

        activeShader->SetInt("numPointLights", (int)pointLights.size());
        
        for (size_t i = 0; i < pointLights.size(); i++) {
            std::string base = "pointLights[" + std::to_string(i) + "]";
            activeShader->SetVec3(base + ".position", pointLights[i].position.x, pointLights[i].position.y, pointLights[i].position.z);
            activeShader->SetVec3(base + ".color", pointLights[i].color.x, pointLights[i].color.y, pointLights[i].color.z);
            activeShader->SetFloat(base + ".intensity", pointLights[i].intensity);
            activeShader->SetFloat(base + ".radius", pointLights[i].radius);
        }

        if (useIBL) {
            activeShader->SetBool("useIBL", true);
            
            // Slots reservados para IBL (ex: 5, 6, 7)
            // Assumindo que materiais usam 0, 1, 2, 3, 4
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblIrradiance);
            activeShader->SetInt("irradianceMap", 5);

            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_CUBE_MAP, iblPrefilter);
            activeShader->SetInt("prefilterMap", 6);

            glActiveTexture(GL_TEXTURE7);
            glBindTexture(GL_TEXTURE_2D, iblBrdf);
            activeShader->SetInt("brdfLUT", 7);
        } else {
            activeShader->SetBool("useIBL", false);
        }

        // Render Loop
        for (const auto& cmd : opaqueQueue) {
            RenderMesh(cmd);
        }
    }

    /**
     * @brief Renderiza o skybox usando o gerenciador compartilhado
     * @param cubemapID ID do cubemap texture
     * @param view Matriz de view
     * @param proj Matriz de projeção
     */
    void DrawSkybox(unsigned int cubemapID, const glm::mat4& view, const glm::mat4& proj) {
        if (!skyboxShader || !skyboxManager.IsInitialized()) {
            return;
        }

        // Salvar e modificar estados OpenGL
        glDepthFunc(GL_LEQUAL);
        GLboolean cullFaceWasEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);
        
        // Configurar shader
        skyboxShader->Use();
        skyboxShader->SetMat4("view", glm::value_ptr(view));
        skyboxShader->SetMat4("projection", glm::value_ptr(proj));
        skyboxShader->SetInt("skybox", 0);

        // Bind cubemap
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapID);
        
        // Renderizar usando SkyboxManager
        skyboxManager.Render();

        // Restaurar estados
        glDepthFunc(GL_LESS);
        if (cullFaceWasEnabled) glEnable(GL_CULL_FACE);
    }

    void SetSkyboxShader(Shader* s) { 
        skyboxShader = s; 
    }

    void DrawScreenQuad(Shader& screenShader, unsigned int textureID) {
        glDisable(GL_DEPTH_TEST);
        
        screenShader.Use();
        screenShader.SetInt("screenTexture", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        
        glBindVertexArray(screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        
        glEnable(GL_DEPTH_TEST);
    }
    
    void DrawScreenQuad() {
        glDisable(GL_DEPTH_TEST);
        glBindVertexArray(screenQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

private:
    void RenderMesh(const RenderCommand& cmd) {
        if (cmd.material) {
            cmd.material->Apply(activeShader->GetProgramID());
            
            activeShader->SetBool("hasTextureDiffuse", cmd.material->HasTextureType(TextureType::DIFFUSE));
            activeShader->SetBool("hasTextureNormal", cmd.material->HasTextureType(TextureType::NORMAL));
            activeShader->SetBool("hasTextureMetallic", cmd.material->HasTextureType(TextureType::METALLIC));
            activeShader->SetBool("hasTextureRoughness", cmd.material->HasTextureType(TextureType::ROUGHNESS));
            activeShader->SetBool("hasTextureAO", cmd.material->HasTextureType(TextureType::AO));
            activeShader->SetBool("hasTextureEmission", cmd.material->HasTextureType(TextureType::EMISSION));
        }

        activeShader->SetMat4("model", glm::value_ptr(cmd.transform));

        glBindVertexArray(cmd.mesh->GetVAO());
        glDrawElements(GL_TRIANGLES, cmd.mesh->GetIndexCount(), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
};

#endif // RENDERER_HPP