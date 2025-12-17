#ifndef SHADER_PREPROCESSOR_HPP
#define SHADER_PREPROCESSOR_HPP

#include "rhi_types.hpp"
#include <regex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace RHI {

enum class ShaderType { Graphics, Compute, RayTracing };

struct ResourceBinding {
  uint32_t binding = 0;
  uint32_t set = 0;
  std::string typeName;
  std::string varName;
  bool isPushConstant = false;
  bool isReadOnly = false;
};

struct ShaderStageSource {
  std::string vertex;
  std::string fragment;
  std::string geometry;
  std::string compute;
  std::string tessControl;
  std::string tessEval;
};

struct ShaderMetadata {
  ShaderType type = ShaderType::Graphics;
  std::vector<ResourceBinding> bindings;
  std::string commonBlock;
  std::string resourcesBlock;
};

class ShaderPreprocessor {
public:
  static ShaderStageSource Process(const std::string &source, API targetAPI,
                                   uint32_t glslVersion = 450) {
    ShaderStageSource output;

    std::string common = ExtractBlock(source, "common");
    std::string resources = ExtractBlock(source, "resources");
    std::string vertex = ExtractBlock(source, "vertex");
    std::string fragment = ExtractBlock(source, "fragment");
    std::string geometry = ExtractBlock(source, "geometry");
    std::string compute = ExtractBlock(source, "compute");

    std::string transformedResources =
        TransformResources(resources, targetAPI, glslVersion);
    std::string versionDirective = GenerateVersion(targetAPI, glslVersion);

    if (!vertex.empty()) {
      output.vertex =
          AssembleStage(versionDirective, common, transformedResources, vertex);
    }

    if (!fragment.empty()) {
      output.fragment = AssembleStage(versionDirective, common,
                                      transformedResources, fragment);
    }

    if (!geometry.empty()) {
      output.geometry = AssembleStage(versionDirective, common,
                                      transformedResources, geometry);
    }

    if (!compute.empty()) {
      output.compute = AssembleStage(versionDirective, common,
                                     transformedResources, compute);
    }

    return output;
  }

  static ShaderMetadata ExtractMetadata(const std::string &source) {
    ShaderMetadata meta;

    std::string typeStr = ExtractPragmaValue(source, "shader_type");
    if (typeStr == "compute")
      meta.type = ShaderType::Compute;
    else if (typeStr == "raytracing")
      meta.type = ShaderType::RayTracing;
    else
      meta.type = ShaderType::Graphics;

    meta.commonBlock = ExtractBlock(source, "common");
    meta.resourcesBlock = ExtractBlock(source, "resources");

    std::regex bindingRegex(
        R"(@binding\s*\(\s*(\d+)\s*\)(?:\s*@set\s*\(\s*(\d+)\s*\))?\s*(uniform|buffer)\s+(\w+)\s+(\w+)\s*(?:\{[^}]*\})?;?)");
    std::smatch match;
    std::string resourcesCopy = meta.resourcesBlock;

    while (std::regex_search(resourcesCopy, match, bindingRegex)) {
      ResourceBinding rb;
      rb.binding = std::stoi(match[1].str());
      rb.set = match[2].matched ? std::stoi(match[2].str()) : 0;
      rb.typeName = match[4].str();
      rb.varName = match[5].str();
      meta.bindings.push_back(rb);
      resourcesCopy = match.suffix();
    }

    std::regex pushRegex(
        R"(@push_constant\s+uniform\s+(\w+)\s*\{[^}]*\}\s*(\w+);)");
    resourcesCopy = meta.resourcesBlock;
    while (std::regex_search(resourcesCopy, match, pushRegex)) {
      ResourceBinding rb;
      rb.typeName = match[1].str();
      rb.varName = match[2].str();
      rb.isPushConstant = true;
      meta.bindings.push_back(rb);
      resourcesCopy = match.suffix();
    }

    return meta;
  }

private:
  static std::string ExtractBlock(const std::string &source,
                                  const std::string &blockName) {
    std::string beginTag = "#pragma begin(" + blockName + ")";
    std::string endTag = "#pragma end(" + blockName + ")";

    size_t start = source.find(beginTag);
    if (start == std::string::npos)
      return "";

    start += beginTag.length();
    size_t end = source.find(endTag, start);
    if (end == std::string::npos)
      return "";

    std::string block = source.substr(start, end - start);
    size_t first = block.find_first_not_of("\n\r");
    if (first != std::string::npos)
      block = block.substr(first);

    return block;
  }

