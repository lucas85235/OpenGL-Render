#ifndef POST_PROCESS_PANEL_HPP
#define POST_PROCESS_PANEL_HPP

#include "../renderer/bloom_pass.hpp"
#include "../renderer/tonemap_pass.hpp"
#include "editor_panel.hpp"
#include <imgui.h>

class PostProcessPanel : public IEditorPanel {
private:
  BloomPass *bloom = nullptr;
  TonemapPass *tonemap = nullptr;
  bool *bloomEnabledPtr = nullptr;
  bool *tonemapEnabledPtr = nullptr;

public:
  PostProcessPanel() {
    panelName = "Post Process";
    visible = false;
  }

  const char *GetName() const override { return panelName.c_str(); }

  void SetBloom(BloomPass *pass, bool *enabledPtr) {
    bloom = pass;
    bloomEnabledPtr = enabledPtr;
  }

  void SetTonemap(TonemapPass *pass, bool *enabledPtr) {
    tonemap = pass;
    tonemapEnabledPtr = enabledPtr;
  }

  void OnRender() override {
    if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (bloomEnabledPtr)
        ImGui::Checkbox("Enabled##Bloom", bloomEnabledPtr);

      if (bloom) {
        float threshold = bloom->GetThreshold();
        if (ImGui::DragFloat("Threshold", &threshold, 0.01f, 0.0f, 10.0f))
          bloom->SetThreshold(threshold);

        float knee = bloom->GetKnee();
        if (ImGui::DragFloat("Knee", &knee, 0.01f, 0.0f, 1.0f))
          bloom->SetKnee(knee);

        float intensity = bloom->GetIntensity();
        if (ImGui::DragFloat("Intensity", &intensity, 0.01f, 0.0f, 5.0f))
          bloom->SetIntensity(intensity);

        float radius = bloom->GetFilterRadius();
        if (ImGui::DragFloat("Filter Radius", &radius, 0.05f, 0.1f, 5.0f))
          bloom->SetFilterRadius(radius);
      } else {
        ImGui::TextDisabled("BloomPass unavailable");
      }
    }

    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Tonemap", ImGuiTreeNodeFlags_DefaultOpen)) {
      if (tonemapEnabledPtr)
        ImGui::Checkbox("Enabled##Tonemap", tonemapEnabledPtr);

      if (tonemap) {
        float exposure = tonemap->GetExposure();
        if (ImGui::DragFloat("Exposure", &exposure, 0.01f, 0.0f, 10.0f))
          tonemap->SetExposure(exposure);

        float gamma = tonemap->GetGamma();
        if (ImGui::DragFloat("Gamma", &gamma, 0.01f, 0.1f, 5.0f))
          tonemap->SetGamma(gamma);
      } else {
        ImGui::TextDisabled("TonemapPass unavailable");
      }
    }
  }
};

#endif // POST_PROCESS_PANEL_HPP
