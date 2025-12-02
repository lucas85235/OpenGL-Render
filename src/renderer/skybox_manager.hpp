#ifndef SKYBOX_MANAGER_HPP
#define SKYBOX_MANAGER_HPP

#include <GL/glew.h>
#include <memory>
#include "mesh.hpp"

/**
 * @brief Gerenciador único para geometria do Skybox
 * 
 * Esta classe centraliza a criação e gerenciamento do cubo invertido
 * usado para renderização de skybox/environment mapping.
 */
class SkyboxManager {
private:
    std::unique_ptr<Mesh> skyboxMesh;
    bool initialized = false;

    /**
     * @brief Cria um cubo com faces invertidas (para renderização interna)
     * 
     * O cubo do skybox precisa ter winding order invertido porque
     * a câmera está DENTRO do cubo olhando para fora.
     */
    static Mesh CreateInvertedCube() {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        // Posições do cubo invertido (CCW quando visto de DENTRO)
        float cubeVertices[] = {
            // Face traseira (-Z) - invertida
            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            // Face frontal (+Z) - invertida
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            // Face esquerda (-X) - invertida
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            // Face direita (+X) - invertida
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,

            // Face inferior (-Y) - invertida
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,

            // Face superior (+Y) - invertida
            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f
        };

        // Converter para estrutura Vertex
        for (int i = 0; i < 36; ++i) {
            Vertex v;
            v.Position = glm::vec3(
                cubeVertices[i * 3 + 0],
                cubeVertices[i * 3 + 1],
                cubeVertices[i * 3 + 2]
            );
            
            // Normal aponta para dentro (inverso da posição normalizada)
            v.Normal = -glm::normalize(v.Position);
            
            // Coordenadas de textura (não usadas para cubemap, mas necessárias)
            v.TexCoords = glm::vec2(0.0f);
            
            // Tangente e bitangente (não usados, mas mantidos por consistência)
            v.Tangent = glm::vec3(0.0f);
            v.Bitangent = glm::vec3(0.0f);
            
            vertices.push_back(v);
        }

        // Índices sequenciais (já estão na ordem correta)
        for (unsigned int i = 0; i < 36; ++i) {
            indices.push_back(i);
        }

        return Mesh(vertices, indices, nullptr);
    }

public:
    SkyboxManager() = default;
    
    ~SkyboxManager() = default;

    /**
     * @brief Inicializa o mesh do skybox
     * @return true se inicializado com sucesso
     */
    bool Initialize() {
        if (initialized) {
            return true;
        }

        try {
            skyboxMesh = std::make_unique<Mesh>(CreateInvertedCube());
            initialized = true;
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Erro ao criar skybox mesh: " << e.what() << std::endl;
            return false;
        }
    }

    /**
     * @brief Obtém o VAO do skybox para renderização
     * @return VAO do mesh do skybox
     */
    unsigned int GetVAO() const {
        if (!initialized || !skyboxMesh) {
            std::cerr << "Skybox não inicializado!" << std::endl;
            return 0;
        }
        return skyboxMesh->GetVAO();
    }

    /**
     * @brief Obtém o número de vértices (sempre 36 para um cubo)
     * @return Número de vértices
     */
    unsigned int GetVertexCount() const {
        return 36;
    }

    /**
     * @brief Verifica se o skybox está inicializado
     * @return true se inicializado
     */
    bool IsInitialized() const {
        return initialized;
    }

    /**
     * @brief Renderiza o skybox (apenas geometria, sem shader setup)
     */
    void Render() const {
        if (!initialized || !skyboxMesh) {
            return;
        }

        glBindVertexArray(skyboxMesh->GetVAO());
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
    }

    // Prevenir cópia
    SkyboxManager(const SkyboxManager&) = delete;
    SkyboxManager& operator=(const SkyboxManager&) = delete;

    // Permitir movimentação
    SkyboxManager(SkyboxManager&&) noexcept = default;
    SkyboxManager& operator=(SkyboxManager&&) noexcept = default;
};

#endif // SKYBOX_MANAGER_HPP
