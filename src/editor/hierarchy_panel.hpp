#ifndef HIERARCHY_PANEL_HPP
#define HIERARCHY_PANEL_HPP

#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "editor_panel.hpp"
#include <algorithm>
#include <imgui.h>

class HierarchyPanel : public IEditorPanel {
private:
  Scene *contextScene = nullptr;
  entt::entity selectedEntity = entt::null;
  ImGuiTextFilter filter;

  static constexpr const char *kDragPayload = "HIERARCHY_ENTITY";

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
      auto &registry = contextScene->GetRegistry();

      filter.Draw("##Search", ImGui::GetContentRegionAvail().x);
      ImGui::Separator();

      // Drop target on the window background = unparent
      ImGui::BeginChild("##HierarchyTree", ImVec2(0, 0), false);
      for (auto entityID : registry.storage<entt::entity>()) {
        if (!registry.all_of<TransformComponent>(entityID))
          continue;
        const auto &tc = registry.get<TransformComponent>(entityID);
        if (tc.Parent != entt::null && registry.valid(tc.Parent))
          continue; // not a root
        DrawEntityNode(entityID);
      }

      // Empty-space click clears selection
      if (ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered() &&
          ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        selectedEntity = entt::null;
      }
      ImGui::EndChild();

      // Drop on background of the child area = reparent to root
      if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload *payload =
                ImGui::AcceptDragDropPayload(kDragPayload)) {
          entt::entity dragged = *(const entt::entity *)payload->Data;
          Reparent(registry, dragged, entt::null);
        }
        ImGui::EndDragDropTarget();
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
  void SetSelectedEntity(entt::entity e) { selectedEntity = e; }

private:
  void DrawEntityNode(entt::entity entityID) {
    auto &registry = contextScene->GetRegistry();

    std::string tag = "Entity";
    if (registry.any_of<TagComponent>(entityID))
      tag = registry.get<TagComponent>(entityID).Tag;

    // Filter: show node if it matches OR any descendant matches
    bool selfMatch = filter.PassFilter(tag.c_str());
    bool descMatch = HasMatchingDescendant(entityID);
    if (!selfMatch && !descMatch)
      return;

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

    std::string displayLabel = tag + hints;

    const auto &tc = registry.get<TransformComponent>(entityID);
    bool hasChildren = !tc.Children.empty();

    ImGuiTreeNodeFlags flags =
        ((selectedEntity == entityID) ? ImGuiTreeNodeFlags_Selected : 0) |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!hasChildren)
      flags |= ImGuiTreeNodeFlags_Leaf;

    bool opened = ImGui::TreeNodeEx((void *)(uint64_t)(uint32_t)entityID, flags,
                                    "%s", displayLabel.c_str());

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
      selectedEntity = entityID;
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
      ImGui::SetDragDropPayload(kDragPayload, &entityID, sizeof(entt::entity));
      ImGui::TextUnformatted(displayLabel.c_str());
      ImGui::EndDragDropSource();
    }

    // Drop target: reparent dragged under this node
    if (ImGui::BeginDragDropTarget()) {
      if (const ImGuiPayload *payload =
              ImGui::AcceptDragDropPayload(kDragPayload)) {
        entt::entity dragged = *(const entt::entity *)payload->Data;
        Reparent(registry, dragged, entityID);
      }
      ImGui::EndDragDropTarget();
    }

    bool entityDeleted = false;
    if (ImGui::BeginPopupContextItem()) {
      if (ImGui::MenuItem("Delete Entity")) {
        entityDeleted = true;
      }
      ImGui::EndPopup();
    }

    if (opened) {
      // Snapshot children — list may mutate during drag-drop
      auto childrenCopy = tc.Children;
      for (auto child : childrenCopy) {
        if (registry.valid(child) &&
            registry.all_of<TransformComponent>(child))
          DrawEntityNode(child);
      }
      ImGui::TreePop();
    }

    if (entityDeleted) {
      // Scene::DestroyEntity recurses children + detaches from parent
      contextScene->DestroyEntity(Entity{entityID, contextScene});
      if (selectedEntity == entityID)
        selectedEntity = entt::null;
    }
  }

  bool HasMatchingDescendant(entt::entity entityID) {
    if (filter.IsActive() == false)
      return false;
    auto &registry = contextScene->GetRegistry();
    const auto &tc = registry.get<TransformComponent>(entityID);
    for (auto child : tc.Children) {
      if (!registry.valid(child) ||
          !registry.all_of<TransformComponent>(child))
        continue;
      std::string ctag = registry.any_of<TagComponent>(child)
                             ? registry.get<TagComponent>(child).Tag
                             : std::string("Entity");
      if (filter.PassFilter(ctag.c_str()))
        return true;
      if (HasMatchingDescendant(child))
        return true;
    }
    return false;
  }

  static void DetachFromParent(entt::registry &reg, entt::entity child) {
    auto &tc = reg.get<TransformComponent>(child);
    if (tc.Parent == entt::null || !reg.valid(tc.Parent) ||
        !reg.all_of<TransformComponent>(tc.Parent))
      return;
    auto &pt = reg.get<TransformComponent>(tc.Parent);
    pt.Children.erase(
        std::remove(pt.Children.begin(), pt.Children.end(), child),
        pt.Children.end());
    tc.Parent = entt::null;
  }

  static void Reparent(entt::registry &reg, entt::entity child,
                       entt::entity newParent) {
    if (child == entt::null || child == newParent)
      return;
    if (!reg.valid(child) || !reg.all_of<TransformComponent>(child))
      return;
    if (newParent != entt::null) {
      if (!reg.valid(newParent) ||
          !reg.all_of<TransformComponent>(newParent))
        return;
      // Cycle guard: newParent must not be descendant of child
      entt::entity cursor = newParent;
      while (cursor != entt::null) {
        if (cursor == child)
          return;
        if (!reg.valid(cursor) ||
            !reg.all_of<TransformComponent>(cursor))
          break;
        cursor = reg.get<TransformComponent>(cursor).Parent;
      }
    }
    DetachFromParent(reg, child);
    auto &ct = reg.get<TransformComponent>(child);
    ct.Parent = newParent;
    if (newParent != entt::null) {
      auto &np = reg.get<TransformComponent>(newParent);
      np.Children.push_back(child);
    }
  }
};

#endif
