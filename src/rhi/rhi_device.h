#ifndef RHI_DEVICE_HPP
#define RHI_DEVICE_HPP

#include "rhi_types.hpp"
#include <memory>
#include <vector>

namespace RHI {

/**
 * @brief Interface abstrata para dispositivos gráficos
 *
 * Esta é a interface principal do RHI que abstrai operações de GPU.
 * Cada API (OpenGL, Vulkan, etc) implementa esta interface.
 */
class IDevice {
public:
  virtual ~IDevice() = default;

  // ========================================================================
  // LIFECYCLE
  // ========================================================================

  /**
   * @brief Inicializa o dispositivo
   * @return true se inicializado com sucesso
   */
  virtual bool Initialize() = 0;

  /**
   * @brief Finaliza e libera recursos do dispositivo
   */
  virtual void Shutdown() = 0;

  /**
   * @brief Inicia um novo frame de renderização
   * @return true se o frame foi iniciado com sucesso
   */
  virtual bool BeginFrame() = 0;

  /**
   * @brief Finaliza o frame atual e apresenta na tela
   */
  virtual void EndFrame() = 0;

  /**
   * @brief Retorna a API gráfica em uso
   */
  virtual API GetAPI() const = 0;

  /**
   * @brief Retorna informações do dispositivo (capabilities, limites)
   */
  virtual DeviceInfo GetDeviceInfo() const = 0;

  // ========================================================================
  // BUFFER MANAGEMENT
  // ========================================================================

  /**
   * @brief Cria um buffer (vertex, index ou uniform)
   * @param desc Descritor do buffer
   * @return Handle para o buffer criado
   */
  virtual BufferHandle CreateBuffer(const BufferDescriptor &desc) = 0;

  /**
   * @brief Atualiza dados de um buffer
   * @param buffer Handle do buffer
   * @param data Novos dados
   * @param size Tamanho dos dados em bytes
   * @param offset Offset no buffer (em bytes)
   */
  virtual void UpdateBuffer(BufferHandle buffer, const void *data,
                            uint32_t size, uint32_t offset = 0) = 0;

  /**
   * @brief Destrói um buffer
   */
  virtual void DestroyBuffer(BufferHandle buffer) = 0;

  // ========================================================================
  // TEXTURE MANAGEMENT
  // ========================================================================

  /**
   * @brief Cria uma textura
   * @param desc Descritor da textura
   * @return Handle para a textura criada
   */
  virtual TextureHandle CreateTexture(const TextureDescriptor &desc) = 0;

  /**
   * @brief Atualiza dados de uma textura
   * @param texture Handle da textura
   * @param data Novos dados
   * @param mipLevel Nível de mipmap (0 = base)
   */
  virtual void UpdateTexture(TextureHandle texture, const void *data,
                             uint32_t mipLevel = 0) = 0;

  /**
   * @brief Updates a specific face of a cubemap texture
   * @param texture Handle to the cubemap texture
   * @param face Which face to update
   * @param data Pixel data for this face
   * @param mipLevel Mipmap level (0 = base)
   */
  virtual void UpdateTextureCubeFace(TextureHandle texture, CubemapFace face,
                                     const void *data,
                                     uint32_t mipLevel = 0) = 0;

  /**
   * @brief Gera mipmaps para uma textura
   */
  virtual void GenerateMipmaps(TextureHandle texture) = 0;

  /**
   * @brief Destrói uma textura
   */
  virtual void DestroyTexture(TextureHandle texture) = 0;

  /**
   * @brief Returns the platform-native texture ID (e.g. OpenGL GLuint).
   *        Required for interop with external systems like ImGui.
   * @return Native ID, or 0 if handle is invalid.
   */
  virtual uint64_t GetNativeTextureID(TextureHandle texture) const = 0;

  // ========================================================================
  // SAMPLER MANAGEMENT
  // ========================================================================

  /**
   * @brief Cria um sampler (objeto de amostragem de textura)
   * @param desc Descritor do sampler
   * @return Handle para o sampler criado
   */
  virtual SamplerHandle CreateSampler(const SamplerDescriptor &desc) = 0;

  /**
   * @brief Destrói um sampler
   */
  virtual void DestroySampler(SamplerHandle sampler) = 0;

  // ========================================================================
  // SHADER MANAGEMENT
  // ========================================================================

  /**
   * @brief Compila e cria um shader program
   * @param stages Vector de descritores de shader (vertex, fragment, etc)
   * @return Handle para o shader program criado
   */
  virtual ShaderHandle
  CreateShader(const std::vector<ShaderDescriptor> &stages) = 0;

  /**
   * @brief Destrói um shader program
   */
  virtual void DestroyShader(ShaderHandle shader) = 0;

  // ========================================================================
  // PIPELINE MANAGEMENT
  // ========================================================================

  /**
   * @brief Cria um pipeline state object
   * @param desc Descritor do pipeline
   * @param shader Shader associado ao pipeline
   * @param layout Layout dos vértices
   * @return Handle para o pipeline criado
   */
  virtual PipelineHandle CreatePipeline(const PipelineDescriptor &desc,
                                        ShaderHandle shader,
                                        const VertexLayout &layout) = 0;

  /**
   * @brief Destrói um pipeline
   */
  virtual void DestroyPipeline(PipelineHandle pipeline) = 0;

  // ========================================================================
  // VERTEX ARRAY MANAGEMENT (VAO)
  // ========================================================================

  /**
   * @brief Cria um Vertex Array Object
   * @param vertexBuffer Buffer de vértices
   * @param indexBuffer Buffer de índices (opcional)
   * @param layout Layout dos atributos
   * @return Handle para o VAO criado
   */
  virtual VertexArrayHandle CreateVertexArray(BufferHandle vertexBuffer,
                                              BufferHandle indexBuffer,
                                              const VertexLayout &layout) = 0;

