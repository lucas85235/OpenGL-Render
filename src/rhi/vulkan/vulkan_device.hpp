#ifndef RHI_VULKAN_DEVICE_HPP
#define RHI_VULKAN_DEVICE_HPP

#include "../rhi_device.h"
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
  uint32_t arrayLayers = 1;
  bool isCubemap = false;
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
  ShaderHandle shader;
};

struct VulkanVertexArray {
  BufferHandle vertexBuffer;
  BufferHandle indexBuffer;
};

struct VulkanFramebuffer {
  VkFramebuffer framebuffer = VK_NULL_HANDLE;
  VkRenderPass renderPass = VK_NULL_HANDLE;
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<TextureHandle> colorAttachments;
  TextureHandle depthAttachment;
  bool hasDepth = false;
  bool ownsRenderPass = false;
};

// Push constants for per-draw data (max 128 bytes on most GPUs)
struct PushConstants {
  float model[16];        // 64 bytes - model matrix
  float materialColor[4]; // 16 bytes - albedo.rgb + metallic
  float materialProps[4]; // 16 bytes - roughness, ao, emission, flags
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

  // Depth buffer resources
  VkImage depthImage = VK_NULL_HANDLE;
  VkDeviceMemory depthImageMemory = VK_NULL_HANDLE;
  VkImageView depthImageView = VK_NULL_HANDLE;
  VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

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
  std::unordered_map<uint64_t, VulkanVertexArray> vertexArrays;
  std::unordered_map<uint64_t, VulkanFramebuffer> framebuffers;

  uint64_t nextId = 1;

  // Render State
  PipelineHandle currentPipeline;
  VertexArrayHandle currentVAO;
  FramebufferHandle currentFramebuffer;
  TextureHandle boundTextures[16];
  SamplerHandle boundSamplers[16];
  ClearColor clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepthValue = 1.0f;

  // Cached viewport/scissor for deferred application
  Viewport cachedViewport = {0.0f, 0.0f, 800, 600, 0.0f, 1.0f};
  Scissor cachedScissor = {0, 0, 800, 600};
  bool viewportDirty = true;
  bool scissorDirty = true;

  // Descriptors (main PBR pipeline)
  VkDescriptorSetLayout descriptorSetLayout;
  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;

  // Skybox dedicated resources (isolated from PBR pipeline)
  VkDescriptorSetLayout skyboxDescriptorSetLayout = VK_NULL_HANDLE;
  VkDescriptorPool skyboxDescriptorPool = VK_NULL_HANDLE;
  std::vector<VkDescriptorSet> skyboxDescriptorSets;
  VkPipelineLayout skyboxPipelineLayout = VK_NULL_HANDLE;
  VkPipeline skyboxPipeline = VK_NULL_HANDLE;
  VkBuffer skyboxCubeVB = VK_NULL_HANDLE;
  VkDeviceMemory skyboxCubeVBMemory = VK_NULL_HANDLE;
  std::vector<VkBuffer> skyboxUBOs;
  std::vector<VkDeviceMemory> skyboxUBOMemories;
  std::vector<void *> skyboxUBOMapped;
  VkShaderModule skyboxVertModule = VK_NULL_HANDLE;
  VkShaderModule skyboxFragModule = VK_NULL_HANDLE;
  bool skyboxInitialized = false;
  void initializeSkyboxResources();
  void cleanupSkyboxResources();

  // Uniform Buffers (one per frame) - scene-wide data only
  struct UniformBufferObject {
    alignas(16) float view[16];
    alignas(16) float proj[16];
    alignas(16) float lightDir[4];   // direction.xyz + intensity
    alignas(16) float lightColor[4]; // color.rgb + unused
    alignas(16) float viewPos[4];    // position.xyz + unused

    // Point lights (4 max) - position.xyz + intensity, color.rgb + radius
    alignas(16) float pointLights[4][8]; // 4 lights * (pos4 + color4)
    alignas(4) int numPointLights;
    alignas(4) int padding[3]; // Align to 16 bytes
  };
  std::vector<VkBuffer> uniformBuffers;
  std::vector<VkDeviceMemory> uniformBuffersMemory;
  std::vector<void *> uniformBuffersMapped;

  // Cached UBO state for updates (scene-wide data)
  UniformBufferObject cachedUbo{};

  // Cached push constants for per-draw data
  PushConstants cachedPushConstants{};

  // Dummy 1x1 white texture for unused descriptor bindings
  VkImage dummyImage = VK_NULL_HANDLE;
  VkDeviceMemory dummyImageMemory = VK_NULL_HANDLE;
  VkImageView dummyImageView = VK_NULL_HANDLE;
  VkSampler dummySampler = VK_NULL_HANDLE;
  void createDummyTexture();

  void createDescriptorSetLayout();
  void createUniformBuffers();
  void createDescriptorPool();
  void createDescriptorSets();
  void updateUniformBuffer(uint32_t currentImage);

  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredExtensions();
  void createInstance();
  void setupDebugMessenger();
  void createSurface();
  void pickPhysicalDevice();
  void createLogicalDevice();
  void createSwapChain();
  void createImageViews();
  void createDepthResources();
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

  VkCommandBuffer beginSingleTimeCommands();
  void endSingleTimeCommands(VkCommandBuffer commandBuffer);
  void transitionImageLayout(VkImage image, VkFormat format,
                             VkImageLayout oldLayout, VkImageLayout newLayout);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  void createImage(uint32_t width, uint32_t height, VkFormat format,
                   VkImageTiling tiling, VkImageUsageFlags usage,
                   VkMemoryPropertyFlags properties, VkImage &image,
                   VkDeviceMemory &imageMemory);
  VkImageView createImageView(VkImage image, VkFormat format);

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
  void UpdateTextureCubeFace(TextureHandle texture, CubemapFace face,
                             const void *data, uint32_t mipLevel) override;
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
  void DrawSkybox(TextureHandle cubemap, SamplerHandle sampler,
                  const float *viewMatrix, const float *projMatrix) override;

  void WaitIdle() override;

  bool BeginFrame() override;
  void EndFrame() override;
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
