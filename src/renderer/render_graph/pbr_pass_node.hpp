#ifndef PBR_PASS_NODE_HPP
#define PBR_PASS_NODE_HPP

#include "../../scene/scene.hpp"
#include "../renderer.hpp"
#include "../shadow_pass.hpp"
#include "render_pass.hpp"

class PBRPassNode : public RenderPass {
public:
  explicit PBRPassNode(Renderer *renderer) : renderer(renderer) {}

  bool Initialize(RenderContext *context) override {
    pbrShader = context->GetShaderManager()->GetShader("pbr");
    if (!pbrShader) {
      std::cerr << "[PBRPass] Shader 'pbr' not found!" << std::endl;
      return false;
    }

    // Also fetch the particle shader so we don't overwrite it with nullptr
    auto particleShader = context->GetShaderManager()->GetShader("particle");

    renderer->Init(context, pbrShader.get(), particleShader.get());
    return true;
  }

  void SetShadowPass(ShadowPass *pass) { shadowPass = pass; }
  void SetShadowEnabledPtr(const bool *ptr) { shadowEnabledPtr = ptr; }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (!scene)
      return;

    bool lightCasts = true;
    float biasConstant = 0.0005f;
    float biasSlope = 0.005f;
    auto lightView = scene->GetRegistry().view<DirectionalLightComponent>();
    for (auto entity : lightView) {
      auto &light = lightView.get<DirectionalLightComponent>(entity);
      lightCasts = light.castShadow;
      biasConstant = light.shadowBiasConstant;
      biasSlope = light.shadowBiasSlope;
      break;
    }

    bool shadowOn = shadowPass && shadowPass->IsInitialized() && lightCasts &&
                    (!shadowEnabledPtr || *shadowEnabledPtr);
    if (shadowOn) {
      renderer->SetShadow(shadowPass->GetDepthTexture(),
                          shadowPass->GetLightSpaceMatrix(), true,
                          biasConstant, biasSlope);
    } else {
      renderer->SetShadow(RHI::TextureHandle{0}, glm::mat4(1.0f), false);
    }

    renderer->BeginScene(data.view, data.projection, data.cameraPos);
    scene->OnRender(*renderer);
    renderer->EndScene(data.cmdList);
  }

  std::string GetName() const override { return "PBRPass"; }

private:
  Renderer *renderer;
  std::shared_ptr<Shader> pbrShader;
  ShadowPass *shadowPass = nullptr;
  const bool *shadowEnabledPtr = nullptr;
};

#endif
