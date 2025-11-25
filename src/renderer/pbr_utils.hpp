#ifndef PBR_UTILS_HPP
#define PBR_UTILS_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <iostream>
#include <cmath>
#include <algorithm>

#include "shader.hpp"
#include "texture.hpp"
#include "skybox_manager.hpp"
#include "custom_shaders.hpp" // Ensure access to Prefilter/Irradiance shader sources

namespace PBRUtils {

// ----------------------------------------------------------------------------
// Shaders for Equirectangular to Cubemap Conversion
// ----------------------------------------------------------------------------

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

// ----------------------------------------------------------------------------
// Shaders for BRDF LUT Generation
// ----------------------------------------------------------------------------

const char* BrdfVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;
out vec2 TexCoords;
void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}
)";

const char* BrdfFragmentShader = R"(
#version 330 core
out vec2 FragColor;
in vec2 TexCoords;
const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness*roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta*cosTheta);
    
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    
    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec2 IntegrateBRDF(float NdotV, float roughness) {
    vec3 V;
    V.x = sqrt(1.0 - NdotV*NdotV);
    V.y = 0.0;
    V.z = NdotV;
    float A = 0.0;
    float B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);
    const uint SAMPLE_COUNT = 1024u;
    for(uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H  = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        if(NdotL > 0.0) {
            float G = GeometrySmith(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    return vec2(A / float(SAMPLE_COUNT), B / float(SAMPLE_COUNT));
}

void main() {
    vec2 integratedBRDF = IntegrateBRDF(TexCoords.x, TexCoords.y);
    FragColor = integratedBRDF;
}
)";

/**
 * @brief Manages PBR IBL (Image Based Lighting) Map Generation.
 * * Handles loading HDR equirectangular maps and converting them into:
 * 1. Environment Cubemap
 * 2. Irradiance Map (Diffuse)
 * 3. Prefilter Map (Specular)
 * 4. BRDF Look-Up Table (Specular Integration)
 */
class EnvironmentMap {
private:
    const int CUBEMAP_SIZE = 1024;
    const int IRRADIANCE_SIZE = 32;
    const int PREFILTER_SIZE = 128;

    SkyboxManager skyboxManager;

    // Setup capture view matrices for the 6 cubemap faces
    void SetupCaptureMatrices(glm::mat4* views, glm::mat4& proj) {
        proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
        views[0] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
        views[1] = glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
        views[2] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
        views[3] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
        views[4] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
        views[5] = glm::lookAt(glm::vec3(0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
    }

public:
    unsigned int envCubemap;
    unsigned int irradianceMap;
    unsigned int prefilterMap;
    unsigned int brdfLUTTexture;
    
    EnvironmentMap() : envCubemap(0), irradianceMap(0), prefilterMap(0), brdfLUTTexture(0) {}

    ~EnvironmentMap() {
        if (envCubemap) glDeleteTextures(1, &envCubemap);
        if (irradianceMap) glDeleteTextures(1, &irradianceMap);
        if (prefilterMap) glDeleteTextures(1, &prefilterMap);
        if (brdfLUTTexture) glDeleteTextures(1, &brdfLUTTexture);
    }
    
    /**
     * @brief Loads HDR image and generates all necessary IBL maps.
     */
    void LoadFromHDR(const std::string& path) {
        if (!skyboxManager.Initialize()) {
            std::cerr << "[IBL] Failed to initialize SkyboxManager!" << std::endl;
            return;
        }

        Texture hdrTexture;
        if (!hdrTexture.LoadHDR(path)) {
            std::cerr << "[IBL] Failed to load HDR: " << path << std::endl;
            return;
        }

        // Setup Capture Framebuffer
        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);

        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, CUBEMAP_SIZE, CUBEMAP_SIZE);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

        // Create and configure environment cubemap
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
        
        // Convert HDR (Equirectangular) to Cubemap
        Shader convertShader;
        convertShader.CompileFromSource(EquirectToCubeVertex, EquirectToCubeFragment);
        convertShader.Use();
        convertShader.SetInt("equirectangularMap", 0);
        
        glm::mat4 captureProjection;
        glm::mat4 captureViews[6];
        SetupCaptureMatrices(captureViews, captureProjection);
        convertShader.SetMat4("projection", glm::value_ptr(captureProjection));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTexture.GetID());

        glViewport(0, 0, CUBEMAP_SIZE, CUBEMAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        // Disable culling to render inside the cube
        GLboolean cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        for (unsigned int i = 0; i < 6; ++i) {
            convertShader.SetMat4("view", glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            skyboxManager.Render();
        }
        
        // Generate Mipmaps for the environment map (needed for correct sampling)
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        
        // Fix: Clamp mipmap levels to avoid sampling artifacts at high roughness
        int maxMipLevels = std::floor(std::log2(std::max(CUBEMAP_SIZE, CUBEMAP_SIZE))) + 1;
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxMipLevels - 1);

        if (cullFaceEnabled) glEnable(GL_CULL_FACE);
        
        // Cleanup buffers
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
        
        // Generate IBL maps
        GenerateIrradianceMap();
        GeneratePrefilterMap();
        GenerateBRDFLUT();

        std::cout << "[IBL] Environment maps generated successfully." << std::endl;
    }
    
    void GenerateIrradianceMap() {
        glGenTextures(1, &irradianceMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 
                        IRRADIANCE_SIZE, IRRADIANCE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Fix: Irradiance map is diffuse and doesn't need mipmaps
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, 0);

        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRRADIANCE_SIZE, IRRADIANCE_SIZE);

        // Assuming IrradianceConvolutionFragment is available in CustomShaders
        Shader irradianceShader;
        irradianceShader.CompileFromSource(EquirectToCubeVertex, CustomShaders::IrradianceConvolutionFragment);

        glm::mat4 captureProjection;
        glm::mat4 captureViews[6];
        SetupCaptureMatrices(captureViews, captureProjection);

        irradianceShader.Use();
        irradianceShader.SetInt("environmentMap", 0);
        irradianceShader.SetMat4("projection", glm::value_ptr(captureProjection));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        glViewport(0, 0, IRRADIANCE_SIZE, IRRADIANCE_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        for (unsigned int i = 0; i < 6; ++i) {
            irradianceShader.SetMat4("view", glm::value_ptr(captureViews[i]));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            skyboxManager.Render();
        }

        if (cullEnabled) glEnable(GL_CULL_FACE);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
    }

    void GeneratePrefilterMap() {
        glGenTextures(1, &prefilterMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
        
        // Pre-allocate mipmap levels
        unsigned int maxMipLevels = 5;
        for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth  = PREFILTER_SIZE * std::pow(0.5, mip);
            unsigned int mipHeight = PREFILTER_SIZE * std::pow(0.5, mip);
            for (unsigned int i = 0; i < 6; ++i) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mip, GL_RGB16F, 
                            mipWidth, mipHeight, 0, GL_RGB, GL_FLOAT, nullptr);
            }
        }
        
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        // Fix: Ensure only generated mip levels are accessed
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL, maxMipLevels - 1);
        
        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

        Shader prefilterShader;
        prefilterShader.CompileFromSource(EquirectToCubeVertex, CustomShaders::PrefilterFragment);

        glm::mat4 captureProjection;
        glm::mat4 captureViews[6];
        SetupCaptureMatrices(captureViews, captureProjection);

        prefilterShader.Use();
        prefilterShader.SetInt("environmentMap", 0);
        prefilterShader.SetMat4("projection", glm::value_ptr(captureProjection));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

        GLboolean cullEnabled = glIsEnabled(GL_CULL_FACE);
        glDisable(GL_CULL_FACE);

        for (unsigned int mip = 0; mip < maxMipLevels; ++mip) {
            unsigned int mipWidth  = PREFILTER_SIZE * std::pow(0.5, mip);
            unsigned int mipHeight = PREFILTER_SIZE * std::pow(0.5, mip);
            
            glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
            glViewport(0, 0, mipWidth, mipHeight);

            float roughness = (float)mip / (float)(maxMipLevels - 1);
            prefilterShader.SetFloat("roughness", roughness);
            
            for (unsigned int i = 0; i < 6; ++i) {
                prefilterShader.SetMat4("view", glm::value_ptr(captureViews[i]));
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
                                       GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                skyboxManager.Render();
            }
        }

        if (cullEnabled) glEnable(GL_CULL_FACE);
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
    }

    void GenerateBRDFLUT() {
        glGenTextures(1, &brdfLUTTexture);
        glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
        
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        unsigned int captureFBO, captureRBO;
        glGenFramebuffers(1, &captureFBO);
        glGenRenderbuffers(1, &captureRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

        Shader brdfShader;
        brdfShader.CompileFromSource(BrdfVertexShader, BrdfFragmentShader);
        brdfShader.Use();

        glViewport(0, 0, 512, 512);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render simple quad for BRDF LUT
        unsigned int quadVAO = 0, quadVBO;
        float quadVertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glBindVertexArray(0);
        glDeleteVertexArrays(1, &quadVAO);
        glDeleteBuffers(1, &quadVBO);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &captureFBO);
        glDeleteRenderbuffers(1, &captureRBO);
    }

    unsigned int GetCubemapID() const { return envCubemap; }
    unsigned int GetIrradianceMapID() const { return irradianceMap; }
    unsigned int GetPrefilterMapID() const { return prefilterMap; }
    unsigned int GetBrdfLUTID() const { return brdfLUTTexture; }

    bool IsValid() const { return envCubemap != 0; }

    // Delete copy constructor and assignment operator
    EnvironmentMap(const EnvironmentMap&) = delete;
    EnvironmentMap& operator=(const EnvironmentMap&) = delete;

    // Allow move semantics
    EnvironmentMap(EnvironmentMap&& other) noexcept 
        : envCubemap(other.envCubemap), irradianceMap(other.irradianceMap), 
          prefilterMap(other.prefilterMap), brdfLUTTexture(other.brdfLUTTexture) {
        other.envCubemap = 0;
        other.irradianceMap = 0;
        other.prefilterMap = 0;
        other.brdfLUTTexture = 0;
    }

    EnvironmentMap& operator=(EnvironmentMap&& other) noexcept {
        if (this != &other) {
            if (envCubemap) glDeleteTextures(1, &envCubemap);
            if (irradianceMap) glDeleteTextures(1, &irradianceMap);
            if (prefilterMap) glDeleteTextures(1, &prefilterMap);
            if (brdfLUTTexture) glDeleteTextures(1, &brdfLUTTexture);
            
            envCubemap = other.envCubemap;
            irradianceMap = other.irradianceMap;
            prefilterMap = other.prefilterMap;
            brdfLUTTexture = other.brdfLUTTexture;
            
            other.envCubemap = 0;
            other.irradianceMap = 0;
            other.prefilterMap = 0;
            other.brdfLUTTexture = 0;
        }
        return *this;
    }
};

} // namespace PBRUtils

#endif // PBR_UTILS_HPP