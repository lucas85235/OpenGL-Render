#ifndef PBR_UTILS_HPP
#define PBR_UTILS_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>

#include "shader.hpp"
#include "texture.hpp"
#include "skybox_manager.hpp"

namespace PBRUtils {

// Shader para converter 2D -> Cubo
const char* EquirectToCubeVertex = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 localPos;
uniform mat4 projection;
uniform mat4 view;
void main() {
    localPos = aPos;  
    gl_Position = projection * view * vec4(localPos, 1.0);
}
)";

const char* EquirectToCubeFragment = R"(
#version 330 core
out vec4 FragColor;
in vec3 localPos;
uniform sampler2D equirectangularMap;
const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= invAtan;
    uv += 0.5;
    return uv;
}

void main() {		
    vec2 uv = SampleSphericalMap(normalize(localPos));
    vec3 color = texture(equirectangularMap, uv).rgb;
    FragColor = vec4(color, 1.0);
}
)";

/**
 * @brief Classe para gerenciar Environment Maps (IBL)
 * 
 * Responsável por:
 * - Carregar HDR e converter para cubemap
 * - Gerar mapas de irradiância e pré-filtrados
 * - Fornecer texturas para IBL
 */
class EnvironmentMap {
private:
    const int CUBEMAP_SIZE = 1024;
    SkyboxManager skyboxManager; // Gerenciador compartilhado

public:
    unsigned int envCubemap;
    
    EnvironmentMap() : envCubemap(0) {}
    
    ~EnvironmentMap() {
        if (envCubemap) glDeleteTextures(1, &envCubemap);
    }
    
    /**
     * @brief Carrega HDR e converte para cubemap
     * @param path Caminho para o arquivo HDR
     */
    void LoadFromHDR(const std::string& path) {
        // 1. Inicializar geometria do skybox
        if (!skyboxManager.Initialize()) {
            std::cerr << "Falha ao inicializar SkyboxManager!" << std::endl;
            return;
        }

        // 2. Carregar HDR como textura 2D
        Texture hdrTexture;
        if (!hdrTexture.LoadHDR(path)) {
            std::cerr << "Falha ao carregar HDR: " << path << std::endl;
            return;
        }

        // 3. Setup do Framebuffer de Captura
        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CUBEMAP_SIZE, CUBEMAP_SIZE);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        // 4. Criar o Cubemap vazio
        glGenTextures(1, &envCubemap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                         CUBEMAP_SIZE, CUBEMAP_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // 5. Matrizes de Captura (6 direções)
        glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        glm::mat4 captureViews[] = {
           glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
           glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
           glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
           glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
        };

        // 6. Configurar Shader de Conversão
        Shader convertShader;
        convertShader.CompileFromSource(EquirectToCubeVertex, EquirectToCubeFragment);
        convertShader.Use();
        convertShader.SetInt("equirectangularMap", 0);
        convertShader.SetMat4("projection", glm::value_ptr(captureProjection));

        // 7. Salvar e desabilitar culling para captura
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        // 8. Loop de Renderização (6 faces do cubemap)
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture.GetID());

        glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        for (unsigned int i = 0; i < 6; ++i) {
            convertShader.SetMat4("view", glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Renderizar usando SkyboxManager
            skyboxManager.Render();
        }
        
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        
        // 9. Gerar mipmaps
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        
        // 10. Restaurar estado de culling
        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
        
        // 11. Limpeza
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
        
        std::cout << "✓ Environment Cubemap gerado com sucesso!" << std::endl;
        std::cout << "  - Resolução: " << CUBEMAP_SIZE << "x" << CUBEMAP_SIZE << " por face" << std::endl;
        std::cout << "  - Formato: RGB16F (HDR)" << std::endl;
        std::cout << "  - Mipmaps: Gerados" << std::endl;
    }
    
    /**
     * @brief Obtém o ID do cubemap para uso em shaders
     * @return ID da textura cubemap
     */
    unsigned int GetCubemapID() const {
        return envCubemap;
    }
    
    /**
     * @brief Verifica se o cubemap foi gerado
     * @return true se válido
     */
    bool IsValid() const {
        return envCubemap != 0;
    }

    // Prevenir cópia
    EnvironmentMap(const EnvironmentMap&) = delete;
    EnvironmentMap& operator=(const EnvironmentMap&) = delete;

    // Permitir movimentação
    EnvironmentMap(EnvironmentMap&& other) noexcept 
        : envCubemap(other.envCubemap) {
        other.envCubemap = 0;
    }

    EnvironmentMap& operator=(EnvironmentMap&& other) noexcept {
        if (this != &other) {
            if (envCubemap) glDeleteTextures(1, &envCubemap);
            envCubemap = other.envCubemap;
            other.envCubemap = 0;
        }
        return *this;
    }
};

} // namespace PBRUtils

#endif // PBR_UTILS_HPP