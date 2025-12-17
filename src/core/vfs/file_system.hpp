#ifndef I_FILE_SYSTEM_HPP
#define I_FILE_SYSTEM_HPP

#include <memory>
#include <string>
#include <vector>

class IFileSystem {
public:
  virtual ~IFileSystem() = default;

  // Check if a file or directory exists
  virtual bool Exists(const std::string &path) const = 0;

  // Read entire file into a byte vector
  virtual std::vector<uint8_t> ReadFile(const std::string &path) const = 0;

  // Read entire file into a string
  virtual std::string ReadTextFile(const std::string &path) const = 0;

  // Resolve a relative path to an absolute system path
  // Useful for libraries that require absolute paths (e.g. Assimp, stbi)
  virtual std::string GetAbsolutePath(const std::string &path) const = 0;
};

#endif // I_FILE_SYSTEM_HPP
