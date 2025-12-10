#ifndef FRAMEBUFFER_HPP
#define FRAMEBUFFER_HPP

#include "../rhi/rhi_device.h"
#include <GL/glew.h>
#include <iostream>

class FrameBuffer {
private:
  // RHI resources
  RHI::IDevice *device = nullptr;
  RHI::FramebufferHandle rhiFBO;
  RHI::TextureHandle rhiColorTexture;
  bool useRHI = false;

  // Legacy OpenGL resources
  unsigned int framebuffer = 0;
  unsigned int textureColorbuffer = 0;
  unsigned int rbo = 0;

  int width;
  int height;
  bool initialized;

public:
  FrameBuffer(int w = 800, int h = 600)
      : width(w), height(h), initialized(false) {}

  FrameBuffer(RHI::IDevice *rhiDevice, int w = 800, int h = 600)
      : device(rhiDevice), width(w), height(h), initialized(false) {}

  ~FrameBuffer() { Cleanup(); }

  bool Init() {
    if (initialized) {
      std::cerr << "[FrameBuffer] Already initialized" << std::endl;
      return false;
    }

    if (device) {
      return InitRHI();
    } else {
      return InitOpenGL();
    }
  }

private:
  bool InitRHI() {
    RHI::FramebufferDescriptor desc;
    desc.width = width;
    desc.height = height;
    desc.colorFormats = {RHI::TextureFormat::RGBA16F};
    desc.hasDepth = true;

    rhiFBO = device->CreateFramebuffer(desc);
    if (!RHI::IsValid(rhiFBO)) {
      std::cerr << "[FrameBuffer] RHI: Failed to create framebuffer"
                << std::endl;
      return false;
    }

    rhiColorTexture = device->GetFramebufferTexture(
        rhiFBO, RHI::FramebufferAttachment::Color0);
    initialized = true;
    useRHI = true;

    std::cout << "[FrameBuffer] RHI: Initialized (" << width << "x" << height
              << ")" << std::endl;
    return true;
  }

  bool InitOpenGL() {
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           textureColorbuffer, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                              GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
      std::cerr << "[FrameBuffer] OpenGL: Incomplete framebuffer" << std::endl;
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    initialized = true;

    std::cout << "[FrameBuffer] OpenGL: Initialized (" << width << "x" << height
              << ")" << std::endl;
    return true;
  }

public:
  void Bind() {
    if (!initialized) {
      std::cerr << "[FrameBuffer] Trying to bind uninitialized framebuffer"
                << std::endl;
      return;
    }

    if (useRHI && device) {
      device->BindFramebuffer(rhiFBO);
    } else {
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
      glEnable(GL_DEPTH_TEST);
      glViewport(0, 0, width, height);
    }
  }

  void Unbind() {
    if (useRHI && device) {
      device->BindFramebuffer(RHI::FramebufferHandle{0});
    } else {
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
  }

  void Resize(int w, int h) {
    if (!initialized)
      return;

    width = w;
    height = h;

    if (useRHI && device) {
      device->ResizeFramebuffer(rhiFBO, w, h);
      rhiColorTexture = device->GetFramebufferTexture(
          rhiFBO, RHI::FramebufferAttachment::Color0);
    } else {
      glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, NULL);

      glBindRenderbuffer(GL_RENDERBUFFER, rbo);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width,
                            height);
    }

    std::cout << "[FrameBuffer] Resized to " << width << "x" << height
              << std::endl;
  }

  unsigned int GetTexture() const {
    // Legacy OpenGL texture ID (for backwards compatibility with renderer that
    // uses glBindTexture)
    return textureColorbuffer;
  }

  RHI::TextureHandle GetRHITexture() const { return rhiColorTexture; }

  unsigned int GetFramebufferId() const { return framebuffer; }

  bool IsUsingRHI() const { return useRHI; }

  void Cleanup() {
    if (!initialized)
      return;

    if (useRHI && device) {
      device->DestroyFramebuffer(rhiFBO);
      rhiFBO = RHI::FramebufferHandle{0};
      rhiColorTexture = RHI::TextureHandle{0};
    } else {
      if (framebuffer)
        glDeleteFramebuffers(1, &framebuffer);
      if (textureColorbuffer)
        glDeleteTextures(1, &textureColorbuffer);
      if (rbo)
        glDeleteRenderbuffers(1, &rbo);
      framebuffer = 0;
      textureColorbuffer = 0;
      rbo = 0;
    }

    initialized = false;
    std::cout << "[FrameBuffer] Cleaned up" << std::endl;
  }

  FrameBuffer(const FrameBuffer &) = delete;
  FrameBuffer &operator=(const FrameBuffer &) = delete;

  FrameBuffer(FrameBuffer &&other) noexcept
      : device(other.device), rhiFBO(other.rhiFBO),
        rhiColorTexture(other.rhiColorTexture), useRHI(other.useRHI),
        framebuffer(other.framebuffer),
        textureColorbuffer(other.textureColorbuffer), rbo(other.rbo),
        width(other.width), height(other.height),
        initialized(other.initialized) {
    other.device = nullptr;
    other.rhiFBO = RHI::FramebufferHandle{0};
    other.rhiColorTexture = RHI::TextureHandle{0};
    other.framebuffer = 0;
    other.textureColorbuffer = 0;
    other.rbo = 0;
    other.initialized = false;
  }

  FrameBuffer &operator=(FrameBuffer &&other) noexcept {
    if (this != &other) {
      Cleanup();

      device = other.device;
      rhiFBO = other.rhiFBO;
      rhiColorTexture = other.rhiColorTexture;
      useRHI = other.useRHI;
      framebuffer = other.framebuffer;
      textureColorbuffer = other.textureColorbuffer;
      rbo = other.rbo;
      width = other.width;
      height = other.height;
      initialized = other.initialized;

      other.device = nullptr;
      other.rhiFBO = RHI::FramebufferHandle{0};
      other.rhiColorTexture = RHI::TextureHandle{0};
      other.framebuffer = 0;
      other.textureColorbuffer = 0;
      other.rbo = 0;
      other.initialized = false;
    }
    return *this;
  }
};

#endif // FRAMEBUFFER_HPP
