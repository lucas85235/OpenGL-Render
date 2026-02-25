#ifndef EXAMPLE_PANEL_HPP
#define EXAMPLE_PANEL_HPP

#include "editor_panel.hpp"
#include "imgui.h"

// Minimal example panel demonstrating how to extend the editor.
// Copy this file and implement your own panel logic.
class ExamplePanel : public IEditorPanel {
private:
  int clickCount = 0;

public:
  const char *GetName() const override { return "Example"; }

  void OnRender() override {
    ImGui::TextWrapped(
        "This is an example panel. Use it as a template for new panels.");
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Click Me")) {
      clickCount++;
    }
    ImGui::SameLine();
    ImGui::Text("Clicks: %d", clickCount);

    ImGui::Spacing();
    ImGui::TextDisabled("Inherit from IEditorPanel and register in EditorUI.");
  }
};

#endif // EXAMPLE_PANEL_HPP
