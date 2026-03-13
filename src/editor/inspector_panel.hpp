#ifndef INSPECTOR_PANEL_HPP
#define INSPECTOR_PANEL_HPP

#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "editor_panel.hpp"
#include "hierarchy_panel.hpp"
#include <imgui.h>

class InspectorPanel : public IEditorPanel {
private:
  Scene *contextScene = nullptr;
  HierarchyPanel *hierarchyContext = nullptr;

public:
  InspectorPanel() { panelName = "Inspector"; }

  const char *GetName() const override { return panelName.c_str(); }

  void SetContext(Scene *scene, HierarchyPanel *hierarchy) {
    contextScene = scene;
    hierarchyContext = hierarchy;
  }

  void OnRender() override {
    if (!visible)
      return;

    ImGui::Begin(panelName.c_str(), &visible);

    if (contextScene && hierarchyContext) {
      entt::entity selectedEntity = hierarchyContext->GetSelectedEntity();

      if (selectedEntity != entt::null &&
          contextScene->GetRegistry().valid(selectedEntity)) {
        DrawComponents(selectedEntity);
      } else {
        ImGui::Text("No entity selected");
      }
    }

    ImGui::End();
  }

private:
  template <typename T, typename UIFunction>
  void DrawComponent(const std::string &name, entt::entity entity,
                     UIFunction uiFunction) {
    auto &registry = contextScene->GetRegistry();

    if (registry.any_of<T>(entity)) {
      ImGuiTreeNodeFlags treeNodeFlags =
          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
          ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowOverlap |
          ImGuiTreeNodeFlags_FramePadding;

      // Allow removing components
      ImGui::PushID((void *)typeid(T).hash_code());

      bool opened = ImGui::TreeNodeEx((void *)typeid(T).hash_code(),
                                      treeNodeFlags, "%s", name.c_str());

      ImGui::SameLine(ImGui::GetWindowWidth() - 25.0f);
      if (ImGui::Button("-", ImVec2(20, 20))) {
        registry.remove<T>(entity);
        if (opened)
          ImGui::TreePop();
        ImGui::PopID();
        return;
      }

      if (opened) {
        auto &component = registry.get<T>(entity);
        uiFunction(component);
        ImGui::TreePop();
      }
      ImGui::PopID();
    }
  }

  void DrawComponents(entt::entity entity) {
    auto &registry = contextScene->GetRegistry();

    // TagComponent
    if (registry.any_of<TagComponent>(entity)) {
      auto &tag = registry.get<TagComponent>(entity).Tag;
      char buffer[256];
      memset(buffer, 0, sizeof(buffer));
      strncpy(buffer, tag.c_str(), sizeof(buffer) - 1);
      if (ImGui::InputText("##Tag", buffer, sizeof(buffer))) {
        tag = std::string(buffer);
      }
    }

    ImGui::Separator();

    DrawComponent<TransformComponent>("Transform", entity, [](auto &component) {
      ImGui::DragFloat3("Position", &component.Position[0], 0.1f);
      ImGui::DragFloat3("Rotation", &component.Rotation[0], 0.1f);
      ImGui::DragFloat3("Scale", &component.Scale[0], 0.1f);
    });

    DrawComponent<DirectionalLightComponent>(
        "Directional Light", entity, [](auto &component) {
          ImGui::ColorEdit3("Color", &component.color[0]);
          ImGui::DragFloat("Intensity", &component.intensity, 0.1f, 0.0f,
                           100.0f);
          ImGui::DragFloat3("Direction", &component.direction[0], 0.01f, -1.0f,
                            1.0f);
        });

    DrawComponent<PointLightComponent>(
        "Point Light", entity, [](auto &component) {
          ImGui::ColorEdit3("Color", &component.color[0]);
          ImGui::DragFloat("Intensity", &component.intensity, 0.1f, 0.0f,
                           100.0f);
          ImGui::DragFloat("Radius", &component.radius, 0.1f, 0.0f, 100.0f);
        });

    DrawComponent<ParticleSystemComponent>(
        "Particle System", entity, [](auto &component) {
          ImGui::DragInt("Amount", (int *)&component.params.amount, 10.0f, 0,
                         100000);
          ImGui::ColorEdit3("Base Color", &component.params.baseColor[0]);
          ImGui::DragFloat("System Radius", &component.params.radius, 0.1f,
                           0.1f, 50.0f);
          ImGui::DragFloat2("Particle Size", &component.params.size[0], 0.01f,
                            0.01f, 5.0f);
        });

    DrawComponent<NativeScriptComponent>(
        "Native Script", entity, [](auto &component) {
          ImGui::TextDisabled("Script logic runs via engine systems.");
        });

    // Add Component Menu
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    if (ImGui::Button("Add Component", ImVec2(-1, 0))) {
      ImGui::OpenPopup("AddComponentPopup");
    }

    if (ImGui::BeginPopup("AddComponentPopup")) {
      if (!registry.any_of<DirectionalLightComponent>(entity) &&
          ImGui::MenuItem("Directional Light")) {
        registry.emplace<DirectionalLightComponent>(entity);
        ImGui::CloseCurrentPopup();
      }
      if (!registry.any_of<PointLightComponent>(entity) &&
          ImGui::MenuItem("Point Light")) {
        registry.emplace<PointLightComponent>(entity);
        ImGui::CloseCurrentPopup();
      }
      if (!registry.any_of<ParticleSystemComponent>(entity) &&
          ImGui::MenuItem("Particle System")) {
        ParticleSystemParams pParams;
        registry.emplace<ParticleSystemComponent>(entity, pParams);
        ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
    }
  }
};

#endif
