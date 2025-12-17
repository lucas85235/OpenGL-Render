#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include "../core/vfs/file_system.hpp"
#include "shader.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class ShaderManager {
public:
  ShaderManager(RHI::IDevice *device, IFileSystem *fs, RHI::API api)
      : device(device), fs(fs), api(api) {}

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
