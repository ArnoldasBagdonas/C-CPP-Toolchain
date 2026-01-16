#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

/**
 * @brief Normalize path separators to forward slashes for cross-platform testing.
 *
 * Converts all backslash characters to forward slashes to ensure consistent
 * path comparison across Windows and Unix-like systems.
 *
 * @param[in] paths Vector of path strings to normalize
 * @return Vector of normalized path strings
 */
inline std::vector<std::string> NormalizePaths(const std::vector<std::string>& paths)
{
    std::vector<std::string> normalizedPaths;
    for (const auto& path : paths)
    {
        std::string normalizedPath = path;
        std::replace(normalizedPath.begin(), normalizedPath.end(), '\\', '/');
        normalizedPaths.push_back(normalizedPath);
    }
    return normalizedPaths;
}


enum class DirectoryListingMode
{
    Recursive,
    NonRecursive
};

/**
 * @brief Get directory contents as a sorted vector of strings.
 *
 * Traverses the specified directory and returns all file and subdirectory
 * paths. Paths are normalized and sorted for consistent test assertions.
 *
 * @param[in] directoryPath Path to the directory to traverse
 * @param[in] mode Specifies the traversal mode (Recursive or NonRecursive).
 * @return Sorted vector of normalized paths (relative if recursive, filenames
 * if not)
 */
inline std::vector<std::string> GetDirectoryEntries(
    const fs::path& directoryPath,
    DirectoryListingMode mode = DirectoryListingMode::NonRecursive)
{
    std::vector<std::string> contents;
    if ((fs::exists(directoryPath)) && (fs::is_directory(directoryPath)))
    {
        if (mode == DirectoryListingMode::Recursive)
        {
            for (const auto& entry : fs::recursive_directory_iterator(directoryPath))
            {
                contents.push_back(fs::relative(entry.path(), directoryPath).string());
            }
        }
        else
        {
            for (const auto& entry : fs::directory_iterator(directoryPath))
            {
                contents.push_back(entry.path().filename().string());
            }
        }
    }
    std::sort(contents.begin(), contents.end());
    return NormalizePaths(contents);
}
