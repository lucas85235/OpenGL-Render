#include "opengl_device.hpp"
#include "../shader_cross_compiler.hpp"
#include <cstring>
#include <iostream>

#include "imgui.h"
#include "imgui_impl_opengl3.h"

namespace RHI {

OpenGLDevice::~OpenGLDevice() { Shutdown(); }

bool OpenGLDevice::Initialize() {
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);

  // Enable Vulkan-compatible depth range [0,1] for unified shaders
  // This allows SPIR-V shaders written for Vulkan to work correctly in OpenGL
  if (GLEW_ARB_clip_control) {
    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
    std::cout << "[OpenGL] Depth range set to [0,1] (Vulkan-compatible)"
              << std::endl;
  } else {
    std::cout << "[OpenGL] Warning: ARB_clip_control not available"
              << std::endl;
  }

  std::cout << "[OpenGL] Device initialized" << std::endl;
  return true;
}

DeviceInfo OpenGLDevice::GetDeviceInfo() const {
  DeviceInfo info;

  info.vendorName = reinterpret_cast<const char *>(glGetString(GL_VENDOR));
  info.rendererName = reinterpret_cast<const char *>(glGetString(GL_RENDERER));
  info.apiVersion = reinterpret_cast<const char *>(glGetString(GL_VERSION));

  GLint value;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &value);
  info.maxTextureSize = static_cast<uint32_t>(value);

  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &value);
  info.maxTextureUnits = static_cast<uint32_t>(value);

  glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &value);
  info.maxVertexAttributes = static_cast<uint32_t>(value);

  glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
  info.maxUniformBufferSize = static_cast<uint32_t>(value);

  glGetIntegerv(GL_MAX_COLOR_ATTACHMENTS, &value);
  info.maxColorAttachments = static_cast<uint32_t>(value);

  GLint major, minor;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);

  info.supportsCompute = (major > 4) || (major == 4 && minor >= 3);
  info.supportsGeometryShader = (major >= 3 && minor >= 2) || major > 3;
  info.supportsTessellation = (major > 4) || (major == 4 && minor >= 0);

  return info;
}

void OpenGLDevice::Shutdown() {
  for (auto &[id, buf] : buffers)
    glDeleteBuffers(1, &buf.id);
  for (auto &[id, tex] : textures)
    glDeleteTextures(1, &tex.id);
  for (auto &[id, smp] : samplers)
    glDeleteSamplers(1, &smp.id);
  for (auto &[id, shd] : shaders)
    glDeleteProgram(shd.program);
  for (auto &[id, vao] : vertexArrays)
    glDeleteVertexArrays(1, &vao.vao);
  for (auto &[id, fb] : framebuffers) {
    glDeleteFramebuffers(1, &fb.fbo);
    if (fb.rbo)
      glDeleteRenderbuffers(1, &fb.rbo);
  }

  buffers.clear();
  textures.clear();
  samplers.clear();
  shaders.clear();
  pipelines.clear();
  vertexArrays.clear();
  framebuffers.clear();
}

bool OpenGLDevice::BeginFrame() {
  glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_DEPTH_TEST);
  return true;
}

void OpenGLDevice::EndFrame() {}

GLenum OpenGLDevice::ToGLBufferTarget(BufferType type) {
  switch (type) {
  case BufferType::Vertex:
    return GL_ARRAY_BUFFER;
  case BufferType::Index:
    return GL_ELEMENT_ARRAY_BUFFER;
  case BufferType::Uniform:
    return GL_UNIFORM_BUFFER;
  default:
    return GL_ARRAY_BUFFER;
  }
}

GLenum OpenGLDevice::ToGLBufferUsage(BufferUsage usage) {
  switch (usage) {
  case BufferUsage::Static:
    return GL_STATIC_DRAW;
  case BufferUsage::Dynamic:
    return GL_DYNAMIC_DRAW;
  case BufferUsage::Stream:
    return GL_STREAM_DRAW;
  default:
    return GL_STATIC_DRAW;
  }
}

GLenum OpenGLDevice::ToGLTextureTarget(TextureType type) {
  switch (type) {
  case TextureType::Texture2D:
    return GL_TEXTURE_2D;
  case TextureType::Texture3D:
    return GL_TEXTURE_3D;
  case TextureType::TextureCube:
    return GL_TEXTURE_CUBE_MAP;
  default:
    return GL_TEXTURE_2D;
  }
}

GLenum OpenGLDevice::ToGLTextureFormat(TextureFormat format) {
  switch (format) {
  case TextureFormat::R8:
    return GL_RED;
  case TextureFormat::RG8:
    return GL_RG;
  case TextureFormat::RGB8:
    return GL_RGB;
  case TextureFormat::RGBA8:
    return GL_RGBA;
  case TextureFormat::R16F:
    return GL_RED;
  case TextureFormat::RG16F:
    return GL_RG;
  case TextureFormat::RGB16F:
    return GL_RGB;
  case TextureFormat::RGBA16F:
    return GL_RGBA;
  case TextureFormat::R32F:
    return GL_RED;
  case TextureFormat::RG32F:
    return GL_RG;
  case TextureFormat::RGB32F:
    return GL_RGB;
  case TextureFormat::RGBA32F:
    return GL_RGBA;
  case TextureFormat::Depth24Stencil8:
    return GL_DEPTH_STENCIL;
  case TextureFormat::Depth32F:
    return GL_DEPTH_COMPONENT;
  case TextureFormat::SRGB8:
    return GL_RGB;
  case TextureFormat::SRGB8_Alpha8:
    return GL_RGBA;
  default:
    return GL_RGBA;
  }
}

