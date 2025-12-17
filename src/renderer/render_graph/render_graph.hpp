#ifndef RENDER_GRAPH_HPP
#define RENDER_GRAPH_HPP

#include "render_pass.hpp"
#include <iostream>
#include <memory>
#include <vector>

class RenderGraph {
private:
  std::vector<std::unique_ptr<RenderPass>> passes;
  RenderContext *context = nullptr;

public:
  RenderGraph(RenderContext *ctx) : context(ctx) {}

  void AddPass(std::unique_ptr<RenderPass> pass) {
    if (pass->Initialize(context)) {
      passes.push_back(std::move(pass));
    } else {
      std::cerr << "[RenderGraph] Failed to initialize pass: "
                << pass->GetName() << std::endl;
    }
  }

  void Execute(const RenderPassData &data, Scene *scene) {
    for (const auto &pass : passes) {
      pass->Execute(context, data, scene);
    }
  }

  void Clear() { passes.clear(); }
};

#endif // RENDER_GRAPH_HPP
