#ifndef RHI_COMMAND_LIST_HPP
#define RHI_COMMAND_LIST_HPP

#include "rhi_types.hpp"
#include <string>

namespace RHI {

class ICommandList {
public:
  virtual ~ICommandList() = default;

  virtual void Begin() = 0;
  virtual void End() = 0;

  virtual void BeginRenderPass(const RenderPassDescriptor &desc) = 0;
  virtual void EndRenderPass() = 0;

  virtual void SetViewport(const Viewport &viewport) = 0;
  virtual void SetScissor(const Scissor &scissor) = 0;
  virtual void DisableScissor() = 0;

  virtual void BindPipeline(PipelineHandle pipeline) = 0;
  virtual void BindVertexArray(VertexArrayHandle vao) = 0;
  virtual void BindTexture(uint32_t slot, TextureHandle texture) = 0;
  virtual void BindSampler(uint32_t slot, SamplerHandle sampler) = 0;

  // Legacy uniform binding (to be replaced by Descriptor Sets later)
  virtual void SetUniform(ShaderHandle shader, const std::string &name,
                          int value) = 0;
  virtual void SetUniform(ShaderHandle shader, const std::string &name,
                          float value) = 0;
  virtual void SetUniform(ShaderHandle shader, const std::string &name,
                          const float *value, uint32_t count) = 0;
  virtual void SetUniformMatrix4(ShaderHandle shader, const std::string &name,
                                 const float *matrix) = 0;

  virtual void Draw(const DrawCommand &cmd) = 0;
  virtual void DrawIndexed(const DrawIndexedCommand &cmd) = 0;
};

} // namespace RHI

#endif // RHI_COMMAND_LIST_HPP
