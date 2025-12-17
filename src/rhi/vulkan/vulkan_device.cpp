#include "vulkan_device.hpp"
#include <cmath>
#include <fstream>

namespace RHI {

VulkanDevice::~VulkanDevice() { Shutdown(); }

bool VulkanDevice::Initialize() {
  if (!window) {
    std::cerr << "[Vulkan] Error: Window not set. Call SetWindow() before "
                 "Initialize()"
              << std::endl;
    return false;
  }

  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createDepthResources();
  createRenderPass();
  createDescriptorSetLayout();
  createFramebuffers();
  createCommandPool();
  createUniformBuffers();
  createDescriptorPool();
  createDummyTexture();
  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();

  // Initialize UBO matrices to identity (scene-wide data)
  memset(cachedUbo.view, 0, sizeof(cachedUbo.view));
  memset(cachedUbo.proj, 0, sizeof(cachedUbo.proj));
  cachedUbo.view[0] = cachedUbo.view[5] = cachedUbo.view[10] =
      cachedUbo.view[15] = 1.0f;
  cachedUbo.proj[0] = cachedUbo.proj[5] = cachedUbo.proj[10] =
      cachedUbo.proj[15] = 1.0f;

  // Initialize push constants (per-draw data)
  memset(cachedPushConstants.model, 0, sizeof(cachedPushConstants.model));
  cachedPushConstants.model[0] = cachedPushConstants.model[5] =
      cachedPushConstants.model[10] = cachedPushConstants.model[15] = 1.0f;

  // Initialize default material (gold-ish) in push constants
  cachedPushConstants.materialColor[0] = 1.0f;   // Albedo R
  cachedPushConstants.materialColor[1] = 0.765f; // Albedo G
  cachedPushConstants.materialColor[2] = 0.336f; // Albedo B
  cachedPushConstants.materialColor[3] = 1.0f;   // Metallic

  cachedPushConstants.materialProps[0] = 0.3f; // Roughness
  cachedPushConstants.materialProps[1] = 1.0f; // AO
  cachedPushConstants.materialProps[2] = 0.0f; // Emission strength
  cachedPushConstants.materialProps[3] = 0.0f; // Flags

  // Initialize light values in UBO (scene-wide)
  cachedUbo.lightDir[0] = -0.2f; // Light direction X
  cachedUbo.lightDir[1] = -1.0f; // Light direction Y
  cachedUbo.lightDir[2] = -0.3f; // Light direction Z
  cachedUbo.lightDir[3] = 3.0f;  // Light intensity

  cachedUbo.lightColor[0] = 1.0f; // Light color R
  cachedUbo.lightColor[1] = 0.9f; // Light color G
  cachedUbo.lightColor[2] = 0.8f; // Light color B
  cachedUbo.lightColor[3] = 1.0f; // Unused

  cachedUbo.viewPos[0] = 0.0f; // View position X
  cachedUbo.viewPos[1] = 0.0f; // View position Y
  cachedUbo.viewPos[2] = 5.0f; // View position Z
  cachedUbo.viewPos[3] = 1.0f; // Unused

  // Initialize point lights
  cachedUbo.numPointLights = 0;
  memset(cachedUbo.pointLights, 0, sizeof(cachedUbo.pointLights));

  std::cout << "[Vulkan] Device initialized successfully" << std::endl;
  return true;
}

void VulkanDevice::Shutdown() {
  if (device == VK_NULL_HANDLE)
    return;

  std::cout << "[Vulkan] Shutting down device..." << std::endl;
  WaitIdle();

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);

  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  vkDestroyRenderPass(device, renderPass, nullptr);

  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);

  if (descriptorPool != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  if (descriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  for (size_t i = 0; i < uniformBuffers.size(); i++) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }

  for (auto &pair : buffers) {
    vkDestroyBuffer(device, pair.second.buffer, nullptr);
    vkFreeMemory(device, pair.second.memory, nullptr);
  }

  for (auto &pair : textures) {
    vkDestroyImageView(device, pair.second.imageView, nullptr);
    vkDestroyImage(device, pair.second.image, nullptr);
    vkFreeMemory(device, pair.second.memory, nullptr);
  }

  for (auto &pair : samplers) {
    vkDestroySampler(device, pair.second.sampler, nullptr);
  }

  for (auto &pair : pipelines) {
    vkDestroyPipeline(device, pair.second.pipeline, nullptr);
    vkDestroyPipelineLayout(device, pair.second.layout, nullptr);
  }

  for (auto &pair : shaders) {
    vkDestroyShaderModule(device, pair.second.vertModule, nullptr);
    vkDestroyShaderModule(device, pair.second.fragModule, nullptr);
  }

  vkDestroyDevice(device, nullptr);
  device = VK_NULL_HANDLE;

  if (enableValidationLayers && debugMessenger != VK_NULL_HANDLE) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    debugMessenger = VK_NULL_HANDLE;
  }

  if (surface != VK_NULL_HANDLE) {
    vkDestroySurfaceKHR(instance, surface, nullptr);
    surface = VK_NULL_HANDLE;
  }

  if (instance != VK_NULL_HANDLE) {
    vkDestroyInstance(instance, nullptr);
    instance = VK_NULL_HANDLE;
  }

  std::cout << "[Vulkan] Device shutdown complete" << std::endl;
}

DeviceInfo VulkanDevice::GetDeviceInfo() const {
  DeviceInfo info;

  if (physicalDevice == VK_NULL_HANDLE)
    return info;

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(physicalDevice, &props);

  info.vendorName = "Vulkan";
  info.rendererName = props.deviceName;
  info.apiVersion = std::to_string(VK_VERSION_MAJOR(props.apiVersion)) + "." +
                    std::to_string(VK_VERSION_MINOR(props.apiVersion)) + "." +
                    std::to_string(VK_VERSION_PATCH(props.apiVersion));
  info.maxTextureSize = props.limits.maxImageDimension2D;
  info.maxTextureUnits = props.limits.maxDescriptorSetSampledImages;
  info.maxVertexAttributes = props.limits.maxVertexInputAttributes;
  info.maxUniformBufferSize = props.limits.maxUniformBufferRange;
  info.maxColorAttachments = props.limits.maxColorAttachments;

  VkPhysicalDeviceFeatures features;
  vkGetPhysicalDeviceFeatures(physicalDevice, &features);
  info.supportsGeometryShader = features.geometryShader;
  info.supportsTessellation = features.tessellationShader;
  info.supportsCompute = true;

  return info;
}

bool VulkanDevice::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;
    for (const auto &layerProperties : availableLayers) {
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound)
      return false;
  }
  return true;
}

std::vector<const char *> VulkanDevice::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions =
      glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions,
                                       glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void VulkanDevice::createInstance() {
  bool useValidationLayers = enableValidationLayers;

  if (useValidationLayers && !checkValidationLayerSupport()) {
    std::cerr << "[Vulkan] Validation layers not available, disabling"
              << std::endl;
    useValidationLayers = false;
  }

  VkApplicationInfo appInfo{};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "RHI Application";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "RHI Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_2;

  VkInstanceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;

  auto extensions = getRequiredExtensions();
  createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
  if (useValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    debugCreateInfo.sType =
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;
    createInfo.pNext = &debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create instance");
  }

  std::cout << "[Vulkan] Instance created" << std::endl;
}

void VulkanDevice::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debugCallback;

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to set up debug messenger" << std::endl;
  }
}

void VulkanDevice::createSurface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create window surface");
  }
  std::cout << "[Vulkan] Surface created" << std::endl;
}

QueueFamilyIndices VulkanDevice::findQueueFamilies(VkPhysicalDevice dev) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &presentSupport);
    if (presentSupport) {
      indices.presentFamily = i;
    }

    if (indices.isComplete())
      break;
    i++;
  }

  return indices;
}

bool VulkanDevice::checkDeviceExtensionSupport(VkPhysicalDevice dev) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(dev, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                           deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

SwapChainSupportDetails
VulkanDevice::querySwapChainSupport(VkPhysicalDevice dev) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount, nullptr);
  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount,
                                            nullptr);
  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentModeCount,
                                              details.presentModes.data());
  }

  return details;
}

bool VulkanDevice::isDeviceSuitable(VkPhysicalDevice dev) {
  QueueFamilyIndices indices = findQueueFamilies(dev);

  bool extensionsSupported = checkDeviceExtensionSupport(dev);

  bool swapChainAdequate = false;
  if (extensionsSupported) {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(dev);
    swapChainAdequate = !swapChainSupport.formats.empty() &&
                        !swapChainSupport.presentModes.empty();
  }

  return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

void VulkanDevice::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error(
        "[Vulkan] Failed to find GPUs with Vulkan support");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &dev : devices) {
    if (isDeviceSuitable(dev)) {
      physicalDevice = dev;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("[Vulkan] Failed to find a suitable GPU");
  }

  VkPhysicalDeviceProperties props;
  vkGetPhysicalDeviceProperties(physicalDevice, &props);
  std::cout << "[Vulkan] Selected GPU: " << props.deviceName << std::endl;
}

void VulkanDevice::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(),
                                            indices.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures{};

  VkDeviceCreateInfo createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
  } else {
    createInfo.enabledLayerCount = 0;
  }

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create logical device");
  }

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

  std::cout << "[Vulkan] Logical device created" << std::endl;
}

VkSurfaceFormatKHR VulkanDevice::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &formats) {
  for (const auto &format : formats) {
    if (format.format == VK_FORMAT_B8G8R8A8_SRGB &&
        format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return format;
    }
  }
  return formats[0];
}

VkPresentModeKHR VulkanDevice::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &presentModes) {
  for (const auto &mode : presentModes) {
    if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
      return mode;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
VulkanDevice::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  }

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);

  VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                             static_cast<uint32_t>(height)};
  actualExtent.width =
      std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                 capabilities.maxImageExtent.width);
  actualExtent.height =
      std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                 capabilities.maxImageExtent.height);

  return actualExtent;
}

void VulkanDevice::createSwapChain() {
  SwapChainSupportDetails swapChainSupport =
      querySwapChainSupport(physicalDevice);

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  VkSwapchainCreateInfoKHR createInfo{};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = surface;
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = extent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
  uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                   indices.presentFamily.value()};

  if (indices.graphicsFamily != indices.presentFamily) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain = VK_NULL_HANDLE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create swap chain");
  }

  vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
  swapChainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapChain, &imageCount,
                          swapChainImages.data());

  swapChainImageFormat = surfaceFormat.format;
  swapChainExtent = extent;

  std::cout << "[Vulkan] Swap chain created (" << extent.width << "x"
            << extent.height << ")" << std::endl;
}

