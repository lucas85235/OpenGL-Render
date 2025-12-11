#ifndef SHADER_CROSS_COMPILER_HPP
#define SHADER_CROSS_COMPILER_HPP

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// SPIRV-Cross headers (optional, enabled with -DUSE_SPIRV_CROSS=ON)
#ifdef HAS_SPIRV_CROSS
#include <spirv_cross/spirv_glsl.hpp>
#define SPIRV_CROSS_AVAILABLE 1
#else
#define SPIRV_CROSS_AVAILABLE 0
#endif

namespace RHI {

enum class RenderAPI { Vulkan, OpenGL };

enum class ShaderStageType {
  Vertex,
  Fragment,
  Geometry,
  TessControl,
  TessEvaluation,
  Compute
};

// Result structure containing either SPIR-V binary or GLSL source
struct CrossCompileResult {
  bool success = false;
  std::string errorMessage;

  // For OpenGL: transpiled GLSL source code
  std::string glslSource;

  // For Vulkan: original SPIR-V bytecode (passthrough)
  std::vector<uint32_t> spirvBinary;

  // Indicates which output is valid
  RenderAPI targetAPI;
};

/**
 * @brief ShaderCrossCompiler - Unified shader management using SPIR-V as
 * intermediate format
 *
 * Usage:
 * 1. Write shaders in GLSL (Vulkan-style with explicit bindings)
 * 2. Compile to SPIR-V offline using glslangValidator
 * 3. At runtime, use this class to:
 *    - Pass SPIR-V directly to Vulkan
 *    - Transpile SPIR-V to GLSL for OpenGL via SPIRV-Cross
 *
 * COORDINATE SYSTEM HANDLING:
 * ---------------------------
 * Vulkan and OpenGL have different coordinate conventions:
 *
 * 1. Y-Axis Flip:
 *    - Vulkan: Y-down (0 at top, 1 at bottom)
 *    - OpenGL: Y-up (0 at bottom, 1 at top)
 *
 *    SOLUTION: Flip Y in the projection matrix (NOT in shaders):
 *    ```cpp
 *    glm::mat4 proj = glm::perspective(...);
 *    if (api == RenderAPI::Vulkan) {
 *        proj[1][1] *= -1.0f;  // Flip Y for Vulkan
 *    }
 *    ```
 *
 * 2. Depth Range (NDC Z):
 *    - Vulkan: 0 to 1 (zero-to-one)
 *    - OpenGL: -1 to 1 (negative-one-to-one) by default
 *
 *    SOLUTION for OpenGL 4.5+:
 *    ```cpp
 *    // Call this once after context creation to match Vulkan's Z range
 *    glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
 *    ```
 *    This makes OpenGL use 0-1 depth range like Vulkan, eliminating
 *    the need for different projection matrices or shader modifications.
 *
 *    For older OpenGL (< 4.5), use ARB_clip_control extension or
 *    adjust the projection matrix to remap Z.
 */
class ShaderCrossCompiler {
public:
  /**
   * @brief Process SPIR-V bytecode for the target rendering API
   *
   * @param spirvBinary The SPIR-V bytecode loaded from disk
   * @param targetAPI The backend to compile for (Vulkan or OpenGL)
   * @param stage The shader stage type (Vertex, Fragment, etc.)
   * @param glslVersion GLSL version for OpenGL (e.g., 330, 450)
   *
   * @return CrossCompileResult containing either GLSL source or SPIR-V binary
   */
  static CrossCompileResult Process(const std::vector<uint32_t> &spirvBinary,
                                    RenderAPI targetAPI, ShaderStageType stage,
                                    uint32_t glslVersion = 330) {
    CrossCompileResult result;
    result.targetAPI = targetAPI;

    if (spirvBinary.empty()) {
      result.success = false;
      result.errorMessage = "Empty SPIR-V binary provided";
      return result;
    }

    try {
      if (targetAPI == RenderAPI::Vulkan) {
        // Vulkan uses SPIR-V natively - just pass through
        result.spirvBinary = spirvBinary;
        result.success = true;
        std::cout << "[ShaderCrossCompiler] Vulkan: SPIR-V passthrough ("
                  << spirvBinary.size() * 4 << " bytes)" << std::endl;
      } else if (targetAPI == RenderAPI::OpenGL) {
#if SPIRV_CROSS_AVAILABLE
        // OpenGL needs GLSL - transpile using SPIRV-Cross
        result.glslSource = TranspileToGLSL(spirvBinary, glslVersion, stage);
        result.success = !result.glslSource.empty();
        std::cout << "[ShaderCrossCompiler] OpenGL: Transpiled to GLSL "
                  << glslVersion << " (" << result.glslSource.size()
                  << " chars)" << std::endl;
#else
        result.success = false;
        result.errorMessage =
            "SPIRV-Cross not available. Build with -DUSE_SPIRV_CROSS=ON";
        std::cerr << "[ShaderCrossCompiler] " << result.errorMessage
                  << std::endl;
#endif
      }
    }
#if SPIRV_CROSS_AVAILABLE
    catch (const spirv_cross::CompilerError &e) {
      result.success = false;
      result.errorMessage = std::string("SPIRV-Cross error: ") + e.what();
      std::cerr << "[ShaderCrossCompiler] " << result.errorMessage << std::endl;
    }
#endif
    catch (const std::exception &e) {
      result.success = false;
      result.errorMessage = std::string("Error: ") + e.what();
      std::cerr << "[ShaderCrossCompiler] " << result.errorMessage << std::endl;
    }

    return result;
  }

private:
#if SPIRV_CROSS_AVAILABLE
  /**
   * @brief Transpile SPIR-V to GLSL using SPIRV-Cross
   *
   * SPIRV-Cross CompilerGLSL::Options explanation:
   *
   * - version: Target GLSL version (330 for OpenGL 3.3, 450 for OpenGL 4.5)
   *
   * - es: Set to true for GLSL ES (mobile), false for desktop GLSL
   *
   * - vulkan_semantics: Must be FALSE for OpenGL output. When true,
   *   keeps Vulkan-specific features that OpenGL doesn't support.
   *
   * - enable_420pack_extension: Allows more compact uniform declarations
   *   in GLSL 4.20+. Safe for 420+ target versions.
   *
   * - emit_push_constant_as_uniform_buffer: Converts Vulkan push constants
   *   to uniform buffers for OpenGL compatibility.
   *
   * - emit_uniform_buffer_as_plain_uniforms: For GLSL < 140, flattens UBOs
   *   to individual uniform variables (maximum compatibility).
   *
   * - remove_unused_variables: Eliminates dead code from output.
   *
   * - explicit_binding: When true, emits layout(binding=X) qualifiers.
   *   For GLSL < 420, you may need to handle bindings manually.
   */
  static std::string TranspileToGLSL(const std::vector<uint32_t> &spirvBinary,
                                     uint32_t glslVersion,
                                     ShaderStageType stage) {
    // Create SPIRV-Cross GLSL compiler
    spirv_cross::CompilerGLSL compiler(spirvBinary);

    // Configure compiler options
    spirv_cross::CompilerGLSL::Options options;

    // Target GLSL version (330 = OpenGL 3.3, 450 = OpenGL 4.5)
    options.version = glslVersion;

    // Desktop GLSL, not ES
    options.es = false;

    // CRITICAL: Disable Vulkan semantics for OpenGL output
    // This removes Vulkan-specific features like:
    // - subpass inputs
    // - push constant blocks kept as-is
    // - Vulkan-only built-in variables
    options.vulkan_semantics = false;

    // Convert push constants to uniform buffer for OpenGL
    // Vulkan push constants become a regular uniform block
    options.emit_push_constant_as_uniform_buffer = true;

    // For GLSL 330, flatten uniform buffers to plain uniforms
    // This provides maximum compatibility with older OpenGL
    if (glslVersion < 140) {
      options.emit_uniform_buffer_as_plain_uniforms = true;
    }

    // Enable ARB_shading_language_420pack for explicit bindings in 420+
    // This allows layout(binding=X) qualifiers
    options.enable_420pack_extension = (glslVersion >= 420);

    // Apply options
    compiler.set_common_options(options);

    // Handle explicit bindings for older GLSL versions
    // GLSL < 420 doesn't support layout(binding=X) directly
    if (glslVersion < 420) {
      RemoveExplicitBindings(compiler);
    }

    // Compile to GLSL source
    std::string glslSource = compiler.compile();

    // Add version directive if not present
    if (glslSource.find("#version") == std::string::npos) {
      std::string versionDirective = "#version " + std::to_string(glslVersion);
      if (glslVersion >= 140) {
        versionDirective += " core";
      }
      versionDirective += "\n";
      glslSource = versionDirective + glslSource;
    }

    return glslSource;
  }

