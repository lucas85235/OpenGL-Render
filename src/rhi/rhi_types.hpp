#ifndef RHI_TYPES_HPP
#define RHI_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace RHI {

// ============================================================================
// ENUMS
// ============================================================================

enum class API { OpenGL, Vulkan, DirectX12, Metal };

enum class BufferUsage { Static, Dynamic, Stream };

enum class BufferType { Vertex, Index, Uniform };

enum class PrimitiveTopology {
  TriangleList,
  TriangleStrip,
  LineList,
  LineStrip,
  PointList
};

enum class CompareOp {
  Never,
  Less,
  Equal,
  LessOrEqual,
  Greater,
  NotEqual,
  GreaterOrEqual,
  Always
};

enum class CullMode { None, Front, Back, FrontAndBack };

enum class FrontFace { Clockwise, CounterClockwise };

enum class BlendFactor {
  Zero,
  One,
  SrcColor,
  OneMinusSrcColor,
  DstColor,
  OneMinusDstColor,
  SrcAlpha,
  OneMinusSrcAlpha,
  DstAlpha,
  OneMinusDstAlpha
};

enum class BlendOp { Add, Subtract, ReverseSubtract, Min, Max };

enum class TextureFormat {
  R8,
  RG8,
  RGB8,
  RGBA8,
  R16F,
  RG16F,
  RGB16F,
  RGBA16F,
  R32F,
  RG32F,
  RGB32F,
  RGBA32F,
  Depth24Stencil8,
  Depth32F,
  SRGB8,
  SRGB8_Alpha8
};

enum class TextureWrapMode {
  Repeat,
  MirroredRepeat,
  ClampToEdge,
  ClampToBorder
};

enum class TextureFilterMode {
  Nearest,
  Linear,
  NearestMipmapNearest,
  NearestMipmapLinear,
  LinearMipmapNearest,
  LinearMipmapLinear
};

enum class TextureType { Texture2D, Texture3D, TextureCube };

enum class ShaderStage { Vertex, Fragment, Geometry, Compute };

enum class VertexAttributeType {
  Float,
  Float2,
  Float3,
  Float4,
  Int,
  Int2,
  Int3,
  Int4,
  UInt,
  UInt2,
  UInt3,
  UInt4
};

enum class IndexType { UInt16, UInt32 };

enum class FramebufferAttachment {
  Color0,
  Color1,
  Color2,
  Color3,
  Depth,
  Stencil,
  DepthStencil
};

// ============================================================================
// STRUCTS
// ============================================================================

struct Viewport {
  float x = 0.0f;
  float y = 0.0f;
  int width = 800;
  int height = 600;
  float minDepth = 0.0f;
  float maxDepth = 1.0f;
};

struct Scissor {
  int x = 0;
  int y = 0;
  uint32_t width = 800;
  uint32_t height = 600;
};

struct ClearColor {
  float r = 0.0f;
  float g = 0.0f;
  float b = 0.0f;
  float a = 1.0f;
};

struct VertexAttribute {
  uint32_t location;
  VertexAttributeType type;
  uint32_t offset;
  bool normalized = false;
};

struct VertexLayout {
  uint32_t stride;
  std::vector<VertexAttribute> attributes;
};

struct BufferDescriptor {
  BufferType type;
  BufferUsage usage;
  uint32_t size;
  const void *data = nullptr;
};

struct TextureDescriptor {
  TextureType type = TextureType::Texture2D;
  TextureFormat format = TextureFormat::RGBA8;
  uint32_t width = 0;
  uint32_t height = 0;
  uint32_t depth = 1;
  uint32_t mipLevels = 1;
  bool generateMipmaps = false;
  const void *data = nullptr;
};

struct SamplerDescriptor {
  TextureWrapMode wrapS = TextureWrapMode::Repeat;
  TextureWrapMode wrapT = TextureWrapMode::Repeat;
  TextureWrapMode wrapR = TextureWrapMode::Repeat;
  TextureFilterMode minFilter = TextureFilterMode::LinearMipmapLinear;
  TextureFilterMode magFilter = TextureFilterMode::Linear;
  float anisotropy = 1.0f;
};

