#ifndef BLOOM_PASS_NODE_HPP
#define BLOOM_PASS_NODE_HPP

#include "../bloom_pass.hpp"
#include "render_pass.hpp"

/**
 * @brief Node wrapper around BloomPass.
 *
 * Unlike most RenderGraph nodes, BloomPass opens its own render passes on
 * private mip FBOs and then on the scene framebuffer. It is therefore NOT
 * added to the RenderGraph — Application owns it directly and invokes
 * Execute after the main scene render pass has ended. The node exists to
 * mirror the shadow/picking pattern for ownership/toggle plumbing.
 */
class BloomPassNode : public RenderPass {
public:
  BloomPassNode() { bloomPass = std::make_unique<BloomPass>(); }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        context->GetFileSystem()->GetAbsolutePath("shaders");
    return bloomPass->Initialize(context->GetDevice(), shaderPath);
  }

  void Execute(RenderContext *, const RenderPassData &, Scene *) override {
    // No-op in the graph — see class comment. Caller invokes the inner
    // pass directly through GetPass()->Execute(...).
  }

  std::string GetName() const override { return "BloomPass"; }

  BloomPass *GetPass() { return bloomPass.get(); }
  bool *GetEnabledPtr() { return &enabled; }
  bool IsEnabled() const { return enabled; }

private:
  std::unique_ptr<BloomPass> bloomPass;
  bool enabled = false;
};

#endif // BLOOM_PASS_NODE_HPP