  /**
   * @brief Remove explicit binding decorations for older GLSL
   *
   * For GLSL versions < 420, we cannot use layout(binding=X).
   * SPIRV-Cross will handle this by either:
   * - Removing binding qualifiers
   * - Letting the application set bindings via glUniform/glGetUniformLocation
   */
  static void RemoveExplicitBindings(spirv_cross::CompilerGLSL &compiler) {
    // Get all shader resources
    spirv_cross::ShaderResources resources = compiler.get_shader_resources();

    // Unset bindings for uniform buffers
    for (auto &ubo : resources.uniform_buffers) {
      compiler.unset_decoration(ubo.id, spv::DecorationBinding);
    }

    // Unset bindings for samplers/textures
    for (auto &sampler : resources.sampled_images) {
      compiler.unset_decoration(sampler.id, spv::DecorationBinding);
    }

    // Unset bindings for separate images
    for (auto &image : resources.separate_images) {
      compiler.unset_decoration(image.id, spv::DecorationBinding);
    }

    // Unset bindings for separate samplers
    for (auto &sampler : resources.separate_samplers) {
      compiler.unset_decoration(sampler.id, spv::DecorationBinding);
    }

    // Unset bindings for storage buffers (SSBOs)
    for (auto &ssbo : resources.storage_buffers) {
      compiler.unset_decoration(ssbo.id, spv::DecorationBinding);
    }
  }
#endif // SPIRV_CROSS_AVAILABLE
};

/**
 * INTEGRATION EXAMPLE:
 * ====================
 *
 * // In your RHI device initialization:
 * void CreateShaderFromSPIRV(const std::vector<uint32_t>& spirv,
 * ShaderStageType stage) { RenderAPI currentAPI = GetCurrentAPI(); //
 * RenderAPI::Vulkan or RenderAPI::OpenGL
 *
 *     auto result = ShaderCrossCompiler::Process(spirv, currentAPI, stage,
 * 330);
 *
 *     if (!result.success) {
 *         std::cerr << "Shader compilation failed: " << result.errorMessage <<
 * std::endl; return;
 *     }
 *
 *     if (currentAPI == RenderAPI::Vulkan) {
 *         // Use result.spirvBinary to create VkShaderModule
 *         VkShaderModuleCreateInfo createInfo{};
 *         createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
 *         createInfo.codeSize = result.spirvBinary.size() * sizeof(uint32_t);
 *         createInfo.pCode = result.spirvBinary.data();
 *         vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
 *     }
 *     else if (currentAPI == RenderAPI::OpenGL) {
 *         // Use result.glslSource to create GL shader
 *         GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);
 *         const char* src = result.glslSource.c_str();
 *         glShaderSource(shader, 1, &src, nullptr);
 *         glCompileShader(shader);
 *     }
 * }
 *
 *
 * COORDINATE SYSTEM SETUP:
 * ========================
 *
 * For a unified coordinate system across Vulkan and OpenGL:
 *
 * 1. In OpenGL context initialization (OpenGLDevice::Initialize):
 *    ```cpp
 *    // Match Vulkan's depth range [0, 1] instead of OpenGL's [-1, 1]
 *    // Requires OpenGL 4.5 or ARB_clip_control extension
 *    if (GLAD_GL_VERSION_4_5 || GLAD_GL_ARB_clip_control) {
 *        glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
 *    }
 *    ```
 *
 * 2. In projection matrix calculation:
 *    ```cpp
 *    glm::mat4 projection = glm::perspective(fov, aspect, near, far);
 *
 *    // Vulkan has Y-axis inverted compared to OpenGL
 *    // Flip Y in the projection matrix for Vulkan
 *    if (api == RenderAPI::Vulkan) {
 *        projection[1][1] *= -1.0f;
 *    }
 *    ```
 *
 * This approach keeps shaders identical across both APIs!
 */

} // namespace RHI

#endif // SHADER_CROSS_COMPILER_HPP
