#ifndef MODEL_HPP
#define MODEL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Apenas inclusão do cabeçalho, sem implementação
#include "stb_image.h"

#include "mesh.hpp"
#include "material.hpp"
#include "texture.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <memory>
#include <map>

class Model {
private:
    std::vector<Mesh> meshes;
    std::string directory;

    void loadModel(std::string path) {
        Assimp::Importer importer;
        // Flags combinadas para melhor qualidade e PBR
        const aiScene *scene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_CalcTangentSpace |
            aiProcess_GenNormals |
            aiProcess_EmbedTextures |   // Importante para GLB
            aiProcess_OptimizeMeshes |  // Otimização extra
            aiProcess_OptimizeGraph
        );

        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            std::cerr << "Erro ao carregar modelo (Assimp): " 
                      << importer.GetErrorString() << std::endl;
            return;
        }

        directory = path.substr(0, path.find_last_of('/'));
        processNode(scene->mRootNode, scene);
        
        std::cout << "Modelo carregado: " << path << " (" << meshes.size() << " meshes)" << std::endl;
    }

    void processNode(aiNode *node, const aiScene *scene) {
        // Processar meshes do nó atual
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }
        // Processar filhos
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        // 1. Processar Vértices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            // Posição
            vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

            // Normal
            if(mesh->HasNormals()) {
                vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // UVs
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            // Tangente e Bitangente
            if(mesh->HasTangentsAndBitangents()) {
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            } else {
                // Valores padrão seguros se não houver tangentes
                vertex.Tangent = glm::vec3(1.0f, 0.0f, 0.0f);
                vertex.Bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // 2. Processar Índices
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // 3. Processar Material
        std::shared_ptr<Material> material = std::make_shared<Material>("Material_" + std::to_string(mesh->mMaterialIndex));

        if(mesh->mMaterialIndex >= 0) {
            aiMaterial *aiMat = scene->mMaterials[mesh->mMaterialIndex];
            loadMaterialProperties(material, aiMat, scene);
        }

        return Mesh(vertices, indices, material);
    }

    void loadMaterialProperties(std::shared_ptr<Material> material, aiMaterial *aiMat, const aiScene *scene) {
        aiColor3D color(1.0f, 1.0f, 1.0f);
        float value;

        // --- Propriedades Escalares ---
        if(aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
            material->SetAlbedo(glm::vec3(color.r, color.g, color.b));
            
        if(aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
            material->SetSpecular(glm::vec3(color.r, color.g, color.b));

        // Shininess para Roughness
        if(aiMat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
            material->SetShininess(value);
            float roughness = 1.0f - (sqrt(value) / sqrt(100.0f)); 
            material->SetRoughness(glm::clamp(roughness, 0.05f, 1.0f));
        }

        // --- Carregamento de Texturas (Padrão + PBR) ---
        // Diffuse / Albedo
        loadMaterialTextures(material, aiMat, aiTextureType_DIFFUSE, TextureType::DIFFUSE, scene);
        loadMaterialTextures(material, aiMat, aiTextureType_BASE_COLOR, TextureType::DIFFUSE, scene); // GLTF PBR

        // Specular
        loadMaterialTextures(material, aiMat, aiTextureType_SPECULAR, TextureType::SPECULAR, scene);

        // Normal Map (Tenta NORMALS e HEIGHT - OBJ costuma usar HEIGHT)
        loadMaterialTextures(material, aiMat, aiTextureType_NORMALS, TextureType::NORMAL, scene);
        loadMaterialTextures(material, aiMat, aiTextureType_HEIGHT, TextureType::NORMAL, scene);

        // PBR Maps (Metallic, Roughness, AO)
        loadMaterialTextures(material, aiMat, aiTextureType_METALNESS, TextureType::METALLIC, scene);
        loadMaterialTextures(material, aiMat, aiTextureType_DIFFUSE_ROUGHNESS, TextureType::ROUGHNESS, scene);
        loadMaterialTextures(material, aiMat, aiTextureType_AMBIENT_OCCLUSION, TextureType::AO, scene);
        loadMaterialTextures(material, aiMat, aiTextureType_LIGHTMAP, TextureType::AO, scene); // As vezes AO vem no Lightmap

        // Emission
        loadMaterialTextures(material, aiMat, aiTextureType_EMISSIVE, TextureType::EMISSION, scene);
    }

    void loadMaterialTextures(std::shared_ptr<Material> targetMat, aiMaterial *mat, 
                              aiTextureType aiType, TextureType texType, const aiScene* scene) {
        
        // Se já tiver carregado uma textura desse tipo para este material, evita duplicar (ex: normal map em height e normals)
        if (targetMat->HasTextureType(texType)) return;

        for(unsigned int i = 0; i < mat->GetTextureCount(aiType); i++) {
            aiString str;
            mat->GetTexture(aiType, i, &str);
            std::string filename = std::string(str.C_Str());
            
            // --- CASO 1: TEXTURA EMBUTIDA (GLB/FBX Embed) ---
            if (filename.length() > 0 && filename[0] == '*') {
                int textureIndex = std::stoi(filename.substr(1));
                if (textureIndex < (int)scene->mNumTextures) {
                    aiTexture* aiTex = scene->mTextures[textureIndex];
                    
                    // Criamos textura direta (sem Cache do Manager por enquanto para embutidas)
                    auto embeddedTex = std::make_shared<Texture>();
                    
                    int size = (aiTex->mHeight == 0) ? aiTex->mWidth : aiTex->mWidth * aiTex->mHeight * 4;
                    
                    TextureParams params;
                    // Normal maps geralmente não devem ser flipados verticalmente dependendo da origem
                    // Mas Assimp geralmente entrega correto se usarmos aiProcess_FlipUVs no loadModel
                    
                    bool loaded = embeddedTex->LoadFromMemory(
                        reinterpret_cast<unsigned char*>(aiTex->pcData),
                        size,
                        texType,
                        params
                    );

                    if (loaded) {
                        targetMat->AddTexture(embeddedTex);
                        // Para texturas embutidas, geralmente pegamos só a primeira de cada tipo
                        return; 
                    }
                }
            } 
            // --- CASO 2: ARQUIVO EM DISCO (OBJ, GLTF externo) ---
            else {
                std::string fullPath = directory + '/' + filename;
                
                // Usamos o Manager para evitar recarregar "madeira.jpg" se 10 objetos usam ela
                auto tex = TextureManager::GetInstance().LoadTexture(fullPath, texType);
                if (tex) {
                    targetMat->AddTexture(tex);
                }
            }
        }
    }

public:
    Model(const std::string &path) {
        loadModel(path);
    }

    void Draw(unsigned int shaderProgram) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram);
    }

    // Acessores úteis
    size_t GetMeshCount() const { return meshes.size(); }
    const Mesh& GetMesh(size_t index) const { return meshes[index]; }

    void SetMaterialAll(std::shared_ptr<Material> material) {
        for(auto& mesh : meshes) {
            mesh.SetMaterial(material);
        }
    }
};

#endif // MODEL_HPP