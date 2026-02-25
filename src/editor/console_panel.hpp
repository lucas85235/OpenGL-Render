#ifndef CONSOLE_PANEL_HPP
#define CONSOLE_PANEL_HPP

#include "editor_panel.hpp"
#include "imgui.h"

#include <algorithm>
#include <cstdarg>
#include <deque>
#include <mutex>
#include <string>

enum class LogSeverity { Info, Warning, Error };

struct LogEntry {
  LogSeverity severity;
  std::string message;
};

class ConsolePanel : public IEditorPanel {
private:
  static constexpr size_t kMaxEntries = 1024;

  static std::deque<LogEntry> &GetEntries() {
    static std::deque<LogEntry> entries;
    return entries;
  }

  static std::mutex &GetMutex() {
    static std::mutex mtx;
    return mtx;
  }

  bool showInfo = true;
  bool showWarning = true;
  bool showError = true;
  bool autoScroll = true;
  char filterBuf[128] = {0};

  ImVec4 GetColor(LogSeverity s) const {
    switch (s) {
    case LogSeverity::Warning:
      return {1.0f, 0.85f, 0.2f, 1.0f};
    case LogSeverity::Error:
      return {1.0f, 0.3f, 0.3f, 1.0f};
    default:
      return {0.85f, 0.85f, 0.85f, 1.0f};
    }
  }

  const char *GetPrefix(LogSeverity s) const {
    switch (s) {
    case LogSeverity::Warning:
      return "[WARN] ";
    case LogSeverity::Error:
      return "[ERR]  ";
    default:
      return "[INFO] ";
    }
  }

  bool PassesFilter(const LogEntry &entry) const {
    if (entry.severity == LogSeverity::Info && !showInfo)
      return false;
    if (entry.severity == LogSeverity::Warning && !showWarning)
      return false;
    if (entry.severity == LogSeverity::Error && !showError)
      return false;

    if (filterBuf[0] != '\0') {
      return entry.message.find(filterBuf) != std::string::npos;
    }
    return true;
  }

public:
  const char *GetName() const override { return "Console"; }

  static void Log(LogSeverity severity, const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    std::lock_guard<std::mutex> lock(GetMutex());
    auto &entries = GetEntries();
    entries.push_back({severity, std::string(buf)});
    if (entries.size() > kMaxEntries) {
      entries.pop_front();
    }
  }

  static void LogInfo(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Log(LogSeverity::Info, "%s", buf);
  }

  static void LogWarn(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Log(LogSeverity::Warning, "%s", buf);
  }

  static void LogError(const char *fmt, ...) {
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    Log(LogSeverity::Error, "%s", buf);
  }

  static void Clear() {
    std::lock_guard<std::mutex> lock(GetMutex());
    GetEntries().clear();
  }

  void OnRender() override {
    // Toolbar
    if (ImGui::Button("Clear"))
      Clear();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,
                          showInfo ? ImVec4(0.2f, 0.5f, 0.8f, 1.0f)
                                   : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Info"))
      showInfo = !showInfo;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,
                          showWarning ? ImVec4(0.8f, 0.7f, 0.1f, 1.0f)
                                      : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Warn"))
      showWarning = !showWarning;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,
                          showError ? ImVec4(0.8f, 0.2f, 0.2f, 1.0f)
                                    : ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Error"))
      showError = !showError;
    ImGui::PopStyleColor();
    ImGui::SameLine();

    ImGui::SetNextItemWidth(200.0f);
    ImGui::InputTextWithHint("##filter", "Filter...", filterBuf,
                             sizeof(filterBuf));
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &autoScroll);

    ImGui::Separator();

    // Log entries
    ImGui::BeginChild("LogArea", ImVec2(0, 0), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);

    std::lock_guard<std::mutex> lock(GetMutex());
    for (const auto &entry : GetEntries()) {
      if (!PassesFilter(entry))
        continue;

      ImGui::PushStyleColor(ImGuiCol_Text, GetColor(entry.severity));
      ImGui::TextUnformatted(
          (std::string(GetPrefix(entry.severity)) + entry.message).c_str());
      ImGui::PopStyleColor();
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
      ImGui::SetScrollHereY(1.0f);

    ImGui::EndChild();
  }
};

#endif // CONSOLE_PANEL_HPP