GLenum OpenGLDevice::ToGLTextureInternalFormat(TextureFormat format) {
  switch (format) {
  case TextureFormat::R8:
    return GL_R8;
  case TextureFormat::RG8:
    return GL_RG8;
  case TextureFormat::RGB8:
    return GL_RGB8;
  case TextureFormat::RGBA8:
    return GL_RGBA8;
  case TextureFormat::R16F:
    return GL_R16F;
  case TextureFormat::RG16F:
    return GL_RG16F;
  case TextureFormat::RGB16F:
    return GL_RGB16F;
  case TextureFormat::RGBA16F:
    return GL_RGBA16F;
  case TextureFormat::R32F:
    return GL_R32F;
  case TextureFormat::RG32F:
    return GL_RG32F;
  case TextureFormat::RGB32F:
    return GL_RGB32F;
  case TextureFormat::RGBA32F:
    return GL_RGBA32F;
  case TextureFormat::Depth24Stencil8:
    return GL_DEPTH24_STENCIL8;
  case TextureFormat::Depth32F:
    return GL_DEPTH_COMPONENT32F;
  case TextureFormat::SRGB8:
    return GL_SRGB8;
  case TextureFormat::SRGB8_Alpha8:
    return GL_SRGB8_ALPHA8;
  default:
    return GL_RGBA8;
  }
}

GLenum OpenGLDevice::ToGLTextureType(TextureFormat format) {
  switch (format) {
  case TextureFormat::R16F:
  case TextureFormat::RG16F:
  case TextureFormat::RGB16F:
  case TextureFormat::RGBA16F:
    return GL_HALF_FLOAT;
  case TextureFormat::R32F:
  case TextureFormat::RG32F:
  case TextureFormat::RGB32F:
  case TextureFormat::RGBA32F:
  case TextureFormat::Depth32F:
    return GL_FLOAT;
  case TextureFormat::Depth24Stencil8:
    return GL_UNSIGNED_INT_24_8;
  default:
    return GL_UNSIGNED_BYTE;
  }
}

GLenum OpenGLDevice::ToGLWrapMode(TextureWrapMode mode) {
  switch (mode) {
  case TextureWrapMode::Repeat:
    return GL_REPEAT;
  case TextureWrapMode::MirroredRepeat:
    return GL_MIRRORED_REPEAT;
  case TextureWrapMode::ClampToEdge:
    return GL_CLAMP_TO_EDGE;
  case TextureWrapMode::ClampToBorder:
    return GL_CLAMP_TO_BORDER;
  default:
    return GL_REPEAT;
  }
}

GLenum OpenGLDevice::ToGLFilterMode(TextureFilterMode mode) {
  switch (mode) {
  case TextureFilterMode::Nearest:
    return GL_NEAREST;
  case TextureFilterMode::Linear:
    return GL_LINEAR;
  case TextureFilterMode::NearestMipmapNearest:
    return GL_NEAREST_MIPMAP_NEAREST;
  case TextureFilterMode::NearestMipmapLinear:
    return GL_NEAREST_MIPMAP_LINEAR;
  case TextureFilterMode::LinearMipmapNearest:
    return GL_LINEAR_MIPMAP_NEAREST;
  case TextureFilterMode::LinearMipmapLinear:
    return GL_LINEAR_MIPMAP_LINEAR;
  default:
    return GL_LINEAR;
  }
}

GLenum OpenGLDevice::ToGLPrimitiveTopology(PrimitiveTopology topology) {
  switch (topology) {
  case PrimitiveTopology::TriangleList:
    return GL_TRIANGLES;
  case PrimitiveTopology::TriangleStrip:
    return GL_TRIANGLE_STRIP;
  case PrimitiveTopology::LineList:
    return GL_LINES;
  case PrimitiveTopology::LineStrip:
    return GL_LINE_STRIP;
  case PrimitiveTopology::PointList:
    return GL_POINTS;
  default:
    return GL_TRIANGLES;
  }
}

GLenum OpenGLDevice::ToGLCompareOp(CompareOp op) {
  switch (op) {
  case CompareOp::Never:
    return GL_NEVER;
  case CompareOp::Less:
    return GL_LESS;
  case CompareOp::Equal:
    return GL_EQUAL;
  case CompareOp::LessOrEqual:
    return GL_LEQUAL;
  case CompareOp::Greater:
    return GL_GREATER;
  case CompareOp::NotEqual:
    return GL_NOTEQUAL;
  case CompareOp::GreaterOrEqual:
    return GL_GEQUAL;
  case CompareOp::Always:
    return GL_ALWAYS;
  default:
    return GL_LESS;
  }
}

GLenum OpenGLDevice::ToGLBlendFactor(BlendFactor factor) {
  switch (factor) {
  case BlendFactor::Zero:
    return GL_ZERO;
  case BlendFactor::One:
    return GL_ONE;
  case BlendFactor::SrcColor:
    return GL_SRC_COLOR;
  case BlendFactor::OneMinusSrcColor:
    return GL_ONE_MINUS_SRC_COLOR;
  case BlendFactor::DstColor:
    return GL_DST_COLOR;
  case BlendFactor::OneMinusDstColor:
    return GL_ONE_MINUS_DST_COLOR;
  case BlendFactor::SrcAlpha:
    return GL_SRC_ALPHA;
  case BlendFactor::OneMinusSrcAlpha:
    return GL_ONE_MINUS_SRC_ALPHA;
  case BlendFactor::DstAlpha:
    return GL_DST_ALPHA;
  case BlendFactor::OneMinusDstAlpha:
    return GL_ONE_MINUS_DST_ALPHA;
  default:
    return GL_ONE;
  }
}