void VulkanDevice::createImageViews() {
  swapChainImageViews.resize(swapChainImages.size());

  for (size_t i = 0; i < swapChainImages.size(); i++) {
    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = swapChainImages[i];
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = swapChainImageFormat;
    createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    createInfo.subresourceRange.baseMipLevel = 0;
    createInfo.subresourceRange.levelCount = 1;
    createInfo.subresourceRange.baseArrayLayer = 0;
    createInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device, &createInfo, nullptr,
                          &swapChainImageViews[i]) != VK_SUCCESS) {
      throw std::runtime_error("[Vulkan] Failed to create image views");
    }
  }
}

void VulkanDevice::createDepthResources() {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = swapChainExtent.width;
  imageInfo.extent.height = swapChainExtent.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = depthFormat;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

  if (vkCreateImage(device, &imageInfo, nullptr, &depthImage) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create depth image");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, depthImage, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;

  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  uint32_t memoryTypeIndex = 0;
  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((memRequirements.memoryTypeBits & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags &
         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
      memoryTypeIndex = i;
      break;
    }
  }
  allocInfo.memoryTypeIndex = memoryTypeIndex;

  if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to allocate depth image memory");
  }

  vkBindImageMemory(device, depthImage, depthImageMemory, 0);

  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = depthImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = depthFormat;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageView) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create depth image view");
  }

  std::cout << "[Vulkan] Depth buffer created" << std::endl;
}

void VulkanDevice::createRenderPass() {
  // Color attachment
  VkAttachmentDescription colorAttachment{};
  colorAttachment.format = swapChainImageFormat;
  colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef{};
  colorAttachmentRef.attachment = 0;
  colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  // Depth attachment
  VkAttachmentDescription depthAttachment{};
  depthAttachment.format = depthFormat;
  depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
  depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout =
      VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef{};
  depthAttachmentRef.attachment = 1;
  depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 2> attachments = {colorAttachment,
                                                        depthAttachment};
  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create render pass");
  }

  std::cout << "[Vulkan] Render pass created" << std::endl;
}

void VulkanDevice::createFramebuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    std::array<VkImageView, 2> attachments = {swapChainImageViews[i],
                                              depthImageView};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments = attachments.data();
    framebufferInfo.width = swapChainExtent.width;
    framebufferInfo.height = swapChainExtent.height;
    framebufferInfo.layers = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr,
                            &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("[Vulkan] Failed to create framebuffer");
    }
  }

  std::cout << "[Vulkan] Framebuffers created" << std::endl;
}

void VulkanDevice::createCommandPool() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  VkCommandPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  poolInfo.queueFamilyIndex = indices.graphicsFamily.value();

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create command pool");
  }

  std::cout << "[Vulkan] Command pool created" << std::endl;
}

void VulkanDevice::createCommandBuffers() {
  commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = commandPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to allocate command buffers");
  }

  std::cout << "[Vulkan] Command buffers allocated" << std::endl;
}

void VulkanDevice::createSyncObjects() {
  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo{};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
            VK_SUCCESS) {
      throw std::runtime_error("[Vulkan] Failed to create sync objects");
    }
  }

  std::cout << "[Vulkan] Sync objects created" << std::endl;
}

void VulkanDevice::createDescriptorSetLayout() {
  // Binding 0: UBO for scene-wide data
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  // Bindings 1-6: Texture samplers for PBR materials
  constexpr uint32_t NUM_TEXTURE_SLOTS = 6;
  std::array<VkDescriptorSetLayoutBinding, 1 + NUM_TEXTURE_SLOTS> bindings;
  bindings[0] = uboLayoutBinding;

  for (uint32_t i = 0; i < NUM_TEXTURE_SLOTS; i++) {
    VkDescriptorSetLayoutBinding &texBinding = bindings[1 + i];
    texBinding.binding =
        1 + i; // 1=diffuse, 2=normal, 3=metallic, 4=roughness, 5=ao, 6=emission
    texBinding.descriptorCount = 1;
    texBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    texBinding.pImmutableSamplers = nullptr;
    texBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                                  &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create descriptor set layout");
  }
}

void VulkanDevice::createUniformBuffers() {
  VkDeviceSize bufferSize = sizeof(UniformBufferObject);

  uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
  uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    // Create standard buffer but map persistently
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &bufferInfo, nullptr, &uniformBuffers[i]);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, uniformBuffers[i], &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex =
        findMemoryType(memRequirements.memoryTypeBits,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    vkAllocateMemory(device, &allocInfo, nullptr, &uniformBuffersMemory[i]);
    vkBindBufferMemory(device, uniformBuffers[i], uniformBuffersMemory[i], 0);

    vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0,
                &uniformBuffersMapped[i]);
  }
}

void VulkanDevice::createDescriptorPool() {
  // Increase pool sizes to support multiple textures per frame
  constexpr uint32_t MAX_SAMPLERS = 100;

  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = MAX_SAMPLERS;

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = MAX_SAMPLERS;

  if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create descriptor pool");
  }
}

void VulkanDevice::createDescriptorSets() {
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             descriptorSetLayout);
  VkDescriptorSetAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool = descriptorPool;
  allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  allocInfo.pSetLayouts = layouts.data();

  descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to allocate descriptor sets");
  }

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = uniformBuffers[i];
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSets[i];
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

    // Initialize all 6 texture bindings (1-6) with dummy texture
    VkDescriptorImageInfo dummyImageInfo{};
    dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    dummyImageInfo.imageView = dummyImageView;
    dummyImageInfo.sampler = dummySampler;

    for (uint32_t binding = 1; binding <= 6; binding++) {
      VkWriteDescriptorSet texWrite{};
      texWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      texWrite.dstSet = descriptorSets[i];
      texWrite.dstBinding = binding;
      texWrite.dstArrayElement = 0;
      texWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
      texWrite.descriptorCount = 1;
      texWrite.pImageInfo = &dummyImageInfo;

      vkUpdateDescriptorSets(device, 1, &texWrite, 0, nullptr);
    }
  }

  std::cout << "[Vulkan] Descriptor sets initialized with dummy textures"
            << std::endl;
}

void VulkanDevice::createDummyTexture() {
  // Create a 1x1 white texture for unused descriptor bindings
  uint32_t whitePixel = 0xFFFFFFFF;

  VkDeviceSize imageSize = 4; // 1x1 RGBA
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  // Create staging buffer inline (same pattern as CreateTexture)
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = imageSize;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create dummy staging buffer" << std::endl;
    return;
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to allocate dummy staging buffer memory"
              << std::endl;
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    return;
  }
  vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

  void *data;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, &whitePixel, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);

  createImage(1, 1, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
              VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, dummyImage,
              dummyImageMemory);

  transitionImageLayout(dummyImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stagingBuffer, dummyImage, 1, 1);
  transitionImageLayout(dummyImage, VK_FORMAT_R8G8B8A8_SRGB,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  dummyImageView = createImageView(dummyImage, VK_FORMAT_R8G8B8A8_SRGB);

  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable = VK_FALSE;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

  if (vkCreateSampler(device, &samplerInfo, nullptr, &dummySampler) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create dummy sampler" << std::endl;
  }

  std::cout << "[Vulkan] Dummy texture created" << std::endl;
}

void VulkanDevice::updateUniformBuffer(uint32_t currentImage) {
  // This is where we would copy data from host internal state to the mapped
  // uniform buffer For now we will update it in SetUniform calls directly or
  // let the user do it
}

uint32_t VulkanDevice::findMemoryType(uint32_t typeFilter,
                                      VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags &
                                    properties) == properties) {
      return i;
    }
  }

  throw std::runtime_error("[Vulkan] Failed to find suitable memory type");
}

// Helper functions
VkCommandBuffer VulkanDevice::beginSingleTimeCommands() {
  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool = commandPool;
  allocInfo.commandBufferCount = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void VulkanDevice::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void VulkanDevice::createImage(uint32_t width, uint32_t height, VkFormat format,
                               VkImageTiling tiling, VkImageUsageFlags usage,
                               VkMemoryPropertyFlags properties, VkImage &image,
                               VkDeviceMemory &imageMemory) {
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = width;
  imageInfo.extent.height = height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.format = format;
  imageInfo.tiling = tiling;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage = usage;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create image");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  // Use dedicated allocation to prevent memory overlap on Intel UMA
  VkMemoryDedicatedAllocateInfo dedicatedInfo{};
  dedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
  dedicatedInfo.image = image;
  dedicatedInfo.buffer = VK_NULL_HANDLE;

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = &dedicatedInfo; // Chain dedicated allocation
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to allocate image memory");
  }

  vkBindImageMemory(device, image, imageMemory, 0);

  std::cout << "[Vulkan] createImage: " << width << "x" << height
            << " format=" << format
            << " image=" << reinterpret_cast<uint64_t>(image)
            << " mem=" << reinterpret_cast<uint64_t>(imageMemory)
            << " size=" << memRequirements.size << std::endl;
}

void VulkanDevice::transitionImageLayout(VkImage image, VkFormat format,
                                         VkImageLayout oldLayout,
                                         VkImageLayout newLayout) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;

  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED &&
      newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else {
    throw std::invalid_argument("[Vulkan] Unsupported layout transition!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void VulkanDevice::copyBufferToImage(VkBuffer buffer, VkImage image,
                                     uint32_t width, uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;   // 0 = tightly packed (no padding between rows)
  region.bufferImageHeight = 0; // 0 = tightly packed (contiguous image data)
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

VkImageView VulkanDevice::createImageView(VkImage image, VkFormat format) {
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = image;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = format;
  // Explicit component mapping for driver compatibility
  viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = 1;

  VkImageView imageView;
  if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create texture image view");
  }

  return imageView;
}

// Buffer implementations
BufferHandle VulkanDevice::CreateBuffer(const BufferDescriptor &desc) {
  VulkanBuffer vkBuffer;
  vkBuffer.size = desc.size;
  vkBuffer.type = desc.type;

  VkBufferUsageFlags usage = 0;
  switch (desc.type) {
  case BufferType::Vertex:
    usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    break;
  case BufferType::Index:
    usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    break;
  case BufferType::Uniform:
    usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    break;
  }

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = desc.size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &vkBuffer.buffer) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create buffer" << std::endl;
    return BufferHandle{0};
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, vkBuffer.buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &vkBuffer.memory) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to allocate buffer memory" << std::endl;
    vkDestroyBuffer(device, vkBuffer.buffer, nullptr);
    return BufferHandle{0};
  }

  vkBindBufferMemory(device, vkBuffer.buffer, vkBuffer.memory, 0);

  if (desc.data) {
    void *mappedData;
    vkMapMemory(device, vkBuffer.memory, 0, desc.size, 0, &mappedData);
    memcpy(mappedData, desc.data, desc.size);
    vkUnmapMemory(device, vkBuffer.memory);
  }

  BufferHandle handle{nextId++};
  buffers[handle.id] = vkBuffer;
  return handle;
}

