#ifndef SHADER_HPP
#define SHADER_HPP

#include "../rhi/rhi_device.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Shader {
private:
  RHI::IDevice *device = nullptr;
  RHI::ShaderHandle shaderHandle;
  bool compiled = false;

public:
  Shader() = default;
  explicit Shader(RHI::IDevice *dev) : device(dev) {}

  ~Shader() {
    if (device && RHI::IsValid(shaderHandle)) {
      device->DestroyShader(shaderHandle);
    }
  }

  void SetDevice(RHI::IDevice *dev) { device = dev; }

  bool CompileFromSource(const char *vertexSource, const char *fragmentSource) {
    if (!device) {
      std::cerr << "[Shader] No device set" << std::endl;
      return false;
    }

    std::vector<RHI::ShaderDescriptor> stages = {
        {RHI::ShaderStage::Vertex, vertexSource},
        {RHI::ShaderStage::Fragment, fragmentSource}};

    shaderHandle = device->CreateShader(stages);
    if (!RHI::IsValid(shaderHandle)) {
      std::cerr << "[Shader] Failed to compile shader" << std::endl;
      return false;
    }

    compiled = true;
    return true;
  }

  bool CompileFromFile(const std::string &vertexPath,
                       const std::string &fragmentPath) {
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    try {
      vShaderFile.open(vertexPath);
      fShaderFile.open(fragmentPath);
      std::stringstream vStream, fStream;
      vStream << vShaderFile.rdbuf();
      fStream << fShaderFile.rdbuf();
      vShaderFile.close();
      fShaderFile.close();
      vertexCode = vStream.str();
      fragmentCode = fStream.str();
    } catch (std::ifstream::failure &e) {
      std::cerr << "[Shader] Failed to read files: " << e.what() << std::endl;
      return false;
    }

    return CompileFromSource(vertexCode.c_str(), fragmentCode.c_str());
  }

  RHI::ShaderHandle GetHandle() const { return shaderHandle; }
  bool IsCompiled() const { return compiled; }
  RHI::IDevice *GetDevice() const { return device; }

  void SetBool(const std::string &name, bool value) const {
    if (device && compiled) {
      device->SetUniform(shaderHandle, name, static_cast<int>(value));
    }
  }

  void SetInt(const std::string &name, int value) const {
    if (device && compiled) {
      device->SetUniform(shaderHandle, name, value);
    }
  }

  void SetFloat(const std::string &name, float value) const {
    if (device && compiled) {
      device->SetUniform(shaderHandle, name, value);
    }
  }

  void SetVec3(const std::string &name, float x, float y, float z) const {
    if (device && compiled) {
      float v[3] = {x, y, z};
      device->SetUniform(shaderHandle, name, v, 3);
    }
  }

  void SetVec3(const std::string &name, const float *value) const {
    if (device && compiled) {
      device->SetUniform(shaderHandle, name, value, 3);
    }
  }

  void SetMat4(const std::string &name, const float *value) const {
    if (device && compiled) {
      device->SetUniformMatrix4(shaderHandle, name, value);
    }
  }

  Shader(const Shader &) = delete;
  Shader &operator=(const Shader &) = delete;

  Shader(Shader &&other) noexcept
      : device(other.device), shaderHandle(other.shaderHandle),
        compiled(other.compiled) {
    other.device = nullptr;
    other.shaderHandle = RHI::ShaderHandle{0};
    other.compiled = false;
  }

  Shader &operator=(Shader &&other) noexcept {
    if (this != &other) {
      if (device && RHI::IsValid(shaderHandle)) {
        device->DestroyShader(shaderHandle);
      }
      device = other.device;
      shaderHandle = other.shaderHandle;
      compiled = other.compiled;
      other.device = nullptr;
      other.shaderHandle = RHI::ShaderHandle{0};
      other.compiled = false;
    }
    return *this;
  }
};

#endif // SHADER_HPP
