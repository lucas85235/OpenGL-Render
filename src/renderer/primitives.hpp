#ifndef PRIMITIVES_HPP
#define PRIMITIVES_HPP

#include "../rhi/rhi_device.h"
#include "mesh.hpp"
#include <cmath>
#include <vector>

namespace Primitives {

inline Mesh CreateQuad(RHI::IDevice *device, float width = 1.0f,
                       float height = 1.0f) {
  float hw = width / 2.0f;
  float hh = height / 2.0f;

  std::vector<Vertex> vertices = {
      {{-hw, -hh, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
      {{hw, -hh, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
      {{hw, hh, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
      {{-hw, hh, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}}};

  std::vector<unsigned int> indices = {0, 1, 2, 2, 3, 0};
  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreatePlane(RHI::IDevice *device, float size = 10.0f,
                        int subdivisions = 1) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  float step = size / subdivisions;
  float half = size / 2.0f;
  float uvStep = 1.0f / subdivisions;

  for (int z = 0; z <= subdivisions; ++z) {
    for (int x = 0; x <= subdivisions; ++x) {
      float px = -half + x * step;
      float pz = -half + z * step;
      float u = x * uvStep;
      float v = z * uvStep;
      vertices.push_back({{px, 0.0f, pz}, {0.0f, 1.0f, 0.0f}, {u, v}});
    }
  }

  for (int z = 0; z < subdivisions; ++z) {
    for (int x = 0; x < subdivisions; ++x) {
      int tl = z * (subdivisions + 1) + x;
      int tr = tl + 1;
      int bl = tl + subdivisions + 1;
      int br = bl + 1;
      indices.insert(indices.end(), {(unsigned)tl, (unsigned)bl, (unsigned)tr,
                                     (unsigned)tr, (unsigned)bl, (unsigned)br});
    }
  }

  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreateCube(RHI::IDevice *device, float size = 1.0f) {
  float h = size / 2.0f;

  std::vector<Vertex> vertices = {
      // Front
      {{-h, -h, h}, {0, 0, 1}, {0, 0}},
      {{h, -h, h}, {0, 0, 1}, {1, 0}},
      {{h, h, h}, {0, 0, 1}, {1, 1}},
      {{-h, h, h}, {0, 0, 1}, {0, 1}},
      // Back
      {{h, -h, -h}, {0, 0, -1}, {0, 0}},
      {{-h, -h, -h}, {0, 0, -1}, {1, 0}},
      {{-h, h, -h}, {0, 0, -1}, {1, 1}},
      {{h, h, -h}, {0, 0, -1}, {0, 1}},
      // Top
      {{-h, h, h}, {0, 1, 0}, {0, 0}},
      {{h, h, h}, {0, 1, 0}, {1, 0}},
      {{h, h, -h}, {0, 1, 0}, {1, 1}},
      {{-h, h, -h}, {0, 1, 0}, {0, 1}},
      // Bottom
      {{-h, -h, -h}, {0, -1, 0}, {0, 0}},
      {{h, -h, -h}, {0, -1, 0}, {1, 0}},
      {{h, -h, h}, {0, -1, 0}, {1, 1}},
      {{-h, -h, h}, {0, -1, 0}, {0, 1}},
      // Right
      {{h, -h, h}, {1, 0, 0}, {0, 0}},
      {{h, -h, -h}, {1, 0, 0}, {1, 0}},
      {{h, h, -h}, {1, 0, 0}, {1, 1}},
      {{h, h, h}, {1, 0, 0}, {0, 1}},
      // Left
      {{-h, -h, -h}, {-1, 0, 0}, {0, 0}},
      {{-h, -h, h}, {-1, 0, 0}, {1, 0}},
      {{-h, h, h}, {-1, 0, 0}, {1, 1}},
      {{-h, h, -h}, {-1, 0, 0}, {0, 1}},
  };

  std::vector<unsigned int> indices;
  for (unsigned int face = 0; face < 6; ++face) {
    unsigned int base = face * 4;
    indices.insert(indices.end(),
                   {base, base + 1, base + 2, base + 2, base + 3, base});
  }

  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreateSphere(RHI::IDevice *device, float radius = 1.0f,
                         int sectors = 36, int stacks = 18) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  float sectorStep = 2.0f * M_PI / sectors;
  float stackStep = M_PI / stacks;

  for (int i = 0; i <= stacks; ++i) {
    float stackAngle = M_PI / 2.0f - i * stackStep;
    float xy = radius * cosf(stackAngle);
    float z = radius * sinf(stackAngle);

    for (int j = 0; j <= sectors; ++j) {
      float sectorAngle = j * sectorStep;
      float x = xy * cosf(sectorAngle);
      float y = xy * sinf(sectorAngle);

      glm::vec3 pos(x, z, y);
      glm::vec3 normal = glm::normalize(pos);
      glm::vec2 uv((float)j / sectors, (float)i / stacks);

      vertices.push_back({pos, normal, uv});
    }
  }

  for (int i = 0; i < stacks; ++i) {
    int k1 = i * (sectors + 1);
    int k2 = k1 + sectors + 1;
    for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
      if (i != 0) {
        indices.insert(indices.end(),
                       {(unsigned)k1, (unsigned)k2, (unsigned)(k1 + 1)});
      }
      if (i != (stacks - 1)) {
        indices.insert(indices.end(),
                       {(unsigned)(k1 + 1), (unsigned)k2, (unsigned)(k2 + 1)});
      }
    }
  }

  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreateCylinder(RHI::IDevice *device, float radius = 0.5f,
                           float height = 2.0f, int sectors = 36) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  float halfH = height / 2.0f;
  float sectorStep = 2.0f * M_PI / sectors;

  // Side vertices
  for (int i = 0; i <= sectors; ++i) {
    float angle = i * sectorStep;
    float x = radius * cosf(angle);
    float z = radius * sinf(angle);
    glm::vec3 normal(cosf(angle), 0.0f, sinf(angle));
    float u = (float)i / sectors;

    vertices.push_back({{x, -halfH, z}, normal, {u, 0.0f}});
    vertices.push_back({{x, halfH, z}, normal, {u, 1.0f}});
  }

  // Side indices
  for (int i = 0; i < sectors; ++i) {
    int k1 = i * 2;
    int k2 = k1 + 2;
    indices.insert(indices.end(),
                   {(unsigned)k1, (unsigned)k2, (unsigned)(k1 + 1),
                    (unsigned)(k1 + 1), (unsigned)k2, (unsigned)(k2 + 1)});
  }

  // Top cap center
  unsigned int topCenter = vertices.size();
  vertices.push_back({{0, halfH, 0}, {0, 1, 0}, {0.5f, 0.5f}});

  for (int i = 0; i <= sectors; ++i) {
    float angle = i * sectorStep;
    float x = radius * cosf(angle);
    float z = radius * sinf(angle);
    vertices.push_back(
        {{x, halfH, z},
         {0, 1, 0},
         {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)}});
  }

  for (int i = 0; i < sectors; ++i) {
    indices.insert(indices.end(), {topCenter, topCenter + 1 + (unsigned)i,
                                   topCenter + 2 + (unsigned)i});
  }

  // Bottom cap center
  unsigned int botCenter = vertices.size();
  vertices.push_back({{0, -halfH, 0}, {0, -1, 0}, {0.5f, 0.5f}});

  for (int i = 0; i <= sectors; ++i) {
    float angle = i * sectorStep;
    float x = radius * cosf(angle);
    float z = radius * sinf(angle);
    vertices.push_back(
        {{x, -halfH, z},
         {0, -1, 0},
         {0.5f + 0.5f * cosf(angle), 0.5f - 0.5f * sinf(angle)}});
  }

  for (int i = 0; i < sectors; ++i) {
    indices.insert(indices.end(), {botCenter, botCenter + 2 + (unsigned)i,
                                   botCenter + 1 + (unsigned)i});
  }

  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreateCone(RHI::IDevice *device, float radius = 0.5f,
                       float height = 2.0f, int sectors = 36) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  float halfH = height / 2.0f;
  float sectorStep = 2.0f * M_PI / sectors;
  float slopeAngle = atanf(radius / height);
  float ny = sinf(slopeAngle);
  float nxz = cosf(slopeAngle);

  // Apex
  unsigned int apex = 0;
  vertices.push_back({{0, halfH, 0}, {0, 1, 0}, {0.5f, 1.0f}});

  // Base ring
  for (int i = 0; i <= sectors; ++i) {
    float angle = i * sectorStep;
    float x = radius * cosf(angle);
    float z = radius * sinf(angle);
    glm::vec3 normal(nxz * cosf(angle), ny, nxz * sinf(angle));
    vertices.push_back({{x, -halfH, z}, normal, {(float)i / sectors, 0.0f}});
  }

  // Side faces
  for (int i = 0; i < sectors; ++i) {
    indices.insert(indices.end(), {apex, (unsigned)(1 + i), (unsigned)(2 + i)});
  }

  // Base cap
  unsigned int baseCenter = vertices.size();
  vertices.push_back({{0, -halfH, 0}, {0, -1, 0}, {0.5f, 0.5f}});

  for (int i = 0; i <= sectors; ++i) {
    float angle = i * sectorStep;
    float x = radius * cosf(angle);
    float z = radius * sinf(angle);
    vertices.push_back(
        {{x, -halfH, z},
         {0, -1, 0},
         {0.5f + 0.5f * cosf(angle), 0.5f - 0.5f * sinf(angle)}});
  }

  for (int i = 0; i < sectors; ++i) {
    indices.insert(indices.end(), {baseCenter, baseCenter + 2 + (unsigned)i,
                                   baseCenter + 1 + (unsigned)i});
  }

  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreateTorus(RHI::IDevice *device, float majorR = 1.0f,
                        float minorR = 0.3f, int majorSegments = 48,
                        int minorSegments = 24) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  for (int i = 0; i <= majorSegments; ++i) {
    float theta = i * 2.0f * M_PI / majorSegments;
    float cosT = cosf(theta);
    float sinT = sinf(theta);

    for (int j = 0; j <= minorSegments; ++j) {
      float phi = j * 2.0f * M_PI / minorSegments;
      float cosP = cosf(phi);
      float sinP = sinf(phi);

      glm::vec3 pos((majorR + minorR * cosP) * cosT, minorR * sinP,
                    (majorR + minorR * cosP) * sinT);

      glm::vec3 normal(cosP * cosT, sinP, cosP * sinT);
      glm::vec2 uv((float)i / majorSegments, (float)j / minorSegments);

      vertices.push_back({pos, normal, uv});
    }
  }

  for (int i = 0; i < majorSegments; ++i) {
    for (int j = 0; j < minorSegments; ++j) {
      int curr = i * (minorSegments + 1) + j;
      int next = curr + minorSegments + 1;

      indices.insert(indices.end(), {(unsigned)curr, (unsigned)next,
                                     (unsigned)(curr + 1), (unsigned)(curr + 1),
                                     (unsigned)next, (unsigned)(next + 1)});
    }
  }

  return Mesh(device, vertices, indices, nullptr);
}

inline Mesh CreateCapsule(RHI::IDevice *device, float radius = 0.5f,
                          float height = 2.0f, int sectors = 36,
                          int stacks = 8) {
  std::vector<Vertex> vertices;
  std::vector<unsigned int> indices;

  float halfHeight = (height - 2.0f * radius) / 2.0f;
  if (halfHeight < 0)
    halfHeight = 0;

  float sectorStep = 2.0f * M_PI / sectors;
  float stackStep = M_PI / 2.0f / stacks;

  // Top hemisphere
  for (int i = 0; i <= stacks; ++i) {
    float stackAngle = M_PI / 2.0f - i * stackStep;
    float xy = radius * cosf(stackAngle);
    float z = radius * sinf(stackAngle) + halfHeight;

    for (int j = 0; j <= sectors; ++j) {
      float sectorAngle = j * sectorStep;
      float x = xy * cosf(sectorAngle);
      float y = xy * sinf(sectorAngle);

      glm::vec3 pos(x, z, y);
      glm::vec3 normal(cosf(stackAngle) * cosf(sectorAngle), sinf(stackAngle),
                       cosf(stackAngle) * sinf(sectorAngle));
      glm::vec2 uv((float)j / sectors,
                   0.5f + 0.25f * (1.0f - (float)i / stacks));

      vertices.push_back({pos, normal, uv});
    }
  }

  // Cylinder part
  for (int i = 0; i <= 1; ++i) {
    float y = halfHeight - i * 2.0f * halfHeight;
    for (int j = 0; j <= sectors; ++j) {
      float angle = j * sectorStep;
      float x = radius * cosf(angle);
      float z = radius * sinf(angle);

      glm::vec3 normal(cosf(angle), 0.0f, sinf(angle));
      glm::vec2 uv((float)j / sectors, 0.5f - 0.25f * i);

      vertices.push_back({{x, y, z}, normal, uv});
    }
  }

  // Bottom hemisphere
  for (int i = 0; i <= stacks; ++i) {
    float stackAngle = -i * stackStep;
    float xy = radius * cosf(stackAngle);
    float z = radius * sinf(stackAngle) - halfHeight;

    for (int j = 0; j <= sectors; ++j) {
      float sectorAngle = j * sectorStep;
      float x = xy * cosf(sectorAngle);
      float y = xy * sinf(sectorAngle);

      glm::vec3 pos(x, z, y);
      glm::vec3 normal(cosf(stackAngle) * cosf(sectorAngle), sinf(stackAngle),
                       cosf(stackAngle) * sinf(sectorAngle));
      glm::vec2 uv((float)j / sectors, 0.25f * (float)i / stacks);

      vertices.push_back({pos, normal, uv});
    }
  }

  // Generate indices for all parts
  int vertsPerRing = sectors + 1;
  int totalRings = (stacks + 1) + 2 + (stacks + 1);

  for (int ring = 0; ring < totalRings - 1; ++ring) {
    for (int s = 0; s < sectors; ++s) {
      int curr = ring * vertsPerRing + s;
      int next = curr + vertsPerRing;
      indices.insert(indices.end(), {(unsigned)curr, (unsigned)next,
                                     (unsigned)(curr + 1), (unsigned)(curr + 1),
                                     (unsigned)next, (unsigned)(next + 1)});
    }
  }

  return Mesh(device, vertices, indices, nullptr);
}

} // namespace Primitives

#endif // PRIMITIVES_HPP
