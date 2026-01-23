#pragma once

#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component for enumerating files on the filesystem.
 */
class FileIterator
{
  public:
    void Iterate(const fs::path& path, const std::function<void(const fs::path&)>& onFile) const;
};
