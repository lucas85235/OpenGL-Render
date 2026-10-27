#ifndef VIEWPORT_PANEL_HPP
#define VIEWPORT_PANEL_HPP

#include "../core/camera.hpp"
#include "../renderer/framebuffer.hpp"
#include "../scene/components.hpp"
#include "../scene/scene.hpp"
#include "console_panel.hpp"
#include "editor_panel.hpp"
#include "hierarchy_panel.hpp"
#include "imgui.h"

#include <algorithm>
#include <functional>
#include <limits>

class ViewportPanel : public IEditorPanel {
private:
  FrameBuffer *sceneFB = nullptr;
  FrameBuffer *displayFB = nullptr;
  const bool *displayEnabledPtr = nullptr;
  Camera *camera = nullptr;
  Scene *scene = nullptr;
  HierarchyPanel *hierarchyPanel = nullptr;
  int lastWidth = 0;
  int lastHeight = 0;
  bool focused = false;
  bool hovered = false;

  // Pending pick request. Set when user left-clicks inside the viewport;
  // consumed by Application one frame later after the picking pass renders.
  bool pickRequested = false;
  int pickX = 0;
  int pickY = 0;

  std::function<void(int, int)> resizeCallback;

public:
  const char *GetName() const override { return "Viewport"; }

  void SetFrameBuffer(FrameBuffer *fb) { sceneFB = fb; }
  void SetDisplayFrameBuffer(FrameBuffer *fb) { displayFB = fb; }
  void SetDisplayEnabledPtr(const bool *p) { displayEnabledPtr = p; }
  void SetCamera(Camera *cam) { camera = cam; }
  void SetScene(Scene *s) { scene = s; }
  void SetHierarchyPanel(HierarchyPanel *p) { hierarchyPanel = p; }

  void SetResizeCallback(std::function<void(int, int)> cb) {
    resizeCallback = std::move(cb);
  }

  int GetViewportWidth() const { return lastWidth; }
  int GetViewportHeight() const { return lastHeight; }
  bool IsViewportFocused() const { return focused; }
  bool IsViewportHovered() const { return hovered; }

  bool HasPendingPick() const { return pickRequested; }
  int GetPickX() const { return pickX; }
  int GetPickY() const { return pickY; }
  void ClearPendingPick() { pickRequested = false; }

  void OnRender() override {
    focused = ImGui::IsWindowFocused();
    hovered = ImGui::IsWindowHovered();

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
      // Prefer display FB (post-tonemap output) when it's active, else fall
      // back to raw HDR sceneFB (will look washed out — toggle off is a
      // debug-only view).
      bool useDisplay = displayFB && displayFB->IsInitialized() &&
                        (!displayEnabledPtr || *displayEnabledPtr);
      FrameBuffer *source = useDisplay ? displayFB : sceneFB;
      ImTextureID texID = (ImTextureID)(uintptr_t)source->GetNativeTextureID();
      ImGui::Image(texID, ImVec2(static_cast<float>(w), static_cast<float>(h)),
                   ImVec2(0, 1), ImVec2(1, 0));

      if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
          !pickRequested) {
        ImVec2 imgMin = ImGui::GetItemRectMin();
        ImVec2 mp = ImGui::GetMousePos();
        int lx = static_cast<int>(mp.x - imgMin.x);
        int ly = static_cast<int>(mp.y - imgMin.y);
        if (lx >= 0 && ly >= 0 && lx < lastWidth && ly < lastHeight) {
          pickX = lx;
          pickY = ly;
          pickRequested = true;
        }
      }

      DrawAxisGizmo();
    } else {
      ImGui::TextDisabled("No framebuffer attached.");
    }

    if (hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
      ImGui::SetWindowFocus();
    }

    ForwardInputToCamera();
  }

