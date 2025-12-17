#ifndef PBR_PASS_NODE_HPP
#define PBR_PASS_NODE_HPP

#include "../../scene/scene.hpp"
#include "../renderer.hpp"
#include "render_pass.hpp"

class PBRPassNode : public RenderPass {
private:
  Renderer *renderer; // Pointer to existing Renderer system (could be
                      // unique_ptr later)
  std::shared_ptr<Shader> pbrShader;

public:
  PBRPassNode(Renderer *rendererSystem) : renderer(rendererSystem) {}

  bool Initialize(RenderContext *context) override {
    // Shader logic could move here or stay injected
    // For now, we fetch it from ShaderManager
    pbrShader = context->GetShaderManager()->GetShader("pbr");
    if (!pbrShader) {
      // Try to load default if not found (fallback/lazy load)
      // But usually App loads it.
      // Let's assume App loaded it for now as per previous step.
      std::cerr << "[PBRPass] Shader 'pbr' not found in manager!" << std::endl;
      return false;
    }

    renderer->Init(context, pbrShader.get());
    return true;
  }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (!scene)
      return;

    renderer->BeginScene(data.view, data.projection, data.cameraPos);
    scene->OnRender(*renderer);
    renderer->EndScene();
  }

  std::string GetName() const override { return "PBRPass"; }
};

#endif // PBR_PASS_NODE_HPP
