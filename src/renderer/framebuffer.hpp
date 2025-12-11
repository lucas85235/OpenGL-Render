#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "../rhi/rhi_device.h"
#include <iostream>

class FrameBuffer {
private:
  RHI::IDevice *device = nullptr;
  RHI::FramebufferHandle fbo;
  RHI::TextureHandle colorTexture;
  int width, height;
  bool initialized = false;

public:
  FrameBuffer(RHI::IDevice *rhiDevice, int w = 800, int h = 600)
      : device(rhiDevice), width(w), height(h) {}

  ~FrameBuffer() { Cleanup(); }

  bool Init() {
    if (initialized) {
      std::cerr << "[FrameBuffer] Already initialized" << std::endl;
      return false;
    }
    if (!device) {
      std::cerr << "[FrameBuffer] No device set" << std::endl;
      return false;
    }

    // Create color texture first
    RHI::TextureDescriptor texDesc;
    texDesc.type = RHI::TextureType::Texture2D;
    texDesc.format = RHI::TextureFormat::RGBA16F;
    texDesc.width = width;
    texDesc.height = height;
    texDesc.mipLevels = 1;
    colorTexture = device->CreateTexture(texDesc);

    // Create framebuffer with attachment
    RHI::FramebufferDescriptor desc;
    desc.width = width;
    desc.height = height;
    RHI::RenderTargetAttachment colorAttachment;
    colorAttachment.texture = colorTexture;
    desc.colorAttachments.push_back(colorAttachment);
    desc.hasDepth = true;

    fbo = device->CreateFramebuffer(desc);
    if (!RHI::IsValid(fbo)) {
      std::cerr << "[FrameBuffer] Failed to create framebuffer" << std::endl;
      return false;
    }

    initialized = true;
    std::cout << "[FrameBuffer] Initialized (" << width << "x" << height << ")"
              << std::endl;
    return true;
  }

  void Bind() {
    if (!initialized || !device)
      return;
    device->BindFramebuffer(fbo);
  }

  void Unbind() {
    if (device)
      device->BindFramebuffer(RHI::FramebufferHandle{0});
  }

  void Resize(int w, int h) {
    if (!initialized || !device)
      return;
    width = w;
    height = h;
    device->ResizeFramebuffer(fbo, w, h);
    colorTexture =
        device->GetFramebufferTexture(fbo, RHI::FramebufferAttachment::Color0);
    std::cout << "[FrameBuffer] Resized to " << width << "x" << height
              << std::endl;
  }

  RHI::TextureHandle GetTexture() const { return colorTexture; }
  RHI::FramebufferHandle GetHandle() const { return fbo; }
  int GetWidth() const { return width; }
  int GetHeight() const { return height; }
  bool IsInitialized() const { return initialized; }

  void Cleanup() {
    if (!initialized || !device)
      return;
    device->DestroyFramebuffer(fbo);
    fbo = RHI::FramebufferHandle{0};
    colorTexture = RHI::TextureHandle{0};
    initialized = false;
    std::cout << "[FrameBuffer] Cleaned up" << std::endl;
  }

  FrameBuffer(const FrameBuffer &) = delete;
  FrameBuffer &operator=(const FrameBuffer &) = delete;

  FrameBuffer(FrameBuffer &&other) noexcept
      : device(other.device), fbo(other.fbo), colorTexture(other.colorTexture),
        width(other.width), height(other.height),
        initialized(other.initialized) {
    other.device = nullptr;
    other.fbo = RHI::FramebufferHandle{0};
    other.colorTexture = RHI::TextureHandle{0};
    other.initialized = false;
  }

  FrameBuffer &operator=(FrameBuffer &&other) noexcept {
    if (this != &other) {
      Cleanup();
      device = other.device;
      fbo = other.fbo;
      colorTexture = other.colorTexture;
      width = other.width;
      height = other.height;
      initialized = other.initialized;
      other.device = nullptr;
      other.fbo = RHI::FramebufferHandle{0};
      other.colorTexture = RHI::TextureHandle{0};
      other.initialized = false;
    }
    return *this;
  }
};

#endif // FRAMEBUFFER_HPP
