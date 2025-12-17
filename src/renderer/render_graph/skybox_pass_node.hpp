#ifndef SKYBOX_PASS_NODE_HPP
#define SKYBOX_PASS_NODE_HPP

#include "../pbr_utils.hpp"
#include "../skybox_pass.hpp"
#include "render_pass.hpp"
#include <glm/gtc/type_ptr.hpp>

class SkyboxPassNode : public RenderPass {
public:
  explicit SkyboxPassNode(PBRUtils::EnvironmentMap *env) : envMap(env) {
    skyboxPass = std::make_unique<SkyboxPass>();
  }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        (context->GetAPI() == RHI::API::Vulkan)
            ? context->GetFileSystem()->GetAbsolutePath("shaders/unified")
            : context->GetFileSystem()->GetAbsolutePath("shaders");
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

private:
  std::unique_ptr<SkyboxPass> skyboxPass;
  PBRUtils::EnvironmentMap *envMap = nullptr;
};

#endif
