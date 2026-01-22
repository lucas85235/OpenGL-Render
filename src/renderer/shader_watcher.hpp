#ifndef SHADER_WATCHER_HPP
#define SHADER_WATCHER_HPP

#include "shader_manager.hpp"
#include <chrono>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <unordered_map>

namespace fs = std::filesystem;

struct ShaderFileInfo {
  std::string path;
  std::string shaderName;
  fs::file_time_type lastModified;
};

class ShaderWatcher {
private:
  std::unordered_map<std::string, ShaderFileInfo> watchedFiles;
  ShaderManager *shaderManager = nullptr;
  IFileSystem *fileSystem = nullptr;
  RHI::API api;

  std::function<void(const std::string &)> onReloadCallback;
  bool enabled = true;
  float checkInterval = 1.0f; // seconds
  float timeSinceLastCheck = 0.0f;

public:
  ShaderWatcher(ShaderManager *mgr, IFileSystem *fs, RHI::API targetApi)
      : shaderManager(mgr), fileSystem(fs), api(targetApi) {}

  void WatchShader(const std::string &shaderName,
                   const std::string &shaderPath) {
    std::string absPath = fileSystem->GetAbsolutePath(shaderPath);

    if (!fs::exists(absPath)) {
      std::cerr << "[ShaderWatcher] File not found: " << absPath << std::endl;
      return;
    }

    ShaderFileInfo info;
    info.path = absPath;
    info.shaderName = shaderName;
    info.lastModified = fs::last_write_time(absPath);

    watchedFiles[shaderName] = info;
    std::cout << "[ShaderWatcher] Watching: " << shaderPath << std::endl;
  }

  void UnwatchShader(const std::string &shaderName) {
    watchedFiles.erase(shaderName);
  }

  void SetOnReloadCallback(std::function<void(const std::string &)> callback) {
    onReloadCallback = callback;
  }

  void SetCheckInterval(float seconds) { checkInterval = seconds; }

  void SetEnabled(bool e) { enabled = e; }

  bool IsEnabled() const { return enabled; }

  void Update(float deltaTime) {
    if (!enabled || !shaderManager)
      return;

    timeSinceLastCheck += deltaTime;
    if (timeSinceLastCheck < checkInterval)
      return;
    timeSinceLastCheck = 0.0f;

    CheckForChanges();
  }

  void CheckForChanges() {
    for (auto &[name, info] : watchedFiles) {
      if (!fs::exists(info.path))
        continue;

      auto currentTime = fs::last_write_time(info.path);
      if (currentTime != info.lastModified) {
        info.lastModified = currentTime;
        ReloadShader(name, info.path);
      }
    }
  }

  void ForceReloadAll() {
    for (auto &[name, info] : watchedFiles) {
      ReloadShader(name, info.path);
    }
  }

private:
  void ReloadShader(const std::string &name, const std::string &absPath) {
    std::cout << "[ShaderWatcher] Reloading shader: " << name << std::endl;

    // Read new source
    std::string relativePath = absPath;
    std::string source;

    std::ifstream file(absPath);
    if (file.is_open()) {
      std::stringstream buffer;
      buffer << file.rdbuf();
      source = buffer.str();
      file.close();
    }

    if (source.empty()) {
      std::cerr << "[ShaderWatcher] Failed to read shader file: " << absPath
                << std::endl;
      return;
    }

    // Recompile
    uint32_t glslVersion = 450;
    RHI::ShaderStageSource stages =
        RHI::ShaderPreprocessor::Process(source, api, glslVersion);

    if (stages.vertex.empty() || stages.fragment.empty()) {
      std::cerr << "[ShaderWatcher] Shader parse error in: " << name
                << std::endl;
      return;
    }

    auto shader = shaderManager->GetShader(name);
    if (!shader) {
      std::cerr << "[ShaderWatcher] Shader not found in manager: " << name
                << std::endl;
      return;
    }

    bool success = shader->CompileFromSource(stages.vertex.c_str(),
                                             stages.fragment.c_str());

    if (success) {
      std::cout << "[ShaderWatcher] ✓ Shader reloaded: " << name << std::endl;
      if (onReloadCallback) {
        onReloadCallback(name);
      }
    } else {
      std::cerr << "[ShaderWatcher] ✗ Failed to recompile: " << name
                << std::endl;
    }
  }
};

#endif // SHADER_WATCHER_HPP
