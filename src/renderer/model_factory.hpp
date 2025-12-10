#ifndef MODEL_FACTORY_HPP
#define MODEL_FACTORY_HPP

#include "mesh.hpp"
#include <cmath>
#include <memory>
#include <vector>

class ModelFactory {
public:
  static Mesh CreateSphere(RHI::IDevice *device, float radius = 1.0f,
                           int sectors = 36, int stacks = 18) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    float lengthInv = 1.0f / radius;
    float sectorStep = 2.0f * static_cast<float>(M_PI) / sectors;
    float stackStep = static_cast<float>(M_PI) / stacks;

    for (int i = 0; i <= stacks; ++i) {
      float stackAngle = static_cast<float>(M_PI) / 2.0f - i * stackStep;
      float xy = radius * cosf(stackAngle);
      float z = radius * sinf(stackAngle);

      for (int j = 0; j <= sectors; ++j) {
        float sectorAngle = j * sectorStep;
        float x = xy * cosf(sectorAngle);
        float y = xy * sinf(sectorAngle);

        Vertex v;
        v.Position = glm::vec3(x, y, z);
        v.Normal = glm::vec3(x * lengthInv, y * lengthInv, z * lengthInv);
        v.TexCoords = glm::vec2(static_cast<float>(j) / sectors,
                                static_cast<float>(i) / stacks);
        v.Tangent = glm::normalize(
            glm::vec3(-sinf(sectorAngle), cosf(sectorAngle), 0.0f));
        v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));
        vertices.push_back(v);
      }
    }

    for (int i = 0; i < stacks; ++i) {
      int k1 = i * (sectors + 1);
      int k2 = k1 + sectors + 1;
      for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
        if (i != 0) {
          indices.push_back(k1);
          indices.push_back(k1 + 1);
          indices.push_back(k2);
        }
        if (i != (stacks - 1)) {
          indices.push_back(k1 + 1);
          indices.push_back(k2 + 1);
          indices.push_back(k2);
        }
      }
    }

    return Mesh(device, vertices, indices, nullptr);
  }

  static Mesh CreateCube(RHI::IDevice *device, float size = 1.0f) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float half = size / 2.0f;

    // Front (+Z)
    vertices.push_back(
        {{-half, -half, half}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}});
    vertices.push_back(
        {{half, -half, half}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}});
    vertices.push_back(
        {{half, half, half}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}});
    vertices.push_back(
        {{-half, half, half}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}});

    // Back (-Z)
    vertices.push_back(
        {{half, -half, -half}, {0, 0, -1}, {0, 0}, {-1, 0, 0}, {0, 1, 0}});
    vertices.push_back(
        {{-half, -half, -half}, {0, 0, -1}, {1, 0}, {-1, 0, 0}, {0, 1, 0}});
    vertices.push_back(
        {{-half, half, -half}, {0, 0, -1}, {1, 1}, {-1, 0, 0}, {0, 1, 0}});
    vertices.push_back(
        {{half, half, -half}, {0, 0, -1}, {0, 1}, {-1, 0, 0}, {0, 1, 0}});

    // Top (+Y)
    vertices.push_back(
        {{-half, half, half}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}});
    vertices.push_back(
        {{half, half, half}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0, 1}});
    vertices.push_back(
        {{half, half, -half}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}});
    vertices.push_back(
        {{-half, half, -half}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0, 1}});

    // Bottom (-Y)
    vertices.push_back(
        {{-half, -half, -half}, {0, -1, 0}, {0, 0}, {1, 0, 0}, {0, 0, -1}});
    vertices.push_back(
        {{half, -half, -half}, {0, -1, 0}, {1, 0}, {1, 0, 0}, {0, 0, -1}});
    vertices.push_back(
        {{half, -half, half}, {0, -1, 0}, {1, 1}, {1, 0, 0}, {0, 0, -1}});
    vertices.push_back(
        {{-half, -half, half}, {0, -1, 0}, {0, 1}, {1, 0, 0}, {0, 0, -1}});

    // Right (+X)
    vertices.push_back(
        {{half, -half, half}, {1, 0, 0}, {0, 0}, {0, 0, -1}, {0, 1, 0}});
    vertices.push_back(
        {{half, -half, -half}, {1, 0, 0}, {1, 0}, {0, 0, -1}, {0, 1, 0}});
    vertices.push_back(
        {{half, half, -half}, {1, 0, 0}, {1, 1}, {0, 0, -1}, {0, 1, 0}});
    vertices.push_back(
        {{half, half, half}, {1, 0, 0}, {0, 1}, {0, 0, -1}, {0, 1, 0}});

    // Left (-X)
    vertices.push_back(
        {{-half, -half, -half}, {-1, 0, 0}, {0, 0}, {0, 0, 1}, {0, 1, 0}});
    vertices.push_back(
        {{-half, -half, half}, {-1, 0, 0}, {1, 0}, {0, 0, 1}, {0, 1, 0}});
    vertices.push_back(
        {{-half, half, half}, {-1, 0, 0}, {1, 1}, {0, 0, 1}, {0, 1, 0}});
    vertices.push_back(
        {{-half, half, -half}, {-1, 0, 0}, {0, 1}, {0, 0, 1}, {0, 1, 0}});

    for (int i = 0; i < 6; ++i) {
      int offset = i * 4;
      indices.push_back(offset + 0);
      indices.push_back(offset + 1);
      indices.push_back(offset + 2);
      indices.push_back(offset + 2);
      indices.push_back(offset + 3);
      indices.push_back(offset + 0);
    }

    return Mesh(device, vertices, indices, nullptr);
  }

  static Mesh CreatePlane(RHI::IDevice *device, float size = 20.0f) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float half = size / 2.0f;

    glm::vec3 n = {0, 1, 0}, t = {1, 0, 0}, b = {0, 0, 1};
    vertices = {{{-half, 0, -half}, n, {0, 0}, t, b},
                {{half, 0, -half}, n, {1, 0}, t, b},
                {{half, 0, half}, n, {1, 1}, t, b},
                {{-half, 0, half}, n, {0, 1}, t, b}};
    indices = {0, 2, 1, 2, 0, 3};

    return Mesh(device, vertices, indices, nullptr);
  }

  static Mesh CreateCylinder(RHI::IDevice *device, float radius = 0.5f,
                             float height = 2.0f, int sectors = 36) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float halfHeight = height / 2.0f;
    float sectorStep = 2.0f * static_cast<float>(M_PI) / sectors;

    // Body
    for (int i = 0; i <= sectors; ++i) {
      float angle = i * sectorStep;
      float x = radius * cosf(angle);
      float z = radius * sinf(angle);
      glm::vec3 n = glm::normalize(glm::vec3(x, 0, z));
      glm::vec3 t = glm::normalize(glm::vec3(-sinf(angle), 0, cosf(angle)));
      glm::vec3 b = {0, 1, 0};
      float u = static_cast<float>(i) / sectors;
      vertices.push_back({{x, -halfHeight, z}, n, {u, 0}, t, b});
      vertices.push_back({{x, halfHeight, z}, n, {u, 1}, t, b});
    }

    for (int i = 0; i < sectors; ++i) {
      int c = i * 2, nx = c + 2;
      indices.insert(indices.end(),
                     {static_cast<uint32_t>(c), static_cast<uint32_t>(nx),
                      static_cast<uint32_t>(c + 1),
                      static_cast<uint32_t>(c + 1), static_cast<uint32_t>(nx),
                      static_cast<uint32_t>(nx + 1)});
    }

    // Caps
    auto addCap = [&](float y, float ny) {
      int center = static_cast<int>(vertices.size());
      vertices.push_back(
          {{0, y, 0}, {0, ny, 0}, {0.5f, 0.5f}, {1, 0, 0}, {0, 0, 1}});
      for (int i = 0; i < sectors; ++i) {
        float a = i * sectorStep;
        vertices.push_back({{radius * cosf(a), y, radius * sinf(a)},
                            {0, ny, 0},
                            {0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a)},
                            {1, 0, 0},
                            {0, 0, 1}});
      }
      for (int i = 0; i < sectors; ++i) {
        int a = center + 1 + i, b = center + 1 + (i + 1) % sectors;
        if (ny > 0)
          indices.insert(indices.end(),
                         {static_cast<uint32_t>(center),
                          static_cast<uint32_t>(a), static_cast<uint32_t>(b)});
        else
          indices.insert(indices.end(),
                         {static_cast<uint32_t>(center),
                          static_cast<uint32_t>(b), static_cast<uint32_t>(a)});
      }
    };
    addCap(-halfHeight, -1);
    addCap(halfHeight, 1);

    return Mesh(device, vertices, indices, nullptr);
  }

  static Mesh CreateCone(RHI::IDevice *device, float radius = 0.5f,
                         float height = 2.0f, int sectors = 36) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float sectorStep = 2.0f * static_cast<float>(M_PI) / sectors;
    float slant = sqrtf(radius * radius + height * height);
    float cosSlant = height / slant, sinSlant = radius / slant;

    vertices.push_back(
        {{0, height, 0}, {0, cosSlant, 0}, {0.5f, 1}, {1, 0, 0}, {0, 0, 1}});
    for (int i = 0; i <= sectors; ++i) {
      float a = i * sectorStep;
      glm::vec3 n = glm::normalize(
          glm::vec3(cosf(a) * cosSlant, sinSlant, sinf(a) * cosSlant));
      vertices.push_back(
          {{radius * cosf(a), 0, radius * sinf(a)},
           n,
           {static_cast<float>(i) / sectors, 0},
           glm::normalize(glm::vec3(-sinf(a), 0, cosf(a))),
           glm::cross(n, glm::normalize(glm::vec3(-sinf(a), 0, cosf(a))))});
    }
    for (int i = 1; i <= sectors; ++i)
      indices.insert(indices.end(), {0u, static_cast<uint32_t>(i),
                                     static_cast<uint32_t>(i + 1)});

    int bc = static_cast<int>(vertices.size());
    vertices.push_back(
        {{0, 0, 0}, {0, -1, 0}, {0.5f, 0.5f}, {1, 0, 0}, {0, 0, 1}});
    for (int i = 0; i < sectors; ++i) {
      float a = i * sectorStep;
      vertices.push_back({{radius * cosf(a), 0, radius * sinf(a)},
                          {0, -1, 0},
                          {0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a)},
                          {1, 0, 0},
                          {0, 0, 1}});
    }
    for (int i = 0; i < sectors; ++i)
      indices.insert(indices.end(),
                     {static_cast<uint32_t>(bc),
                      static_cast<uint32_t>(bc + 1 + (i + 1) % sectors),
                      static_cast<uint32_t>(bc + 1 + i)});

    return Mesh(device, vertices, indices, nullptr);
  }

  static Mesh CreateTorus(RHI::IDevice *device, float majorR = 1.0f,
                          float minorR = 0.3f, int majS = 48, int minS = 24) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    float majStep = 2.0f * static_cast<float>(M_PI) / majS;
    float minStep = 2.0f * static_cast<float>(M_PI) / minS;

    for (int i = 0; i <= majS; ++i) {
      float u = i * majStep;
      for (int j = 0; j <= minS; ++j) {
        float v = j * minStep;
        float x = (majorR + minorR * cosf(v)) * cosf(u);
        float y = minorR * sinf(v);
        float z = (majorR + minorR * cosf(v)) * sinf(u);
        glm::vec3 n = glm::normalize(
            glm::vec3(cosf(v) * cosf(u), sinf(v), cosf(v) * sinf(u)));
        glm::vec3 t = glm::normalize(glm::vec3(-sinf(u), 0, cosf(u)));
        vertices.push_back(
            {{x, y, z},
             n,
             {static_cast<float>(i) / majS, static_cast<float>(j) / minS},
             t,
             glm::cross(n, t)});
      }
    }
    for (int i = 0; i < majS; ++i) {
      int i1 = i * (minS + 1), i2 = (i + 1) * (minS + 1);
      for (int j = 0; j < minS; ++j)
        indices.insert(indices.end(), {static_cast<uint32_t>(i1 + j),
                                       static_cast<uint32_t>(i2 + j),
                                       static_cast<uint32_t>(i1 + j + 1),
                                       static_cast<uint32_t>(i1 + j + 1),
                                       static_cast<uint32_t>(i2 + j),
                                       static_cast<uint32_t>(i2 + j + 1)});
    }
    return Mesh(device, vertices, indices, nullptr);
  }
};

#endif // MODEL_FACTORY_HPP