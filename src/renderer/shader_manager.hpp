#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include "../core/vfs/file_system.hpp"
#include "../rhi/shader_preprocessor.hpp"
#include "shader.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class ShaderManager {
public:
  ShaderManager(RHI::IDevice *device, IFileSystem *fs, RHI::API api)
      : device(device), fs(fs), api(api) {}

  // Load from separate vertex/fragment files (legacy)
  std::shared_ptr<Shader> LoadShader(const std::string &name,
                                     const std::string &vertexPath,
                                     const std::string &fragmentPath) {
    auto it = shaders.find(name);
    if (it != shaders.end())
      return it->second;

    auto shader = std::make_shared<Shader>(device);
    std::string vertAbs = fs->GetAbsolutePath(vertexPath);
    std::string fragAbs = fs->GetAbsolutePath(fragmentPath);

    bool success = (api == RHI::API::Vulkan)
                       ? shader->CompileFromSPIRV(vertAbs, fragAbs)
                       : shader->CompileFromFile(vertAbs, fragAbs);

    if (success) {
      shaders[name] = shader;
      return shader;
    }
    return nullptr;
  }

  // Load from unified .shader file (new)
  std::shared_ptr<Shader> LoadUnifiedShader(const std::string &name,
                                            const std::string &shaderPath) {
    auto it = shaders.find(name);
    if (it != shaders.end())
      return it->second;

    std::string source = fs->ReadTextFile(shaderPath);
    if (source.empty()) {
      std::cerr << "[ShaderManager] Failed to read: " << shaderPath
                << std::endl;
      return nullptr;
    }

    uint32_t glslVersion = (api == RHI::API::Vulkan) ? 450 : 450;
    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, api, glslVersion);

    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[ShaderManager] Missing vertex or fragment stage in: "
                << shaderPath << std::endl;
      return nullptr;
    }

    auto shader = std::make_shared<Shader>(device);
    bool success = shader->CompileFromSource(stages.vertex.c_str(),
                                             stages.fragment.c_str());

    if (success) {
      shaders[name] = shader;
      return shader;
    }

    std::cerr << "[ShaderManager] Failed to compile unified shader: " << name
              << std::endl;
    return nullptr;
  }

  std::shared_ptr<Shader> GetShader(const std::string &name) {
    auto it = shaders.find(name);
    return (it != shaders.end()) ? it->second : nullptr;
  }

  void Clear() { shaders.clear(); }

private:
  std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
  RHI::IDevice *device = nullptr;
  IFileSystem *fs = nullptr;
  RHI::API api;
};

#endif
