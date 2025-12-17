#ifndef NATIVE_FILE_SYSTEM_HPP
#define NATIVE_FILE_SYSTEM_HPP

#include "file_system.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

class NativeFileSystem : public IFileSystem {
public:
  bool Exists(const std::string &path) const override {
    return fs::exists(ResolvePath(path));
  }

  std::vector<uint8_t> ReadFile(const std::string &path) const override {
    std::string fullPath = ResolvePath(path);
    std::ifstream file(fullPath, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
      std::cerr << "[VFS] Failed to open file: " << fullPath << std::endl;
      return {};
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<uint8_t> buffer(fileSize);
    file.seekg(0);
    file.read(reinterpret_cast<char *>(buffer.data()), fileSize);
    file.close();

    return buffer;
  }

  std::string ReadTextFile(const std::string &path) const override {
    std::string fullPath = ResolvePath(path);
    std::ifstream file(fullPath);

    if (!file.is_open()) {
      std::cerr << "[VFS] Failed to open text file: " << fullPath << std::endl;
      return "";
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    return content;
  }

  std::string GetAbsolutePath(const std::string &path) const override {
    return ResolvePath(path);
  }

private:
  std::string ResolvePath(const std::string &path) const {
    // Reuse logic from original Filesystem class or simplify
    // 1. Check if absolute
    fs::path p(path);
    if (p.is_absolute() && fs::exists(p))
      return p.string();

    // 2. Check relative to CWD
    if (fs::exists(p))
      return fs::absolute(p).string();

// 3. Check ROOT_DIR if defined
#ifdef ROOT_DIR
    fs::path root(ROOT_DIR);
    fs::path rootPath = root / p;
    if (fs::exists(rootPath))
      return rootPath.string();

    // 4. Try src folder inside root (legacy project structure)
    fs::path srcPath = root / "src" / p;
    if (fs::exists(srcPath))
      return srcPath.string();
#endif

    return path; // Return as is if not found (let consumer fail)
  }
};

#endif // NATIVE_FILE_SYSTEM_HPP
