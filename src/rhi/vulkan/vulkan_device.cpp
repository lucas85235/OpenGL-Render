#include "vulkan_device.hpp"
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
  createDescriptorSets();
  createCommandBuffers();
  createSyncObjects();

  // Initialize default material (gold-ish) and light values
  cachedUbo.materialColor[0] = 1.0f;   // Albedo R
  cachedUbo.materialColor[1] = 0.765f; // Albedo G
  cachedUbo.materialColor[2] = 0.336f; // Albedo B
  cachedUbo.materialColor[3] = 1.0f;   // Metallic

  cachedUbo.materialProps[0] = 0.3f; // Roughness
  cachedUbo.materialProps[1] = 1.0f; // AO
  cachedUbo.materialProps[2] = 0.0f; // Emission strength
  cachedUbo.materialProps[3] = 0.0f; // Unused

  cachedUbo.lightDir[0] = -0.2f; // Light direction X (matches OpenGL dirLight)
  cachedUbo.lightDir[1] = -1.0f; // Light direction Y
  cachedUbo.lightDir[2] = -0.3f; // Light direction Z
  cachedUbo.lightDir[3] = 3.0f;  // Light intensity (increased)

  cachedUbo.lightColor[0] = 1.0f; // Light color R
  cachedUbo.lightColor[1] = 0.9f; // Light color G
  cachedUbo.lightColor[2] = 0.8f; // Light color B (warmer like OpenGL sun)
  cachedUbo.lightColor[3] = 1.0f; // Unused

  cachedUbo.viewPos[0] = 0.0f;  // View position X
  cachedUbo.viewPos[1] = 0.0f;  // View position Y
  cachedUbo.viewPos[2] = 50.0f; // View position Z (matches camera distance)
  cachedUbo.viewPos[3] = 1.0f;  // Unused

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
  VkDescriptorSetLayoutBinding uboLayoutBinding{};
  uboLayoutBinding.binding = 0;
  uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount = 1;
  uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers = nullptr;

  VkDescriptorSetLayoutBinding samplerLayoutBinding{};
  samplerLayoutBinding.binding = 1;
  samplerLayoutBinding.descriptorCount = 1;
  samplerLayoutBinding.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers = nullptr;
  samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding,
                                                          samplerLayoutBinding};
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
  }
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

  VkMemoryAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to allocate image memory");
  }

  vkBindImageMemory(device, image, imageMemory, 0);
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
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;
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

  VkDeviceSize imageSize =
      desc.width * desc.height *
      4; // Assuming RGBA8 for simplicity, should calculate from format

  VkBuffer stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  VkBufferCreateInfo bufferInfo{};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = imageSize;
  bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) !=
      VK_SUCCESS) {
    throw std::runtime_error("[Vulkan] Failed to create staging buffer");
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
    throw std::runtime_error(
        "[Vulkan] Failed to allocate staging buffer memory");
  }

  vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

  if (desc.data) {
    void *data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, desc.data, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
  }

  createImage(
      desc.width, desc.height, vkTexture.format, VK_IMAGE_TILING_OPTIMAL,
      VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vkTexture.image, vkTexture.memory);

  transitionImageLayout(vkTexture.image, vkTexture.format,
                        VK_IMAGE_LAYOUT_UNDEFINED,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
  copyBufferToImage(stagingBuffer, vkTexture.image,
                    static_cast<uint32_t>(desc.width),
                    static_cast<uint32_t>(desc.height));
  transitionImageLayout(vkTexture.image, vkTexture.format,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);

  vkTexture.imageView = createImageView(vkTexture.image, vkTexture.format);

  TextureHandle handle{nextId++};
  textures[handle.id] = vkTexture;
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

void VulkanDevice::GenerateMipmaps(TextureHandle texture) {
  // TODO: Implement using vkCmdBlitImage
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
    vertCode = readSpirvFile("shaders/vulkan/cube.vert.spv");
  }
  if (fragCode.empty()) {
    fragCode = readSpirvFile("shaders/vulkan/cube.frag.spv");
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

  // Note: Unified shaders use UBO for all data, no push constants needed
  // Push constants disabled for compatibility with unified SPIR-V shaders

  // Pipeline layout
  VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = 1;
  pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
  pipelineLayoutInfo.pushConstantRangeCount = 0;
  pipelineLayoutInfo.pPushConstantRanges = nullptr;

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
  pipelines[handle.id] = vkPipeline;
  std::cout << "[Vulkan] Graphics pipeline created" << std::endl;
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
  // TODO: Implement
  return FramebufferHandle{nextId++};
}

void VulkanDevice::AttachTexture(FramebufferHandle framebuffer,
                                 FramebufferAttachment attachment,
                                 TextureHandle texture) {
  // TODO: Implement
}

TextureHandle
VulkanDevice::GetFramebufferTexture(FramebufferHandle framebuffer,
                                    FramebufferAttachment attachment) {
  return TextureHandle{0};
}

void VulkanDevice::ResizeFramebuffer(FramebufferHandle framebuffer,
                                     uint32_t width, uint32_t height) {
  // TODO: Implement
}

void VulkanDevice::DestroyFramebuffer(FramebufferHandle framebuffer) {
  // TODO: Implement
}

void VulkanDevice::SetViewport(const Viewport &viewport) {
  // Set during command buffer recording
}

void VulkanDevice::SetScissor(const Scissor &scissor) {
  // Set during command buffer recording
}

void VulkanDevice::DisableScissor() {
  // No-op for now
}

void VulkanDevice::Clear(bool color, bool depth, bool stencil) {
  // Handled in render pass
}

void VulkanDevice::SetClearColor(const ClearColor &color) {
  clearColor = color;
}

void VulkanDevice::SetClearDepth(float depth) {
  // TODO: Implement
}

void VulkanDevice::BindPipeline(PipelineHandle pipeline) {
  currentPipeline = pipeline;

  auto it = pipelines.find(pipeline.id);
  if (it != pipelines.end()) {
    vkCmdBindPipeline(commandBuffers[currentFrame],
                      VK_PIPELINE_BIND_POINT_GRAPHICS, it->second.pipeline);
  }
}

void VulkanDevice::BindVertexArray(VertexArrayHandle vao) {
  // In Vulkan, we store references and bind during draw
  currentVAO = vao;
}

void VulkanDevice::BindFramebuffer(FramebufferHandle framebuffer) {
  // TODO: Implement
}

void VulkanDevice::BindTexture(uint32_t slot, TextureHandle texture) {
  auto texIt = textures.find(texture.id);
  if (texIt == textures.end())
    return;

  // Get or create default sampler
  VkSampler texSampler = VK_NULL_HANDLE;
  if (!samplers.empty()) {
    texSampler = samplers.begin()->second.sampler;
  } else {
    // Create a default sampler
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler newSampler;
    if (vkCreateSampler(device, &samplerInfo, nullptr, &newSampler) ==
        VK_SUCCESS) {
      VulkanSampler vs;
      vs.sampler = newSampler;
      samplers[nextId] = vs;
      texSampler = newSampler;
      nextId++;
    } else {
      std::cerr << "[Vulkan] Failed to create default sampler" << std::endl;
      return;
    }
  }

  // Update descriptor set for current frame with the texture
  VkDescriptorImageInfo imageInfo{};
  imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  imageInfo.imageView = texIt->second.imageView;
  imageInfo.sampler = texSampler;

  VkWriteDescriptorSet descriptorWrite{};
  descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  descriptorWrite.dstSet = descriptorSets[currentFrame];
  descriptorWrite.dstBinding = 1; // sampler binding
  descriptorWrite.dstArrayElement = 0;
  descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  descriptorWrite.descriptorCount = 1;
  descriptorWrite.pImageInfo = &imageInfo;

  vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);

  // Set hasTexture flag in cachedUbo
  cachedUbo.materialProps[3] = 1.0f;
  if (currentFrame < uniformBuffersMapped.size() &&
      uniformBuffersMapped[currentFrame]) {
    memcpy(uniformBuffersMapped[currentFrame], &cachedUbo,
           sizeof(UniformBufferObject));
  }
}

