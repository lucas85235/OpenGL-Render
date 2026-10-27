#ifndef GRID_PASS_NODE_HPP
#define GRID_PASS_NODE_HPP

#include "../grid_pass.hpp"
#include "render_pass.hpp"

class GridPassNode : public RenderPass {
public:
  GridPassNode() { gridPass = std::make_unique<GridPass>(); }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        context->GetFileSystem()->GetAbsolutePath("shaders");
    return gridPass->Initialize(context->GetDevice(), shaderPath);
  }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (!enabled)
      return;
    if (data.cmdList) {
      gridPass->Execute(data.cmdList, data.view, data.projection);
    }
  }

  std::string GetName() const override { return "GridPass"; }

  bool *GetEnabledPtr() { return &enabled; }

private:
  std::unique_ptr<GridPass> gridPass;
  bool enabled = true;
};

#endif
