#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include <string>
#include "texture.hpp"

struct MaterialProperties {
    glm::vec3 albedo = glm::vec3(1.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float ao = 1.0f; // Ambient Occlusion
    
    glm::vec3 emission = glm::vec3(0.0f);
    float emissionStrength = 0.0f;
    
    // Propriedades Phong (legado)
    glm::vec3 ambient = glm::vec3(0.3f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(0.5f);
    float shininess = 32.0f;
};

class Material {
private:
    std::string name;
    MaterialProperties properties;
    std::vector<std::shared_ptr<Texture>> textures;

public:
    Material(const std::string& materialName = "Default") 
        : name(materialName) {}

    // Adicionar textura
    void AddTexture(std::shared_ptr<Texture> texture) {
        if (texture && texture->IsLoaded()) {
            textures.push_back(texture);
        }
    }

    // Carregar textura diretamente
    bool LoadTexture(const std::string& path, TextureType type,
                     const TextureParams& params = TextureParams()) {
        auto& manager = TextureManager::GetInstance();
        auto texture = manager.LoadTexture(path, type, params);
        
        if (texture) {
            AddTexture(texture);
            return true;
        }
        return false;
    }

    // Aplicar material ao shader
    void Apply(unsigned int shaderProgram) const {
        int diffuseNr = 1;
        int specularNr = 1;
        int normalNr = 1;
        int heightNr = 1;
        int emissionNr = 1;
        int metallicNr = 1;
        int roughnessNr = 1;
        int aoNr = 1;

        // Bind texturas
        for (size_t i = 0; i < textures.size(); i++) {
            textures[i]->Bind(i);

            std::string uniformName;
            TextureType type = textures[i]->GetType();

            switch (type) {
                case TextureType::DIFFUSE:
                    uniformName = "texture_diffuse" + std::to_string(diffuseNr++);
                    break;
                case TextureType::SPECULAR:
                    uniformName = "texture_specular" + std::to_string(specularNr++);
                    break;
                case TextureType::NORMAL:
                    uniformName = "texture_normal" + std::to_string(normalNr++);
                    break;
                case TextureType::HEIGHT:
                    uniformName = "texture_height" + std::to_string(heightNr++);
                    break;
                case TextureType::EMISSION:
                    uniformName = "texture_emission" + std::to_string(emissionNr++);
                    break;
                case TextureType::METALLIC:
                    uniformName = "texture_metallic" + std::to_string(metallicNr++);
                    break;
                case TextureType::ROUGHNESS:
                    uniformName = "texture_roughness" + std::to_string(roughnessNr++);
                    break;
                case TextureType::AO:
                    uniformName = "texture_ao" + std::to_string(aoNr++);
                    break;
            }

            glUniform1i(glGetUniformLocation(shaderProgram, uniformName.c_str()), i);
        }

        // Enviar propriedades do material
        SendProperties(shaderProgram);
    }

    // Enviar propriedades para o shader
    void SendProperties(unsigned int shaderProgram) const {
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.albedo"), 1, &properties.albedo[0]);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.metallic"), properties.metallic);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.roughness"), properties.roughness);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.ao"), properties.ao);
        
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.emission"), 1, &properties.emission[0]);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.emissionStrength"), properties.emissionStrength);
        
        // Phong properties
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.ambient"), 1, &properties.ambient[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.diffuse"), 1, &properties.diffuse[0]);
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.specular"), 1, &properties.specular[0]);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), properties.shininess);
    }

    // Getters e Setters
    void SetName(const std::string& n) { name = n; }
    const std::string& GetName() const { return name; }
    
    MaterialProperties& GetProperties() { return properties; }
    const MaterialProperties& GetProperties() const { return properties; }
    
    void SetAlbedo(const glm::vec3& albedo) { properties.albedo = albedo; }
    void SetMetallic(float metallic) { properties.metallic = metallic; }
    void SetRoughness(float roughness) { properties.roughness = roughness; }
    void SetAO(float ao) { properties.ao = ao; }
    void SetEmission(const glm::vec3& emission) { properties.emission = emission; }
    void SetEmissionStrength(float strength) { properties.emissionStrength = strength; }
    
    void SetAmbient(const glm::vec3& ambient) { properties.ambient = ambient; }
    void SetDiffuse(const glm::vec3& diffuse) { properties.diffuse = diffuse; }
    void SetSpecular(const glm::vec3& specular) { properties.specular = specular; }
    void SetShininess(float shininess) { properties.shininess = shininess; }

    size_t GetTextureCount() const { return textures.size(); }
    std::shared_ptr<Texture> GetTexture(size_t index) const {
        return index < textures.size() ? textures[index] : nullptr;
    }

    bool HasTextureType(TextureType type) const {
        for (const auto& tex : textures) {
            if (tex->GetType() == type) return true;
        }
        return false;
    }

    void Clear() {
        textures.clear();
    }
};

// Biblioteca de materiais pré-definidos
class MaterialLibrary {
public:
    static Material CreateGold() {
        Material mat("Gold");
        mat.SetAlbedo(glm::vec3(1.0f, 0.765557f, 0.336057f));
        mat.SetMetallic(1.0f);
        mat.SetRoughness(0.3f);
        mat.SetAO(1.0f);
        return mat;
    }

    static Material CreateSilver() {
        Material mat("Silver");
        mat.SetAlbedo(glm::vec3(0.972f, 0.960f, 0.915f));
        mat.SetMetallic(1.0f);
        mat.SetRoughness(0.2f);
        mat.SetAO(1.0f);
        return mat;
    }

    static Material CreateCopper() {
        Material mat("Copper");
        mat.SetAlbedo(glm::vec3(0.955f, 0.637f, 0.538f));
        mat.SetMetallic(1.0f);
        mat.SetRoughness(0.4f);
        mat.SetAO(1.0f);
        return mat;
    }

    static Material CreatePlastic() {
        Material mat("Plastic");
        mat.SetAlbedo(glm::vec3(1.0f, 0.0f, 0.0f));
        mat.SetMetallic(0.0f);
        mat.SetRoughness(0.6f);
        mat.SetAO(1.0f);
        return mat;
    }

    static Material CreateRubber() {
        Material mat("Rubber");
        mat.SetAlbedo(glm::vec3(0.2f, 0.2f, 0.2f));
        mat.SetMetallic(0.0f);
        mat.SetRoughness(0.9f);
        mat.SetAO(1.0f);
        return mat;
    }

    static Material CreateEmissive(const glm::vec3& color, float strength = 1.0f) {
        Material mat("Emissive");
        mat.SetAlbedo(color);
        mat.SetEmission(color);
        mat.SetEmissionStrength(strength);
        mat.SetMetallic(0.0f);
        mat.SetRoughness(1.0f);
        return mat;
    }

    static Material CreatePhong(const glm::vec3& diffuseColor) {
        Material mat("Phong");
        mat.SetDiffuse(diffuseColor);
        mat.SetAmbient(diffuseColor * 0.3f);
        mat.SetSpecular(glm::vec3(0.5f));
        mat.SetShininess(32.0f);
        return mat;
    }
};

#endif // MATERIAL_HPP