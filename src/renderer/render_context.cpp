#define STB_IMAGE_IMPLEMENTATION
#include "render_context.hpp"
#include "../rhi/rhi_factory.h"
#include <iostream>

RenderContext::RenderContext(RHI::API api, void *nativeWindow)
    : api(api), nativeWindow(nativeWindow) {}

RenderContext::~RenderContext() { Shutdown(); }

bool RenderContext::Initialize(IFileSystem *fs) {
  std::cout << "[RenderContext] Initializing..." << std::endl;
  fileSystem = fs;

  device = RHI::CreateDevice(api, reinterpret_cast<GLFWwindow *>(nativeWindow));
  if (!device) {
    std::cerr << "[RenderContext] Failed to create RHI device!" << std::endl;
    return false;
  }

  if (!device->Initialize()) {
    std::cerr << "[RenderContext] Failed to initialize RHI device!"
              << std::endl;
    return false;
  }

  textureManager = std::make_unique<TextureManager>();
  textureManager->Initialize(device.get(), fileSystem);

  shaderManager =
      std::make_unique<ShaderManager>(device.get(), fileSystem, api);

  return true;
}

void RenderContext::Shutdown() {
  if (textureManager) {
    textureManager->ClearCache();
    textureManager.reset();
  }

  if (shaderManager) {
    shaderManager->Clear();
    shaderManager.reset();
  }

  if (device) {
    device->Shutdown();
    device.reset();
  }
}
