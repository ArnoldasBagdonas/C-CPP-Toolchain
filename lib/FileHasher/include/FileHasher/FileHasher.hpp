#pragma once

#include <filesystem>
#include <string>

/**
 * @brief Infrastructure component for hashing files using xxHash.
 */
class FileHasher
{
  public:
    /**
     * @brief Compute a content hash for the specified file.
     *
     * @param[in] filePath Path to the file to hash
     * @param[out] outputHash Output hex-encoded hash string
     * @return true on success, false on error
     */
    bool Compute(const std::filesystem::path& filePath, std::string& outputHash) const;
};
