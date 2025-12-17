#define STB_IMAGE_IMPLEMENTATION
#include "render_context.hpp"
#include "../rhi/rhi_factory.h"
#include <iostream>

RenderContext::RenderContext(RHI::API api, void *nativeWindow)
    : api(api), nativeWindow(nativeWindow) {}

RenderContext::~RenderContext() { Shutdown(); }

bool RenderContext::Initialize(IFileSystem *fs) {
  std::cout << "[RenderContext] Initializing..." << std::endl;
  this->fileSystem = fs;

  // Create RHI Device
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

  // Initialize Managers
  // Note: TextureManager singleton removal is pending, so we might need to
  // adjust this later For now, we create a local instance (once refactored to
  // not be singleton) or we just hold the pointer if it remains singleton
  // temporarily? The plan says "Refactor TextureManager (Remove Singleton)". So
  // we assume we will 'new' it here.

  // We can't instantiate TextureManager if its constructor is private.
  // I will modify TextureManager in the next step.
  // Ideally I should refactor TextureManager FIRST or simultaneously.

  // I will write the code assuming TextureManager constructor is public.
  // This file might not compile until I refactor TextureManager.
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
