#ifndef SHADOW_PASS_NODE_HPP
#define SHADOW_PASS_NODE_HPP

#include <memory>

#include "../shadow_pass.hpp"
#include "render_pass.hpp"

#include "../../scene/components.hpp"
#include "../../scene/scene.hpp"

/**
 * @brief Render graph node wrapping ShadowPass.
 *
 * Task 5.1 scope: initializes the shadow map FBO + depth texture and keeps the
 * light-space matrix in sync with the scene's directional light. Geometry
 * submission is deferred to 5.2. `enabled` toggles the update — the pass
 * itself is a no-op inside Execute() until the shader lands.
 */
class ShadowPassNode : public RenderPass {
public:
  ShadowPassNode() { shadowPass = std::make_unique<ShadowPass>(); }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        context->GetFileSystem()->GetAbsolutePath("shaders");
    return shadowPass->Initialize(context->GetDevice(), shaderPath);
  }

  // Caller must invoke this OUTSIDE any active render pass — ShadowPass opens
  // its own render pass on the shadow FBO. Application::RenderScene drives
  // this directly before the scene render pass begins; the node lives outside
  // the main RenderGraph.
  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    (void)context;
    if (!enabled || !shadowPass->IsInitialized() || !scene)
      return;

    glm::vec3 lightDir(-0.2f, -1.0f, -0.3f);
    bool castShadow = true;
    auto view = scene->GetRegistry().view<DirectionalLightComponent>();
    for (auto entity : view) {
      auto &light = view.get<DirectionalLightComponent>(entity);
      lightDir = light.direction;
      castShadow = light.castShadow;
      break;
    }
    if (!castShadow)
      return;
    shadowPass->UpdateLightSpaceMatrix(lightDir);
    shadowPass->Execute(data.cmdList, scene);
  }

  std::string GetName() const override { return "ShadowPass"; }

  ShadowPass *GetPass() { return shadowPass.get(); }
  bool *GetEnabledPtr() { return &enabled; }

private:
  std::unique_ptr<ShadowPass> shadowPass;
  bool enabled = true;
};

#endif // SHADOW_PASS_NODE_HPP
