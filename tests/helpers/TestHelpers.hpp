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

/**
 * @brief Get directory contents recursively as a sorted vector of strings.
 *
 * Recursively traverses the specified directory and returns all file and
 * subdirectory paths relative to the directory root. Paths are normalized
 * and sorted for consistent test assertions.
 *
 * @param[in] directoryPath Path to the directory to traverse
 * @return Sorted vector of normalized relative paths
 */
inline std::vector<std::string> GetDirectoryContents(const fs::path& directoryPath)
{
    std::vector<std::string> contents;
    if ((fs::exists(directoryPath)) && (fs::is_directory(directoryPath)))
    {
        for (const auto& entry : fs::recursive_directory_iterator(directoryPath))
        {
            contents.push_back(fs::relative(entry.path(), directoryPath).string());
        }
    }
    std::sort(contents.begin(), contents.end());
    return NormalizePaths(contents);
}

/**
 * @brief Get directory contents non-recursively as a sorted vector of strings.
 *
 * Lists only the immediate children of the specified directory without
 * traversing subdirectories. Results are sorted for consistent test assertions.
 *
 * @param[in] directoryPath Path to the directory to list
 * @return Sorted vector of filenames and subdirectory names
 */
inline std::vector<std::string> ListDirectory(const fs::path& directoryPath)
{
    std::vector<std::string> contents;
    if ((fs::exists(directoryPath)) && (fs::is_directory(directoryPath)))
    {
        for (const auto& entry : fs::directory_iterator(directoryPath))
        {
            contents.push_back(entry.path().filename().string());
        }
    }
    std::sort(contents.begin(), contents.end());
    return contents;
}
