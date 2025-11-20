#ifndef SHADER_HPP
#define SHADER_HPP

#include <GL/glew.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

class Shader
{
private:
    unsigned int programID;
    bool compiled;

    unsigned int compileShader(const char* source, GLenum type) {
        unsigned int shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, NULL);
        glCompileShader(shader);
        
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetShaderInfoLog(shader, 512, NULL, infoLog);
            std::cerr << "Erro ao compilar shader (" 
                      << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment") 
                      << "): " << infoLog << std::endl;
        }
        return shader;
    }

    bool linkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
        programID = glCreateProgram();
        glAttachShader(programID, vertexShader);
        glAttachShader(programID, fragmentShader);
        glLinkProgram(programID);
        
        int success;
        glGetProgramiv(programID, GL_LINK_STATUS, &success);
        if (!success) {
            char infoLog[512];
            glGetProgramInfoLog(programID, 512, NULL, infoLog);
            std::cerr << "Erro ao linkar programa: " << infoLog << std::endl;
            return false;
        }
        
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return true;
    }

public:
    Shader() : programID(0), compiled(false) {}

    ~Shader() {
        if (compiled) {
            glDeleteProgram(programID);
        }
    }

    // Compilar a partir de strings
    bool CompileFromSource(const char* vertexSource, const char* fragmentSource) {
        unsigned int vertexShader = compileShader(vertexSource, GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader(fragmentSource, GL_FRAGMENT_SHADER);
        
        compiled = linkProgram(vertexShader, fragmentShader);
        return compiled;
    }

    // Compilar a partir de arquivos
    bool CompileFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
        std::string vertexCode;
        std::string fragmentCode;
        std::ifstream vShaderFile;
        std::ifstream fShaderFile;

        vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

        try {
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            std::stringstream vShaderStream, fShaderStream;

            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();

            vShaderFile.close();
            fShaderFile.close();

            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (std::ifstream::failure& e) {
            std::cerr << "Erro ao ler arquivos de shader: " << e.what() << std::endl;
            return false;
        }

        return CompileFromSource(vertexCode.c_str(), fragmentCode.c_str());
    }

    void Use() const {
        if (compiled) {
            glUseProgram(programID);
        }
    }

    unsigned int GetProgramID() const {
        return programID;
    }

    // Utilitários para definir uniforms
    void SetBool(const std::string& name, bool value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
    }

    void SetInt(const std::string& name, int value) const {
        glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }

    void SetFloat(const std::string& name, float value) const {
        glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
    }

    void SetVec3(const std::string& name, float x, float y, float z) const {
        glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z);
    }

    void SetVec3(const std::string& name, const float* value) const {
        glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, value);
    }

    void SetMat4(const std::string& name, const float* value) const {
        glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1, GL_FALSE, value);
    }

    // Prevenir cópia
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    // Permitir movimentação
    Shader(Shader&& other) noexcept
        : programID(other.programID), compiled(other.compiled) {
        other.compiled = false;
    }

    Shader& operator=(Shader&& other) noexcept {
        if (this != &other) {
            if (compiled) {
                glDeleteProgram(programID);
            }
            programID = other.programID;
            compiled = other.compiled;
            other.compiled = false;
        }
        return *this;
    }
};

// ============================================
// SHADERS SOURCES
// ============================================

namespace ShaderSource {

const char* CubeVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* CubeFragmentShader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

void main() {
    FragColor = vec4(TexCoord.x, TexCoord.y, 0.5, 1.0);
}
)";

const char* ScreenVertexShader = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* ScreenFragmentShader = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D screenTexture;

void main() {
    vec3 col = texture(screenTexture, TexCoord).rgb;
    // Efeito de inversão de cores
    FragColor = vec4(vec3(1.0 - col), 1.0);
}
)";

const char* ScreenFragmentShaderGrayscale = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D screenTexture;

void main() {
    vec3 col = texture(screenTexture, TexCoord).rgb;
    float average = 0.2126 * col.r + 0.7152 * col.g + 0.0722 * col.b;
    FragColor = vec4(average, average, average, 1.0);
}
)";

const char* ScreenFragmentShaderBlur = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D screenTexture;

const float offset = 1.0 / 300.0;

void main() {
    vec2 offsets[9] = vec2[](
        vec2(-offset,  offset), vec2( 0.0,    offset), vec2( offset,  offset),
        vec2(-offset,  0.0),    vec2( 0.0,    0.0),    vec2( offset,  0.0),
        vec2(-offset, -offset), vec2( 0.0,   -offset), vec2( offset, -offset)
    );

    float kernel[9] = float[](
        1.0 / 16, 2.0 / 16, 1.0 / 16,
        2.0 / 16, 4.0 / 16, 2.0 / 16,
        1.0 / 16, 2.0 / 16, 1.0 / 16
    );
    
    vec3 sampleTex[9];
    for(int i = 0; i < 9; i++) {
        sampleTex[i] = vec3(texture(screenTexture, TexCoord.st + offsets[i]));
    }
    
    vec3 col = vec3(0.0);
    for(int i = 0; i < 9; i++) {
        col += sampleTex[i] * kernel[i];
    }
    
    FragColor = vec4(col, 1.0);
}
)";

const char* ModelVertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* ModelFragmentShader = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D texture_diffuse1;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * texture(texture_diffuse1, TexCoords).rgb;
    FragColor = vec4(result, 1.0);
}
)";

const char* ModelFragmentShaderNoTexture = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main() {
    // Ambient
    float ambientStrength = 0.3;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

} // namespace ShaderSource

#endif // SHADER_HPP