void VulkanDevice::UpdateBuffer(BufferHandle buffer, const void *data,
                                uint32_t size, uint32_t offset) {
  auto it = buffers.find(buffer.id);
  if (it == buffers.end())
    return;

  void *mappedData;
  vkMapMemory(device, it->second.memory, offset, size, 0, &mappedData);
  memcpy(mappedData, data, size);
  vkUnmapMemory(device, it->second.memory);
}

void VulkanDevice::DestroyBuffer(BufferHandle buffer) {
  auto it = buffers.find(buffer.id);
  if (it != buffers.end()) {
    vkDestroyBuffer(device, it->second.buffer, nullptr);
    vkFreeMemory(device, it->second.memory, nullptr);
    buffers.erase(it);
  }
}

TextureHandle VulkanDevice::CreateTexture(const TextureDescriptor &desc) {
  VulkanTexture vkTexture{};
  vkTexture.width = desc.width;
  vkTexture.height = desc.height;
  vkTexture.depth = desc.depth;
  vkTexture.format = toVkFormat(desc.format);
  vkTexture.isCubemap = (desc.type == TextureType::TextureCube);
  vkTexture.arrayLayers = vkTexture.isCubemap ? 6 : 1;

  std::cout << "[Vulkan] CreateTexture: " << desc.width << "x" << desc.height
            << " format=" << static_cast<int>(desc.format)
            << " type=" << (vkTexture.isCubemap ? "Cubemap" : "2D")
            << std::endl;

  // Create the image
  VkImageCreateInfo imageInfo{};
  imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width = desc.width;
  imageInfo.extent.height = desc.height;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = vkTexture.arrayLayers;
  imageInfo.format = vkTexture.format;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage =
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkTexture.isCubemap) {
    imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
  }

  if (vkCreateImage(device, &imageInfo, nullptr, &vkTexture.image) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create image" << std::endl;
    return TextureHandle{0};
  }

  // Allocate memory for the image
  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, vkTexture.image, &memRequirements);

  VkMemoryDedicatedAllocateInfo dedicatedInfo{};
  dedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
  dedicatedInfo.image = vkTexture.image;
  dedicatedInfo.buffer = VK_NULL_HANDLE;

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.pNext = &dedicatedInfo;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &vkTexture.memory) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to allocate image memory" << std::endl;
    vkDestroyImage(device, vkTexture.image, nullptr);
    return TextureHandle{0};
  }

  vkBindImageMemory(device, vkTexture.image, vkTexture.memory, 0);

  // Transition all layers to SHADER_READ_ONLY (faces will be updated
  // individually)
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = vkTexture.image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = vkTexture.arrayLayers;
  barrier.srcAccessMask = 0;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);

  // If data is provided for 2D texture, upload it
  if (desc.data && !vkTexture.isCubemap) {
    VkDeviceSize imageSize = static_cast<VkDeviceSize>(desc.width) *
                             static_cast<VkDeviceSize>(desc.height) * 4;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);

    VkMemoryRequirements bufMemReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &bufMemReq);

    VkMemoryAllocateInfo bufAllocInfo{};
    bufAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    bufAllocInfo.allocationSize = bufMemReq.size;
    bufAllocInfo.memoryTypeIndex = findMemoryType(
        bufMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &bufAllocInfo, nullptr, &stagingBufferMemory);
    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    void *mappedData;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
    memcpy(mappedData, desc.data, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    // Transition to TRANSFER_DST, copy, then back to SHADER_READ
    VkCommandBuffer cmd = beginSingleTimeCommands();

    VkImageMemoryBarrier toDst{};
    toDst.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toDst.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toDst.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toDst.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toDst.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toDst.image = vkTexture.image;
    toDst.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toDst.subresourceRange.baseMipLevel = 0;
    toDst.subresourceRange.levelCount = 1;
    toDst.subresourceRange.baseArrayLayer = 0;
    toDst.subresourceRange.layerCount = 1;
    toDst.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    toDst.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &toDst);

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {desc.width, desc.height, 1};

    vkCmdCopyBufferToImage(cmd, stagingBuffer, vkTexture.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier toRead{};
    toRead.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toRead.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    toRead.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    toRead.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toRead.image = vkTexture.image;
    toRead.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    toRead.subresourceRange.baseMipLevel = 0;
    toRead.subresourceRange.levelCount = 1;
    toRead.subresourceRange.baseArrayLayer = 0;
    toRead.subresourceRange.layerCount = 1;
    toRead.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    toRead.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &toRead);

    endSingleTimeCommands(cmd);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
  }

  // Create the image view
  VkImageViewCreateInfo viewInfo{};
  viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image = vkTexture.image;
  viewInfo.viewType =
      vkTexture.isCubemap ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = vkTexture.format;
  viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
  viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  viewInfo.subresourceRange.baseMipLevel = 0;
  viewInfo.subresourceRange.levelCount = 1;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount = vkTexture.arrayLayers;

  if (vkCreateImageView(device, &viewInfo, nullptr, &vkTexture.imageView) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create texture image view" << std::endl;
    vkDestroyImage(device, vkTexture.image, nullptr);
    vkFreeMemory(device, vkTexture.memory, nullptr);
    return TextureHandle{0};
  }

  TextureHandle handle{nextId++};
  textures[handle.id] = vkTexture;
  std::cout << "[Vulkan] Texture created id=" << handle.id
            << (vkTexture.isCubemap ? " (cubemap)" : "") << std::endl;
  return handle;
}

void VulkanDevice::UpdateTexture(TextureHandle texture, const void *data,
                                 uint32_t mipLevel) {
  auto it = textures.find(texture.id);
  if (it == textures.end())
    return;

  // Simplified update: create staging buffer, copy to image
  // Note: Assuming full update of mip 0 for now
  VkDeviceSize imageSize = it->second.width * it->second.height * 4;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  // Create staging buffer (simplified duplication of logic for brevity)
  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = imageSize;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory);
  vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

  void *mappedData;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
  memcpy(mappedData, data, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);

  transitionImageLayout(it->second.image, it->second.format,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stagingBuffer, it->second.image, it->second.width,
                    it->second.height);
  transitionImageLayout(it->second.image, it->second.format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanDevice::UpdateTextureCubeFace(TextureHandle texture,
                                         CubemapFace face, const void *data,
                                         uint32_t mipLevel) {
  auto it = textures.find(texture.id);
  if (it == textures.end())
    return;

  uint32_t faceLayer = static_cast<uint32_t>(face);
  std::cout << "[Vulkan] UpdateTextureCubeFace: face=" << faceLayer
            << " mip=" << mipLevel << std::endl;

  // RGBA16F = 4 components × 2 bytes (half-float) = 8 bytes per pixel
  VkDeviceSize imageSize = it->second.width * it->second.height * 8;

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = imageSize;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vkAllocateMemory(device, &allocInfo, nullptr, &stagingBufferMemory);
  vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

  void *mappedData;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &mappedData);
  memcpy(mappedData, data, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);

  // For 2D textures used as cubemap placeholders, just do regular copy
  // Real cubemap would copy to specific array layer
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = it->second.image;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel = mipLevel;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = faceLayer;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
  barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  VkBufferImageCopy region{};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = mipLevel;
  region.imageSubresource.baseArrayLayer = faceLayer;
  region.imageSubresource.layerCount = 1;
  region.imageOffset = {0, 0, 0};
  region.imageExtent = {it->second.width, it->second.height, 1};

  vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, it->second.image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanDevice::GenerateMipmaps(TextureHandle texture) {
  auto it = textures.find(texture.id);
  if (it == textures.end())
    return;

  VulkanTexture &tex = it->second;

  // FIXME: Currently the VkImage is created with mipLevels=1
  // Generating mipmaps on an image without allocated mip storage is UB
  // This needs to be fixed by creating images with proper mip level count
  std::cout << "[Vulkan] GenerateMipmaps: skipping (image has only 1 mip level)"
            << std::endl;

  // Image is already in SHADER_READ_ONLY_OPTIMAL from CreateTexture
  return;

  int32_t mipWidth = tex.width;
  int32_t mipHeight = tex.height;

  // Calculate number of mip levels
  uint32_t mipLevels = static_cast<uint32_t>(std::floor(
                           std::log2(std::max(mipWidth, mipHeight)))) +
                       1;

  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkImageMemoryBarrier barrier{};
  barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image = tex.image;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.subresourceRange.levelCount = 1;

  for (uint32_t i = 1; i < mipLevels; i++) {
    // Transition previous level to TRANSFER_SRC
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0,
                         nullptr, 1, &barrier);

    // Setup blit from level i-1 to level i
    VkImageBlit blit{};
    blit.srcOffsets[0] = {0, 0, 0};
    blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};
    blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1,
                          mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;

    vkCmdBlitImage(commandBuffer, tex.image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, tex.image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit,
                   VK_FILTER_LINEAR);

    // Transition previous level to SHADER_READ_ONLY
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr,
                         0, nullptr, 1, &barrier);

    if (mipWidth > 1)
      mipWidth /= 2;
    if (mipHeight > 1)
      mipHeight /= 2;
  }

  // Transition last mip level to SHADER_READ_ONLY
  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0,
                       nullptr, 1, &barrier);

  endSingleTimeCommands(commandBuffer);
}

void VulkanDevice::DestroyTexture(TextureHandle texture) {
  auto it = textures.find(texture.id);
  if (it != textures.end()) {
    vkDestroyImageView(device, it->second.imageView, nullptr);
    vkDestroyImage(device, it->second.image, nullptr);
    vkFreeMemory(device, it->second.memory, nullptr);
    textures.erase(it);
  }
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDescriptor &desc) {
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = toVkFilter(desc.magFilter);
  samplerInfo.minFilter = toVkFilter(desc.minFilter);
  samplerInfo.addressModeU = toVkAddressMode(desc.wrapS);
  samplerInfo.addressModeV = toVkAddressMode(desc.wrapT);
  samplerInfo.addressModeW = toVkAddressMode(desc.wrapR);
  samplerInfo.anisotropyEnable = VK_FALSE; // Enable if feature supported
  samplerInfo.maxAnisotropy = 1.0f;
  samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable = VK_FALSE;
  samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 0.0f;

  VulkanSampler vkSampler;
  if (vkCreateSampler(device, &samplerInfo, nullptr, &vkSampler.sampler) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create sampler");
  }

  SamplerHandle handle{nextId++};
  samplers[handle.id] = vkSampler;
  return handle;
}

