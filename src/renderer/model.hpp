#ifndef MODEL_HPP
#define MODEL_HPP

#include "../rhi/rhi_device.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <glm/glm.hpp>


#include "material.hpp"
#include "mesh.hpp"
#include "texture.hpp"


#include <iostream>
#include <memory>
#include <string>
#include <vector>


class Model {
private:
  RHI::IDevice *device = nullptr;
  std::vector<Mesh> meshes;
  std::string directory;

  void loadModel(const std::string &path) {
    Assimp::Importer importer;

    const aiScene *scene = importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_CalcTangentSpace |
                  aiProcess_GenNormals | aiProcess_OptimizeMeshes |
                  aiProcess_FlipUVs | aiProcess_GenUVCoords);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
      std::cerr << "[Model] Assimp error: " << importer.GetErrorString()
                << std::endl;
      return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);

    std::cout << "[Model] Loaded: " << path << " (" << meshes.size()
              << " meshes)" << std::endl;
  }

  void processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
      aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
      meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
      processNode(node->mChildren[i], scene);
    }
  }

  Mesh processMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
      Vertex vertex;
      vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                                  mesh->mVertices[i].z);

      if (mesh->HasNormals()) {
        vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                                  mesh->mNormals[i].z);
      } else {
        vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
      }

      if (mesh->mTextureCoords[0]) {
        vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x,
                                     mesh->mTextureCoords[0][i].y);
      } else {
        vertex.TexCoords = glm::vec2(0.0f, 0.0f);
      }

      if (mesh->HasTangentsAndBitangents()) {
        vertex.Tangent = glm::normalize(glm::vec3(
            mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z));
        vertex.Bitangent = glm::normalize(glm::vec3(mesh->mBitangents[i].x,
                                                    mesh->mBitangents[i].y,
                                                    mesh->mBitangents[i].z));
      } else {
        glm::vec3 c1 = glm::cross(vertex.Normal, glm::vec3(0.0f, 0.0f, 1.0f));
        glm::vec3 c2 = glm::cross(vertex.Normal, glm::vec3(0.0f, 1.0f, 0.0f));
        vertex.Tangent =
            glm::normalize(glm::length(c1) > glm::length(c2) ? c1 : c2);
        vertex.Bitangent =
            glm::normalize(glm::cross(vertex.Normal, vertex.Tangent));
      }
      vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
      aiFace face = mesh->mFaces[i];
      for (unsigned int j = 0; j < face.mNumIndices; j++)
        indices.push_back(face.mIndices[j]);
    }

    auto material = std::make_shared<Material>(
        "Material_" + std::to_string(mesh->mMaterialIndex));
    if (mesh->mMaterialIndex >= 0) {
      aiMaterial *aiMat = scene->mMaterials[mesh->mMaterialIndex];
      loadMaterialProperties(material, aiMat, scene);
    }

    return Mesh(device, vertices, indices, material);
  }

  void loadMaterialProperties(std::shared_ptr<Material> material,
                              aiMaterial *aiMat, const aiScene *scene) {
    aiColor3D color(1.0f, 1.0f, 1.0f);
    float value;

    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
      material->SetAlbedo(glm::vec3(color.r, color.g, color.b));

    if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS)
      material->SetSpecular(glm::vec3(color.r, color.g, color.b));

    if (aiMat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
      material->SetShininess(value);
      float roughness = 1.0f - (sqrt(value) / sqrt(100.0f));
      material->SetRoughness(glm::clamp(roughness, 0.05f, 1.0f));
    }

    loadMaterialTextures(material, aiMat, aiTextureType_DIFFUSE,
                         TextureType::DIFFUSE, scene);
    loadMaterialTextures(material, aiMat, aiTextureType_BASE_COLOR,
                         TextureType::DIFFUSE, scene);
    loadMaterialTextures(material, aiMat, aiTextureType_NORMALS,
                         TextureType::NORMAL, scene);
    loadMaterialTextures(material, aiMat, aiTextureType_METALNESS,
                         TextureType::METALLIC, scene);
    loadMaterialTextures(material, aiMat, aiTextureType_DIFFUSE_ROUGHNESS,
                         TextureType::ROUGHNESS, scene);
    loadMaterialTextures(material, aiMat, aiTextureType_AMBIENT_OCCLUSION,
                         TextureType::AO, scene);
    loadMaterialTextures(material, aiMat, aiTextureType_EMISSIVE,
                         TextureType::EMISSION, scene);
  }

  void loadMaterialTextures(std::shared_ptr<Material> targetMat,
                            aiMaterial *mat, aiTextureType aiType,
                            TextureType texType, const aiScene *scene) {
    if (targetMat->HasTextureType(texType))
      return;

    for (unsigned int i = 0; i < mat->GetTextureCount(aiType); i++) {
      aiString str;
      mat->GetTexture(aiType, i, &str);
      std::string filename = std::string(str.C_Str());

      if (filename.length() > 0 && filename[0] == '*') {
        int textureIndex = std::stoi(filename.substr(1));
        if (textureIndex < (int)scene->mNumTextures) {
          aiTexture *aiTex = scene->mTextures[textureIndex];
          auto embeddedTex = std::make_shared<Texture>(device);
          int size = (aiTex->mHeight == 0) ? aiTex->mWidth
                                           : aiTex->mWidth * aiTex->mHeight * 4;
          TextureParams params;
          params.flipVertically = false;

          if (embeddedTex->LoadFromMemory(
                  reinterpret_cast<unsigned char *>(aiTex->pcData), size,
                  texType, params)) {
            targetMat->AddTexture(embeddedTex);
            return;
          }
        }
      } else {
        std::string fullPath = directory + '/' + filename;
        auto tex = TextureManager::GetInstance().LoadTexture(fullPath, texType);
        if (tex) {
          targetMat->AddTexture(tex);
        }
      }
    }
  }

public:
  Model(RHI::IDevice *dev, const std::string &path) : device(dev) {
    loadModel(path);
  }

  void Draw(RHI::IDevice *dev, RHI::ShaderHandle shader) {
    for (auto &mesh : meshes) {
      mesh.Draw(dev, shader);
    }
  }

  size_t GetMeshCount() const { return meshes.size(); }
  const Mesh &GetMesh(size_t index) const { return meshes[index]; }

  void SetMaterialAll(std::shared_ptr<Material> material) {
    for (auto &mesh : meshes) {
      mesh.SetMaterial(material);
    }
  }
};

#endif // MODEL_HPP