GLenum OpenGLDevice::ToGLBlendOp(BlendOp op) {
  switch (op) {
  case BlendOp::Add:
    return GL_FUNC_ADD;
  case BlendOp::Subtract:
    return GL_FUNC_SUBTRACT;
  case BlendOp::ReverseSubtract:
    return GL_FUNC_REVERSE_SUBTRACT;
  case BlendOp::Min:
    return GL_MIN;
  case BlendOp::Max:
    return GL_MAX;
  default:
    return GL_FUNC_ADD;
  }
}

GLenum OpenGLDevice::ToGLShaderStage(ShaderStage stage) {
  switch (stage) {
  case ShaderStage::Vertex:
    return GL_VERTEX_SHADER;
  case ShaderStage::Fragment:
    return GL_FRAGMENT_SHADER;
  case ShaderStage::Geometry:
    return GL_GEOMETRY_SHADER;
  case ShaderStage::Compute:
    return GL_COMPUTE_SHADER;
  default:
    return GL_VERTEX_SHADER;
  }
}

GLenum
OpenGLDevice::ToGLFramebufferAttachment(FramebufferAttachment attachment) {
  switch (attachment) {
  case FramebufferAttachment::Color0:
    return GL_COLOR_ATTACHMENT0;
  case FramebufferAttachment::Color1:
    return GL_COLOR_ATTACHMENT1;
  case FramebufferAttachment::Color2:
    return GL_COLOR_ATTACHMENT2;
  case FramebufferAttachment::Color3:
    return GL_COLOR_ATTACHMENT3;
  case FramebufferAttachment::Depth:
    return GL_DEPTH_ATTACHMENT;
  case FramebufferAttachment::Stencil:
    return GL_STENCIL_ATTACHMENT;
  case FramebufferAttachment::DepthStencil:
    return GL_DEPTH_STENCIL_ATTACHMENT;
  default:
    return GL_COLOR_ATTACHMENT0;
  }
}

GLint OpenGLDevice::GetAttributeSize(VertexAttributeType type) {
  switch (type) {
  case VertexAttributeType::Float:
    return 1;
  case VertexAttributeType::Float2:
    return 2;
  case VertexAttributeType::Float3:
    return 3;
  case VertexAttributeType::Float4:
    return 4;
  case VertexAttributeType::Int:
    return 1;
  case VertexAttributeType::Int2:
    return 2;
  case VertexAttributeType::Int3:
    return 3;
  case VertexAttributeType::Int4:
    return 4;
  case VertexAttributeType::UInt:
    return 1;
  case VertexAttributeType::UInt2:
    return 2;
  case VertexAttributeType::UInt3:
    return 3;
  case VertexAttributeType::UInt4:
    return 4;
  default:
    return 1;
  }
}

GLenum OpenGLDevice::GetAttributeType(VertexAttributeType type) {
  switch (type) {
  case VertexAttributeType::Float:
  case VertexAttributeType::Float2:
  case VertexAttributeType::Float3:
  case VertexAttributeType::Float4:
    return GL_FLOAT;
  case VertexAttributeType::Int:
  case VertexAttributeType::Int2:
  case VertexAttributeType::Int3:
  case VertexAttributeType::Int4:
    return GL_INT;
  case VertexAttributeType::UInt:
  case VertexAttributeType::UInt2:
  case VertexAttributeType::UInt3:
  case VertexAttributeType::UInt4:
    return GL_UNSIGNED_INT;
  default:
    return GL_FLOAT;
  }
}

void OpenGLDevice::ApplyRasterizerState(const RasterizerState &state) {
  switch (state.cullMode) {
  case CullMode::None:
    glDisable(GL_CULL_FACE);
    break;
  case CullMode::Front:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    break;
  case CullMode::Back:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    break;
  case CullMode::FrontAndBack:
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT_AND_BACK);
    break;
  }

  glFrontFace(state.frontFace == FrontFace::Clockwise ? GL_CW : GL_CCW);

  if (state.depthBiasEnable) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(state.depthBiasSlope, state.depthBiasConstant);
  } else {
    glDisable(GL_POLYGON_OFFSET_FILL);
  }
}

void OpenGLDevice::ApplyDepthStencilState(const DepthStencilState &state) {
  if (state.depthTestEnable) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(ToGLCompareOp(state.depthCompareOp));
  } else {
    glDisable(GL_DEPTH_TEST);
  }

  glDepthMask(state.depthWriteEnable ? GL_TRUE : GL_FALSE);

  if (state.stencilTestEnable) {
    glEnable(GL_STENCIL_TEST);
  } else {
    glDisable(GL_STENCIL_TEST);
  }
}

void OpenGLDevice::ApplyBlendState(const BlendState &state) {
  if (state.blendEnable) {
    glEnable(GL_BLEND);
    glBlendFuncSeparate(ToGLBlendFactor(state.srcColorFactor),
                        ToGLBlendFactor(state.dstColorFactor),
                        ToGLBlendFactor(state.srcAlphaFactor),
                        ToGLBlendFactor(state.dstAlphaFactor));
    glBlendEquationSeparate(ToGLBlendOp(state.colorOp),
                            ToGLBlendOp(state.alphaOp));
  } else {
    glDisable(GL_BLEND);
  }
}

BufferHandle OpenGLDevice::CreateBuffer(const BufferDescriptor &desc) {
  BufferObject buffer;
  buffer.target = ToGLBufferTarget(desc.type);
  buffer.usage = ToGLBufferUsage(desc.usage);
  buffer.size = desc.size;

  glGenBuffers(1, &buffer.id);
  glBindBuffer(buffer.target, buffer.id);
  glBufferData(buffer.target, desc.size, desc.data, buffer.usage);
  glBindBuffer(buffer.target, 0);

  BufferHandle handle{nextId++};
  buffers[handle.id] = buffer;

  return handle;
}

