#include "opengl_command_list.hpp"
#include <iostream>

namespace RHI {

void OpenGLCommandList::Begin() {
  commands.clear();
  isRecording = true;
}

void OpenGLCommandList::End() { isRecording = false; }

void OpenGLCommandList::Execute() {
  for (auto &cmd : commands) {
    cmd();
  }
}

void OpenGLCommandList::BeginRenderPass(const RenderPassDescriptor &desc) {
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

void OpenGLCommandList::EndRenderPass() {
  if (!isRecording)
    return;
  commands.push_back([this]() { device->BindFramebuffer({0}); });
}

void OpenGLCommandList::SetViewport(const Viewport &viewport) {
  if (!isRecording)
    return;
  commands.push_back([this, viewport]() { device->SetViewport(viewport); });
}

void OpenGLCommandList::SetScissor(const Scissor &scissor) {
  if (!isRecording)
    return;
  commands.push_back([this, scissor]() { device->SetScissor(scissor); });
}

void OpenGLCommandList::DisableScissor() {
  if (!isRecording)
    return;
  commands.push_back([this]() { device->DisableScissor(); });
}

void OpenGLCommandList::BindPipeline(PipelineHandle pipeline) {
  if (!isRecording)
    return;
  commands.push_back([this, pipeline]() { device->BindPipeline(pipeline); });
}

void OpenGLCommandList::BindVertexArray(VertexArrayHandle vao) {
  if (!isRecording)
    return;
  commands.push_back([this, vao]() { device->BindVertexArray(vao); });
}

void OpenGLCommandList::BindTexture(uint32_t slot, TextureHandle texture) {
  if (!isRecording)
    return;
  commands.push_back(
      [this, slot, texture]() { device->BindTexture(slot, texture); });
}

void OpenGLCommandList::BindSampler(uint32_t slot, SamplerHandle sampler) {
  if (!isRecording)
    return;
  commands.push_back(
      [this, slot, sampler]() { device->BindSampler(slot, sampler); });
}

void OpenGLCommandList::SetUniform(ShaderHandle shader, const std::string &name,
                                   int value) {
  if (!isRecording)
    return;
  commands.push_back([this, shader, name, value]() {
    device->SetUniform(shader, name, value);
  });
}

void OpenGLCommandList::SetUniform(ShaderHandle shader, const std::string &name,
                                   float value) {
  if (!isRecording)
    return;
  commands.push_back([this, shader, name, value]() {
    device->SetUniform(shader, name, value);
  });
}

void OpenGLCommandList::SetUniform(ShaderHandle shader, const std::string &name,
                                   const float *value, uint32_t count) {
  if (!isRecording)
    return;
  // Deep copy the array to avoid dangling pointers
  std::vector<float> valCopy(value, value + count);
  commands.push_back([this, shader, name, valCopy]() {
    device->SetUniform(shader, name, valCopy.data(), valCopy.size());
  });
}

void OpenGLCommandList::SetUniformMatrix4(ShaderHandle shader,
                                          const std::string &name,
                                          const float *matrix) {
  if (!isRecording)
    return;
  std::vector<float> matCopy(matrix, matrix + 16);
  commands.push_back([this, shader, name, matCopy]() {
    device->SetUniformMatrix4(shader, name, matCopy.data());
  });
}

void OpenGLCommandList::Draw(const DrawCommand &cmd) {
  if (!isRecording)
    return;
  commands.push_back([this, cmd]() { device->Draw(cmd); });
}

void OpenGLCommandList::DrawIndexed(const DrawIndexedCommand &cmd) {
  if (!isRecording)
    return;
  commands.push_back([this, cmd]() { device->DrawIndexed(cmd); });
}

void OpenGLCommandList::DrawSkybox(TextureHandle cubemap, SamplerHandle sampler,
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
