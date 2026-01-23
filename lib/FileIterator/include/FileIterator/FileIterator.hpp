#pragma once

#include <filesystem>
#include <functional>

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
    void Iterate(const std::filesystem::path& path,
           const std::function<void(const std::filesystem::path&)>& onFile) const;
};
