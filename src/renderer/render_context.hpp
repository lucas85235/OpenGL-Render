#ifndef RENDER_CONTEXT_HPP
#define RENDER_CONTEXT_HPP

#include "../rhi/rhi_device.h"
#include "shader.hpp"  // Shader
#include "texture.hpp" // TextureManager
#include <memory>

class RenderContext {
public:
  RenderContext(RHI::API api, void *nativeWindow);
  ~RenderContext();

  bool Initialize();
  void Shutdown();

  // Accessors
  RHI::IDevice *GetDevice() const { return device.get(); }
  TextureManager *GetTextureManager() const { return textureManager.get(); }

  // Shader Factory/Cache could be added here later
  // For now, we keep simpler management or move ShaderManager here

private:
  std::unique_ptr<RHI::IDevice> device;
  std::unique_ptr<TextureManager> textureManager;

  RHI::API api;
  void *nativeWindow;
};

#endif // RENDER_CONTEXT_HPP