  static std::string ExtractPragmaValue(const std::string &source,
                                        const std::string &pragmaName) {
    std::regex pragmaRegex("#pragma\\s+" + pragmaName +
                           "\\s*\\(\\s*(\\w+)\\s*\\)");
    std::smatch match;
    if (std::regex_search(source, match, pragmaRegex)) {
      return match[1].str();
    }
    return "";
  }

  static std::string TransformResources(const std::string &resources,
                                        API targetAPI, uint32_t glslVersion) {
    if (resources.empty())
      return "";

    std::string result = resources;

    if (targetAPI == API::Vulkan) {
      // @binding(N) -> layout(set=0, binding=N)
      // @binding(N) @set(M) -> layout(set=M, binding=N)
      std::regex bindingSetRegex(
          R"(@binding\s*\(\s*(\d+)\s*\)\s*@set\s*\(\s*(\d+)\s*\))");
      result = std::regex_replace(result, bindingSetRegex,
                                  "layout(set = $2, binding = $1)");

      std::regex bindingOnlyRegex(R"(@binding\s*\(\s*(\d+)\s*\))");
      result = std::regex_replace(result, bindingOnlyRegex,
                                  "layout(set = 0, binding = $1)");

      // @push_constant -> layout(push_constant)
      std::regex pushRegex(R"(@push_constant)");
      result = std::regex_replace(result, pushRegex, "layout(push_constant)");

      // @readonly -> readonly
      std::regex readonlyRegex(R"(@readonly)");
      result = std::regex_replace(result, readonlyRegex, "readonly");
    } else {
      // OpenGL
      if (glslVersion >= 420) {
        // @binding(N) -> layout(binding=N)
        std::regex bindingRegex(
            R"(@binding\s*\(\s*(\d+)\s*\)(?:\s*@set\s*\(\s*\d+\s*\))?)");
        result =
            std::regex_replace(result, bindingRegex, "layout(binding = $1)");
      } else {
        // Remove bindings for older GLSL
        std::regex bindingRegex(
            R"(@binding\s*\(\s*\d+\s*\)(?:\s*@set\s*\(\s*\d+\s*\))?\s*)");
        result = std::regex_replace(result, bindingRegex, "");
      }

      // @push_constant -> plain uniform
      std::regex pushRegex(R"(@push_constant\s*)");
      result = std::regex_replace(result, pushRegex, "");

      // @readonly -> remove (not needed for OpenGL uniforms)
      std::regex readonlyRegex(R"(@readonly\s*)");
      result = std::regex_replace(result, readonlyRegex, "");
    }

    return result;
  }

  static std::string GenerateVersion(API targetAPI, uint32_t glslVersion) {
    std::ostringstream ss;
    ss << "#version " << glslVersion;
    if (glslVersion >= 140) {
      ss << " core";
    }
    ss << "\n";
    return ss.str();
  }

  static std::string AssembleStage(const std::string &version,
                                   const std::string &common,
                                   const std::string &resources,
                                   const std::string &stageCode) {
    std::ostringstream ss;
    ss << version;
    ss << "\n// ===== COMMON =====\n";
    ss << common;
    ss << "\n// ===== RESOURCES =====\n";
    ss << resources;
    ss << "\n// ===== STAGE =====\n";
    ss << stageCode;
    return ss.str();
  }
};

} // namespace RHI

#endif
