#include "vulkan_device.hpp"

namespace RHI {

VulkanDevice::~VulkanDevice() { Shutdown(); }

bool VulkanDevice::Initialize() {
  if (!window) {
    std::cerr << "[Vulkan] Window not set before Initialize()" << std::endl;
    return false;
  }

  createInstance();
  setupDebugMessenger();
  createSurface();
  pickPhysicalDevice();
  createLogicalDevice();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createFramebuffers();
  createCommandPool();
  createCommandBuffers();
  createSyncObjects();

  std::cout << "[Vulkan] Device initialized successfully" << std::endl;
  return true;
}

void VulkanDevice::Shutdown() {
  if (device == VK_NULL_HANDLE)
    return;

  vkDeviceWaitIdle(device);

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

  for (auto &[id, buf] : buffers) {
    vkDestroyBuffer(device, buf.buffer, nullptr);
    vkFreeMemory(device, buf.memory, nullptr);
  }

  for (auto &[id, tex] : textures) {
    vkDestroyImageView(device, tex.imageView, nullptr);
    vkDestroyImage(device, tex.image, nullptr);
    vkFreeMemory(device, tex.memory, nullptr);
  }

  for (auto &[id, smp] : samplers) {
    vkDestroySampler(device, smp.sampler, nullptr);
  }

  for (auto &[id, pipe] : pipelines) {
    vkDestroyPipeline(device, pipe.pipeline, nullptr);
    vkDestroyPipelineLayout(device, pipe.layout, nullptr);
  }

  for (auto &[id, shd] : shaders) {
    vkDestroyShaderModule(device, shd.vertModule, nullptr);
    vkDestroyShaderModule(device, shd.fragModule, nullptr);
  }

  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

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
  if (enableValidationLayers && !checkValidationLayerSupport()) {
    std::cerr << "[Vulkan] Validation layers requested but not available"
              << std::endl;
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
  if (enableValidationLayers) {
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

void VulkanDevice::createRenderPass() {
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

  VkSubpassDescription subpass{};
  subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount = 1;
  subpass.pColorAttachments = &colorAttachmentRef;

  VkSubpassDependency dependency{};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkRenderPassCreateInfo renderPassInfo{};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = 1;
  renderPassInfo.pAttachments = &colorAttachment;
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
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{};
    framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass = renderPass;
    framebufferInfo.attachmentCount = 1;
    framebufferInfo.pAttachments = attachments;
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

// Stub implementations for remaining interface methods
BufferHandle VulkanDevice::CreateBuffer(const BufferDescriptor &desc) {
  // TODO: Implement
  return BufferHandle{nextId++};
}

void VulkanDevice::UpdateBuffer(BufferHandle buffer, const void *data,
                                uint32_t size, uint32_t offset) {
  // TODO: Implement
}

void VulkanDevice::DestroyBuffer(BufferHandle buffer) {
  // TODO: Implement
}

TextureHandle VulkanDevice::CreateTexture(const TextureDescriptor &desc) {
  // TODO: Implement
  return TextureHandle{nextId++};
}

void VulkanDevice::UpdateTexture(TextureHandle texture, const void *data,
                                 uint32_t mipLevel) {
  // TODO: Implement
}

void VulkanDevice::GenerateMipmaps(TextureHandle texture) {
  // TODO: Implement
}

void VulkanDevice::DestroyTexture(TextureHandle texture) {
  // TODO: Implement
}

SamplerHandle VulkanDevice::CreateSampler(const SamplerDescriptor &desc) {
  // TODO: Implement
  return SamplerHandle{nextId++};
}

void VulkanDevice::DestroySampler(SamplerHandle sampler) {
  // TODO: Implement
}

ShaderHandle
VulkanDevice::CreateShader(const std::vector<ShaderDescriptor> &stages) {
  // TODO: Implement SPIR-V compilation
  return ShaderHandle{nextId++};
}

void VulkanDevice::DestroyShader(ShaderHandle shader) {
  // TODO: Implement
}

PipelineHandle VulkanDevice::CreatePipeline(const PipelineDescriptor &desc,
                                            ShaderHandle shader,
                                            const VertexLayout &layout) {
  // TODO: Implement
  return PipelineHandle{nextId++};
}

void VulkanDevice::DestroyPipeline(PipelineHandle pipeline) {
  // TODO: Implement
}

VertexArrayHandle VulkanDevice::CreateVertexArray(BufferHandle vertexBuffer,
                                                  BufferHandle indexBuffer,
                                                  const VertexLayout &layout) {
  // VAO is OpenGL-specific, not needed in Vulkan
  return VertexArrayHandle{nextId++};
}

void VulkanDevice::DestroyVertexArray(VertexArrayHandle vao) {
  // No-op for Vulkan
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
}

void VulkanDevice::BindVertexArray(VertexArrayHandle vao) {
  // No-op for Vulkan
}

void VulkanDevice::BindFramebuffer(FramebufferHandle framebuffer) {
  // TODO: Implement
}

void VulkanDevice::BindTexture(uint32_t slot, TextureHandle texture) {
  // TODO: Implement descriptor binding
}

void VulkanDevice::BindSampler(uint32_t slot, SamplerHandle sampler) {
  // TODO: Implement descriptor binding
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              int value) {
  // Vulkan uses push constants or descriptor sets
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              float value) {
  // Vulkan uses push constants or descriptor sets
}

void VulkanDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              const float *value, uint32_t count) {
  // Vulkan uses push constants or descriptor sets
}

void VulkanDevice::SetUniformMatrix4(ShaderHandle shader,
                                     const std::string &name,
                                     const float *matrix) {
  // Vulkan uses push constants or descriptor sets
}

void VulkanDevice::Draw(const DrawCommand &cmd) {
  // TODO: Implement
}

void VulkanDevice::DrawIndexed(const DrawIndexedCommand &cmd) {
  // TODO: Implement
}

void VulkanDevice::WaitIdle() {
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }
}

void VulkanDevice::BeginFrame() {
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE,
                  UINT64_MAX);

  VkResult result = vkAcquireNextImageKHR(
      device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame],
      VK_NULL_HANDLE, &currentImageIndex);

  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    // Recreate swap chain
    return;
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

  VkClearValue clearValue = {
      {{clearColor.r, clearColor.g, clearColor.b, clearColor.a}}};
  renderPassInfo.clearValueCount = 1;
  renderPassInfo.pClearValues = &clearValue;

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

} // namespace RHI
