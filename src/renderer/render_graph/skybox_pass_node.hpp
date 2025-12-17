#ifndef SKYBOX_PASS_NODE_HPP
#define SKYBOX_PASS_NODE_HPP

#include "../../core/filesystem.hpp"
#include "../pbr_utils.hpp" // For EnvironmentMap
#include "../skybox_pass.hpp"
#include "render_pass.hpp"
#include <glm/gtc/type_ptr.hpp>

class SkyboxPassNode : public RenderPass {
private:
  std::unique_ptr<SkyboxPass> skyboxPass;
  PBRUtils::EnvironmentMap *envMap = nullptr; // Reference to env map

public:
  SkyboxPassNode(PBRUtils::EnvironmentMap *env) : envMap(env) {
    skyboxPass = std::make_unique<SkyboxPass>();
  }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath;
    if (context->GetAPI() == RHI::API::Vulkan) {
      shaderPath = FS::GetPath("shaders/unified");
    } else {
      shaderPath = FS::GetPath("shaders");
    }

    return skyboxPass->Initialize(context->GetDevice(), shaderPath);
  }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (envMap && envMap->IsValid()) {
      context->GetDevice()->DrawSkybox(
          envMap->GetCubemap(), envMap->GetSampler(), glm::value_ptr(data.view),
          glm::value_ptr(data.projection));
    }
  }

  std::string GetName() const override { return "SkyboxPass"; }
};

#endif // SKYBOX_PASS_NODE_HPP
