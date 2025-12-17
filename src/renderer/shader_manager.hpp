#ifndef SHADER_MANAGER_HPP
#define SHADER_MANAGER_HPP

#include "../core/vfs/file_system.hpp"
#include "shader.hpp"
#include <memory>
#include <string>
#include <unordered_map>

class ShaderManager {
private:
  std::unordered_map<std::string, std::shared_ptr<Shader>> shaders;
  RHI::IDevice *device = nullptr;
  IFileSystem *fs = nullptr;
  RHI::API api;

public:
  ShaderManager(RHI::IDevice *dev, IFileSystem *fileSys, RHI::API apiType)
      : device(dev), fs(fileSys), api(apiType) {}

  std::shared_ptr<Shader> LoadShader(const std::string &name,
                                     const std::string &vertexPath,
                                     const std::string &fragmentPath) {
    if (shaders.find(name) != shaders.end()) {
      return shaders[name];
    }

    auto shader = std::make_shared<Shader>(device);
    bool result = false;

    // Path handling should ideally be smarter, but for now we follow
    // Application logic If API is Vulkan, we expect .spv extension or we append
    // it? Let's assume the caller provides relative paths like
    // "shaders/pbr.vert" and we adjust based on API, OR the caller provides
    // simpler names.

    // Better: The caller says LoadShader("pbr", "shaders/pbr") and we handle
    // extensions. But to keep it compatible with current hardcoded paths in
    // App, let's keep it flexible.

    // Actually, let's enforce a convention to simplify App.
    // Application code:
    // if Vulkan: Load("pbr", "shaders/unified/pbr.vert.spv", ...)
    // This is still messy.

    // Let's allow passing direct paths for now to match current behavior.
    // Note: The previous code used FS::GetPath.

    std::string vertAbs = fs->GetAbsolutePath(vertexPath);
    std::string fragAbs = fs->GetAbsolutePath(fragmentPath);

    if (api == RHI::API::Vulkan) {
      result = shader->CompileFromSPIRV(vertAbs, fragAbs);
    } else {
      result = shader->CompileFromFile(vertAbs, fragAbs);
    }

    if (result) {
      shaders[name] = shader;
      return shader;
    }

    return nullptr;
  }

  std::shared_ptr<Shader> GetShader(const std::string &name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
      return it->second;
    }
    return nullptr;
  }

  void Clear() { shaders.clear(); }
};

#endif // SHADER_MANAGER_HPP
