#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "../core/vfs/file_system.hpp"
#include "../rhi/rhi_device.h"
#include "shader_manager.hpp"
#include "texture.hpp"
#include <memory>

class RenderContext {
public:
  RenderContext(RHI::API api, void *nativeWindow);
  ~RenderContext();

  bool Initialize(IFileSystem *fs);
  void Shutdown();

  RHI::IDevice *GetDevice() const { return device.get(); }
  TextureManager *GetTextureManager() const { return textureManager.get(); }
  ShaderManager *GetShaderManager() const { return shaderManager.get(); }
  IFileSystem *GetFileSystem() const { return fileSystem; }
  RHI::API GetAPI() const { return api; }

private:
  std::unique_ptr<RHI::IDevice> device;
  std::unique_ptr<TextureManager> textureManager;
  std::unique_ptr<ShaderManager> shaderManager;
  IFileSystem *fileSystem = nullptr;
  RHI::API api;
  void *nativeWindow;
};

#endif
