#pragma once

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component for hashing files using xxHash.
 */
class FileHasher
{
  public:
    bool Compute(const fs::path& filePath, std::string& outputHash) const;
};
