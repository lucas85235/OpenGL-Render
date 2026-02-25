#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "../renderer/framebuffer.hpp"
#include "console_panel.hpp"
#include "editor_panel.hpp"
#include "imgui.h"

#include <functional>

class ViewportPanel : public IEditorPanel {
private:
  FrameBuffer *sceneFB = nullptr;
  int lastWidth = 0;
  int lastHeight = 0;

  std::function<void(int, int)> resizeCallback;

public:
  const char *GetName() const override { return "Viewport"; }

  void SetFrameBuffer(FrameBuffer *fb) { sceneFB = fb; }

  void SetResizeCallback(std::function<void(int, int)> cb) {
    resizeCallback = std::move(cb);
  }

  int GetViewportWidth() const { return lastWidth; }
  int GetViewportHeight() const { return lastHeight; }

  void OnRender() override {
    ImVec2 size = ImGui::GetContentRegionAvail();
    int w = static_cast<int>(size.x);
    int h = static_cast<int>(size.y);

    if (w < 1)
      w = 1;
    if (h < 1)
      h = 1;

    if (sceneFB && (w != lastWidth || h != lastHeight)) {
      lastWidth = w;
      lastHeight = h;
      sceneFB->Resize(w, h);
      if (resizeCallback)
        resizeCallback(w, h);
    }

    if (sceneFB && sceneFB->IsInitialized()) {
      // ImGui::Image expects a texture ID. For OpenGL, the raw handle is the
      // GLuint cast to ImTextureID.
      ImTextureID texID = (ImTextureID)(uintptr_t)sceneFB->GetTexture().id;
      ImGui::Image(texID, ImVec2(static_cast<float>(w), static_cast<float>(h)),
                   ImVec2(0, 1), ImVec2(1, 0));
    } else {
      ImGui::TextDisabled("No framebuffer attached.");
    }
  }
};

#endif // VIEWPORT_PANEL_HPP
