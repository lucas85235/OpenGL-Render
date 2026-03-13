#ifndef RHI_VULKAN_COMMAND_LIST_HPP
#define RHI_VULKAN_COMMAND_LIST_HPP

#include "../command_list.hpp"
#include "vulkan_device.hpp"
#include <functional>
#include <vector>

namespace RHI {

class VulkanDevice;

class VulkanCommandList : public ICommandList {
private:
  VulkanDevice *device = nullptr;
  std::vector<std::function<void()>> commands;
  bool isRecording = false;

public:
  VulkanCommandList(VulkanDevice *dev) : device(dev) {}
  ~VulkanCommandList() override = default;

  void Begin() override;
  void End() override;

  void BeginRenderPass(const RenderPassDescriptor &desc) override;
  void EndRenderPass() override;

  void SetViewport(const Viewport &viewport) override;
  void SetScissor(const Scissor &scissor) override;
  void DisableScissor() override;

  void BindPipeline(PipelineHandle pipeline) override;
  void BindVertexArray(VertexArrayHandle vao) override;
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
                  const float *view, const float *proj) override;

  void Execute();
};

} // namespace RHI

#endif // RHI_VULKAN_COMMAND_LIST_HPP
