#ifndef TONEMAP_PASS_NODE_HPP
#define TONEMAP_PASS_NODE_HPP

#include "../tonemap_pass.hpp"
#include "render_pass.hpp"

/**
 * @brief Node wrapper around TonemapPass.
 *
 * Like shadow/bloom, TonemapPass opens its own render pass on a dedicated
 * output framebuffer. It is NOT added to the RenderGraph — Application owns
 * it directly and invokes Execute after the scene pass + bloom composite
 * have finished writing to sceneFB.
 */
class TonemapPassNode : public RenderPass {
public:
  TonemapPassNode() { tonemapPass = std::make_unique<TonemapPass>(); }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        context->GetFileSystem()->GetAbsolutePath("shaders");
    return tonemapPass->Initialize(context->GetDevice(), shaderPath);
  }

  void Execute(RenderContext *, const RenderPassData &, Scene *) override {
    // No-op in the graph — see class comment.
  }

  std::string GetName() const override { return "TonemapPass"; }

  TonemapPass *GetPass() { return tonemapPass.get(); }
  bool *GetEnabledPtr() { return &enabled; }
  bool IsEnabled() const { return enabled; }

private:
  std::unique_ptr<TonemapPass> tonemapPass;
  bool enabled = true;
};

#endif // TONEMAP_PASS_NODE_HPP