void VulkanDevice::DestroySampler(SamplerHandle sampler) {
  auto it = samplers.find(sampler.id);
  if (it != samplers.end()) {
    vkDestroySampler(device, it->second.sampler, nullptr);
    samplers.erase(it);
  }
}

// Helper function to read SPIR-V binary file
static std::vector<uint32_t> readSpirvFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    return {};
  }

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

  file.seekg(0);
  file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
  file.close();

  return buffer;
}

ShaderHandle
VulkanDevice::CreateShader(const std::vector<ShaderDescriptor> &stages) {
  VulkanShader vkShader{};

  std::vector<uint32_t> vertCode;
  std::vector<uint32_t> fragCode;

  // Extract SPIR-V from descriptors or fallback to file loading
  for (const auto &stage : stages) {
    if (stage.useSPIRV && !stage.spirvBinary.empty()) {
      // Use SPIR-V directly from descriptor
      if (stage.stage == ShaderStage::Vertex) {
        vertCode = stage.spirvBinary;
        std::cout << "[Vulkan] Using SPIR-V from descriptor (vertex)"
                  << std::endl;
      } else if (stage.stage == ShaderStage::Fragment) {
        fragCode = stage.spirvBinary;
        std::cout << "[Vulkan] Using SPIR-V from descriptor (fragment)"
                  << std::endl;
      }
    }
  }

  // Fallback: load from files if descriptors didn't provide SPIR-V
  if (vertCode.empty()) {
    std::cerr
        << "[Vulkan] WARNING: No vertex SPIR-V provided, using fallback shader"
        << std::endl;
    vertCode = readSpirvFile("shaders/cube.vert.spv");
  }
  if (fragCode.empty()) {
    std::cerr << "[Vulkan] WARNING: No fragment SPIR-V provided, using "
                 "fallback shader"
              << std::endl;
    fragCode = readSpirvFile("shaders/cube.frag.spv");
  }

  if (vertCode.empty() || fragCode.empty()) {
    std::cerr << "[Vulkan] Failed to load SPIR-V shader files" << std::endl;
    std::cerr << "[Vulkan] Make sure shaders are compiled (check CMake output)"
              << std::endl;
    return ShaderHandle{0};
  }

  std::cout << "[Vulkan] Loaded SPIR-V shaders successfully" << std::endl;

  VkShaderModuleCreateInfo vertInfo{};
  vertInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vertInfo.codeSize = vertCode.size() * sizeof(uint32_t);
  vertInfo.pCode = vertCode.data();

  if (vkCreateShaderModule(device, &vertInfo, nullptr, &vkShader.vertModule) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create vertex shader module" << std::endl;
    return ShaderHandle{0};
  }

  VkShaderModuleCreateInfo fragInfo{};
  fragInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fragInfo.codeSize = fragCode.size() * sizeof(uint32_t);
  fragInfo.pCode = fragCode.data();

  if (vkCreateShaderModule(device, &fragInfo, nullptr, &vkShader.fragModule) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create fragment shader module"
              << std::endl;
    vkDestroyShaderModule(device, vkShader.vertModule, nullptr);
    return ShaderHandle{0};
  }

  ShaderHandle handle{nextId++};
  shaders[handle.id] = vkShader;
  std::cout << "[Vulkan] Shader created" << std::endl;
  return handle;
}

void VulkanDevice::DestroyShader(ShaderHandle shader) {
  auto it = shaders.find(shader.id);
  if (it != shaders.end()) {
    vkDestroyShaderModule(device, it->second.vertModule, nullptr);
    vkDestroyShaderModule(device, it->second.fragModule, nullptr);
    shaders.erase(it);
  }
}

VkFormat getVkFormat(VertexAttributeType type) {
  switch (type) {
  case VertexAttributeType::Float:
    return VK_FORMAT_R32_SFLOAT;
  case VertexAttributeType::Float2:
    return VK_FORMAT_R32G32_SFLOAT;
  case VertexAttributeType::Float3:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case VertexAttributeType::Float4:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  default:
    return VK_FORMAT_R32G32B32_SFLOAT;
  }
}

PipelineHandle VulkanDevice::CreatePipeline(const PipelineDescriptor &desc,
                                            ShaderHandle shader,
                                            const VertexLayout &layout) {
  auto shaderIt = shaders.find(shader.id);
  if (shaderIt == shaders.end()) {
    std::cerr << "[Vulkan] Invalid shader handle for pipeline" << std::endl;
    return PipelineHandle{0};
  }

  VulkanPipeline vkPipeline{};

  // Shader stages
  VkPipelineShaderStageCreateInfo vertStageInfo{};
  vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStageInfo.module = shaderIt->second.vertModule;
  vertStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo fragStageInfo{};
  fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStageInfo.module = shaderIt->second.fragModule;
  fragStageInfo.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo,
                                                    fragStageInfo};

  // Vertex input
  VkVertexInputBindingDescription bindingDesc{};
  bindingDesc.binding = 0;
  bindingDesc.stride = layout.stride;
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  std::vector<VkVertexInputAttributeDescription> attrDescs;
  for (const auto &attr : layout.attributes) {
    VkVertexInputAttributeDescription attrDesc{};
    attrDesc.binding = 0;
    attrDesc.location = attr.location;
    attrDesc.format = getVkFormat(attr.type);
    attrDesc.offset = attr.offset;
    attrDescs.push_back(attrDesc);
  }

  VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount = 1;
  vertexInputInfo.pVertexBindingDescriptions = &bindingDesc;
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attrDescs.size());
  vertexInputInfo.pVertexAttributeDescriptions = attrDescs.data();

  // Input assembly
  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  // Dynamic state
  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  // Viewport state
  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  // Rasterizer
  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;

  // Use descriptor values
  switch (desc.rasterizer.cullMode) {
  case CullMode::None:
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    break;
  case CullMode::Front:
    rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
    break;
  case CullMode::Back:
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    break;
  default:
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
  }

  // Vulkan Y-axis is flipped vs OpenGL, so we invert the frontFace
  rasterizer.frontFace = (desc.rasterizer.frontFace == FrontFace::Clockwise)
                             ? VK_FRONT_FACE_COUNTER_CLOCKWISE
                             : VK_FRONT_FACE_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  // Multisampling
  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // Color blending
  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  // Depth stencil state
  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable =
      desc.depthStencil.depthTestEnable ? VK_TRUE : VK_FALSE;
  depthStencil.depthWriteEnable =
      desc.depthStencil.depthWriteEnable ? VK_TRUE : VK_FALSE;

  switch (desc.depthStencil.depthCompareOp) {
  case CompareOp::Never:
    depthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
    break;
  case CompareOp::Less:
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    break;
  case CompareOp::Equal:
    depthStencil.depthCompareOp = VK_COMPARE_OP_EQUAL;
    break;
  case CompareOp::LessOrEqual:
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    break;
  case CompareOp::Greater:
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER;
    break;
  case CompareOp::NotEqual:
    depthStencil.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
    break;
  case CompareOp::GreaterOrEqual:
    depthStencil.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
    break;
  case CompareOp::Always:
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    break;
  default:
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
  }
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  // Push constant range for per-draw data (model matrix + material)
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags =
      VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstants);

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &vkPipeline.layout) != VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create pipeline layout" << std::endl;
    return PipelineHandle{0};
  }

  // Graphics pipeline
  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = vkPipeline.layout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &vkPipeline.pipeline) != VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create graphics pipeline" << std::endl;
    vkDestroyPipelineLayout(device, vkPipeline.layout, nullptr);
    return PipelineHandle{0};
  }

  PipelineHandle handle{nextId++};
  vkPipeline.shader = shader;
  pipelines[handle.id] = vkPipeline;
  std::cout << "[Vulkan] Graphics pipeline created (id=" << handle.id << ")"
            << std::endl;
  return handle;
}

void VulkanDevice::DestroyPipeline(PipelineHandle pipeline) {
  auto it = pipelines.find(pipeline.id);
  if (it != pipelines.end()) {
    vkDestroyPipeline(device, it->second.pipeline, nullptr);
    vkDestroyPipelineLayout(device, it->second.layout, nullptr);
    pipelines.erase(it);
  }
}

VertexArrayHandle VulkanDevice::CreateVertexArray(BufferHandle vertexBuffer,
                                                  BufferHandle indexBuffer,
                                                  const VertexLayout &layout) {
  VulkanVertexArray vao;
  vao.vertexBuffer = vertexBuffer;
  vao.indexBuffer = indexBuffer;

  VertexArrayHandle handle{nextId++};
  vertexArrays[handle.id] = vao;
  return handle;
}

void VulkanDevice::DestroyVertexArray(VertexArrayHandle vao) {
  vertexArrays.erase(vao.id);
}

FramebufferHandle
VulkanDevice::CreateFramebuffer(const FramebufferDescriptor &desc) {
  VulkanFramebuffer vkFb{};
  vkFb.width = desc.width;
  vkFb.height = desc.height;
  vkFb.hasDepth = desc.hasDepth;

  std::vector<VkAttachmentDescription> attachments;
  std::vector<VkAttachmentReference> colorRefs;
  std::vector<VkImageView> attachmentViews;

  // Color attachments
  for (size_t i = 0; i < desc.colorAttachments.size(); ++i) {
    auto texIt = textures.find(desc.colorAttachments[i].texture.id);
    if (texIt == textures.end()) {
      std::cerr
          << "[Vulkan] CreateFramebuffer: invalid color attachment texture"
          << std::endl;
      continue;
    }

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = texIt->second.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = static_cast<uint32_t>(attachments.size());
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments.push_back(colorAttachment);
    colorRefs.push_back(colorRef);
    attachmentViews.push_back(texIt->second.imageView);
    vkFb.colorAttachments.push_back(desc.colorAttachments[i].texture);
  }

  // Depth attachment
  VkAttachmentReference depthRef{};
  if (desc.hasDepth && IsValid(desc.depthAttachment.texture)) {
    auto texIt = textures.find(desc.depthAttachment.texture.id);
    if (texIt != textures.end()) {
      VkAttachmentDescription depthAttachment{};
      depthAttachment.format = texIt->second.format;
      depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
      depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
      depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      depthAttachment.finalLayout =
          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      depthRef.attachment = static_cast<uint32_t>(attachments.size());
      depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

      attachments.push_back(depthAttachment);
      attachmentViews.push_back(texIt->second.imageView);
      vkFb.depthAttachment = desc.depthAttachment.texture;
    }
  }

  // Create render pass for this framebuffer
  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = static_cast<uint32_t>(colorRefs.size());
  subpass.pColorAttachments = colorRefs.data();
  subpass.pDepthStencilAttachment =
      (desc.hasDepth && IsValid(desc.depthAttachment.texture)) ? &depthRef
                                                               : nullptr;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments = attachments.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpass;
  renderPassInfo.dependencyCount = 1;
  renderPassInfo.pDependencies = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &vkFb.renderPass) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create framebuffer render pass"
              << std::endl;
    return FramebufferHandle{0};
  }
  vkFb.ownsRenderPass = true;

  // Create framebuffer
  VkFramebufferCreateInfo fbInfo{};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.renderPass = vkFb.renderPass;
  fbInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
  fbInfo.pAttachments = attachmentViews.data();
  fbInfo.width = desc.width;
  fbInfo.height = desc.height;
  fbInfo.layers = 1;

  if (vkCreateFramebuffer(device, &fbInfo, nullptr, &vkFb.framebuffer) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create framebuffer" << std::endl;
    vkDestroyRenderPass(device, vkFb.renderPass, nullptr);
    return FramebufferHandle{0};
  }

  FramebufferHandle handle{nextId++};
  framebuffers[handle.id] = vkFb;
  std::cout << "[Vulkan] Framebuffer created (id=" << handle.id << ", "
            << desc.width << "x" << desc.height << ")" << std::endl;
  return handle;
}

