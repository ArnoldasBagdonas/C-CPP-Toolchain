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
    /**
     * @brief Iterate files under the provided path.
     *
     * @param[in] path Root file or directory to enumerate
     * @param[in] onFile Callback invoked for each file
     */
    void Iterate(const fs::path& path, const std::function<void(const fs::path&)>& onFile) const;
};
