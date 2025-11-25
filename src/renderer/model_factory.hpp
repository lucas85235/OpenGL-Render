#ifndef MODEL_FACTORY_HPP
#define MODEL_FACTORY_HPP

#include <vector>
#include <cmath>
#include <memory>
#include "mesh.hpp"

class ModelFactory {
public:
    static Mesh CreateSphere(float radius = 1.0f, int sectors = 36, int stacks = 18) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;

        float x, y, z, xy;
        float nx, ny, nz, lengthInv = 1.0f / radius;
        float s, t;

        float sectorStep = 2 * M_PI / sectors;
        float stackStep = M_PI / stacks;
        float sectorAngle, stackAngle;

        for(int i = 0; i <= stacks; ++i) {
            stackAngle = M_PI / 2 - i * stackStep;
            xy = radius * cosf(stackAngle);
            z = radius * sinf(stackAngle);

            for(int j = 0; j <= sectors; ++j) {
                sectorAngle = j * sectorStep;

                Vertex v;
                x = xy * cosf(sectorAngle);
                y = xy * sinf(sectorAngle);
                v.Position = glm::vec3(x, y, z);

                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                v.Normal = glm::vec3(nx, ny, nz);

                s = (float)j / sectors;
                t = (float)i / stacks;
                v.TexCoords = glm::vec2(s, t);

                v.Tangent = glm::normalize(glm::vec3(-sinf(sectorAngle), cosf(sectorAngle), 0.0f)); 
                v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));

