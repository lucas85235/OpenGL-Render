#include "vulkan_command_list.hpp"
#include <iostream>

namespace RHI {

void VulkanCommandList::Begin() {
  commands.clear();
  isRecording = true;
}

void VulkanCommandList::End() { isRecording = false; }

void VulkanCommandList::Execute() {
  for (auto &cmd : commands) {
    cmd();
  }
}

void VulkanCommandList::BeginRenderPass(const RenderPassDescriptor &desc) {
  if (!isRecording)
    return;
  commands.push_back([this, desc]() {
    device->BindFramebuffer(desc.framebuffer);

    device->SetClearColor(desc.clearColor);
    device->SetClearDepth(desc.clearDepth);

    if (desc.clearColorBuffer || desc.clearDepthBuffer ||
        desc.clearStencilBuffer) {
      device->Clear(desc.clearColorBuffer, desc.clearDepthBuffer,
                    desc.clearStencilBuffer);
    }
  });
}

void VulkanCommandList::EndRenderPass() {
  if (!isRecording)
    return;
  commands.push_back([this]() { device->BindFramebuffer({0}); });
}

void VulkanCommandList::SetViewport(const Viewport &viewport) {
  if (!isRecording)
    return;
  commands.push_back([this, viewport]() { device->SetViewport(viewport); });
}

void VulkanCommandList::SetScissor(const Scissor &scissor) {
  if (!isRecording)
    return;
  commands.push_back([this, scissor]() { device->SetScissor(scissor); });
}

void VulkanCommandList::DisableScissor() {
  if (!isRecording)
    return;
  commands.push_back([this]() { device->DisableScissor(); });
}

void VulkanCommandList::BindPipeline(PipelineHandle pipeline) {
  if (!isRecording)
    return;
  commands.push_back([this, pipeline]() { device->BindPipeline(pipeline); });
}

void VulkanCommandList::BindVertexArray(VertexArrayHandle vao) {
  if (!isRecording)
    return;
  commands.push_back([this, vao]() { device->BindVertexArray(vao); });
}

void VulkanCommandList::BindTexture(uint32_t slot, TextureHandle texture) {
  if (!isRecording)
    return;
  commands.push_back(
      [this, slot, texture]() { device->BindTexture(slot, texture); });
}

void VulkanCommandList::BindSampler(uint32_t slot, SamplerHandle sampler) {
  if (!isRecording)
    return;
  commands.push_back(
      [this, slot, sampler]() { device->BindSampler(slot, sampler); });
}

void VulkanCommandList::SetUniform(ShaderHandle shader, const std::string &name,
                                   int value) {
  if (!isRecording)
    return;
  commands.push_back([this, shader, name, value]() {
    device->SetUniform(shader, name, value);
  });
}

void VulkanCommandList::SetUniform(ShaderHandle shader, const std::string &name,
                                   float value) {
  if (!isRecording)
    return;
  commands.push_back([this, shader, name, value]() {
    device->SetUniform(shader, name, value);
  });
}

void VulkanCommandList::SetUniform(ShaderHandle shader, const std::string &name,
                                   const float *value, uint32_t count) {
  if (!isRecording)
    return;
  std::vector<float> valCopy(value, value + count);
  commands.push_back([this, shader, name, valCopy]() {
    device->SetUniform(shader, name, valCopy.data(), valCopy.size());
  });
}

void VulkanCommandList::SetUniformMatrix4(ShaderHandle shader,
                                          const std::string &name,
                                          const float *matrix) {
  if (!isRecording)
    return;
  std::vector<float> matCopy(matrix, matrix + 16);
  commands.push_back([this, shader, name, matCopy]() {
    device->SetUniformMatrix4(shader, name, matCopy.data());
  });
}

void VulkanCommandList::Draw(const DrawCommand &cmd) {
  if (!isRecording)
    return;
  commands.push_back([this, cmd]() { device->Draw(cmd); });
}

void VulkanCommandList::DrawIndexed(const DrawIndexedCommand &cmd) {
  if (!isRecording)
    return;
  commands.push_back([this, cmd]() { device->DrawIndexed(cmd); });
}

void VulkanCommandList::DrawSkybox(TextureHandle cubemap, SamplerHandle sampler,
                                   const float *view, const float *proj) {
  if (!isRecording)
    return;
  std::vector<float> vCopy(view, view + 16);
  std::vector<float> pCopy(proj, proj + 16);
  commands.push_back([this, cubemap, sampler, vCopy, pCopy]() {
    device->DrawSkybox(cubemap, sampler, vCopy.data(), pCopy.data());
  });
}

} // namespace RHI
