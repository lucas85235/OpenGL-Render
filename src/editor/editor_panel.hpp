#ifndef EDITOR_PANEL_HPP
#define EDITOR_PANEL_HPP

#include <string>

class IEditorPanel {
protected:
  bool visible = true;
  std::string panelName;

public:
  virtual ~IEditorPanel() = default;

  virtual const char *GetName() const = 0;

  virtual void OnInit() {}
  virtual void OnShutdown() {}
  virtual void OnUpdate(float dt) {}

  // ImGui draw calls go here (called between Begin/End by EditorUI)
  virtual void OnRender() = 0;

  bool IsVisible() const { return visible; }
  void SetVisible(bool v) { visible = v; }
  void ToggleVisible() { visible = !visible; }
};

#endif // EDITOR_PANEL_HPP