void OpenGLDevice::UpdateBuffer(BufferHandle buffer, const void *data,
                                uint32_t size, uint32_t offset) {
  auto it = buffers.find(buffer.id);
  if (it == buffers.end())
    return;

  glBindBuffer(it->second.target, it->second.id);
  glBufferSubData(it->second.target, offset, size, data);
  glBindBuffer(it->second.target, 0);
}

void OpenGLDevice::DestroyBuffer(BufferHandle buffer) {
  auto it = buffers.find(buffer.id);
  if (it != buffers.end()) {
    glDeleteBuffers(1, &it->second.id);
    buffers.erase(it);
  }
}

TextureHandle OpenGLDevice::CreateTexture(const TextureDescriptor &desc) {
  TextureObject texture;
  texture.target = ToGLTextureTarget(desc.type);
  texture.format = desc.format;
  texture.width = desc.width;
  texture.height = desc.height;
  texture.depth = desc.depth;

  glGenTextures(1, &texture.id);
  glBindTexture(texture.target, texture.id);

  GLenum format = ToGLTextureFormat(desc.format);
  GLenum internalFormat = ToGLTextureInternalFormat(desc.format);
  GLenum type = ToGLTextureType(desc.format);

  if (desc.type == TextureType::Texture2D) {
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, desc.width, desc.height, 0,
                 format, type, desc.data);
  } else if (desc.type == TextureType::Texture3D) {
    glTexImage3D(GL_TEXTURE_3D, 0, internalFormat, desc.width, desc.height,
                 desc.depth, 0, format, type, desc.data);
  } else if (desc.type == TextureType::TextureCube) {
    for (int i = 0; i < 6; i++) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat,
                   desc.width, desc.height, 0, format, type, nullptr);
    }
  }

  if (desc.generateMipmaps) {
    glGenerateMipmap(texture.target);
    glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER,
                    GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  } else {
    // Without mipmaps, the default GL_NEAREST_MIPMAP_LINEAR makes the texture
    // incomplete. Set GL_LINEAR to ensure valid sampling.
    glTexParameteri(texture.target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(texture.target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }
  glTexParameteri(texture.target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(texture.target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glBindTexture(texture.target, 0);

  TextureHandle handle{nextId++};
  textures[handle.id] = texture;

  return handle;
}

void OpenGLDevice::UpdateTexture(TextureHandle texture, const void *data,
                                 uint32_t mipLevel) {
  auto it = textures.find(texture.id);
  if (it == textures.end())
    return;

  glBindTexture(it->second.target, it->second.id);

  GLenum format = ToGLTextureFormat(it->second.format);
  GLenum type = ToGLTextureType(it->second.format);

  glTexSubImage2D(it->second.target, mipLevel, 0, 0, it->second.width,
                  it->second.height, format, type, data);

  glBindTexture(it->second.target, 0);
}

void OpenGLDevice::UpdateTextureCubeFace(TextureHandle texture,
                                         CubemapFace face, const void *data,
                                         uint32_t mipLevel) {
  auto it = textures.find(texture.id);
  if (it == textures.end())
    return;

  if (it->second.target != GL_TEXTURE_CUBE_MAP) {
    std::cerr << "[OpenGL] UpdateTextureCubeFace: texture is not a cubemap"
              << std::endl;
    return;
  }

  glBindTexture(GL_TEXTURE_CUBE_MAP, it->second.id);

  GLenum format = ToGLTextureFormat(it->second.format);
  GLenum internalFormat = ToGLTextureInternalFormat(it->second.format);
  GLenum type = ToGLTextureType(it->second.format);
  GLenum faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X + static_cast<int>(face);

  glTexImage2D(faceTarget, mipLevel, internalFormat, it->second.width,
               it->second.height, 0, format, type, data);

  glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}

void OpenGLDevice::GenerateMipmaps(TextureHandle texture) {
  auto it = textures.find(texture.id);
  if (it == textures.end())
    return;

  glBindTexture(it->second.target, it->second.id);
  glGenerateMipmap(it->second.target);
  glBindTexture(it->second.target, 0);
}

void OpenGLDevice::DestroyTexture(TextureHandle texture) {
  auto it = textures.find(texture.id);
  if (it != textures.end()) {
    glDeleteTextures(1, &it->second.id);
    textures.erase(it);
  }
}

uint64_t OpenGLDevice::GetNativeTextureID(TextureHandle texture) const {
  auto it = textures.find(texture.id);
  if (it != textures.end()) {
    return static_cast<uint64_t>(it->second.id);
  }
  return 0;
}

SamplerHandle OpenGLDevice::CreateSampler(const SamplerDescriptor &desc) {
  SamplerObject sampler;
  glGenSamplers(1, &sampler.id);

  glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_S, ToGLWrapMode(desc.wrapS));
  glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_T, ToGLWrapMode(desc.wrapT));
  glSamplerParameteri(sampler.id, GL_TEXTURE_WRAP_R, ToGLWrapMode(desc.wrapR));
  glSamplerParameteri(sampler.id, GL_TEXTURE_MIN_FILTER,
                      ToGLFilterMode(desc.minFilter));
  glSamplerParameteri(sampler.id, GL_TEXTURE_MAG_FILTER,
                      ToGLFilterMode(desc.magFilter));

  if (desc.anisotropy > 1.0f) {
    glSamplerParameterf(sampler.id, GL_TEXTURE_MAX_ANISOTROPY, desc.anisotropy);
  }

  SamplerHandle handle{nextId++};
  samplers[handle.id] = sampler;

  return handle;
}

void OpenGLDevice::DestroySampler(SamplerHandle sampler) {
  auto it = samplers.find(sampler.id);
  if (it != samplers.end()) {
    glDeleteSamplers(1, &it->second.id);
    samplers.erase(it);
  }
}

GLuint OpenGLDevice::CompileShaderStage(const std::string &source,
                                        GLenum stage) {
  GLuint shader = glCreateShader(stage);
  const char *src = source.c_str();
  glShaderSource(shader, 1, &src, nullptr);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    std::cerr << "[OpenGL] Shader compilation failed: " << infoLog << std::endl;
    glDeleteShader(shader);
    return 0;
  }

  return shader;
}

