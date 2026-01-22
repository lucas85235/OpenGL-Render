#ifndef DEBUG_UI_HPP
#define DEBUG_UI_HPP

#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Console-based debug UI system without external dependencies
// Provides stats overlay and console commands for runtime debugging

struct FrameStats {
  float fps = 0.0f;
  float frameTime = 0.0f;
  float updateTime = 0.0f;
  float renderTime = 0.0f;
  int drawCalls = 0;
  int triangles = 0;
  size_t memoryUsage = 0;
};

struct DebugSettings {
  bool showStats = true;
  bool showWireframe = false;
  bool showNormals = false;
  bool showBoundingBoxes = false;
  bool pauseUpdate = false;
  float timeScale = 1.0f;
};

class DebugUI {
private:
  FrameStats stats;
  DebugSettings settings;

  // FPS calculation
  int frameCount = 0;
  float fpsTimer = 0.0f;
  float lastFPS = 0.0f;

  // Timing
  std::chrono::high_resolution_clock::time_point frameStart;
  std::chrono::high_resolution_clock::time_point updateStart;
  std::chrono::high_resolution_clock::time_point renderStart;

  // Console commands
  using CommandFunc = std::function<void(const std::vector<std::string> &)>;
  std::unordered_map<std::string, CommandFunc> commands;
  std::vector<std::string> consoleHistory;
  bool consoleVisible = false;

public:
  DebugUI() { RegisterDefaultCommands(); }

  void BeginFrame() { frameStart = std::chrono::high_resolution_clock::now(); }

  void BeginUpdate() {
    updateStart = std::chrono::high_resolution_clock::now();
  }

  void EndUpdate() {
    auto now = std::chrono::high_resolution_clock::now();
    stats.updateTime =
        std::chrono::duration<float, std::milli>(now - updateStart).count();
  }

  void BeginRender() {
    renderStart = std::chrono::high_resolution_clock::now();
  }

  void EndRender() {
    auto now = std::chrono::high_resolution_clock::now();
    stats.renderTime =
        std::chrono::duration<float, std::milli>(now - renderStart).count();
  }

  void EndFrame(float deltaTime) {
    auto now = std::chrono::high_resolution_clock::now();
    stats.frameTime =
        std::chrono::duration<float, std::milli>(now - frameStart).count();

    frameCount++;
    fpsTimer += deltaTime;

    if (fpsTimer >= 1.0f) {
      lastFPS = static_cast<float>(frameCount) / fpsTimer;
      stats.fps = lastFPS;
      frameCount = 0;
      fpsTimer = 0.0f;
    }
  }

  void SetDrawCalls(int count) { stats.drawCalls = count; }
  void SetTriangles(int count) { stats.triangles = count; }
  void SetMemoryUsage(size_t bytes) { stats.memoryUsage = bytes; }

  const FrameStats &GetStats() const { return stats; }
  DebugSettings &GetSettings() { return settings; }
  const DebugSettings &GetSettings() const { return settings; }

  void ToggleStats() { settings.showStats = !settings.showStats; }
  void ToggleWireframe() { settings.showWireframe = !settings.showWireframe; }
  void TogglePause() { settings.pauseUpdate = !settings.pauseUpdate; }
  void ToggleConsole() { consoleVisible = !consoleVisible; }

  float GetTimeScale() const {
    return settings.pauseUpdate ? 0.0f : settings.timeScale;
  }

  void SetTimeScale(float scale) {
    settings.timeScale = std::max(0.0f, std::min(scale, 10.0f));
  }

  // Print stats to console
  void PrintStats() const {
    std::cout << "\n===== Debug Stats =====\n";
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "FPS: " << stats.fps << " (" << stats.frameTime << " ms)\n";
    std::cout << "Update: " << stats.updateTime << " ms\n";
    std::cout << "Render: " << stats.renderTime << " ms\n";
    std::cout << "Draw Calls: " << stats.drawCalls << "\n";
    std::cout << "Triangles: " << stats.triangles << "\n";
    std::cout << "Memory: " << (stats.memoryUsage / 1024 / 1024) << " MB\n";
    std::cout << "=======================\n";
  }

  std::string GetStatsString() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "FPS: " << stats.fps << " | ";
    ss << "Frame: " << stats.frameTime << "ms | ";
    ss << "Update: " << stats.updateTime << "ms | ";
    ss << "Render: " << stats.renderTime << "ms";
    return ss.str();
  }

  // Console command system
  void RegisterCommand(const std::string &name, CommandFunc func) {
    commands[name] = func;
  }

  void ExecuteCommand(const std::string &input) {
    if (input.empty())
      return;

    consoleHistory.push_back(input);

    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
      args.push_back(arg);
    }

    auto it = commands.find(cmd);
    if (it != commands.end()) {
      it->second(args);
    } else {
      std::cout << "[DebugUI] Unknown command: " << cmd << std::endl;
    }
  }

  void PrintHelp() const {
    std::cout << "\n===== Debug Commands =====\n";
    for (const auto &[name, _] : commands) {
      std::cout << "  " << name << "\n";
    }
    std::cout << "==========================\n";
  }

  // Process keyboard shortcuts
  void ProcessInput(int key, bool pressed) {
    if (!pressed)
      return;

    switch (key) {
    case 'F': // F1 - Toggle stats
      ToggleStats();
      std::cout << "[DebugUI] Stats: " << (settings.showStats ? "ON" : "OFF")
                << std::endl;
      break;
    case 'G': // F2 - Toggle wireframe
      ToggleWireframe();
      std::cout << "[DebugUI] Wireframe: "
                << (settings.showWireframe ? "ON" : "OFF") << std::endl;
      break;
    case 'H': // F3 - Pause
      TogglePause();
      std::cout << "[DebugUI] Pause: " << (settings.pauseUpdate ? "ON" : "OFF")
                << std::endl;
      break;
    case '`': // ` - Toggle console
      ToggleConsole();
      break;
    }
  }

private:
  void RegisterDefaultCommands() {
    RegisterCommand("help", [this](const auto &) { PrintHelp(); });
    RegisterCommand("stats", [this](const auto &) { PrintStats(); });
    RegisterCommand("wireframe", [this](const auto &) { ToggleWireframe(); });
    RegisterCommand("pause", [this](const auto &) { TogglePause(); });

    RegisterCommand("timescale", [this](const auto &args) {
      if (args.empty()) {
        std::cout << "Current timescale: " << settings.timeScale << std::endl;
      } else {
        SetTimeScale(std::stof(args[0]));
        std::cout << "Timescale set to: " << settings.timeScale << std::endl;
      }
    });

    RegisterCommand("clear", [](const auto &) {
      std::cout << "\033[2J\033[H"; // ANSI clear screen
    });
  }
};

#endif // DEBUG_UI_HPP
