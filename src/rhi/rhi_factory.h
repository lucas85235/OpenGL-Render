#ifndef RHI_FACTORY_H
#define RHI_FACTORY_H

#include "opengl/opengl_device.hpp"
#include "rhi_device.h"
#include "rhi_types.hpp"
#include "vulkan/vulkan_device.hpp"
#include <iostream>
#include <memory>

struct GLFWwindow;

namespace RHI {

inline std::unique_ptr<IDevice> CreateDevice(API api, GLFWwindow *window) {
  switch (api) {
  case API::OpenGL: {
    auto device = std::make_unique<OpenGLDevice>();
    if (!device->Initialize()) {
      std::cerr << "[RHI] Failed to initialize OpenGL device" << std::endl;
      return nullptr;
    }
    return device;
  }
  case API::Vulkan: {
    auto device = std::make_unique<VulkanDevice>();
    device->SetWindow(window);
    if (!device->Initialize()) {
      std::cerr << "[RHI] Failed to initialize Vulkan device" << std::endl;
      return nullptr;
    }
    return device;
  }
  default:
    std::cerr << "[RHI] Unknown API type" << std::endl;
    return nullptr;
  }
}

} // namespace RHI

#endif // RHI_FACTORY_H
