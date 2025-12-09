#ifndef VULKAN_DEVICE_HPP
#define VULKAN_DEVICE_HPP

#include "rhi_device.h"
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <optional>
#include <set>
#include <vector>

namespace RHI {

#ifdef NDEBUG
constexpr bool enableValidationLayers = false;
#else
constexpr bool enableValidationLayers = true;
#endif

const std::vector<const char *> validationLayers = {
    "VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

  bool isComplete() const {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct VulkanBuffer {
  VkBuffer buffer;
  VkDeviceMemory memory;
  VkDeviceSize size;
  BufferType type;
};

struct VulkanTexture {
  VkImage image;
  VkDeviceMemory memory;
  VkImageView imageView;
  VkFormat format;
  uint32_t width, height, depth;
};

struct VulkanSampler {
  VkSampler sampler;
};

struct VulkanShader {
  VkShaderModule vertModule;
  VkShaderModule fragModule;
};

struct VulkanPipeline {
  VkPipeline pipeline;
  VkPipelineLayout layout;
  VkRenderPass renderPass;
};

class VulkanDevice : public IDevice {
private:
  GLFWwindow *window = nullptr;

  VkInstance instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
  VkSurfaceKHR surface = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkDevice device = VK_NULL_HANDLE;

  VkQueue graphicsQueue = VK_NULL_HANDLE;
  VkQueue presentQueue = VK_NULL_HANDLE;

  VkSwapchainKHR swapChain = VK_NULL_HANDLE;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImageView> swapChainImageViews;
  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkRenderPass renderPass = VK_NULL_HANDLE;
  VkCommandPool commandPool = VK_NULL_HANDLE;
  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;
  uint32_t currentFrame = 0;
  uint32_t currentImageIndex = 0;

  std::unordered_map<uint64_t, VulkanBuffer> buffers;
  std::unordered_map<uint64_t, VulkanTexture> textures;
  std::unordered_map<uint64_t, VulkanSampler> samplers;
  std::unordered_map<uint64_t, VulkanShader> shaders;
  std::unordered_map<uint64_t, VulkanPipeline> pipelines;

  uint64_t nextId = 1;
  PipelineHandle currentPipeline;
  ClearColor clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredExtensions();
  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffers();
  void createSyncObjects();

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
  bool isDeviceSuitable(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
  VkFormat toVkFormat(TextureFormat format);
  VkFilter toVkFilter(TextureFilterMode mode);
  VkSamplerAddressMode toVkAddressMode(TextureWrapMode mode);
  VkCompareOp toVkCompareOp(CompareOp op);
  VkBlendFactor toVkBlendFactor(BlendFactor factor);
  VkBlendOp toVkBlendOp(BlendOp op);
  VkCullModeFlags toVkCullMode(CullMode mode);
  VkFrontFace toVkFrontFace(FrontFace face);
  VkPrimitiveTopology toVkTopology(PrimitiveTopology topology);

public:
  VulkanDevice() = default;
  ~VulkanDevice() override;

  void SetWindow(GLFWwindow *win) { window = win; }

  bool Initialize() override;
  void Shutdown() override;
  API GetAPI() const override { return API::Vulkan; }
  DeviceInfo GetDeviceInfo() const override;

  BufferHandle CreateBuffer(const BufferDescriptor &desc) override;
  void UpdateBuffer(BufferHandle buffer, const void *data, uint32_t size,
                    uint32_t offset) override;
  void DestroyBuffer(BufferHandle buffer) override;

  TextureHandle CreateTexture(const TextureDescriptor &desc) override;
  void UpdateTexture(TextureHandle texture, const void *data,
                     uint32_t mipLevel) override;
  void GenerateMipmaps(TextureHandle texture) override;
  void DestroyTexture(TextureHandle texture) override;

  SamplerHandle CreateSampler(const SamplerDescriptor &desc) override;
  void DestroySampler(SamplerHandle sampler) override;

  ShaderHandle
  CreateShader(const std::vector<ShaderDescriptor> &stages) override;
  void DestroyShader(ShaderHandle shader) override;

  PipelineHandle CreatePipeline(const PipelineDescriptor &desc,
                                ShaderHandle shader,
                                const VertexLayout &layout) override;
  void DestroyPipeline(PipelineHandle pipeline) override;

  VertexArrayHandle CreateVertexArray(BufferHandle vertexBuffer,
                                      BufferHandle indexBuffer,
                                      const VertexLayout &layout) override;
  void DestroyVertexArray(VertexArrayHandle vao) override;

  FramebufferHandle
  CreateFramebuffer(const FramebufferDescriptor &desc) override;
  void AttachTexture(FramebufferHandle framebuffer,
                     FramebufferAttachment attachment,
                     TextureHandle texture) override;
  TextureHandle
  GetFramebufferTexture(FramebufferHandle framebuffer,
                        FramebufferAttachment attachment) override;
  void ResizeFramebuffer(FramebufferHandle framebuffer, uint32_t width,
                         uint32_t height) override;
  void DestroyFramebuffer(FramebufferHandle framebuffer) override;

  void SetViewport(const Viewport &viewport) override;
  void SetScissor(const Scissor &scissor) override;
  void DisableScissor() override;
  void Clear(bool color, bool depth, bool stencil) override;
  void SetClearColor(const ClearColor &color) override;
  void SetClearDepth(float depth) override;

  void BindPipeline(PipelineHandle pipeline) override;
  void BindVertexArray(VertexArrayHandle vao) override;
  void BindFramebuffer(FramebufferHandle framebuffer) override;
  void BindTexture(uint32_t slot, TextureHandle texture) override;
  void BindSampler(uint32_t slot, SamplerHandle sampler) override;

  void SetUniform(ShaderHandle shader, const std::string &name,
                  int value) override;
  void SetUniform(ShaderHandle shader, const std::string &name,
                  float value) override;
  void SetUniform(ShaderHandle shader, const std::string &name,
                  const float *value, uint32_t count) override;
  void SetUniformMatrix4(ShaderHandle shader, const std::string &name,
                         const float *matrix) override;

  void Draw(const DrawCommand &cmd) override;
  void DrawIndexed(const DrawIndexedCommand &cmd) override;

  void WaitIdle() override;

  void BeginFrame();
  void EndFrame();
};

// Debug callback
static VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "[Vulkan] " << pCallbackData->pMessage << std::endl;
  }
  return VK_FALSE;
}

// Proxy functions for debug messenger
inline VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  }
  return VK_ERROR_EXTENSION_NOT_PRESENT;
}

inline void
DestroyDebugUtilsMessengerEXT(VkInstance instance,
                              VkDebugUtilsMessengerEXT debugMessenger,
                              const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, debugMessenger, pAllocator);
  }
}

} // namespace RHI

#endif // VULKAN_DEVICE_HPP
