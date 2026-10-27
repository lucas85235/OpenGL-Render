#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "../core/vfs/file_system.hpp"
#include "../rhi/rhi_device.h"
#include "shader_manager.hpp"
#include "texture.hpp"
#include <cstdint>
#include <memory>

struct FrameStats {
  uint64_t frameCount = 0;
  uint32_t drawCalls = 0;
  uint32_t triangles = 0;
  uint32_t vertices = 0;
};

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

  const FrameStats &GetFrameStats() const { return frameStats; }
  FrameStats &GetFrameStatsMutable() { return frameStats; }
  void BeginFrameStats() {
    frameStats.drawCalls = 0;
    frameStats.triangles = 0;
    frameStats.vertices = 0;
    ++frameStats.frameCount;
  }

private:
  std::unique_ptr<RHI::IDevice> device;
  std::unique_ptr<TextureManager> textureManager;
  std::unique_ptr<ShaderManager> shaderManager;
  IFileSystem *fileSystem = nullptr;
  RHI::API api;
  void *nativeWindow;
  FrameStats frameStats;
};

#endif
