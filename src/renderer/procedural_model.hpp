#ifndef PROCEDURAL_MODEL_HPP
#define PROCEDURAL_MODEL_HPP

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>
#include <cmath>

class ProceduralModel {
private:
    unsigned int VAO, VBO, EBO;
    unsigned int vertexCount;
    unsigned int indexCount;

public:
    ProceduralModel() : VAO(0), VBO(0), EBO(0), vertexCount(0), indexCount(0) {}

    ~ProceduralModel() {
        if (VAO) glDeleteVertexArrays(1, &VAO);
        if (VBO) glDeleteBuffers(1, &VBO);
        if (EBO) glDeleteBuffers(1, &EBO);
    }

    // Criar um cubo colorido
    void CreateCube() {
        float vertices[] = {
            // Posições          // Normais          // TexCoords
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,

            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,

            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f
        };

        unsigned int indices[] = {
            0,  1,  2,  2,  3,  0,
            4,  5,  6,  6,  7,  4,
            8,  9, 10, 10, 11,  8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        setupBuffers(vertices, sizeof(vertices), indices, sizeof(indices) / sizeof(unsigned int));
    }

    // Criar uma esfera
    void CreateSphere(float radius = 1.0f, int sectors = 36, int stacks = 18) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        float x, y, z, xy;
        float nx, ny, nz, lengthInv = 1.0f / radius;
        float s, t;

        float sectorStep = 2 * M_PI / sectors;
        float stackStep = M_PI / stacks;
        float sectorAngle, stackAngle;

        // Gerar vértices
        for(int i = 0; i <= stacks; ++i) {
            stackAngle = M_PI / 2 - i * stackStep;
            xy = radius * cosf(stackAngle);
            z = radius * sinf(stackAngle);

            for(int j = 0; j <= sectors; ++j) {
                sectorAngle = j * sectorStep;

                x = xy * cosf(sectorAngle);
                y = xy * sinf(sectorAngle);
                
                // Posição
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                // Normal
                nx = x * lengthInv;
                ny = y * lengthInv;
                nz = z * lengthInv;
                vertices.push_back(nx);
                vertices.push_back(ny);
                vertices.push_back(nz);

                // Coordenadas de textura
                s = (float)j / sectors;
                t = (float)i / stacks;
                vertices.push_back(s);
                vertices.push_back(t);
            }
        }

        // Gerar índices
        int k1, k2;
        for(int i = 0; i < stacks; ++i) {
            k1 = i * (sectors + 1);
            k2 = k1 + sectors + 1;

            for(int j = 0; j < sectors; ++j, ++k1, ++k2) {
                if(i != 0) {
                    indices.push_back(k1);
                    indices.push_back(k2);
                    indices.push_back(k1 + 1);
                }

                if(i != (stacks-1)) {
                    indices.push_back(k1 + 1);
                    indices.push_back(k2);
                    indices.push_back(k2 + 1);
                }
            }
        }

        setupBuffers(vertices.data(), vertices.size() * sizeof(float), 
                     indices.data(), indices.size());
    }

    // Criar um plano
    void CreatePlane(float size = 10.0f) {
        float half = size / 2.0f;
        float vertices[] = {
            // Posições          // Normais        // TexCoords
            -half, 0.0f, -half,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
             half, 0.0f, -half,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
             half, 0.0f,  half,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
            -half, 0.0f,  half,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        setupBuffers(vertices, sizeof(vertices), indices, 6);
    }

    // Criar um cilindro
    void CreateCylinder(float radius = 0.5f, float height = 1.0f, int sectors = 36) {
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        float sectorStep = 2 * M_PI / sectors;
        float sectorAngle;

        // Gerar vértices do corpo
        for(int i = 0; i <= 1; ++i) {
            float y = -height/2 + i * height;
            
            for(int j = 0; j <= sectors; ++j) {
                sectorAngle = j * sectorStep;
                float x = radius * cos(sectorAngle);
                float z = radius * sin(sectorAngle);

                // Posição
                vertices.push_back(x);
                vertices.push_back(y);
                vertices.push_back(z);

                // Normal
                float nx = x / radius;
                float nz = z / radius;
                vertices.push_back(nx);
                vertices.push_back(0.0f);
                vertices.push_back(nz);

                // TexCoords
                vertices.push_back((float)j / sectors);
                vertices.push_back((float)i);
            }
        }

        // Índices do corpo
        for(int i = 0; i < 1; ++i) {
            int k1 = i * (sectors + 1);
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

        setupBuffers(vertices.data(), vertices.size() * sizeof(float),
                     indices.data(), indices.size());
    }

    void Draw() {
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

private:
    void setupBuffers(const void* vertexData, size_t vertexSize,
                      const unsigned int* indexData, unsigned int numIndices) {
        indexCount = numIndices;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertexSize, vertexData, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, numIndices * sizeof(unsigned int),
                     indexData, GL_STATIC_DRAW);

        // Posição
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

        // Normal
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)(3 * sizeof(float)));

        // TexCoords
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)(6 * sizeof(float)));

        glBindVertexArray(0);
    }
};

#endif // PROCEDURAL_MODEL_HPP
