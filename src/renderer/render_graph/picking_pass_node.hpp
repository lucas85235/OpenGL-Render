#ifndef PICKING_PASS_NODE_HPP
#define PICKING_PASS_NODE_HPP

#include "../picking_pass.hpp"
#include "render_pass.hpp"

/**
 * @brief Render graph node wrapping PickingPass.
 *
 * The picking pass owns its own framebuffer and opens/closes its own render
 * pass, so it cannot run inside a scene render pass that's already active.
 * By default this node is disabled — the click handler in ViewportPanel will
 * flip `enabled` for one frame when a pick is needed (task 3.2). While
 * disabled Execute is a no-op, keeping the main graph's render-pass state
 * intact.
 *
 * Exposing GetPass() also lets external code drive the picking pass directly
 * without going through the graph, which is how the click-to-pick flow in 3.2
 * will consume it.
 */
class PickingPassNode : public RenderPass {
public:
  PickingPassNode() { pickingPass = std::make_unique<PickingPass>(); }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        context->GetFileSystem()->GetAbsolutePath("shaders");
    // Default size; caller should call Resize() to match the viewport.
    return pickingPass->Initialize(context->GetDevice(), shaderPath, 1, 1);
  }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (!enabled || !data.cmdList || !scene)
      return;
    pickingPass->Resize(static_cast<uint32_t>(data.windowWidth),
                        static_cast<uint32_t>(data.windowHeight));
    pickingPass->Execute(data.cmdList, scene, data.view, data.projection);
  }

  std::string GetName() const override { return "PickingPass"; }

  PickingPass *GetPass() { return pickingPass.get(); }
  bool *GetEnabledPtr() { return &enabled; }

private:
  std::unique_ptr<PickingPass> pickingPass;
  bool enabled = false;
};

#endif // PICKING_PASS_NODE_HPP