GLuint OpenGLDevice::LinkProgram(const std::vector<GLuint> &stages) {
  GLuint program = glCreateProgram();

  for (GLuint stage : stages) {
    glAttachShader(program, stage);
  }

  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    std::cerr << "[OpenGL] Program linking failed: " << infoLog << std::endl;
    glDeleteProgram(program);
    return 0;
  }

  for (GLuint stage : stages) {
    glDeleteShader(stage);
  }

  return program;
}

ShaderHandle
OpenGLDevice::CreateShader(const std::vector<ShaderDescriptor> &stages) {
  std::vector<GLuint> compiledStages;

  for (const auto &stage : stages) {
    std::string glslSource = stage.source;

    // If SPIR-V provided, transpile to GLSL via ShaderCrossCompiler
    if (stage.useSPIRV && !stage.spirvBinary.empty()) {
#if SPIRV_CROSS_AVAILABLE
      ShaderStageType stageType = ShaderStageType::Vertex;
      if (stage.stage == ShaderStage::Fragment) {
        stageType = ShaderStageType::Fragment;
      } else if (stage.stage == ShaderStage::Geometry) {
        stageType = ShaderStageType::Geometry;
      } else if (stage.stage == ShaderStage::Compute) {
        stageType = ShaderStageType::Compute;
      }

      auto result = ShaderCrossCompiler::Process(
          stage.spirvBinary, RenderAPI::OpenGL, stageType, 450);

      if (!result.success) {
        std::cerr << "[OpenGL] SPIR-V transpilation failed: "
                  << result.errorMessage << std::endl;
        for (GLuint s : compiledStages)
          glDeleteShader(s);
        return ShaderHandle{0};
      }
      glslSource = result.glslSource;
      std::cout << "[OpenGL] Transpiled SPIR-V to GLSL 450" << std::endl;
#else
      std::cerr
          << "[OpenGL] SPIRV-Cross not available, cannot use SPIR-V shaders"
          << std::endl;
      return ShaderHandle{0};
#endif
    }

    GLuint compiled =
        CompileShaderStage(glslSource, ToGLShaderStage(stage.stage));
    if (compiled == 0) {
      for (GLuint s : compiledStages)
        glDeleteShader(s);
      return ShaderHandle{0};
    }
    compiledStages.push_back(compiled);
  }

  GLuint program = LinkProgram(compiledStages);
  if (program == 0) {
    return ShaderHandle{0};
  }

  ShaderObject shader;
  shader.program = program;

  ShaderHandle handle{nextId++};
  shaders[handle.id] = shader;

  return handle;
}

void OpenGLDevice::DestroyShader(ShaderHandle shader) {
  auto it = shaders.find(shader.id);
  if (it != shaders.end()) {
    glDeleteProgram(it->second.program);
    shaders.erase(it);
  }
}

PipelineHandle OpenGLDevice::CreatePipeline(const PipelineDescriptor &desc,
                                            ShaderHandle shader,
                                            const VertexLayout &layout) {
  PipelineObject pipeline;
  pipeline.shader = shader;
  pipeline.rasterizer = desc.rasterizer;
  pipeline.depthStencil = desc.depthStencil;
  pipeline.blend = desc.blend;
  pipeline.topology = ToGLPrimitiveTopology(desc.topology);

  PipelineHandle handle{nextId++};
  pipelines[handle.id] = pipeline;

  return handle;
}

void OpenGLDevice::DestroyPipeline(PipelineHandle pipeline) {
  pipelines.erase(pipeline.id);
}

VertexArrayHandle OpenGLDevice::CreateVertexArray(BufferHandle vertexBuffer,
                                                  BufferHandle indexBuffer,
                                                  const VertexLayout &layout) {
  auto vbIt = buffers.find(vertexBuffer.id);
  if (vbIt == buffers.end())
    return VertexArrayHandle{0};

  VertexArrayObject vao;
  glGenVertexArrays(1, &vao.vao);
  glBindVertexArray(vao.vao);

  glBindBuffer(GL_ARRAY_BUFFER, vbIt->second.id);

  for (const auto &attr : layout.attributes) {
    glEnableVertexAttribArray(attr.location);
    glVertexAttribPointer(attr.location, GetAttributeSize(attr.type),
                          GetAttributeType(attr.type),
                          attr.normalized ? GL_TRUE : GL_FALSE, layout.stride,
                          (void *)(uintptr_t)attr.offset);
  }

  if (IsValid(indexBuffer)) {
    auto ibIt = buffers.find(indexBuffer.id);
    if (ibIt != buffers.end()) {
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibIt->second.id);
      vao.indexBuffer = indexBuffer;
    }
  }

  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  vao.vertexBuffer = vertexBuffer;

  VertexArrayHandle handle{nextId++};
  vertexArrays[handle.id] = vao;

  return handle;
}

void OpenGLDevice::DestroyVertexArray(VertexArrayHandle vao) {
  auto it = vertexArrays.find(vao.id);
  if (it != vertexArrays.end()) {
    glDeleteVertexArrays(1, &it->second.vao);
    vertexArrays.erase(it);
  }
}

