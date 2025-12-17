#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "../core/vfs/file_system.hpp" // IFileSystem
#include "../rhi/rhi_device.h"
#include "shader.hpp" // Shader
#include "shader_manager.hpp"
#include "texture.hpp" // TextureManager
#include <memory>

class RenderContext {
public:
  RenderContext(RHI::API api, void *nativeWindow);
  ~RenderContext();

  bool Initialize(IFileSystem *fs);
  void Shutdown();

  // Accessors
  RHI::IDevice *GetDevice() const { return device.get(); }
  TextureManager *GetTextureManager() const { return textureManager.get(); }
  ShaderManager *GetShaderManager() const { return shaderManager.get(); }
  IFileSystem *GetFileSystem() const { return fileSystem; }

  // Shader Factory/Cache could be added here later
  // For now, we keep simpler management or move ShaderManager here

  RHI::API GetAPI() const { return api; }

private:
  std::unique_ptr<RHI::IDevice> device;
  std::unique_ptr<TextureManager> textureManager;
  std::unique_ptr<ShaderManager> shaderManager;
  IFileSystem *fileSystem = nullptr; // Non-owning pointer

  // Actually let's use shared ownership or raw.
  // To keep it simple: Application owns unique_ptr<IFileSystem>, passes raw ptr
  // to Context.

  RHI::API api;
  void *nativeWindow;
};

#endif // RENDER_CONTEXT_HPP
