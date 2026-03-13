#ifndef HIERARCHY_PANEL_HPP
#define HIERARCHY_PANEL_HPP

#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "editor_panel.hpp"
#include <imgui.h>

class HierarchyPanel : public IEditorPanel {
private:
  Scene *contextScene = nullptr;
  entt::entity selectedEntity = entt::null;
  ImGuiTextFilter filter;

public:
  HierarchyPanel() { panelName = "Hierarchy"; }

  const char *GetName() const override { return panelName.c_str(); }

  void SetContext(Scene *scene) {
    contextScene = scene;
    selectedEntity = entt::null;
  }

  void OnRender() override {
    if (!visible)
      return;

    ImGui::Begin(panelName.c_str(), &visible);

    if (contextScene) {
      filter.Draw("##Search", ImGui::GetContentRegionAvail().x);
      ImGui::Separator();

      for (auto entityID :
           contextScene->GetRegistry().storage<entt::entity>()) {
        std::string tag = "Entity";
        if (contextScene->GetRegistry().any_of<TagComponent>(entityID)) {
          tag = contextScene->GetRegistry().get<TagComponent>(entityID).Tag;
        }

        if (filter.PassFilter(tag.c_str())) {
          DrawEntityNode(entityID, tag);
        }
      }
      if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) {
        selectedEntity = entt::null;
      }

      // Context menu on empty space
      if (ImGui::BeginPopupContextWindow(0,
                                         ImGuiPopupFlags_MouseButtonRight |
                                             ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Empty Entity")) {
          contextScene->CreateEntity("Empty Entity");
        }
        if (ImGui::MenuItem("Create Directional Light")) {
          auto e = contextScene->CreateEntity("Directional Light");
          e.AddComponent<DirectionalLightComponent>();
        }
        if (ImGui::MenuItem("Create Point Light")) {
          auto e = contextScene->CreateEntity("Point Light");
          e.AddComponent<PointLightComponent>();
        }
        if (ImGui::MenuItem("Create Particle System")) {
          auto e = contextScene->CreateEntity("Particle System");
          ParticleSystemParams pParams;
          e.AddComponent<ParticleSystemComponent>(pParams);
        }
        ImGui::EndPopup();
      }
    }

    ImGui::End();
  }

  entt::entity GetSelectedEntity() const { return selectedEntity; }

private:
  void DrawEntityNode(entt::entity entityID, const std::string &baseTag) {
    auto &registry = contextScene->GetRegistry();

    // Add component hints
    std::string hints = "";
    if (registry.any_of<MeshRendererComponent>(entityID) ||
        registry.any_of<SimpleMeshRendererComponent>(entityID))
      hints += " [Mesh]";
    if (registry.any_of<DirectionalLightComponent>(entityID) ||
        registry.any_of<PointLightComponent>(entityID))
      hints += " [Light]";
    if (registry.any_of<ParticleSystemComponent>(entityID))
      hints += " [Particles]";
    if (registry.any_of<NativeScriptComponent>(entityID))
      hints += " [Script]";

    std::string displayLabel = baseTag + hints;

    ImGuiTreeNodeFlags flags =
        ((selectedEntity == entityID) ? ImGuiTreeNodeFlags_Selected : 0) |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    // Since we don't have parent/child relationships yet, everything is a leaf
    flags |= ImGuiTreeNodeFlags_Leaf;

    bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entityID, flags,
                                    "%s", displayLabel.c_str());

    if (ImGui::IsItemClicked()) {
      selectedEntity = entityID;
    }

    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Delete Entity")) {
        entityDeleted = true;
      }
      ImGui::EndPopup();
    }

    if (opened) {
      ImGui::TreePop();
    }

    if (entityDeleted) {
      contextScene->DestroyEntity(Entity{entityID, contextScene});
      if (selectedEntity == entityID) {
        selectedEntity = entt::null;
      }
    }
  }
};

#endif