FramebufferHandle
OpenGLDevice::CreateFramebuffer(const FramebufferDescriptor &desc) {
  FramebufferObject fb;
  fb.width = desc.width;
  fb.height = desc.height;

  glGenFramebuffers(1, &fb.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

  // Attach color textures from RenderTargetAttachment
  for (size_t i = 0; i < desc.colorAttachments.size(); i++) {
    const auto &attachment = desc.colorAttachments[i];
    auto texIt = textures.find(attachment.texture.id);
    if (texIt == textures.end())
      continue;

    GLenum glAttachment = GL_COLOR_ATTACHMENT0 + static_cast<GLenum>(i);

    if (texIt->second.target == GL_TEXTURE_CUBE_MAP) {
      // Attach cubemap face
      GLenum faceTarget = GL_TEXTURE_CUBE_MAP_POSITIVE_X +
                          static_cast<int>(attachment.cubeFace);
      glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachment, faceTarget,
                             texIt->second.id, attachment.mipLevel);
    } else {
      glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachment, texIt->second.target,
                             texIt->second.id, attachment.mipLevel);
    }

    FramebufferAttachment rhiAttachment = static_cast<FramebufferAttachment>(
        static_cast<int>(FramebufferAttachment::Color0) + i);
    fb.attachments[rhiAttachment] = attachment.texture;
  }

  // Attach depth if specified
  if (desc.hasDepth) {
    if (IsValid(desc.depthAttachment.texture)) {
      auto texIt = textures.find(desc.depthAttachment.texture.id);
      if (texIt != textures.end()) {
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                               texIt->second.target, texIt->second.id,
                               desc.depthAttachment.mipLevel);
        fb.attachments[FramebufferAttachment::Depth] =
            desc.depthAttachment.texture;
      }
    } else {
      // Create renderbuffer for depth
      glGenRenderbuffers(1, &fb.rbo);
      glBindRenderbuffer(GL_RENDERBUFFER, fb.rbo);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, desc.width,
                            desc.height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
                                GL_RENDERBUFFER, fb.rbo);
    }
  }

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
    std::cerr << "[OpenGL] Framebuffer is not complete!" << std::endl;
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  FramebufferHandle handle{nextId++};
  framebuffers[handle.id] = fb;

  return handle;
}

