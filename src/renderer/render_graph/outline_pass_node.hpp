#ifndef OUTLINE_PASS_NODE_HPP
#define OUTLINE_PASS_NODE_HPP

#include "../../editor/hierarchy_panel.hpp"
#include "../outline_pass.hpp"
#include "render_pass.hpp"
#include <entt/entt.hpp>

/**
 * @brief Render graph node wrapping OutlinePass.
 *
 * Reads the currently selected entity from the HierarchyPanel and draws an
 * inflated-shell outline around it. No-op when no HierarchyPanel is wired
 * or nothing is selected.
 */
class OutlinePassNode : public RenderPass {
public:
  OutlinePassNode() { outlinePass = std::make_unique<OutlinePass>(); }

  bool Initialize(RenderContext *context) override {
    std::string shaderPath =
        context->GetFileSystem()->GetAbsolutePath("shaders");
    return outlinePass->Initialize(context->GetDevice(), shaderPath);
  }

  void Execute(RenderContext *context, const RenderPassData &data,
               Scene *scene) override {
    if (!enabled || !data.cmdList || !scene || !hierarchyPanel)
      return;
    entt::entity selected = hierarchyPanel->GetSelectedEntity();
    outlinePass->Execute(data.cmdList, scene, selected, data.view,
                         data.projection);
  }

  std::string GetName() const override { return "OutlinePass"; }

  void SetHierarchyPanel(HierarchyPanel *panel) { hierarchyPanel = panel; }
  OutlinePass *GetPass() { return outlinePass.get(); }
  bool *GetEnabledPtr() { return &enabled; }

private:
  std::unique_ptr<OutlinePass> outlinePass;
  HierarchyPanel *hierarchyPanel = nullptr;
  bool enabled = true;
};

#endif // OUTLINE_PASS_NODE_HPP