  /**
   * @brief Destrói um VAO
   */
  virtual void DestroyVertexArray(VertexArrayHandle vao) = 0;

  // ========================================================================
  // FRAMEBUFFER MANAGEMENT
  // ========================================================================

  /**
   * @brief Cria um framebuffer
   * @param desc Descritor do framebuffer
   * @return Handle para o framebuffer criado
   */
  virtual FramebufferHandle
  CreateFramebuffer(const FramebufferDescriptor &desc) = 0;

  /**
   * @brief Anexa uma textura a um framebuffer
   * @param framebuffer Handle do framebuffer
   * @param attachment Ponto de anexo
   * @param texture Textura a ser anexada
   */
  virtual void AttachTexture(FramebufferHandle framebuffer,
                             FramebufferAttachment attachment,
                             TextureHandle texture) = 0;

  /**
   * @brief Obtém a textura de um attachment do framebuffer
   */
  virtual TextureHandle
  GetFramebufferTexture(FramebufferHandle framebuffer,
                        FramebufferAttachment attachment) = 0;

  /**
   * @brief Redimensiona um framebuffer
   */
  virtual void ResizeFramebuffer(FramebufferHandle framebuffer, uint32_t width,
                                 uint32_t height) = 0;

  /**
   * @brief Destrói um framebuffer
   */
  virtual void DestroyFramebuffer(FramebufferHandle framebuffer) = 0;

  // ========================================================================
  // RENDER STATE
  // ========================================================================

  /**
   * @brief Define o viewport
   */
  virtual void SetViewport(const Viewport &viewport) = 0;

  /**
   * @brief Define a região de scissor test
   */
  virtual void SetScissor(const Scissor &scissor) = 0;

  /**
   * @brief Desabilita o scissor test
   */
  virtual void DisableScissor() = 0;

  /**
   * @brief Limpa buffers
   * @param color Se true, limpa color buffer
   * @param depth Se true, limpa depth buffer
   * @param stencil Se true, limpa stencil buffer
   */
  virtual void Clear(bool color = true, bool depth = true,
                     bool stencil = false) = 0;

  /**
   * @brief Define a cor de limpeza
   */
  virtual void SetClearColor(const ClearColor &color) = 0;

  /**
   * @brief Define o valor de limpeza do depth buffer
   */
  virtual void SetClearDepth(float depth) = 0;

  // ========================================================================
  // BINDING
  // ========================================================================

  /**
   * @brief Bind de pipeline
   */
  virtual void BindPipeline(PipelineHandle pipeline) = 0;

  /**
   * @brief Bind de vertex array
   */
  virtual void BindVertexArray(VertexArrayHandle vao) = 0;

  /**
   * @brief Bind de framebuffer (nullptr = default framebuffer)
   */
  virtual void BindFramebuffer(FramebufferHandle framebuffer) = 0;

  /**
   * @brief Bind de textura em um slot específico
   */
  virtual void BindTexture(uint32_t slot, TextureHandle texture) = 0;

  /**
   * @brief Bind de sampler em um slot específico
   */
  virtual void BindSampler(uint32_t slot, SamplerHandle sampler) = 0;

  // ========================================================================
  // SHADER UNIFORMS
  // ========================================================================

  virtual void SetUniform(ShaderHandle shader, const std::string &name,
                          int value) = 0;
  virtual void SetUniform(ShaderHandle shader, const std::string &name,
                          float value) = 0;
  virtual void SetUniform(ShaderHandle shader, const std::string &name,
                          const float *value, uint32_t count) = 0;
  virtual void SetUniformMatrix4(ShaderHandle shader, const std::string &name,
                                 const float *matrix) = 0;

  // ========================================================================
  // DRAW COMMANDS
  // ========================================================================

  /**
   * @brief Draw sem índices
   */
  virtual void Draw(const DrawCommand &cmd) = 0;

  /**
   * @brief Draw com índices
   */
  virtual void DrawIndexed(const DrawIndexedCommand &cmd) = 0;

  /**
   * @brief Draw skybox with isolated pipeline state
   * @param cubemap Handle to the cubemap texture
   * @param sampler Handle to the sampler
   * @param viewMatrix View matrix (translation will be removed internally)
   * @param projMatrix Projection matrix
   *
   * This method uses a dedicated descriptor set and pipeline to avoid
   * corrupting the main scene's render state.
   */
  virtual void DrawSkybox(TextureHandle cubemap, SamplerHandle sampler,
                          const float *viewMatrix, const float *projMatrix) = 0;

  // ========================================================================
  // UTILITY
  // ========================================================================

  /**
   * @brief Aguarda a GPU finalizar todas as operações pendentes
   */
  virtual void WaitIdle() = 0;

  // ========================================================================
  // IMGUI INTEGRATION
  // ========================================================================

  /**
   * @brief Inicializa a integração gráfica do ImGui (backend de renderização)
   */
  virtual void InitImGui(void *window) = 0;

  /**
   * @brief Finaliza a integração gráfica do ImGui
   */
  virtual void ShutdownImGui() = 0;

  /**
   * @brief Inicia um novo frame gráfico do ImGui
   */
  virtual void NewFrameImGui() = 0;

  /**
   * @brief Renderiza os dados de desenho (DrawData) acumulados do ImGui
   */
  virtual void RenderImGui() = 0;
};

/**
 * @brief Factory para criar dispositivos RHI
 */
class DeviceFactory {
public:
  static std::unique_ptr<IDevice> Create(API api);
};

} // namespace RHI

#endif // RHI_DEVICE_HPP