private:
  void ForwardInputToCamera() {
    if (!camera)
      return;

    ImGuiIO &io = ImGui::GetIO();

    if (hovered && io.MouseWheel != 0.0f) {
      camera->ProcessScroll(io.MouseWheel);
    }

    if (!focused)
      return;

    float dt = io.DeltaTime;

    if (ImGui::IsKeyDown(ImGuiKey_W))
      camera->ProcessKeyboard(Camera::FORWARD, dt);
    if (ImGui::IsKeyDown(ImGuiKey_S))
      camera->ProcessKeyboard(Camera::BACKWARD, dt);
    if (ImGui::IsKeyDown(ImGuiKey_A))
      camera->ProcessKeyboard(Camera::LEFT, dt);
    if (ImGui::IsKeyDown(ImGuiKey_D))
      camera->ProcessKeyboard(Camera::RIGHT, dt);
    if (ImGui::IsKeyDown(ImGuiKey_E))
      camera->ProcessKeyboard(Camera::UP, dt);
    if (ImGui::IsKeyDown(ImGuiKey_Q))
      camera->ProcessKeyboard(Camera::DOWN, dt);

    if (ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
      ImVec2 d = io.MouseDelta;
      if (d.x != 0.0f || d.y != 0.0f) {
        camera->ProcessMouse(d.x, -d.y);
      }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
      FocusOnSelected();
    }
  }

  void DrawAxisGizmo() {
    if (!camera)
      return;

    ImVec2 imgMin = ImGui::GetItemRectMin();
    ImVec2 imgMax = ImGui::GetItemRectMax();
    const float radius = 36.0f;
    const float margin = 12.0f;
    ImVec2 center(imgMin.x + margin + radius, imgMax.y - margin - radius);

    glm::mat3 R = glm::mat3(camera->GetViewMatrix());

    struct Item {
      ImVec2 end;
      ImU32 color;
      const char *label;
      float z;
    };
    Item items[6];
    int n = 0;

    struct Axis {
      glm::vec3 dir;
      ImU32 color;
      const char *label;
    };
    const Axis axes[3] = {
        {{1, 0, 0}, IM_COL32(230, 70, 70, 255), "X"},
        {{0, 1, 0}, IM_COL32(90, 200, 90, 255), "Y"},
        {{0, 0, 1}, IM_COL32(80, 130, 230, 255), "Z"},
    };

    for (const Axis &a : axes) {
      glm::vec3 v = R * a.dir;
      ImVec2 endPos(center.x + v.x * radius, center.y - v.y * radius);
      items[n++] = {endPos, a.color, a.label, v.z};

      glm::vec3 nv = -v;
      ImVec2 nEnd(center.x + nv.x * radius, center.y - nv.y * radius);
      ImU32 dim = (a.color & 0x00FFFFFFu) | (0x80u << 24);
      items[n++] = {nEnd, dim, nullptr, nv.z};
    }

    std::sort(items, items + n,
              [](const Item &a, const Item &b) { return a.z < b.z; });

    ImDrawList *dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(center, radius + 4.0f, IM_COL32(20, 20, 20, 120), 32);

    for (int i = 0; i < n; ++i) {
      if (items[i].label) {
        dl->AddLine(center, items[i].end, items[i].color, 2.0f);
        dl->AddCircleFilled(items[i].end, 7.0f, items[i].color);
        ImVec2 ts = ImGui::CalcTextSize(items[i].label);
        dl->AddText({items[i].end.x - ts.x * 0.5f,
                     items[i].end.y - ts.y * 0.5f},
                    IM_COL32(15, 15, 15, 255), items[i].label);
      } else {
        dl->AddCircle(items[i].end, 5.0f, items[i].color, 12, 1.5f);
      }
    }
  }

  void FocusOnSelected() {
    if (!camera || !scene || !hierarchyPanel)
      return;
    entt::entity eid = hierarchyPanel->GetSelectedEntity();
    if (eid == entt::null)
      return;

    auto &reg = scene->GetRegistry();
    if (!reg.valid(eid) || !reg.any_of<TransformComponent>(eid))
      return;

    glm::mat4 world = reg.get<TransformComponent>(eid).GetMatrix();
    glm::vec3 aabbMin(std::numeric_limits<float>::max());
    glm::vec3 aabbMax(std::numeric_limits<float>::lowest());
    bool hasBounds = false;

    if (reg.any_of<MeshRendererComponent>(eid)) {
      auto &mr = reg.get<MeshRendererComponent>(eid);
      if (mr.model) {
        for (size_t i = 0; i < mr.model->GetMeshCount(); ++i) {
          const Mesh &m = mr.model->GetMesh(i);
          for (const Vertex &v : m.vertices) {
            glm::vec3 p = glm::vec3(world * glm::vec4(v.Position, 1.0f));
            aabbMin = glm::min(aabbMin, p);
            aabbMax = glm::max(aabbMax, p);
            hasBounds = true;
          }
        }
      }
    }

    if (reg.any_of<SimpleMeshRendererComponent>(eid)) {
      auto &sr = reg.get<SimpleMeshRendererComponent>(eid);
      if (sr.mesh) {
        for (const Vertex &v : sr.mesh->vertices) {
          glm::vec3 p = glm::vec3(world * glm::vec4(v.Position, 1.0f));
          aabbMin = glm::min(aabbMin, p);
          aabbMax = glm::max(aabbMax, p);
          hasBounds = true;
        }
      }
    }

    glm::vec3 center;
    float radius;
    if (hasBounds) {
      center = 0.5f * (aabbMin + aabbMax);
      radius = glm::length(aabbMax - center);
      if (radius < 0.001f)
        radius = 1.0f;
    } else {
      center = reg.get<TransformComponent>(eid).Position;
      radius = 3.0f;
    }

    float dist = glm::max(radius * 2.5f, 1.0f);
    camera->SetOrbit(center, dist, camera->GetYaw(), camera->GetPitch());
  }
};

#endif // VIEWPORT_PANEL_HPP
