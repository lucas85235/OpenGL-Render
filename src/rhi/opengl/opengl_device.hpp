#ifndef RHI_OPENGL_DEVICE_HPP
#define RHI_OPENGL_DEVICE_HPP

#include "../rhi_device.h"
#include <GL/glew.h>
#include <unordered_map>

namespace RHI {

class OpenGLDevice : public IDevice {
private:
  struct BufferObject {
    GLuint id;
    GLenum target;
    GLenum usage;
    uint32_t size;
  };

  struct TextureObject {
    GLuint id;
    GLenum target;
    TextureFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
  };

  struct SamplerObject {
    GLuint id;
  };

  struct ShaderObject {
    GLuint program;
    std::unordered_map<std::string, GLint> uniformLocations;
  };

  struct PipelineObject {
    ShaderHandle shader;
    RasterizerState rasterizer;
    DepthStencilState depthStencil;
    BlendState blend;
    GLenum topology;
  };

  struct VertexArrayObject {
    GLuint vao;
    BufferHandle vertexBuffer;
    BufferHandle indexBuffer;
  };

  struct FramebufferObject {
    GLuint fbo;
    GLuint rbo;
    uint32_t width;
    uint32_t height;
    std::unordered_map<FramebufferAttachment, TextureHandle> attachments;
  };

  std::unordered_map<uint64_t, BufferObject> buffers;
  std::unordered_map<uint64_t, TextureObject> textures;
  std::unordered_map<uint64_t, SamplerObject> samplers;
  std::unordered_map<uint64_t, ShaderObject> shaders;
  std::unordered_map<uint64_t, PipelineObject> pipelines;
  std::unordered_map<uint64_t, VertexArrayObject> vertexArrays;
  std::unordered_map<uint64_t, FramebufferObject> framebuffers;

  uint64_t nextId = 1;
  PipelineHandle currentPipeline;
  ClearColor clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
  float clearDepth = 1.0f;

  GLenum ToGLBufferTarget(BufferType type);
  GLenum ToGLBufferUsage(BufferUsage usage);
  GLenum ToGLTextureTarget(TextureType type);
  GLenum ToGLTextureFormat(TextureFormat format);
  GLenum ToGLTextureInternalFormat(TextureFormat format);
  GLenum ToGLTextureType(TextureFormat format);
  GLenum ToGLWrapMode(TextureWrapMode mode);
  GLenum ToGLFilterMode(TextureFilterMode mode);
  GLenum ToGLPrimitiveTopology(PrimitiveTopology topology);
  GLenum ToGLCompareOp(CompareOp op);
  GLenum ToGLBlendFactor(BlendFactor factor);
  GLenum ToGLBlendOp(BlendOp op);
  GLenum ToGLShaderStage(ShaderStage stage);
  GLenum ToGLFramebufferAttachment(FramebufferAttachment attachment);

  GLint GetAttributeSize(VertexAttributeType type);
  GLenum GetAttributeType(VertexAttributeType type);

  void ApplyRasterizerState(const RasterizerState &state);
  void ApplyDepthStencilState(const DepthStencilState &state);
  void ApplyBlendState(const BlendState &state);

  GLuint CompileShaderStage(const std::string &source, GLenum stage);
  GLuint LinkProgram(const std::vector<GLuint> &stages);

public:
  OpenGLDevice() = default;
  ~OpenGLDevice() override;

  bool Initialize() override;
  void Shutdown() override;
  bool BeginFrame() override;
  void EndFrame() override;
  API GetAPI() const override { return API::OpenGL; }
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
};

} // namespace RHI

#endif // RHI_OPENGL_DEVICE_HPP
