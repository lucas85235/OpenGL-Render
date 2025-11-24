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

                // Tangentes padrão (não calculadas para esfera simples)
                v.Tangent = glm::vec3(0.0f);
                v.Bitangent = glm::vec3(0.0f);

                vertices.push_back(v);
            }
        }

        int k1, k2;
        for(int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1);
            k2 = k1 + sectors + 1;
            for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if(i != 0) {
                    indices.push_back(k1); indices.push_back(k2); indices.push_back(k1 + 1);
                }
                if(i != (stacks-1)) {
                    indices.push_back(k1 + 1); indices.push_back(k2); indices.push_back(k2 + 1);
                }
            }
        }
        
        return Mesh(vertices, indices, std::make_shared<Material>("DefaultSphere")); // Placeholder até ajustar Model.hpp
    }
    
    // Versão Simplificada: Retorna apenas o Mesh para usarmos num wrapper temporário se precisar
    static Mesh CreatePlaneMesh(float size = 20.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float half = size / 2.0f;
        // 4 cantos
        Vertex v1, v2, v3, v4;
        v1.Position = {-half, 0, -half}; v1.Normal = {0,1,0}; v1.TexCoords = {0,0};
        v2.Position = { half, 0, -half}; v2.Normal = {0,1,0}; v2.TexCoords = {size/2,0};
        v3.Position = { half, 0,  half}; v3.Normal = {0,1,0}; v3.TexCoords = {size/2,size/2};
        v4.Position = {-half, 0,  half}; v4.Normal = {0,1,0}; v4.TexCoords = {0,size/2};
        
        vertices = {v1, v2, v3, v4};
        indices = {0, 1, 2, 2, 3, 0};
        
        return Mesh(vertices, indices, nullptr);
    }
};
