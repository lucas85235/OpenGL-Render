#ifndef MODEL_HPP
#define MODEL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "mesh.hpp"
#include <string>
#include <vector>
#include <iostream>

class Model {
private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;

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
        
        std::cout << "Modelo carregado com sucesso: " << path << std::endl;
        std::cout << "Total de meshes: " << meshes.size() << std::endl;
    }

    void processNode(aiNode *node, const aiScene *scene) {
        // Processar todos os meshes do nó
        for(unsigned int i = 0; i < node->mNumMeshes; i++) {
            aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
            meshes.push_back(processMesh(mesh, scene));
        }

        // Processar recursivamente os nós filhos
        for(unsigned int i = 0; i < node->mNumChildren; i++) {
            processNode(node->mChildren[i], scene);
        }
    }

    Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        std::vector<Texture> textures;

        // Processar vértices
        for(unsigned int i = 0; i < mesh->mNumVertices; i++) {
            Vertex vertex;
            
            // Posição
            vertex.Position = glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            );

            // Normal
            if(mesh->HasNormals()) {
                vertex.Normal = glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                );
            } else {
                vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Coordenadas de textura
            if(mesh->mTextureCoords[0]) {
                vertex.TexCoords = glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                );
            } else {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            vertices.push_back(vertex);
        }

        // Processar índices
        for(unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for(unsigned int j = 0; j < face.mNumIndices; j++)
                indices.push_back(face.mIndices[j]);
        }

        // Processar material
        if(mesh->mMaterialIndex >= 0) {
            aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

            // Texturas difusas
            std::vector<Texture> diffuseMaps = loadMaterialTextures(material,
                aiTextureType_DIFFUSE, "texture_diffuse", scene);
            textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

            // Texturas especulares
            std::vector<Texture> specularMaps = loadMaterialTextures(material,
                aiTextureType_SPECULAR, "texture_specular", scene);
            textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

            // Texturas normais
            std::vector<Texture> normalMaps = loadMaterialTextures(material,
                aiTextureType_HEIGHT, "texture_normal", scene);
            textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

            // Texturas de altura
            std::vector<Texture> heightMaps = loadMaterialTextures(material,
                aiTextureType_AMBIENT, "texture_height", scene);
            textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());
        }

        return Mesh(vertices, indices, textures);
    }

    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, 
                                               std::string typeName, const aiScene* scene) {
        std::vector<Texture> textures;
        
        for(unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
            aiString str;
            mat->GetTexture(type, i, &str);
            
            // Verificar se a textura já foi carregada
            bool skip = false;
            for(unsigned int j = 0; j < textures_loaded.size(); j++) {
                if(std::strcmp(textures_loaded[j].path.data(), str.C_Str()) == 0) {
                    textures.push_back(textures_loaded[j]);
                    skip = true;
                    std::cout << "TEXTURE: " << textures_loaded[j].id << std::endl;

                    break;
                }
            }
            
            if(!skip) {
                Texture texture;
                texture.id = TextureFromFile(str.C_Str(), directory, scene);
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
        
        return textures;
    }

    unsigned int TextureFromFile(const char *path, const std::string &directory, const aiScene* scene) {
        unsigned int textureID;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char *data = nullptr;

        std::string filename = std::string(path);
        
        // VERIFICAÇÃO DE TEXTURA EMBUTIDA (GLB/GLTF)
        if (filename.length() > 0 && filename[0] == '*') {
            // É um índice de textura embutida (ex: "*0")
            int textureIndex = std::stoi(filename.substr(1));
            
            if (textureIndex < scene->mNumTextures) {
                aiTexture* aiTex = scene->mTextures[textureIndex];
                
                if (aiTex->mHeight == 0) {
                    // Textura comprimida (png/jpg dentro do binário)
                    std::cout << "Carregando textura embutida comprimida (Index " << textureIndex << ")" << std::endl;
                    data = stbi_load_from_memory(
                        reinterpret_cast<unsigned char*>(aiTex->pcData),
                        aiTex->mWidth,
                        &width, &height, &nrComponents, 0
                    );
                } else {
                    // Textura bruta (ARGB8888)
                    std::cout << "Carregando textura embutida RAW (Index " << textureIndex << ")" << std::endl;
                    data = stbi_load_from_memory(
                        reinterpret_cast<unsigned char*>(aiTex->pcData),
                        aiTex->mWidth * aiTex->mHeight * 4, // Tamanho aproximado
                        &width, &height, &nrComponents, 0
                    );
                }
            }
        } else {
            // LÓGICA PADRÃO PARA ARQUIVOS NO DISCO
            filename = directory + '/' + filename;
            std::cout << "Tentando carregar do disco: " << filename << std::endl;
            data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
            // std::cout << "MODEL: " << filename << std::endl;
        }
        
        if (data) {
            GLenum format;
            if (nrComponents == 1)
                format = GL_RED;
            else if (nrComponents == 3)
                format = GL_RGB;
            else if (nrComponents == 4)
                format = GL_RGBA;

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // Parâmetros de wrapping/filter
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        } else {
            std::cerr << "Falha ao carregar textura: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

public:
    Model(const std::string &path) {
        loadModel(path);
    }

    void Draw(unsigned int shaderProgram) {
        for(unsigned int i = 0; i < meshes.size(); i++)
            meshes[i].Draw(shaderProgram);
    }

    // Prevenir cópia
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    // Permitir movimentação
    Model(Model&& other) noexcept
        : meshes(std::move(other.meshes)),
          directory(std::move(other.directory)),
          textures_loaded(std::move(other.textures_loaded)) {
    }

    Model& operator=(Model&& other) noexcept {
        if (this != &other) {
            meshes = std::move(other.meshes);
            directory = std::move(other.directory);
            textures_loaded = std::move(other.textures_loaded);
        }
        return *this;
    }
};

#endif // MODEL_HPP