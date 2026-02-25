#ifndef EDITOR_UI_HPP
#define EDITOR_UI_HPP

#include "editor_panel.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"

#include <GLFW/glfw3.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "../rhi/rhi_device.h"

class EditorUI {
private:
private:
  std::vector<std::unique_ptr<IEditorPanel>> panels;
  bool initialized = false;
  bool showDemoWindow = false;
  RHI::IDevice *rhiDevice = nullptr;

public:
  EditorUI() = default;
  ~EditorUI() { Shutdown(); }

  bool Init(GLFWwindow *window, RHI::IDevice *device) {
    rhiDevice = device;
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // Style
    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 4.0f;
    style.FrameRounding = 2.0f;
    style.TabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.GrabRounding = 2.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.10f, 1.0f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.14f, 0.14f, 0.18f, 1.0f);
    style.Colors[ImGuiCol_Tab] = ImVec4(0.12f, 0.12f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_TabSelected] = ImVec4(0.22f, 0.22f, 0.28f, 1.0f);
    style.Colors[ImGuiCol_TabHovered] = ImVec4(0.28f, 0.28f, 0.35f, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.06f, 0.06f, 0.08f, 1.0f);

    if (rhiDevice->GetAPI() == RHI::API::OpenGL) {
      ImGui_ImplGlfw_InitForOpenGL(window, true);
    } else if (rhiDevice->GetAPI() == RHI::API::Vulkan) {
      ImGui_ImplGlfw_InitForVulkan(window, true);
    }

    rhiDevice->InitImGui(window);

    initialized = true;
    std::cout << "[EditorUI] Initialized" << std::endl;

    for (auto &panel : panels) {
      panel->OnInit();
    }

    return true;
  }

  void Shutdown() {
    if (!initialized)
      return;

    for (auto &panel : panels) {
      panel->OnShutdown();
    }

    if (rhiDevice) {
      rhiDevice->ShutdownImGui();
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    initialized = false;
    std::cout << "[EditorUI] Shutdown" << std::endl;
  }

  // Register a panel. Call before or after Init().
  template <typename T, typename... Args> T *AddPanel(Args &&...args) {
    auto panel = std::make_unique<T>(std::forward<Args>(args)...);
    T *ptr = panel.get();
    panels.push_back(std::move(panel));
    if (initialized) {
      ptr->OnInit();
    }
    return ptr;
  }

  void Update(float dt) {
    for (auto &panel : panels) {
      panel->OnUpdate(dt);
    }
  }

  void BeginFrame() {
    if (!initialized || !rhiDevice)
      return;

    rhiDevice->NewFrameImGui();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
  }

  void Render() {
    if (!initialized)
      return;

    SetupDockSpace();

    for (auto &panel : panels) {
      if (!panel->IsVisible())
        continue;
      ImGui::Begin(panel->GetName());
      panel->OnRender();
      ImGui::End();
    }

    if (showDemoWindow) {
      ImGui::ShowDemoWindow(&showDemoWindow);
    }
  }

  void EndFrame() {
    if (!initialized || !rhiDevice)
      return;

    ImGui::Render();
    rhiDevice->RenderImGui();
  }

  bool IsInitialized() const { return initialized; }

private:
  void SetupDockSpace() {
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
        ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImGui::Begin("DockSpaceWindow", nullptr, windowFlags);
    ImGui::PopStyleVar(3);

    ImGuiID dockspaceId = ImGui::GetID("EngineDockSpace");
    ImGui::DockSpace(dockspaceId, ImVec2(0, 0), ImGuiDockNodeFlags_None);

    DrawMainMenuBar();

    ImGui::End();
  }

  void DrawMainMenuBar() {
    if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("View")) {
        for (auto &panel : panels) {
          bool vis = panel->IsVisible();
          if (ImGui::MenuItem(panel->GetName(), nullptr, &vis)) {
            panel->SetVisible(vis);
          }
        }
        ImGui::Separator();
        ImGui::MenuItem("ImGui Demo", nullptr, &showDemoWindow);
        ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
    }
  }
};

#endif // EDITOR_UI_HPP
