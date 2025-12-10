#ifndef SHADER_HPP
#define SHADER_HPP

#include "../rhi/rhi_device.h"
#include <GL/glew.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader {
private:
  // RHI resources
  RHI::IDevice *device = nullptr;
  RHI::ShaderHandle rhiShader;
  bool useRHI = false;

  // Legacy OpenGL
  unsigned int programID = 0;
  bool compiled = false;

  unsigned int compileShaderGL(const char *source, GLenum type) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
      char infoLog[512];
      glGetShaderInfoLog(shader, 512, NULL, infoLog);
      std::cerr << "[Shader] Compile error ("
                << (type == GL_VERTEX_SHADER ? "Vertex" : "Fragment")
                << "): " << infoLog << std::endl;
    }
    return shader;
  }

  bool linkProgramGL(unsigned int vertexShader, unsigned int fragmentShader) {
    programID = glCreateProgram();
    glAttachShader(programID, vertexShader);
    glAttachShader(programID, fragmentShader);
    glLinkProgram(programID);

    int success;
    glGetProgramiv(programID, GL_LINK_STATUS, &success);
    if (!success) {
      char infoLog[512];
      glGetProgramInfoLog(programID, 512, NULL, infoLog);
      std::cerr << "[Shader] Link error: " << infoLog << std::endl;
      return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return true;
  }

public:
  Shader() = default;

  explicit Shader(RHI::IDevice *dev) : device(dev) {}

  ~Shader() {
    if (useRHI && device) {
      if (RHI::IsValid(rhiShader)) {
        device->DestroyShader(rhiShader);
      }
    } else if (compiled) {
      glDeleteProgram(programID);
    }
  }

  void SetDevice(RHI::IDevice *dev) { device = dev; }

  bool CompileFromSource(const char *vertexSource, const char *fragmentSource) {
    if (device) {
      return CompileRHI(vertexSource, fragmentSource);
    } else {
      return CompileOpenGL(vertexSource, fragmentSource);
    }
  }

private:
  bool CompileRHI(const char *vertexSource, const char *fragmentSource) {
    std::vector<RHI::ShaderDescriptor> stages = {
        {RHI::ShaderStage::Vertex, vertexSource},
        {RHI::ShaderStage::Fragment, fragmentSource}};

    rhiShader = device->CreateShader(stages);
    if (!RHI::IsValid(rhiShader)) {
      std::cerr << "[Shader] RHI: Failed to compile shader" << std::endl;
      return false;
    }

    useRHI = true;
    compiled = true;
    return true;
  }

  bool CompileOpenGL(const char *vertexSource, const char *fragmentSource) {
    unsigned int vertexShader = compileShaderGL(vertexSource, GL_VERTEX_SHADER);
    unsigned int fragmentShader =
        compileShaderGL(fragmentSource, GL_FRAGMENT_SHADER);

    compiled = linkProgramGL(vertexShader, fragmentShader);
    return compiled;
  }

public:
  bool CompileFromFile(const std::string &vertexPath,
                       const std::string &fragmentPath) {
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
    } catch (std::ifstream::failure &e) {
      std::cerr << "[Shader] Failed to read files: " << e.what() << std::endl;
      return false;
    }

    return CompileFromSource(vertexCode.c_str(), fragmentCode.c_str());
  }

  void Use() const {
    if (useRHI && device) {
      // RHI shader binding is done via pipeline binding
      // No direct Use() equivalent in Vulkan-style APIs
    } else if (compiled) {
      glUseProgram(programID);
    }
  }

  unsigned int GetProgramID() const { return programID; }
  RHI::ShaderHandle GetRHIHandle() const { return rhiShader; }
  bool IsCompiled() const { return compiled; }
  bool IsUsingRHI() const { return useRHI; }

  // Uniform setters - legacy OpenGL path
  void SetBool(const std::string &name, bool value) const {
    if (!useRHI && compiled) {
      glUniform1i(glGetUniformLocation(programID, name.c_str()), (int)value);
    }
  }

  void SetInt(const std::string &name, int value) const {
    if (useRHI && device) {
      device->SetUniform(rhiShader, name, value);
    } else if (compiled) {
      glUniform1i(glGetUniformLocation(programID, name.c_str()), value);
    }
  }

  void SetFloat(const std::string &name, float value) const {
    if (useRHI && device) {
      device->SetUniform(rhiShader, name, value);
    } else if (compiled) {
      glUniform1f(glGetUniformLocation(programID, name.c_str()), value);
    }
  }

  void SetVec3(const std::string &name, float x, float y, float z) const {
    if (useRHI && device) {
      float v[3] = {x, y, z};
      device->SetUniform(rhiShader, name, v, 3);
    } else if (compiled) {
      glUniform3f(glGetUniformLocation(programID, name.c_str()), x, y, z);
    }
  }

  void SetVec3(const std::string &name, const float *value) const {
    if (useRHI && device) {
      device->SetUniform(rhiShader, name, value, 3);
    } else if (compiled) {
      glUniform3fv(glGetUniformLocation(programID, name.c_str()), 1, value);
    }
  }

  void SetMat4(const std::string &name, const float *value) const {
    if (useRHI && device) {
      device->SetUniformMatrix4(rhiShader, name, value);
    } else if (compiled) {
      glUniformMatrix4fv(glGetUniformLocation(programID, name.c_str()), 1,
                         GL_FALSE, value);
    }
  }

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  Shader(Shader &&other) noexcept
      : device(other.device), rhiShader(other.rhiShader), useRHI(other.useRHI),
        programID(other.programID), compiled(other.compiled) {
    other.device = nullptr;
    other.rhiShader = RHI::ShaderHandle{0};
    other.programID = 0;
    other.compiled = false;
    other.useRHI = false;
  }

  Shader &operator=(Shader &&other) noexcept {
    if (this != &other) {
      if (useRHI && device) {
        if (RHI::IsValid(rhiShader))
          device->DestroyShader(rhiShader);
      } else if (compiled) {
        glDeleteProgram(programID);
      }

      device = other.device;
      rhiShader = other.rhiShader;
      useRHI = other.useRHI;
      programID = other.programID;
      compiled = other.compiled;

      other.device = nullptr;
      other.rhiShader = RHI::ShaderHandle{0};
      other.programID = 0;
      other.compiled = false;
      other.useRHI = false;
    }
    return *this;
  }
};

#endif // SHADER_HPP