void VulkanDevice::AttachTexture(FramebufferHandle framebuffer,
                                 FramebufferAttachment attachment,
                                 TextureHandle texture) {
  // TODO: Implement
}

TextureHandle
VulkanDevice::GetFramebufferTexture(FramebufferHandle framebuffer,
                                    FramebufferAttachment attachment) {
  auto it = framebuffers.find(framebuffer.id);
  if (it == framebuffers.end())
    return TextureHandle{0};

  if (attachment == FramebufferAttachment::Depth ||
      attachment == FramebufferAttachment::DepthStencil) {
    return it->second.depthAttachment;
  }

  int colorIndex = static_cast<int>(attachment) -
                   static_cast<int>(FramebufferAttachment::Color0);
  if (colorIndex >= 0 &&
      colorIndex < static_cast<int>(it->second.colorAttachments.size())) {
    return it->second.colorAttachments[colorIndex];
  }
  return TextureHandle{0};
}

void VulkanDevice::ResizeFramebuffer(FramebufferHandle framebuffer,
                                     uint32_t width, uint32_t height) {
  auto it = framebuffers.find(framebuffer.id);
  if (it == framebuffers.end())
    return;

  // Store old attachment handles
  auto colorAttachments = it->second.colorAttachments;
  auto depthAttachment = it->second.depthAttachment;
  bool hasDepth = it->second.hasDepth;

  // Destroy old framebuffer (but not the render pass if we'll reuse it)
  if (it->second.framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device, it->second.framebuffer, nullptr);
    it->second.framebuffer = VK_NULL_HANDLE;
  }

  // Update dimensions
  it->second.width = width;
  it->second.height = height;

  // Rebuild attachment views
  std::vector<VkImageView> attachmentViews;
  for (const auto &texHandle : colorAttachments) {
    auto texIt = textures.find(texHandle.id);
    if (texIt != textures.end()) {
      attachmentViews.push_back(texIt->second.imageView);
    }
  }
  if (hasDepth && IsValid(depthAttachment)) {
    auto texIt = textures.find(depthAttachment.id);
    if (texIt != textures.end()) {
      attachmentViews.push_back(texIt->second.imageView);
    }
  }

  // Recreate framebuffer
  VkFramebufferCreateInfo fbInfo{};
  fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
  fbInfo.renderPass = it->second.renderPass;
  fbInfo.attachmentCount = static_cast<uint32_t>(attachmentViews.size());
  fbInfo.pAttachments = attachmentViews.data();
  fbInfo.width = width;
  fbInfo.height = height;
  fbInfo.layers = 1;

  if (vkCreateFramebuffer(device, &fbInfo, nullptr, &it->second.framebuffer) !=
      VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to resize framebuffer" << std::endl;
  }
}

void VulkanDevice::DestroyFramebuffer(FramebufferHandle framebuffer) {
  auto it = framebuffers.find(framebuffer.id);
  if (it == framebuffers.end())
    return;

  if (it->second.framebuffer != VK_NULL_HANDLE) {
    vkDestroyFramebuffer(device, it->second.framebuffer, nullptr);
  }
  if (it->second.ownsRenderPass && it->second.renderPass != VK_NULL_HANDLE) {
    vkDestroyRenderPass(device, it->second.renderPass, nullptr);
  }
  framebuffers.erase(it);
}

void VulkanDevice::SetViewport(const Viewport &viewport) {
  cachedViewport = viewport;
  viewportDirty = true;
}

void VulkanDevice::SetScissor(const Scissor &scissor) {
  cachedScissor = scissor;
  scissorDirty = true;
}

void VulkanDevice::DisableScissor() {
  // Set scissor to match viewport (effectively disabling scissor test)
  cachedScissor.x = static_cast<int>(cachedViewport.x);
  cachedScissor.y = static_cast<int>(cachedViewport.y);
  cachedScissor.width = static_cast<uint32_t>(cachedViewport.width);
  cachedScissor.height = static_cast<uint32_t>(cachedViewport.height);
  scissorDirty = true;
}

void VulkanDevice::Clear(bool color, bool depth, bool stencil) {
  // Handled in render pass load operations
}

void VulkanDevice::SetClearColor(const ClearColor &color) {
  clearColor = color;
}

void VulkanDevice::SetClearDepth(float depth) { clearDepthValue = depth; }

void VulkanDevice::BindPipeline(PipelineHandle pipeline) {
  if (pipeline.id == 0) {
    std::cerr << "[Vulkan] BindPipeline called with invalid handle"
              << std::endl;
    return;
  }

  currentPipeline = pipeline;

  auto it = pipelines.find(pipeline.id);
  if (it != pipelines.end()) {
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS, it->second.pipeline);

    // Bind descriptor sets with the pipeline layout
    vkCmdBindDescriptorSets(commandBuffers[currentFrame],
                            VK_PIPELINE_BIND_POINT_GRAPHICS, it->second.layout,
                            0, 1, &descriptorSets[currentFrame], 0, nullptr);
  } else {
    std::cerr << "[Vulkan] BindPipeline: pipeline id=" << pipeline.id
              << " not found" << std::endl;
  }
}

void VulkanDevice::BindVertexArray(VertexArrayHandle vao) {
  // In Vulkan, we store references and bind during draw
  currentVAO = vao;
}

void VulkanDevice::BindFramebuffer(FramebufferHandle framebuffer) {
  // Handle 0 = default framebuffer (swapchain)
  if (framebuffer.id == 0) {
    currentFramebuffer = FramebufferHandle{0};
    return;
  }

  auto it = framebuffers.find(framebuffer.id);
  if (it == framebuffers.end()) {
    std::cerr << "[Vulkan] BindFramebuffer: framebuffer id=" << framebuffer.id
              << " not found" << std::endl;
    return;
  }

  currentFramebuffer = framebuffer;
}

void VulkanDevice::BindTexture(uint32_t slot, TextureHandle texture) {
  // Support texture slots 0-5 mapping to shader bindings 1-6
  // Slot 0=diffuse(binding 1), 1=normal(2), 2=metallic(3), 3=roughness(4),
  // 4=ao(5), 5=emission(6)
  constexpr uint32_t MAX_TEXTURE_SLOTS = 6;
  if (slot >= MAX_TEXTURE_SLOTS) {
    std::cerr << "[Vulkan] BindTexture: slot " << slot << " out of range"
              << std::endl;
    return;
  }

  auto texIt = textures.find(texture.id);
  if (texIt == textures.end()) {
    std::cerr << "[Vulkan] BindTexture: texture id=" << texture.id
              << " not found" << std::endl;
    return;
  }

  std::cout << "[Vulkan] BindTexture: slot=" << slot
            << " -> binding=" << (1 + slot) << ", texId=" << texture.id
            << std::endl;

  // Use dummySampler which is configured for 2D textures with REPEAT mode
  // Don't use samplers.begin() because it might return cubemap sampler with
  // CLAMP_TO_EDGE
  VkSampler texSampler = dummySampler;
  if (texSampler == VK_NULL_HANDLE) {
    std::cerr << "[Vulkan] BindTexture: dummySampler not initialized"
              << std::endl;
    return;
  }

  // Update descriptor set - slot 0 goes to binding 1, etc.
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = texIt->second.imageView;
  imageInfo.sampler = texSampler;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSets[currentFrame];
  descriptorWrite.dstBinding =
      1 + slot; // slot 0 -> binding 1, slot 1 -> binding 2, etc.
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

  // Set texture flag based on slot
  uint32_t currentFlags =
      static_cast<uint32_t>(cachedPushConstants.materialProps[3]);
  currentFlags |=
      (1u << slot); // FLAG_HAS_DIFFUSE=1, NORMAL=2, METALLIC=4, etc.
  cachedPushConstants.materialProps[3] = static_cast<float>(currentFlags);
}