void OpenGLDevice::AttachTexture(FramebufferHandle framebuffer,
                                 FramebufferAttachment attachment,
                                 TextureHandle texture) {
  auto fbIt = framebuffers.find(framebuffer.id);
  auto texIt = textures.find(texture.id);

  if (fbIt == framebuffers.end() || texIt == textures.end())
    return;

  glBindFramebuffer(GL_FRAMEBUFFER, fbIt->second.fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, ToGLFramebufferAttachment(attachment),
                         GL_TEXTURE_2D, texIt->second.id, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  fbIt->second.attachments[attachment] = texture;
}

TextureHandle
OpenGLDevice::GetFramebufferTexture(FramebufferHandle framebuffer,
                                    FramebufferAttachment attachment) {
  auto it = framebuffers.find(framebuffer.id);
  if (it == framebuffers.end())
    return TextureHandle{0};

  auto texIt = it->second.attachments.find(attachment);
  if (texIt == it->second.attachments.end())
    return TextureHandle{0};

  return texIt->second;
}

void OpenGLDevice::ResizeFramebuffer(FramebufferHandle framebuffer,
                                     uint32_t width, uint32_t height) {
  auto it = framebuffers.find(framebuffer.id);
  if (it == framebuffers.end())
    return;

  it->second.width = width;
  it->second.height = height;

  for (auto &[attachment, texHandle] : it->second.attachments) {
    auto texIt = textures.find(texHandle.id);
    if (texIt != textures.end()) {
      texIt->second.width = width;
      texIt->second.height = height;

      glBindTexture(GL_TEXTURE_2D, texIt->second.id);
      glTexImage2D(GL_TEXTURE_2D, 0,
                   ToGLTextureInternalFormat(texIt->second.format), width,
                   height, 0, ToGLTextureFormat(texIt->second.format),
                   ToGLTextureType(texIt->second.format), nullptr);
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }

  if (it->second.rbo) {
    glBindRenderbuffer(GL_RENDERBUFFER, it->second.rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
  }
}

void OpenGLDevice::DestroyFramebuffer(FramebufferHandle framebuffer) {
  auto it = framebuffers.find(framebuffer.id);
  if (it != framebuffers.end()) {
    glDeleteFramebuffers(1, &it->second.fbo);
    if (it->second.rbo) {
      glDeleteRenderbuffers(1, &it->second.rbo);
    }

    for (auto &[attachment, texHandle] : it->second.attachments) {
      DestroyTexture(texHandle);
    }

    framebuffers.erase(it);
  }
}

void OpenGLDevice::SetViewport(const Viewport &viewport) {
  glViewport(static_cast<GLint>(viewport.x), static_cast<GLint>(viewport.y),
             static_cast<GLsizei>(viewport.width),
             static_cast<GLsizei>(viewport.height));
  glDepthRangef(viewport.minDepth, viewport.maxDepth);
}

void OpenGLDevice::SetScissor(const Scissor &scissor) {
  glEnable(GL_SCISSOR_TEST);
  glScissor(scissor.x, scissor.y, scissor.width, scissor.height);
}

void OpenGLDevice::DisableScissor() { glDisable(GL_SCISSOR_TEST); }

void OpenGLDevice::Clear(bool color, bool depth, bool stencil) {
  GLbitfield mask = 0;
  if (color)
    mask |= GL_COLOR_BUFFER_BIT;
  if (depth)
    mask |= GL_DEPTH_BUFFER_BIT;
  if (stencil)
    mask |= GL_STENCIL_BUFFER_BIT;

  glClear(mask);
}

void OpenGLDevice::SetClearColor(const ClearColor &color) {
  clearColor = color;
  glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLDevice::SetClearDepth(float depth) {
  clearDepth = depth;
  glClearDepth(depth);
}

void OpenGLDevice::BindPipeline(PipelineHandle pipeline) {
  auto it = pipelines.find(pipeline.id);
  if (it == pipelines.end())
    return;

  currentPipeline = pipeline;

  auto shaderIt = shaders.find(it->second.shader.id);
  if (shaderIt != shaders.end()) {
    glUseProgram(shaderIt->second.program);
  }

  ApplyRasterizerState(it->second.rasterizer);
  ApplyDepthStencilState(it->second.depthStencil);
  ApplyBlendState(it->second.blend);
}

void OpenGLDevice::BindVertexArray(VertexArrayHandle vao) {
  auto it = vertexArrays.find(vao.id);
  if (it != vertexArrays.end()) {
    glBindVertexArray(it->second.vao);
  }
}

void OpenGLDevice::BindFramebuffer(FramebufferHandle framebuffer) {
  if (IsValid(framebuffer)) {
    auto it = framebuffers.find(framebuffer.id);
    if (it != framebuffers.end()) {
      glBindFramebuffer(GL_FRAMEBUFFER, it->second.fbo);
      glViewport(0, 0, it->second.width, it->second.height);
    }
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
}

void OpenGLDevice::BindTexture(uint32_t slot, TextureHandle texture) {
  auto it = textures.find(texture.id);
  if (it != textures.end()) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(it->second.target, it->second.id);
  }
}

void OpenGLDevice::BindSampler(uint32_t slot, SamplerHandle sampler) {
  auto it = samplers.find(sampler.id);
  if (it != samplers.end()) {
    glBindSampler(slot, it->second.id);
  }
}

void OpenGLDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              int value) {
  auto it = shaders.find(shader.id);
  if (it == shaders.end())
    return;

  auto &cache = it->second.uniformLocations;
  auto locIt = cache.find(name);
  GLint location;

  if (locIt == cache.end()) {
    location = glGetUniformLocation(it->second.program, name.c_str());
    cache[name] = location;
  } else {
    location = locIt->second;
  }

  if (location != -1) {
    glUniform1i(location, value);
  }
}

void OpenGLDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              float value) {
  auto it = shaders.find(shader.id);
  if (it == shaders.end())
    return;

  auto &cache = it->second.uniformLocations;
  auto locIt = cache.find(name);
  GLint location;

  if (locIt == cache.end()) {
    location = glGetUniformLocation(it->second.program, name.c_str());
    cache[name] = location;
  } else {
    location = locIt->second;
  }

  if (location != -1) {
    glUniform1f(location, value);
  }
}

void OpenGLDevice::SetUniform(ShaderHandle shader, const std::string &name,
                              const float *value, uint32_t count) {
  auto it = shaders.find(shader.id);
  if (it == shaders.end())
    return;

  auto &cache = it->second.uniformLocations;
  auto locIt = cache.find(name);
  GLint location;

  if (locIt == cache.end()) {
    location = glGetUniformLocation(it->second.program, name.c_str());
    cache[name] = location;
  } else {
    location = locIt->second;
  }

  if (location != -1) {
    if (count == 1)
      glUniform1f(location, value[0]);
    else if (count == 2)
      glUniform2fv(location, 1, value);
    else if (count == 3)
      glUniform3fv(location, 1, value);
    else if (count == 4)
      glUniform4fv(location, 1, value);
  }
}

void OpenGLDevice::SetUniformMatrix4(ShaderHandle shader,
                                     const std::string &name,
                                     const float *matrix) {
  auto it = shaders.find(shader.id);
  if (it == shaders.end())
    return;

  auto &cache = it->second.uniformLocations;
  auto locIt = cache.find(name);
  GLint location;

  if (locIt == cache.end()) {
    location = glGetUniformLocation(it->second.program, name.c_str());
    cache[name] = location;
  } else {
    location = locIt->second;
  }

  if (location != -1) {
    glUniformMatrix4fv(location, 1, GL_FALSE, matrix);
  }
}

void OpenGLDevice::Draw(const DrawCommand &cmd) {
  auto it = pipelines.find(currentPipeline.id);
  if (it == pipelines.end())
    return;

  GLenum topology = it->second.topology;

  if (cmd.instanceCount > 1) {
    glDrawArraysInstanced(topology, cmd.firstVertex, cmd.vertexCount,
                          cmd.instanceCount);
  } else {
    glDrawArrays(topology, cmd.firstVertex, cmd.vertexCount);
  }
}

void OpenGLDevice::DrawIndexed(const DrawIndexedCommand &cmd) {
  auto it = pipelines.find(currentPipeline.id);
  if (it == pipelines.end())
    return;

  GLenum topology = it->second.topology;
  GLenum indexType = (cmd.indexType == IndexType::UInt16) ? GL_UNSIGNED_SHORT
                                                          : GL_UNSIGNED_INT;
  size_t indexSize = (cmd.indexType == IndexType::UInt16) ? sizeof(uint16_t)
                                                          : sizeof(uint32_t);

  void *indexOffset = (void *)(uintptr_t)(cmd.firstIndex * indexSize);

  if (cmd.instanceCount > 1) {
    glDrawElementsInstanced(topology, cmd.indexCount, indexType, indexOffset,
                            cmd.instanceCount);
  } else {
    glDrawElements(topology, cmd.indexCount, indexType, indexOffset);
  }
}

void OpenGLDevice::initializeSkyboxResources() {
  std::cout << "[OpenGL] Initializing dedicated skybox resources..."
            << std::endl;

  // Skybox vertex shader (GLSL)
  const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 fragTexCoord;

uniform mat4 projection;
uniform mat4 view;

void main() {
    fragTexCoord = vec3(aPos.x, -aPos.y, aPos.z);
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

  // Skybox fragment shader (GLSL)
  const char *fragmentShaderSource = R"(
#version 330 core
in vec3 fragTexCoord;
out vec4 fragColor;

uniform samplerCube skybox;

void main() {
    vec3 color = texture(skybox, fragTexCoord).rgb;
    
    // Tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    fragColor = vec4(color, 1.0);
}
)";

  // Compile vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
  glCompileShader(vertexShader);

  GLint success;
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
    std::cerr << "[OpenGL] Skybox vertex shader error: " << infoLog
              << std::endl;
  }

  // Compile fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
  glCompileShader(fragmentShader);

  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
    std::cerr << "[OpenGL] Skybox fragment shader error: " << infoLog
              << std::endl;
  }

  // Link program
  skyboxShaderProgram = glCreateProgram();
  glAttachShader(skyboxShaderProgram, vertexShader);
  glAttachShader(skyboxShaderProgram, fragmentShader);
  glLinkProgram(skyboxShaderProgram);

  glGetProgramiv(skyboxShaderProgram, GL_LINK_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(skyboxShaderProgram, 512, nullptr, infoLog);
    std::cerr << "[OpenGL] Skybox shader link error: " << infoLog << std::endl;
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  // Skybox cube vertices (drawn from inside)
  float cubeVertices[] = {
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

  // Create VAO and VBO
  glGenVertexArrays(1, &skyboxVAO);
  glGenBuffers(1, &skyboxVBO);

  glBindVertexArray(skyboxVAO);
  glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
  glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices,
               GL_STATIC_DRAW);

  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

  glBindVertexArray(0);

  skyboxInitialized = true;
  std::cout << "[OpenGL] Skybox resources initialized successfully"
            << std::endl;
}

void OpenGLDevice::DrawSkybox(TextureHandle cubemap, SamplerHandle sampler,
                              const float *viewMatrix,
                              const float *projMatrix) {
  auto texIt = textures.find(cubemap.id);
  if (texIt == textures.end()) {
    return;
  }

  if (!skyboxInitialized) {
    initializeSkyboxResources();
    if (!skyboxInitialized) {
      std::cerr << "[OpenGL] Failed to initialize skybox resources"
                << std::endl;
      return;
    }
  }

  // Save current state
  GLint previousProgram;
  glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
  GLboolean previousDepthMask;
  glGetBooleanv(GL_DEPTH_WRITEMASK, &previousDepthMask);
  GLint previousDepthFunc;
  glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);
  GLint previousCullFaceMode;
  glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);
  GLboolean previousCullFaceEnabled = glIsEnabled(GL_CULL_FACE);

  // Set skybox state
  glDepthFunc(GL_LEQUAL);
  glDepthMask(GL_FALSE);
  glDisable(GL_CULL_FACE); // Draw both sides just in case

  // Use skybox shader
  glUseProgram(skyboxShaderProgram);

  // Remove translation from view matrix
  float viewRotOnly[16];
  memcpy(viewRotOnly, viewMatrix, sizeof(viewRotOnly));
  viewRotOnly[12] = 0.0f;
  viewRotOnly[13] = 0.0f;
  viewRotOnly[14] = 0.0f;

  // Set uniforms
  GLint viewLoc = glGetUniformLocation(skyboxShaderProgram, "view");
  GLint projLoc = glGetUniformLocation(skyboxShaderProgram, "projection");
  GLint skyboxLoc = glGetUniformLocation(skyboxShaderProgram, "skybox");

  glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewRotOnly);
  glUniformMatrix4fv(projLoc, 1, GL_FALSE, projMatrix);
  glUniform1i(skyboxLoc, 0);

  // Bind cubemap texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texIt->second.id);

  // Apply sampler if available
  auto sampIt = samplers.find(sampler.id);
  if (sampIt != samplers.end()) {
    glBindSampler(0, sampIt->second.id);
  }

  // Draw skybox cube
  glBindVertexArray(skyboxVAO);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);

  // Restore previous state
  glDepthFunc(previousDepthFunc);
  glDepthMask(previousDepthMask);
  glCullFace(previousCullFaceMode);
  if (previousCullFaceEnabled) {
    glEnable(GL_CULL_FACE);
  } else {
    glDisable(GL_CULL_FACE);
  }
  glUseProgram(previousProgram);

  // Unbind sampler
  if (sampIt != samplers.end()) {
    glBindSampler(0, 0);
  }
}

void OpenGLDevice::WaitIdle() { glFinish(); }

// ========================================================================
// IMGUI INTEGRATION
// ========================================================================

void OpenGLDevice::InitImGui(void *window) {
  // In OpenGL, ImGui_ImplOpenGL3_Init requires the GLSL version
  ImGui_ImplOpenGL3_Init("#version 330");
}

void OpenGLDevice::ShutdownImGui() { ImGui_ImplOpenGL3_Shutdown(); }

void OpenGLDevice::NewFrameImGui() { ImGui_ImplOpenGL3_NewFrame(); }

void OpenGLDevice::RenderImGui() {
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace RHI

namespace RHI {
CommandListHandle OpenGLDevice::CreateCommandList() { return {0}; }
ICommandList *OpenGLDevice::GetCommandList(CommandListHandle handle) {
  return nullptr;
}
void OpenGLDevice::SubmitCommandList(CommandListHandle handle) {}
void OpenGLDevice::DestroyCommandList(CommandListHandle handle) {}
} // namespace RHI
