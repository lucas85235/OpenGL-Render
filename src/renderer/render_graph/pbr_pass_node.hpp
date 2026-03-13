#ifndef PBR_PASS_NODE_HPP
#define PBR_PASS_NODE_HPP

#include "../../scene/scene.hpp"
#include "../renderer.hpp"
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
    renderer->Init(context, pbrShader.get());
    return true;
  }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (!scene)
      return;
    renderer->BeginScene(data.view, data.projection, data.cameraPos);
    scene->OnRender(*renderer);
    renderer->EndScene(data.cmdList);
  }

  std::string GetName() const override { return "PBRPass"; }

private:
  Renderer *renderer;
  std::shared_ptr<Shader> pbrShader;
};

#endif