struct ShaderDescriptor {
  ShaderStage stage;
  std::string source;                // GLSL source (for legacy/direct OpenGL)
  std::vector<uint32_t> spirvBinary; // SPIR-V bytecode (for unified pipeline)
  std::string entryPoint = "main";
  bool useSPIRV = false; // If true, use spirvBinary instead of source
};

struct RasterizerState {
  CullMode cullMode = CullMode::Back;
  FrontFace frontFace = FrontFace::CounterClockwise;
  bool depthBiasEnable = false;
  float depthBiasConstant = 0.0f;
  float depthBiasSlope = 0.0f;
};

struct DepthStencilState {
  bool depthTestEnable = true;
  bool depthWriteEnable = true;
  CompareOp depthCompareOp = CompareOp::Less;
  bool stencilTestEnable = false;
};

struct BlendState {
  bool blendEnable = false;
  BlendFactor srcColorFactor = BlendFactor::One;
  BlendFactor dstColorFactor = BlendFactor::Zero;
  BlendOp colorOp = BlendOp::Add;
  BlendFactor srcAlphaFactor = BlendFactor::One;
  BlendFactor dstAlphaFactor = BlendFactor::Zero;
  BlendOp alphaOp = BlendOp::Add;
};

struct PipelineDescriptor {
  RasterizerState rasterizer;
  DepthStencilState depthStencil;
  BlendState blend;
  PrimitiveTopology topology = PrimitiveTopology::TriangleList;
};

struct DrawCommand {
  uint32_t vertexCount;
  uint32_t instanceCount = 1;
  uint32_t firstVertex = 0;
  uint32_t firstInstance = 0;
};

struct DrawIndexedCommand {
  uint32_t indexCount;
  uint32_t instanceCount = 1;
  uint32_t firstIndex = 0;
  int32_t vertexOffset = 0;
  uint32_t firstInstance = 0;
  IndexType indexType = IndexType::UInt32;
};

// CubemapFace moved after handles to resolve forward declaration
// ============================================================================
// DEVICE INFO
// ============================================================================

struct DeviceInfo {
  std::string vendorName;
  std::string rendererName;
  std::string apiVersion;
  uint32_t maxTextureSize = 0;
  uint32_t maxTextureUnits = 0;
  uint32_t maxVertexAttributes = 0;
  uint32_t maxUniformBufferSize = 0;
  uint32_t maxColorAttachments = 0;
  bool supportsCompute = false;
  bool supportsGeometryShader = false;
  bool supportsTessellation = false;
};

// ============================================================================
// HANDLES (Opaque pointers)
// ============================================================================

struct BufferHandle {
  uint64_t id = 0;
};
struct TextureHandle {
  uint64_t id = 0;
};
struct SamplerHandle {
  uint64_t id = 0;
};
struct ShaderHandle {
  uint64_t id = 0;
};
struct PipelineHandle {
  uint64_t id = 0;
};
struct FramebufferHandle {
  uint64_t id = 0;
};
struct VertexArrayHandle {
  uint64_t id = 0;
};

inline bool IsValid(BufferHandle h) { return h.id != 0; }
inline bool IsValid(TextureHandle h) { return h.id != 0; }
inline bool IsValid(SamplerHandle h) { return h.id != 0; }
inline bool IsValid(ShaderHandle h) { return h.id != 0; }
inline bool IsValid(PipelineHandle h) { return h.id != 0; }
inline bool IsValid(FramebufferHandle h) { return h.id != 0; }
inline bool IsValid(VertexArrayHandle h) { return h.id != 0; }

// ============================================================================
// RENDER-TO-TEXTURE SUPPORT
// ============================================================================

enum class CubemapFace {
  PositiveX = 0,
  NegativeX = 1,
  PositiveY = 2,
  NegativeY = 3,
  PositiveZ = 4,
  NegativeZ = 5
};

struct RenderTargetAttachment {
  TextureHandle texture;
  uint32_t mipLevel = 0;
  uint32_t layer = 0;
  CubemapFace cubeFace = CubemapFace::PositiveX;
};

struct FramebufferDescriptor {
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<RenderTargetAttachment> colorAttachments;
  RenderTargetAttachment depthAttachment;
  bool hasDepth = false;
};

} // namespace RHI

#endif // RHI_TYPES_HPP