                vertices.push_back(v);
            }
        }

        int k1, k2;
        for(int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1);
            k2 = k1 + sectors + 1;
            for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if(i != 0) {
                    indices.push_back(k1); 
                    indices.push_back(k1 + 1); 
                    indices.push_back(k2);
                }
                if(i != (stacks-1)) {
                    indices.push_back(k1 + 1); 
                    indices.push_back(k2 + 1); 
                    indices.push_back(k2);
                }
            }
        }
        
        return Mesh(vertices, indices, nullptr); 
    }
    
    static Mesh CreateCube(float size = 1.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float half = size / 2.0f;
        
        // Array com as 6 faces do cubo
        // Cada face tem 4 vértices com normal, tangente e bitangente corretos
        
        // FACE FRONTAL (+Z)
        vertices.push_back({{-half, -half,  half}, {0, 0, 1}, {0, 0}, {1, 0, 0}, {0, 1, 0}});
        vertices.push_back({{ half, -half,  half}, {0, 0, 1}, {1, 0}, {1, 0, 0}, {0, 1, 0}});
        vertices.push_back({{ half,  half,  half}, {0, 0, 1}, {1, 1}, {1, 0, 0}, {0, 1, 0}});
        vertices.push_back({{-half,  half,  half}, {0, 0, 1}, {0, 1}, {1, 0, 0}, {0, 1, 0}});
        
        // FACE TRASEIRA (-Z)
        vertices.push_back({{ half, -half, -half}, {0, 0, -1}, {0, 0}, {-1, 0, 0}, {0, 1, 0}});
        vertices.push_back({{-half, -half, -half}, {0, 0, -1}, {1, 0}, {-1, 0, 0}, {0, 1, 0}});
        vertices.push_back({{-half,  half, -half}, {0, 0, -1}, {1, 1}, {-1, 0, 0}, {0, 1, 0}});
        vertices.push_back({{ half,  half, -half}, {0, 0, -1}, {0, 1}, {-1, 0, 0}, {0, 1, 0}});
        
        // FACE SUPERIOR (+Y)
        vertices.push_back({{-half,  half,  half}, {0, 1, 0}, {0, 0}, {1, 0, 0}, {0, 0, 1}});
        vertices.push_back({{ half,  half,  half}, {0, 1, 0}, {1, 0}, {1, 0, 0}, {0, 0, 1}});
        vertices.push_back({{ half,  half, -half}, {0, 1, 0}, {1, 1}, {1, 0, 0}, {0, 0, 1}});
        vertices.push_back({{-half,  half, -half}, {0, 1, 0}, {0, 1}, {1, 0, 0}, {0, 0, 1}});
        
        // FACE INFERIOR (-Y)
        vertices.push_back({{-half, -half, -half}, {0, -1, 0}, {0, 0}, {1, 0, 0}, {0, 0, -1}});
        vertices.push_back({{ half, -half, -half}, {0, -1, 0}, {1, 0}, {1, 0, 0}, {0, 0, -1}});
        vertices.push_back({{ half, -half,  half}, {0, -1, 0}, {1, 1}, {1, 0, 0}, {0, 0, -1}});
        vertices.push_back({{-half, -half,  half}, {0, -1, 0}, {0, 1}, {1, 0, 0}, {0, 0, -1}});
        
        // FACE DIREITA (+X)
        vertices.push_back({{ half, -half,  half}, {1, 0, 0}, {0, 0}, {0, 0, -1}, {0, 1, 0}});
        vertices.push_back({{ half, -half, -half}, {1, 0, 0}, {1, 0}, {0, 0, -1}, {0, 1, 0}});
        vertices.push_back({{ half,  half, -half}, {1, 0, 0}, {1, 1}, {0, 0, -1}, {0, 1, 0}});
        vertices.push_back({{ half,  half,  half}, {1, 0, 0}, {0, 1}, {0, 0, -1}, {0, 1, 0}});
        
        // FACE ESQUERDA (-X)
        vertices.push_back({{-half, -half, -half}, {-1, 0, 0}, {0, 0}, {0, 0, 1}, {0, 1, 0}});
        vertices.push_back({{-half, -half,  half}, {-1, 0, 0}, {1, 0}, {0, 0, 1}, {0, 1, 0}});
        vertices.push_back({{-half,  half,  half}, {-1, 0, 0}, {1, 1}, {0, 0, 1}, {0, 1, 0}});
        vertices.push_back({{-half,  half, -half}, {-1, 0, 0}, {0, 1}, {0, 0, 1}, {0, 1, 0}});
        
        // Índices para as 6 faces (cada face = 2 triângulos)
        for(int i = 0; i < 6; ++i) {
            int offset = i * 4;
            indices.push_back(offset + 0);
            indices.push_back(offset + 1);
            indices.push_back(offset + 2);
            
            indices.push_back(offset + 2);
            indices.push_back(offset + 3);
            indices.push_back(offset + 0);
        }
        
        return Mesh(vertices, indices, nullptr);
    }
    
    static Mesh CreateCylinder(float radius = 0.5f, float height = 2.0f, int sectors = 36) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float halfHeight = height / 2.0f;
        float sectorStep = 2 * M_PI / sectors;
        
        // Vertices do corpo do cilindro
        for(int i = 0; i <= sectors; ++i) {
            float sectorAngle = i * sectorStep;
            float x = radius * cosf(sectorAngle);
            float z = radius * sinf(sectorAngle);
            
            glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));
            glm::vec3 tangent = glm::vec3(-sinf(sectorAngle), 0, cosf(sectorAngle));
            glm::vec3 bitangent = glm::vec3(0, 1, 0);
            
            // Vértice inferior
            vertices.push_back({
                {x, -halfHeight, z},
                normal,
                {(float)i / sectors, 0},
                tangent,
                bitangent
            });
            
            // Vértice superior
            vertices.push_back({
                {x, halfHeight, z},
                normal,
                {(float)i / sectors, 1},
                tangent,
                bitangent
            });
        }
        
        // Índices do corpo
        for(int i = 0; i < sectors; ++i) {
            int current = i * 2;
            int next = current + 2;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(current + 1);
            indices.push_back(next);
            indices.push_back(next + 1);
        }
        
        // Tampa inferior (centro primeiro)
        int bottomCenterIndex = vertices.size();
        vertices.push_back({
            {0, -halfHeight, 0},
            {0, -1, 0},
            {0.5f, 0.5f},
            {1, 0, 0},
            {0, 0, 1}
        });
        
        for(int i = 0; i < sectors; ++i) {
            float angle1 = i * sectorStep;
            float angle2 = (i + 1) * sectorStep;
            
            vertices.push_back({
                {radius * cosf(angle1), -halfHeight, radius * sinf(angle1)},
                {0, -1, 0},
                {0.5f + 0.5f * cosf(angle1), 0.5f + 0.5f * sinf(angle1)},
                {1, 0, 0},
                {0, 0, 1}
            });
        }
        
        for(int i = 0; i < sectors; ++i) {
            indices.push_back(bottomCenterIndex);
            indices.push_back(bottomCenterIndex + 1 + ((i + 1) % sectors));
            indices.push_back(bottomCenterIndex + 1 + i);
        }
        
        // Tampa superior
        int topCenterIndex = vertices.size();
        vertices.push_back({
            {0, halfHeight, 0},
            {0, 1, 0},
            {0.5f, 0.5f},
            {1, 0, 0},
            {0, 0, 1}
        });
        
        for(int i = 0; i < sectors; ++i) {
            float angle = i * sectorStep;
            
            vertices.push_back({
                {radius * cosf(angle), halfHeight, radius * sinf(angle)},
                {0, 1, 0},
                {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)},
                {1, 0, 0},
                {0, 0, 1}
            });
        }
        
        for(int i = 0; i < sectors; ++i) {
            indices.push_back(topCenterIndex);
            indices.push_back(topCenterIndex + 1 + i);
            indices.push_back(topCenterIndex + 1 + ((i + 1) % sectors));
        }
        
        return Mesh(vertices, indices, nullptr);
    }
    
    static Mesh CreateCone(float radius = 0.5f, float height = 2.0f, int sectors = 36) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float sectorStep = 2 * M_PI / sectors;
        
        // Vértice do topo (ápice)
        int apexIndex = 0;
        vertices.push_back({
            {0, height, 0},
            {0, 1, 0},
            {0.5f, 1.0f},
            {1, 0, 0},
            {0, 0, 1}
        });
        
        // Vértices da base
        for(int i = 0; i <= sectors; ++i) {
            float angle = i * sectorStep;
            float x = radius * cosf(angle);
            float z = radius * sinf(angle);
            
            // Normal calculada para superfície cônica
            glm::vec3 surfaceNormal = glm::normalize(glm::vec3(x, radius, z));
            glm::vec3 tangent = glm::vec3(-sinf(angle), 0, cosf(angle));
            glm::vec3 bitangent = glm::cross(surfaceNormal, tangent);
            
            vertices.push_back({
                {x, 0, z},
                surfaceNormal,
                {(float)i / sectors, 0},
                tangent,
                bitangent
            });
        }
        
        // Índices da superfície cônica
        for(int i = 1; i <= sectors; ++i) {
            indices.push_back(apexIndex);
            indices.push_back(i);
            indices.push_back(i + 1);
        }
        
        // Base do cone
        int baseCenterIndex = vertices.size();
        vertices.push_back({
            {0, 0, 0},
            {0, -1, 0},
            {0.5f, 0.5f},
            {1, 0, 0},
            {0, 0, 1}
        });
        
        for(int i = 0; i < sectors; ++i) {
            float angle = i * sectorStep;
            
            vertices.push_back({
                {radius * cosf(angle), 0, radius * sinf(angle)},
                {0, -1, 0},
                {0.5f + 0.5f * cosf(angle), 0.5f + 0.5f * sinf(angle)},
                {1, 0, 0},
                {0, 0, 1}
            });
        }
        
        for(int i = 0; i < sectors; ++i) {
            indices.push_back(baseCenterIndex);
            indices.push_back(baseCenterIndex + 1 + ((i + 1) % sectors));
            indices.push_back(baseCenterIndex + 1 + i);
        }
        
        return Mesh(vertices, indices, nullptr);
    }
    
    static Mesh CreateTorus(float majorRadius = 1.0f, float minorRadius = 0.3f, 
                           int majorSectors = 48, int minorSectors = 24) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float majorStep = 2 * M_PI / majorSectors;
        float minorStep = 2 * M_PI / minorSectors;
        
        for(int i = 0; i <= majorSectors; ++i) {
            float u = i * majorStep;
            float cu = cosf(u);
            float su = sinf(u);
            
            for(int j = 0; j <= minorSectors; ++j) {
                float v = j * minorStep;
                float cv = cosf(v);
                float sv = sinf(v);
                
                // Posição
                float x = (majorRadius + minorRadius * cv) * cu;
                float y = minorRadius * sv;
                float z = (majorRadius + minorRadius * cv) * su;
                
                // Normal
                glm::vec3 normal = glm::normalize(glm::vec3(cv * cu, sv, cv * su));
                
                // Tangente e Bitangente
                glm::vec3 tangent = glm::normalize(glm::vec3(-su, 0, cu));
                glm::vec3 bitangent = glm::cross(normal, tangent);
                
                vertices.push_back({
                    {x, y, z},
                    normal,
                    {(float)i / majorSectors, (float)j / minorSectors},
                    tangent,
                    bitangent
                });
            }
        }
        
        // Índices
        for(int i = 0; i < majorSectors; ++i) {
            int i1 = i * (minorSectors + 1);
            int i2 = (i + 1) * (minorSectors + 1);
            
            for(int j = 0; j < minorSectors; ++j) {
                indices.push_back(i1 + j);
                indices.push_back(i2 + j);
                indices.push_back(i1 + j + 1);
                
                indices.push_back(i1 + j + 1);
                indices.push_back(i2 + j);
                indices.push_back(i2 + j + 1);
            }
        }
        
        return Mesh(vertices, indices, nullptr);
    }
    
    static Mesh CreatePlaneMesh(float size = 20.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float half = size / 2.0f;
        
        glm::vec3 tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

        Vertex v1, v2, v3, v4;

        v1.Position = {-half, 0, -half}; 
        v1.Normal = normal; 
        v1.TexCoords = {0, 0};
        v1.Tangent = tangent;
        v1.Bitangent = bitangent;

        v2.Position = {half, 0, -half}; 
        v2.Normal = normal; 
        v2.TexCoords = {size/2, 0};
        v2.Tangent = tangent;
        v2.Bitangent = bitangent;

        v3.Position = {half, 0, half}; 
        v3.Normal = normal; 
        v3.TexCoords = {size/2, size/2};
        v3.Tangent = tangent;
        v3.Bitangent = bitangent;

        v4.Position = {-half, 0, half}; 
        v4.Normal = normal; 
        v4.TexCoords = {0, size/2};
        v4.Tangent = tangent;
        v4.Bitangent = bitangent;
        
        vertices = {v1, v2, v3, v4};
        indices = {0, 2, 1, 2, 0, 3};
        
        return Mesh(vertices, indices, nullptr);
    }
    
    static Mesh CreateCapsule(float radius = 0.5f, float height = 2.0f, 
                             int sectors = 36, int rings = 8) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float cylinderHeight = height - 2 * radius;
        float halfCylinderHeight = cylinderHeight / 2.0f;
        
        float sectorStep = 2 * M_PI / sectors;
        float ringStep = (M_PI / 2) / rings;
        
        // Hemisfério superior
        for(int i = 0; i <= rings; ++i) {
            float stackAngle = M_PI / 2 - i * ringStep;
            float xy = radius * cosf(stackAngle);
            float z = radius * sinf(stackAngle);
            
            for(int j = 0; j <= sectors; ++j) {
                float sectorAngle = j * sectorStep;
                
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                
                glm::vec3 pos(x, z + halfCylinderHeight, y);
                glm::vec3 normal = glm::normalize(glm::vec3(x, z, y));
                glm::vec3 tangent = glm::normalize(glm::vec3(-sinf(sectorAngle), 0, cosf(sectorAngle)));
                glm::vec3 bitangent = glm::cross(normal, tangent);
                
                vertices.push_back({
                    pos,
                    normal,
                    {(float)j / sectors, (float)i / (rings * 2)},
                    tangent,
                    bitangent
                });
            }
        }
        
        // Cilindro central
        for(int i = 0; i <= 1; ++i) {
            float yPos = (i == 0) ? halfCylinderHeight : -halfCylinderHeight;
            
            for(int j = 0; j <= sectors; ++j) {
                float sectorAngle = j * sectorStep;
                float x = radius * cosf(sectorAngle);
                float z = radius * sinf(sectorAngle);
                
                glm::vec3 normal = glm::normalize(glm::vec3(x, 0, z));
                glm::vec3 tangent = glm::vec3(-sinf(sectorAngle), 0, cosf(sectorAngle));
                glm::vec3 bitangent = glm::vec3(0, 1, 0);
                
                vertices.push_back({
                    {x, yPos, z},
                    normal,
                    {(float)j / sectors, 0.5f + (i == 0 ? -0.25f : 0.25f)},
                    tangent,
                    bitangent
                });
            }
        }
        
        // Hemisfério inferior
        for(int i = 0; i <= rings; ++i) {
            float stackAngle = -i * ringStep;
            float xy = radius * cosf(stackAngle);
            float z = radius * sinf(stackAngle);
            
            for(int j = 0; j <= sectors; ++j) {
                float sectorAngle = j * sectorStep;
                
                float x = xy * cosf(sectorAngle);
                float y = xy * sinf(sectorAngle);
                
                glm::vec3 pos(x, z - halfCylinderHeight, y);
                glm::vec3 normal = glm::normalize(glm::vec3(x, z, y));
                glm::vec3 tangent = glm::normalize(glm::vec3(-sinf(sectorAngle), 0, cosf(sectorAngle)));
                glm::vec3 bitangent = glm::cross(normal, tangent);
                
                vertices.push_back({
                    pos,
                    normal,
                    {(float)j / sectors, 1.0f - (float)i / (rings * 2)},
                    tangent,
                    bitangent
                });
            }
        }
        
        // Gerar índices
        int topHemisphereStart = 0;
        int cylinderStart = (rings + 1) * (sectors + 1);
        int bottomHemisphereStart = cylinderStart + 2 * (sectors + 1);
        
        // Índices do hemisfério superior
        for(int i = 0; i < rings; ++i) {
            int k1 = topHemisphereStart + i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            
            for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
        
        // Índices do cilindro
        for(int j = 0; j < sectors; ++j) {
            int k1 = cylinderStart + j;
            int k2 = k1 + sectors + 1;
            
            indices.push_back(k1);
            indices.push_back(k2);
            indices.push_back(k1 + 1);
            
            indices.push_back(k1 + 1);
            indices.push_back(k2);
            indices.push_back(k2 + 1);
        }
        
        // Índices do hemisfério inferior
        for(int i = 0; i < rings; ++i) {
            int k1 = bottomHemisphereStart + i * (sectors + 1);
            int k2 = k1 + sectors + 1;
            
            for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
                
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
        
        return Mesh(vertices, indices, nullptr);
    }
};

#endif // MODEL_FACTORY_HPP