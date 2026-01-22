#pragma once

#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component for enumerating files on the filesystem.
 */
class FilesystemFileEnumerator
{
  public:
    void Enumerate(const fs::path& path, const std::function<void(const fs::path&)>& onFile) const;
};