void VulkanDevice::BindSampler(uint32_t slot, SamplerHandle sampler) {
  // Sampler is combined with texture in BindTexture
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              int value) {
  bool updated = false;

  // Get current flags from push constants
  uint32_t currentFlags =
      static_cast<uint32_t>(cachedPushConstants.materialProps[3]);

  // Handle texture presence flags - pack into materialProps.a as bitfield
  if (name.find("hasTextureDiffuse") != std::string::npos) {
    if (value)
      currentFlags |= 1u;
    else
      currentFlags &= ~1u;
    updated = true;
  } else if (name.find("hasTextureNormal") != std::string::npos) {
    if (value)
      currentFlags |= 2u;
    else
      currentFlags &= ~2u;
    updated = true;
  } else if (name.find("hasTextureMetallic") != std::string::npos) {
    if (value)
      currentFlags |= 4u;
    else
      currentFlags &= ~4u;
    updated = true;
  } else if (name.find("hasTextureRoughness") != std::string::npos) {
    if (value)
      currentFlags |= 8u;
    else
      currentFlags &= ~8u;
    updated = true;
  } else if (name.find("hasTextureAO") != std::string::npos) {
    if (value)
      currentFlags |= 16u;
    else
      currentFlags &= ~16u;
    updated = true;
  } else if (name.find("hasTextureEmission") != std::string::npos) {
    if (value)
      currentFlags |= 32u;
    else
      currentFlags &= ~32u;
    updated = true;
  } else if (name.find("useIBL") != std::string::npos) {
    if (value)
      currentFlags |= 64u;
    else
      currentFlags &= ~64u;
    updated = true;
  } else if (name.find("numPointLights") != std::string::npos) {
    cachedUbo.numPointLights = value;
    // Sync UBO to GPU
    if (currentFrame < uniformBuffersMapped.size() &&
        uniformBuffersMapped[currentFrame]) {
      memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
             sizeof(UniformBufferObject));
    }
  }

  if (updated) {
    cachedPushConstants.materialProps[3] = static_cast<float>(currentFlags);
  }
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              float value) {
  bool updated = false;

  if (name.find("roughness") != std::string::npos) {
    cachedPushConstants.materialProps[0] = value;
    updated = true;
  } else if (name.find("metallic") != std::string::npos) {
    cachedPushConstants.materialColor[3] = value;
    updated = true;
  } else if (name.find("ao") != std::string::npos &&
             name.find("emission") == std::string::npos) {
    cachedPushConstants.materialProps[1] = value;
    updated = true;
  } else if (name.find("emissionStrength") != std::string::npos) {
    cachedPushConstants.materialProps[2] = value;
    updated = true;
  } else if (name.find("dirLight.intensity") != std::string::npos) {
    cachedUbo.lightDir[3] = value;
    updated = true;
  } else if (name.find("numPointLights") != std::string::npos) {
    cachedUbo.numPointLights = static_cast<int>(value);
    updated = true;
  } else if (name.find("pointLights[") != std::string::npos) {
    // Extract light index from name like "pointLights[0].intensity"
    size_t start = name.find('[') + 1;
    size_t end = name.find(']');
    if (start != std::string::npos && end != std::string::npos) {
      int idx = std::stoi(name.substr(start, end - start));
      if (idx >= 0 && idx < 4) {
        if (name.find(".intensity") != std::string::npos) {
          cachedUbo.pointLights[idx][3] = value; // pos.w = intensity
          updated = true;
        } else if (name.find(".radius") != std::string::npos) {
          cachedUbo.pointLights[idx][7] = value; // color.w = radius
          updated = true;
        }
      }
    }
  }

  if (updated && currentFrame < uniformBuffersMapped.size() &&
      uniformBuffersMapped[currentFrame]) {
    memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
           sizeof(UniformBufferObject));
  }
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              const float *value, uint32_t count) {
  bool updated = false;

  if (name.find("albedo") != std::string::npos && count >= 3) {
    cachedPushConstants.materialColor[0] = value[0];
    cachedPushConstants.materialColor[1] = value[1];
    cachedPushConstants.materialColor[2] = value[2];
    updated = true;
  } else if (name.find("emission") != std::string::npos && count >= 3) {
    // Emission not in current shader but prepared for future
    updated = true;
  } else if ((name.find("viewPos") != std::string::npos) && count >= 3) {
    cachedUbo.viewPos[0] = value[0];
    cachedUbo.viewPos[1] = value[1];
    cachedUbo.viewPos[2] = value[2];
    updated = true;
  } else if ((name.find("lightDir") != std::string::npos ||
              name.find("dirLight.direction") != std::string::npos) &&
             count >= 3) {
    cachedUbo.lightDir[0] = value[0];
    cachedUbo.lightDir[1] = value[1];
    cachedUbo.lightDir[2] = value[2];
    updated = true;
  } else if ((name.find("lightColor") != std::string::npos ||
              name.find("dirLight.color") != std::string::npos) &&
             count >= 3) {
    cachedUbo.lightColor[0] = value[0];
    cachedUbo.lightColor[1] = value[1];
    cachedUbo.lightColor[2] = value[2];
    updated = true;
  } else if (name.find("pointLights[") != std::string::npos && count >= 3) {
    // Extract light index from name like "pointLights[0].position"
    size_t start = name.find('[') + 1;
    size_t end = name.find(']');
    if (start != std::string::npos && end != std::string::npos) {
      int idx = std::stoi(name.substr(start, end - start));
      if (idx >= 0 && idx < 4) {
        if (name.find(".position") != std::string::npos) {
          cachedUbo.pointLights[idx][0] = value[0]; // pos.x
          cachedUbo.pointLights[idx][1] = value[1]; // pos.y
          cachedUbo.pointLights[idx][2] = value[2]; // pos.z
          updated = true;
        } else if (name.find(".color") != std::string::npos) {
          cachedUbo.pointLights[idx][4] = value[0]; // color.r
          cachedUbo.pointLights[idx][5] = value[1]; // color.g
          cachedUbo.pointLights[idx][6] = value[2]; // color.b
          updated = true;
        }
      }
    }
  }

  if (updated && currentFrame < uniformBuffersMapped.size() &&
      uniformBuffersMapped[currentFrame]) {
    memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
           sizeof(UniformBufferObject));
  }
}

void VulkanDevice::SetUniformMatrix4(ShaderHandle shader,
                                     const std::string &name,
                                     const float *matrix) {
  if (!matrix)
    return;

  // Copy matrix to cached state based on uniform name
  if (name == "model" || name.find("model") != std::string::npos) {
    memcpy(cachedPushConstants.model, matrix, sizeof(float) * 16);
    // Debug: Print model matrix translation to verify transform
    static bool firstModel = true;
    if (firstModel) {
      std::cout << "[Vulkan] First Model Transform: pos=[" << matrix[12] << ", "
                << matrix[13] << ", " << matrix[14] << "]" << std::endl;
      firstModel = false;
    }
  } else if (name == "view" || name.find("view") != std::string::npos) {
    memcpy(cachedUbo.view, matrix, sizeof(float) * 16);
    // Debug: Print view matrix translation (camera position)
    static bool firstView = true;
    if (firstView) {
      std::cout << "[Vulkan] First View Matrix: [" << matrix[12] << ", "
                << matrix[13] << ", " << matrix[14] << "]" << std::endl;
      firstView = false;
    }
  } else if (name == "projection" || name == "proj" ||
             name.find("proj") != std::string::npos) {
    memcpy(cachedUbo.proj, matrix, sizeof(float) * 16);
    // Debug: Print projection matrix to verify values
    static bool firstProj = true;
    if (firstProj) {
      std::cout << "[Vulkan] First Proj Matrix: [" << matrix[0] << ", "
                << matrix[5] << ", " << matrix[10] << ", " << matrix[14] << "]"
                << std::endl;
      firstProj = false;
    }
  }

  // Copy cached UBO to mapped buffer for current frame
  if (currentFrame < uniformBuffersMapped.size() &&
      uniformBuffersMapped[currentFrame]) {
    memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
           sizeof(UniformBufferObject));
  }
}

void VulkanDevice::Draw(const DrawCommand &cmd) {
  auto vaoIt = vertexArrays.find(currentVAO.id);
  if (vaoIt != vertexArrays.end()) {
    auto vbIt = buffers.find(vaoIt->second.vertexBuffer.id);
    if (vbIt != buffers.end()) {
      auto pipeIt = pipelines.find(currentPipeline.id);
      if (pipeIt != pipelines.end()) {
        vkCmdBindDescriptorSets(commandBuffers[currentFrame],
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                pipeIt->second.layout, 0, 1,
                                &descriptorSets[currentFrame], 0, nullptr);
      }
      VkBuffer vbs[] = {vbIt->second.buffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vbs, offsets);
    }
  }

  vkCmdDraw(commandBuffers[currentFrame], cmd.vertexCount, cmd.instanceCount,
            cmd.firstVertex, cmd.firstInstance);
}

void VulkanDevice::DrawIndexed(const DrawIndexedCommand &cmd) {
  auto vaoIt = vertexArrays.find(currentVAO.id);
  if (vaoIt != vertexArrays.end()) {
    auto pipeIt = pipelines.find(currentPipeline.id);
    if (pipeIt != pipelines.end()) {
      vkCmdBindDescriptorSets(commandBuffers[currentFrame],
                              VK_PIPELINE_BIND_POINT_GRAPHICS,
                              pipeIt->second.layout, 0, 1,
                              &descriptorSets[currentFrame], 0, nullptr);
    }

    auto vbIt = buffers.find(vaoIt->second.vertexBuffer.id);
    if (vbIt != buffers.end()) {
      VkBuffer vbs[] = {vbIt->second.buffer};
      VkDeviceSize offsets[] = {0};
      vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vbs, offsets);
    }

    auto ibIt = buffers.find(vaoIt->second.indexBuffer.id);
    if (ibIt != buffers.end()) {
      VkIndexType indexType = (cmd.indexType == IndexType::UInt16)
                                  ? VK_INDEX_TYPE_UINT16
                                  : VK_INDEX_TYPE_UINT32;
      vkCmdBindIndexBuffer(commandBuffers[currentFrame], ibIt->second.buffer, 0,
                           indexType);
    }
  }

  // Sync cached UBO to current frame's buffer (scene-wide data)
  if (currentFrame < uniformBuffersMapped.size() &&
      uniformBuffersMapped[currentFrame]) {
    memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
           sizeof(UniformBufferObject));
  }

  // Push per-draw data via push constants (use cached values)
  auto pipeIt = pipelines.find(currentPipeline.id);
  if (pipeIt != pipelines.end()) {
    vkCmdPushConstants(commandBuffers[currentFrame], pipeIt->second.layout,
                       VK_SHADER_STAGE_VERTEX_BIT |
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(PushConstants), &cachedPushConstants);
  }

  vkCmdDrawIndexed(commandBuffers[currentFrame], cmd.indexCount,
                   cmd.instanceCount, cmd.firstIndex, cmd.vertexOffset,
                   cmd.firstInstance);
}

void VulkanDevice::WaitIdle() {
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }
}

