#ifndef MODEL_HPP
#define MODEL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// Garante que a implementação do STB só seja definida uma vez
#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include "mesh.hpp"
#include "material.hpp"
#include "texture.hpp"

#include <string>
#include <vector>
#include <iostream>
#include <map>

class Model {
private:
    std::vector<Mesh> meshes;
    std::string directory;
    // Não precisamos mais de textures_loaded local aqui, pois o TextureManager cuida do cache de arquivos

    void loadModel(std::string path) {
        Assimp::Importer importer;
        const aiScene *scene = importer.ReadFile(path, 
            aiProcess_Triangulate | 
            aiProcess_FlipUVs | 
            aiProcess_CalcTangentSpace |
            aiProcess_GenNormals |
            aiProcess_EmbedTextures
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
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

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

            // TexCoords
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }
            
            // Tangent & Bitangent (Útil para Normal Mapping)
            if (mesh->HasTangentsAndBitangents()) {
                vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
                vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
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
        std::shared_ptr<Material> newMaterial = std::make_shared<Material>("AssimpMat_" + std::to_string(mesh->mMaterialIndex));

        if(mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            // --- Ler Propriedades Básicas (Cores, etc) ---
            aiColor3D color(0.f, 0.f, 0.f);
            float shininess;

            // Diffuse / Albedo
            if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
                newMaterial->SetAlbedo(glm::vec3(color.r, color.g, color.b));
                newMaterial->SetDiffuse(glm::vec3(color.r, color.g, color.b)); // Legado Phong
            }

            // Specular
            if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
                newMaterial->SetSpecular(glm::vec3(color.r, color.g, color.b));
            }

            // Shininess -> Roughness (Aproximação)
            if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
                newMaterial->SetShininess(shininess);
                // Converter shininess (0-1000) para roughness (1-0)
                float roughness = 1.0f - (sqrt(shininess) / sqrt(100.0f)); 
                newMaterial->SetRoughness(glm::clamp(roughness, 0.05f, 1.0f));
            }

            // --- Carregar Texturas ---
            loadMaterialTextures(newMaterial, material, aiTextureType_DIFFUSE, TextureType::DIFFUSE, scene);
            loadMaterialTextures(newMaterial, material, aiTextureType_SPECULAR, TextureType::SPECULAR, scene);
            loadMaterialTextures(newMaterial, material, aiTextureType_HEIGHT, TextureType::NORMAL, scene); // OBJ usa HEIGHT como Normal muitas vezes
            loadMaterialTextures(newMaterial, material, aiTextureType_NORMALS, TextureType::NORMAL, scene);
            loadMaterialTextures(newMaterial, material, aiTextureType_AMBIENT, TextureType::AO, scene);
            loadMaterialTextures(newMaterial, material, aiTextureType_EMISSIVE, TextureType::EMISSION, scene);
            
            // PBR (GLTF usa nomes específicos ou mapeamentos)
            // Assimp mais recente mapeia:
            // metallicRoughnessTexture -> aiTextureType_UNKNOWN ou AI_MATKEY_GLTF_PBRMETALLICROUGHNESS...
            // Por simplicidade, vamos manter o básico. Se precisar de PBR avançado do GLTF, precisa de checagem extra.
        }

        return Mesh(vertices, indices, newMaterial);
    }

    void loadMaterialTextures(std::shared_ptr<Material> targetMat, aiMaterial *mat, 
                              aiTextureType aiType, TextureType texType, const aiScene* scene) {
        
        for(unsigned int i = 0; i < mat->GetTextureCount(aiType); i++) {
            aiString str;
            mat->GetTexture(aiType, i, &str);
            std::string filename = std::string(str.C_Str());
            
            // VERIFICAÇÃO DE TEXTURA EMBUTIDA (GLB/GLTF)
            if (filename.length() > 0 && filename[0] == '*') {
                int textureIndex = std::stoi(filename.substr(1));
                if (textureIndex < scene->mNumTextures) {
                    aiTexture* aiTex = scene->mTextures[textureIndex];
                    
                    // Criar textura manualmente (bypass Manager para embutidas por enquanto)
                    auto embeddedTex = std::make_shared<Texture>();
                    
                    int size = (aiTex->mHeight == 0) ? aiTex->mWidth : aiTex->mWidth * aiTex->mHeight * 4;
                    
                    bool loaded = embeddedTex->LoadFromMemory(
                        reinterpret_cast<unsigned char*>(aiTex->pcData),
                        size,
                        texType
                    );

                    if (loaded) {
                        targetMat->AddTexture(embeddedTex);
                    }
                }
            } 
            else {
                // TEXTURA EM ARQUIVO
                std::string fullPath = directory + '/' + filename;
                
                // Usar TextureManager
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
};

#endif // MODEL_HPP