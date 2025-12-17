#ifndef I_FILE_SYSTEM_HPP
#define I_FILE_SYSTEM_HPP

#include <string>
#include <vector>

class IFileSystem {
public:
  virtual ~IFileSystem() = default;

  virtual bool Exists(const std::string &path) const = 0;
  virtual std::vector<uint8_t> ReadFile(const std::string &path) const = 0;
  virtual std::string ReadTextFile(const std::string &path) const = 0;
  virtual std::string GetAbsolutePath(const std::string &path) const = 0;
};

#endif