bool VulkanDevice::BeginFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(
      device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    return false;
  }

  vkResetFences(device, 1, &inFlightFences[currentFrame]);

  // Reset all texture bindings to dummy texture for this frame
  // This prevents stale bindings from previous frame
  VkDescriptorImageInfo dummyImageInfo{};
  dummyImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  dummyImageInfo.imageView = dummyImageView;
  dummyImageInfo.sampler = dummySampler;

  std::array<VkWriteDescriptorSet, 6> descriptorWrites{};
  for (uint32_t i = 0; i < 6; i++) {
    descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[i].dstSet = descriptorSets[currentFrame];
    descriptorWrites[i].dstBinding = 1 + i; // bindings 1-6
    descriptorWrites[i].dstArrayElement = 0;
    descriptorWrites[i].descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[i].descriptorCount = 1;
    descriptorWrites[i].pImageInfo = &dummyImageInfo;
  }
  vkUpdateDescriptorSets(device, 6, descriptorWrites.data(), 0, nullptr);

  // Reset material flags for this frame
  cachedPushConstants.materialProps[3] = 0.0f;

  vkResetCommandBuffer(commandBuffers[currentFrame], 0);

  VkCommandBufferBeginInfo beginInfo{};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);

  VkRenderPassBeginInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassInfo.renderPass = renderPass;
  renderPassInfo.framebuffer = swapChainFramebuffers[currentImageIndex];
  renderPassInfo.renderArea.offset = {0, 0};
  renderPassInfo.renderArea.extent = swapChainExtent;

  std::array<VkClearValue, 2> clearValues{};
  clearValues[0].color = {
      {clearColor.r, clearColor.g, clearColor.b, clearColor.a}};
  clearValues[1].depthStencil = {1.0f, 0};

  renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
  renderPassInfo.pClearValues = clearValues.data();

  vkCmdBeginRenderPass(commandBuffers[currentFrame], &renderPassInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // Update cached viewport to match swapchain if not explicitly set
  if (viewportDirty) {
    viewportDirty = false;
  } else {
    cachedViewport.x = 0.0f;
    cachedViewport.y = 0.0f;
    cachedViewport.width = static_cast<int>(swapChainExtent.width);
    cachedViewport.height = static_cast<int>(swapChainExtent.height);
  }

  VkViewport viewport{};
  viewport.x = cachedViewport.x;
  viewport.y = cachedViewport.y;
  viewport.width = static_cast<float>(cachedViewport.width);
  viewport.height = static_cast<float>(cachedViewport.height);
  viewport.minDepth = cachedViewport.minDepth;
  viewport.maxDepth = cachedViewport.maxDepth;
  vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

  if (scissorDirty) {
    scissorDirty = false;
  } else {
    cachedScissor.x = 0;
    cachedScissor.y = 0;
    cachedScissor.width = swapChainExtent.width;
    cachedScissor.height = swapChainExtent.height;
  }

  VkRect2D scissor{};
  scissor.offset = {cachedScissor.x, cachedScissor.y};
  scissor.extent = {cachedScissor.width, cachedScissor.height};
  vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

  // Sync cached UBO to current frame's uniform buffer
  // This ensures that uniforms set in previous frames are available in this
  // frame
  if (currentFrame < uniformBuffersMapped.size() &&
      uniformBuffersMapped[currentFrame]) {
    memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
           sizeof(UniformBufferObject));
  }

  return true;
}

