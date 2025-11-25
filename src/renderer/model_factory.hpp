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

                // Tangente segue a longitude (direção da rotação)
                v.Tangent = glm::normalize(glm::vec3(-sinf(sectorAngle), cosf(sectorAngle), 0.0f)); 
                v.Bitangent = glm::normalize(glm::cross(v.Normal, v.Tangent));

                vertices.push_back(v);
            }
        }

        // CORREÇÃO: Ordem dos índices invertida para winding order correto (counter-clockwise)
        int k1, k2;
        for(int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1);
            k2 = k1 + sectors + 1;
            for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if(i != 0) {
                    // Ordem invertida: k1, k1+1, k2 (ao invés de k1, k2, k1+1)
                    indices.push_back(k1); 
                    indices.push_back(k1 + 1); 
                    indices.push_back(k2);
                }
                if(i != (stacks-1)) {
                    // Ordem invertida: k1+1, k2+1, k2 (ao invés de k1+1, k2, k2+1)
                    indices.push_back(k1 + 1); 
                    indices.push_back(k2 + 1); 
                    indices.push_back(k2);
                }
            }
        }
        
        return Mesh(vertices, indices, nullptr); 
    }
    
    static Mesh CreatePlaneMesh(float size = 20.0f) {
        std::vector<Vertex> vertices;
        std::vector<unsigned int> indices;
        
        float half = size / 2.0f;
        
        // Tangentes para um plano horizontal
        glm::vec3 tangent = glm::vec3(1.0f, 0.0f, 0.0f);
        glm::vec3 bitangent = glm::vec3(0.0f, 0.0f, 1.0f);
        glm::vec3 normal = glm::vec3(0.0f, 1.0f, 0.0f);

        Vertex v1, v2, v3, v4;

        // Vértice 1 (Bottom-Left)
        v1.Position = {-half, 0, -half}; 
        v1.Normal = normal; 
        v1.TexCoords = {0, 0};
        v1.Tangent = tangent;
        v1.Bitangent = bitangent;

        // Vértice 2 (Bottom-Right)
        v2.Position = {half, 0, -half}; 
        v2.Normal = normal; 
        v2.TexCoords = {size/2, 0};
        v2.Tangent = tangent;
        v2.Bitangent = bitangent;

        // Vértice 3 (Top-Right)
        v3.Position = {half, 0, half}; 
        v3.Normal = normal; 
        v3.TexCoords = {size/2, size/2};
        v3.Tangent = tangent;
        v3.Bitangent = bitangent;

        // Vértice 4 (Top-Left)
        v4.Position = {-half, 0, half}; 
        v4.Normal = normal; 
        v4.TexCoords = {0, size/2};
        v4.Tangent = tangent;
        v4.Bitangent = bitangent;
        
        vertices = {v1, v2, v3, v4};

        indices = {
            0, 2, 1,  // Primeiro triângulo
            2, 0, 3   // Segundo triângulo
        };
        
        return Mesh(vertices, indices, nullptr);
    }
};

#endif // MODEL_FACTORY_HPP