void VulkanDevice::BindSampler(uint32_t slot, SamplerHandle sampler) {
  // Sampler is combined with texture in BindTexture
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              int value) {
  // Currently no int uniforms needed in the UBO
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              float value) {
  bool updated = false;

  if (name.find("roughness") != std::string::npos) {
    cachedUbo.materialProps[0] = value;
    updated = true;
  } else if (name.find("metallic") != std::string::npos) {
    cachedUbo.materialColor[3] = value;
    updated = true;
  } else if (name.find("ao") != std::string::npos &&
             name.find("emission") == std::string::npos) {
    cachedUbo.materialProps[1] = value;
    updated = true;
  } else if (name.find("emissionStrength") != std::string::npos) {
    cachedUbo.materialProps[2] = value;
    updated = true;
  } else if (name.find("dirLight.intensity") != std::string::npos) {
    cachedUbo.lightDir[3] = value;
    updated = true;
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
    cachedUbo.materialColor[0] = value[0];
    cachedUbo.materialColor[1] = value[1];
    cachedUbo.materialColor[2] = value[2];
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

  // Copy matrix to cached UBO based on uniform name
  if (name == "model" || name.find("model") != std::string::npos) {
    memcpy(cachedUbo.model, matrix, sizeof(float) * 16);
  } else if (name == "view" || name.find("view") != std::string::npos) {
    memcpy(cachedUbo.view, matrix, sizeof(float) * 16);
  } else if (name == "projection" || name == "proj" ||
             name.find("proj") != std::string::npos) {
    memcpy(cachedUbo.proj, matrix, sizeof(float) * 16);
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

    // Note: Push constants disabled for unified shaders (all data in UBO)
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

  VkViewport viewport{};
  viewport.x = 0.0f;
  viewport.y = 0.0f;
  viewport.width = static_cast<float>(swapChainExtent.width);
  viewport.height = static_cast<float>(swapChainExtent.height);
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;
  vkCmdSetViewport(commandBuffers[currentFrame], 0, 1, &viewport);

  VkRect2D scissor{};
  scissor.offset = {0, 0};
  scissor.extent = swapChainExtent;
  vkCmdSetScissor(commandBuffers[currentFrame], 0, 1, &scissor);

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
} // namespace RHI