void VulkanDevice::EndFrame() {
  vkCmdEndRenderPass(commandBuffers[currentFrame]);

  if (vkEndCommandBuffer(commandBuffers[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to record command buffer");
  }

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

  VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo,
                    inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to submit draw command buffer");
  }

  VkPresentInfoKHR presentInfo{};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &currentImageIndex;

  vkQueuePresentKHR(presentQueue, &presentInfo);

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

VkFormat VulkanDevice::toVkFormat(TextureFormat format) {
  switch (format) {
  case TextureFormat::RGB8:
    return VK_FORMAT_R8G8B8_UNORM;
  case TextureFormat::RGBA8:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case TextureFormat::SRGB8:
    return VK_FORMAT_R8G8B8A8_SRGB; // RGB8 SRGB not widely supported, use RGBA
  case TextureFormat::SRGB8_Alpha8:
    return VK_FORMAT_R8G8B8A8_SRGB;
  case TextureFormat::RGBA16F:
    return VK_FORMAT_R16G16B16A16_SFLOAT;
  case TextureFormat::Depth24Stencil8:
    return VK_FORMAT_D24_UNORM_S8_UINT;
  default:
    return VK_FORMAT_R8G8B8A8_UNORM;
  }
}

VkFilter VulkanDevice::toVkFilter(TextureFilterMode mode) {
  switch (mode) {
  case TextureFilterMode::Nearest:
    return VK_FILTER_NEAREST;
  case TextureFilterMode::Linear:
    return VK_FILTER_LINEAR;
  default:
    return VK_FILTER_LINEAR;
  }
}

VkSamplerAddressMode VulkanDevice::toVkAddressMode(TextureWrapMode mode) {
  switch (mode) {
  case TextureWrapMode::Repeat:
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  case TextureWrapMode::MirroredRepeat:
    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  case TextureWrapMode::ClampToEdge:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case TextureWrapMode::ClampToBorder:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  default:
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  }
}

VkCompareOp VulkanDevice::toVkCompareOp(CompareOp op) {
  switch (op) {
  case CompareOp::Never:
    return VK_COMPARE_OP_NEVER;
  case CompareOp::Less:
    return VK_COMPARE_OP_LESS;
  case CompareOp::Equal:
    return VK_COMPARE_OP_EQUAL;
  case CompareOp::LessOrEqual:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case CompareOp::Greater:
    return VK_COMPARE_OP_GREATER;
  case CompareOp::NotEqual:
    return VK_COMPARE_OP_NOT_EQUAL;
  case CompareOp::GreaterOrEqual:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case CompareOp::Always:
    return VK_COMPARE_OP_ALWAYS;
  default:
    return VK_COMPARE_OP_LESS;
  }
}

VkBlendFactor VulkanDevice::toVkBlendFactor(BlendFactor factor) {
  switch (factor) {
  case BlendFactor::Zero:
    return VK_BLEND_FACTOR_ZERO;
  case BlendFactor::One:
    return VK_BLEND_FACTOR_ONE;
  case BlendFactor::SrcColor:
    return VK_BLEND_FACTOR_SRC_COLOR;
  case BlendFactor::OneMinusSrcColor:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  case BlendFactor::SrcAlpha:
    return VK_BLEND_FACTOR_SRC_ALPHA;
  case BlendFactor::OneMinusSrcAlpha:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  case BlendFactor::DstAlpha:
    return VK_BLEND_FACTOR_DST_ALPHA;
  case BlendFactor::OneMinusDstAlpha:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  case BlendFactor::DstColor:
    return VK_BLEND_FACTOR_DST_COLOR;
  case BlendFactor::OneMinusDstColor:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  default:
    return VK_BLEND_FACTOR_ZERO;
  }
}

VkBlendOp VulkanDevice::toVkBlendOp(BlendOp op) {
  switch (op) {
  case BlendOp::Add:
    return VK_BLEND_OP_ADD;
  case BlendOp::Subtract:
    return VK_BLEND_OP_SUBTRACT;
  case BlendOp::ReverseSubtract:
    return VK_BLEND_OP_REVERSE_SUBTRACT;
  case BlendOp::Min:
    return VK_BLEND_OP_MIN;
  case BlendOp::Max:
    return VK_BLEND_OP_MAX;
  default:
    return VK_BLEND_OP_ADD;
  }
}

VkCullModeFlags VulkanDevice::toVkCullMode(CullMode mode) {
  switch (mode) {
  case CullMode::None:
    return VK_CULL_MODE_NONE;
  case CullMode::Front:
    return VK_CULL_MODE_FRONT_BIT;
  case CullMode::Back:
    return VK_CULL_MODE_BACK_BIT;
  default:
    return VK_CULL_MODE_BACK_BIT;
  }
}

VkFrontFace VulkanDevice::toVkFrontFace(FrontFace face) {
  switch (face) {
  case FrontFace::Clockwise:
    return VK_FRONT_FACE_CLOCKWISE;
  case FrontFace::CounterClockwise:
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  default:
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  }
}

VkPrimitiveTopology VulkanDevice::toVkTopology(PrimitiveTopology topology) {
  switch (topology) {
  case PrimitiveTopology::PointList:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case PrimitiveTopology::LineList:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case PrimitiveTopology::LineStrip:
    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  case PrimitiveTopology::TriangleList:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  case PrimitiveTopology::TriangleStrip:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  default:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  }
}

void VulkanDevice::DrawSkybox(TextureHandle cubemap, SamplerHandle sampler,
                              const float *viewMatrix,
                              const float *projMatrix) {
  auto texIt = textures.find(cubemap.id);
  auto sampIt = samplers.find(sampler.id);
  if (texIt == textures.end() || sampIt == samplers.end()) {
    return;
  }

  if (!skyboxInitialized) {
    initializeSkyboxResources();
    if (!skyboxInitialized) {
      std::cerr << "[Vulkan] Failed to initialize skybox resources"
                << std::endl;
      return;
    }
  }

  // Update skybox UBO with view (no translation) and projection
  struct SkyboxUBO {
    float projection[16];
    float view[16];
  } skyboxUbo;

  memcpy(skyboxUbo.projection, projMatrix, 16 * sizeof(float));

  // Remove translation from view matrix (use only rotation)
  float viewRotOnly[16];
  memcpy(viewRotOnly, viewMatrix, 16 * sizeof(float));
  viewRotOnly[12] = 0.0f;
  viewRotOnly[13] = 0.0f;
  viewRotOnly[14] = 0.0f;
  memcpy(skyboxUbo.view, viewRotOnly, 16 * sizeof(float));

  if (currentFrame < skyboxUBOMapped.size() && skyboxUBOMapped[currentFrame]) {
    memcpy(skyboxUBOMapped[currentFrame], &skyboxUbo, sizeof(SkyboxUBO));
  }

  // Update descriptor set with cubemap texture
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = texIt->second.imageView;
  imageInfo.sampler = sampIt->second.sampler;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = skyboxDescriptorSets[currentFrame];
  descriptorWrite.dstBinding = 1;
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

  // Bind skybox pipeline
  vkCmdBindPipeline(commandBuffers[currentFrame],
                    VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipeline);

  // Bind skybox descriptor set
  vkCmdBindDescriptorSets(commandBuffers[currentFrame],
                          VK_PIPELINE_BIND_POINT_GRAPHICS, skyboxPipelineLayout,
                          0, 1, &skyboxDescriptorSets[currentFrame], 0,
                          nullptr);

  // Bind skybox vertex buffer
  VkBuffer vertexBuffers[] = {skyboxCubeVB};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffers[currentFrame], 0, 1, vertexBuffers,
                         offsets);

  // Draw cube (36 vertices)
  vkCmdDraw(commandBuffers[currentFrame], 36, 1, 0, 0);
}

void VulkanDevice::initializeSkyboxResources() {
  std::cout << "[Vulkan] Initializing dedicated skybox resources..."
            << std::endl;

  // Skybox cube vertices (inverted for inside view)
  static const float cubeVertices[] = {
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f,
      1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
      -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
      -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
      -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,
      1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,
      -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,
  };

  // Create vertex buffer
  VkDeviceSize bufferSize = sizeof(cubeVertices);
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  VkBufferCreateInfo stagingInfo{};
  stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  stagingInfo.size = bufferSize;
  stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &stagingInfo, nullptr, &stagingBuffer);

  VkMemoryRequirements memReq;
  vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);
  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memReq.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory);
  vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

  void *data;
  vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &data);
  memcpy(data, cubeVertices, bufferSize);
  vkUnmapMemory(device, stagingMemory);

  VkBufferCreateInfo vbInfo{};
  vbInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  vbInfo.size = bufferSize;
  vbInfo.usage =
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  vbInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  vkCreateBuffer(device, &vbInfo, nullptr, &skyboxCubeVB);

  vkGetBufferMemoryRequirements(device, skyboxCubeVB, &memReq);
  allocInfo.allocationSize = memReq.size;
  allocInfo.memoryTypeIndex = findMemoryType(
      memReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  vkAllocateMemory(device, &allocInfo, nullptr, &skyboxCubeVBMemory);
  vkBindBufferMemory(device, skyboxCubeVB, skyboxCubeVBMemory, 0);

  VkCommandBuffer cmd = beginSingleTimeCommands();
  VkBufferCopy copyRegion{};
  copyRegion.size = bufferSize;
  vkCmdCopyBuffer(cmd, stagingBuffer, skyboxCubeVB, 1, &copyRegion);
  endSingleTimeCommands(cmd);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingMemory, nullptr);

  // Create UBOs (one per frame)
  VkDeviceSize uboSize = 128; // 2 mat4
  skyboxUBOs.resize(MAX_FRAMES_IN_FLIGHT);
  skyboxUBOMemories.resize(MAX_FRAMES_IN_FLIGHT);
  skyboxUBOMapped.resize(MAX_FRAMES_IN_FLIGHT);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkBufferCreateInfo uboInfo{};
    uboInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    uboInfo.size = uboSize;
    uboInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    uboInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateBuffer(device, &uboInfo, nullptr, &skyboxUBOs[i]);

    vkGetBufferMemoryRequirements(device, skyboxUBOs[i], &memReq);
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(
        memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    vkAllocateMemory(device, &allocInfo, nullptr, &skyboxUBOMemories[i]);
    vkBindBufferMemory(device, skyboxUBOs[i], skyboxUBOMemories[i], 0);
    vkMapMemory(device, skyboxUBOMemories[i], 0, uboSize, 0,
                &skyboxUBOMapped[i]);
  }

  // Create descriptor set layout (binding 0: UBO, binding 1: samplerCube)
  VkDescriptorSetLayoutBinding uboBinding{};
  uboBinding.binding = 0;
  uboBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboBinding.descriptorCount = 1;
  uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  VkDescriptorSetLayoutBinding samplerBinding{};
  samplerBinding.binding = 1;
  samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerBinding.descriptorCount = 1;
  samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboBinding,
                                                          samplerBinding};
  VkDescriptorSetLayoutCreateInfo layoutInfo{};
  layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings = bindings.data();
  vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr,
                              &skyboxDescriptorSetLayout);

  // Create descriptor pool
  std::array<VkDescriptorPoolSize, 2> poolSizes{};
  poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

  VkDescriptorPoolCreateInfo poolInfo{};
  poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes = poolSizes.data();
  poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  vkCreateDescriptorPool(device, &poolInfo, nullptr, &skyboxDescriptorPool);

  // Allocate descriptor sets
  std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT,
                                             skyboxDescriptorSetLayout);
  VkDescriptorSetAllocateInfo dsAllocInfo{};
  dsAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  dsAllocInfo.descriptorPool = skyboxDescriptorPool;
  dsAllocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
  dsAllocInfo.pSetLayouts = layouts.data();
  skyboxDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
  vkAllocateDescriptorSets(device, &dsAllocInfo, skyboxDescriptorSets.data());

  // Update descriptor sets with UBO
  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    VkDescriptorBufferInfo bufferInfo{};
    bufferInfo.buffer = skyboxUBOs[i];
    bufferInfo.offset = 0;
    bufferInfo.range = uboSize;

    VkWriteDescriptorSet uboWrite{};
    uboWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    uboWrite.dstSet = skyboxDescriptorSets[i];
    uboWrite.dstBinding = 0;
    uboWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboWrite.descriptorCount = 1;
    uboWrite.pBufferInfo = &bufferInfo;
    vkUpdateDescriptorSets(device, 1, &uboWrite, 0, nullptr);
  }

  // Load skybox shaders
  auto loadSpirv = [](const std::string &path) -> std::vector<uint32_t> {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file.is_open())
      return {};
    size_t fileSize = (size_t)file.tellg();
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();
    return buffer;
  };

  auto vertCode = loadSpirv("shaders/skybox.vert.spv");
  auto fragCode = loadSpirv("shaders/skybox.frag.spv");
  if (vertCode.empty() || fragCode.empty()) {
    std::cerr << "[Vulkan] Failed to load skybox shaders" << std::endl;
    return;
  }

  VkShaderModuleCreateInfo vertInfo{};
  vertInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  vertInfo.codeSize = vertCode.size() * sizeof(uint32_t);
  vertInfo.pCode = vertCode.data();
  vkCreateShaderModule(device, &vertInfo, nullptr, &skyboxVertModule);

  VkShaderModuleCreateInfo fragInfo{};
  fragInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  fragInfo.codeSize = fragCode.size() * sizeof(uint32_t);
  fragInfo.pCode = fragCode.data();
  vkCreateShaderModule(device, &fragInfo, nullptr, &skyboxFragModule);

  // Create pipeline layout
  VkPipelineLayoutCreateInfo pipeLayoutInfo{};
  pipeLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeLayoutInfo.setLayoutCount = 1;
  pipeLayoutInfo.pSetLayouts = &skyboxDescriptorSetLayout;
  vkCreatePipelineLayout(device, &pipeLayoutInfo, nullptr,
                         &skyboxPipelineLayout);

  // Create graphics pipeline
  VkPipelineShaderStageCreateInfo vertStage{};
  vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertStage.module = skyboxVertModule;
  vertStage.pName = "main";

  VkPipelineShaderStageCreateInfo fragStage{};
  fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragStage.module = skyboxFragModule;
  fragStage.pName = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

  VkVertexInputBindingDescription bindingDesc{};
  bindingDesc.binding = 0;
  bindingDesc.stride = 3 * sizeof(float);
  bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attrDesc{};
  attrDesc.binding = 0;
  attrDesc.location = 0;
  attrDesc.format = VK_FORMAT_R32G32B32_SFLOAT;
  attrDesc.offset = 0;

  VkPipelineVertexInputStateCreateInfo vertexInput{};
  vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInput.vertexBindingDescriptionCount = 1;
  vertexInput.pVertexBindingDescriptions = &bindingDesc;
  vertexInput.vertexAttributeDescriptionCount = 1;
  vertexInput.pVertexAttributeDescriptions = &attrDesc;

  VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
  inputAssembly.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportState{};
  viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount = 1;
  viewportState.scissorCount = 1;

  VkPipelineRasterizationStateCreateInfo rasterizer{};
  rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable = VK_FALSE;
  rasterizer.rasterizerDiscardEnable = VK_FALSE;
  rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth = 1.0f;
  rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
  rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable = VK_FALSE;

  VkPipelineMultisampleStateCreateInfo multisampling{};
  multisampling.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable = VK_FALSE;
  multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  VkPipelineDepthStencilStateCreateInfo depthStencil{};
  depthStencil.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable = VK_TRUE;
  depthStencil.depthWriteEnable = VK_FALSE;
  depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
  depthStencil.depthBoundsTestEnable = VK_FALSE;
  depthStencil.stencilTestEnable = VK_FALSE;

  VkPipelineColorBlendAttachmentState colorBlendAttachment{};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable = VK_FALSE;

  VkPipelineColorBlendStateCreateInfo colorBlending{};
  colorBlending.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable = VK_FALSE;
  colorBlending.attachmentCount = 1;
  colorBlending.pAttachments = &colorBlendAttachment;

  std::vector<VkDynamicState> dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                                               VK_DYNAMIC_STATE_SCISSOR};
  VkPipelineDynamicStateCreateInfo dynamicState{};
  dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
  dynamicState.pDynamicStates = dynamicStates.data();

  VkGraphicsPipelineCreateInfo pipelineInfo{};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInput;
  pipelineInfo.pInputAssemblyState = &inputAssembly;
  pipelineInfo.pViewportState = &viewportState;
  pipelineInfo.pRasterizationState = &rasterizer;
  pipelineInfo.pMultisampleState = &multisampling;
  pipelineInfo.pDepthStencilState = &depthStencil;
  pipelineInfo.pColorBlendState = &colorBlending;
  pipelineInfo.pDynamicState = &dynamicState;
  pipelineInfo.layout = skyboxPipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass = 0;

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                nullptr, &skyboxPipeline) != VK_SUCCESS) {
    std::cerr << "[Vulkan] Failed to create skybox pipeline" << std::endl;
    return;
  }

  skyboxInitialized = true;
  std::cout << "[Vulkan] Skybox resources initialized successfully"
            << std::endl;
}

void VulkanDevice::cleanupSkyboxResources() {
  if (!skyboxInitialized)
    return;

  vkDeviceWaitIdle(device);

  if (skyboxPipeline != VK_NULL_HANDLE)
    vkDestroyPipeline(device, skyboxPipeline, nullptr);
  if (skyboxPipelineLayout != VK_NULL_HANDLE)
    vkDestroyPipelineLayout(device, skyboxPipelineLayout, nullptr);
  if (skyboxVertModule != VK_NULL_HANDLE)
    vkDestroyShaderModule(device, skyboxVertModule, nullptr);
  if (skyboxFragModule != VK_NULL_HANDLE)
    vkDestroyShaderModule(device, skyboxFragModule, nullptr);
  if (skyboxDescriptorPool != VK_NULL_HANDLE)
    vkDestroyDescriptorPool(device, skyboxDescriptorPool, nullptr);
  if (skyboxDescriptorSetLayout != VK_NULL_HANDLE)
    vkDestroyDescriptorSetLayout(device, skyboxDescriptorSetLayout, nullptr);
  if (skyboxCubeVB != VK_NULL_HANDLE)
    vkDestroyBuffer(device, skyboxCubeVB, nullptr);
  if (skyboxCubeVBMemory != VK_NULL_HANDLE)
    vkFreeMemory(device, skyboxCubeVBMemory, nullptr);

  for (size_t i = 0; i < skyboxUBOs.size(); i++) {
    if (skyboxUBOs[i] != VK_NULL_HANDLE)
      vkDestroyBuffer(device, skyboxUBOs[i], nullptr);
    if (skyboxUBOMemories[i] != VK_NULL_HANDLE)
      vkFreeMemory(device, skyboxUBOMemories[i], nullptr);
  }

  skyboxInitialized = false;
  std::cout << "[Vulkan] Skybox resources cleaned up" << std::endl;
}

} // namespace RHI
