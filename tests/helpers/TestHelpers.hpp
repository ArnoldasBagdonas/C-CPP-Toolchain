#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <algorithm>

namespace fs = std::filesystem;

// Helper to get directory contents as a sorted vector of strings
inline std::vector<std::string> GetDirectoryContents(const fs::path& dir)
{
    std::vector<std::string> contents;
    if (fs::exists(dir) && fs::is_directory(dir))
    {
        for (const auto& entry : fs::recursive_directory_iterator(dir))
        {
            contents.push_back(fs::relative(entry.path(), dir).string());
        }
    }
    std::sort(contents.begin(), contents.end());
    return contents;
}

// Helper to get directory contents non-recursively
inline std::vector<std::string> ListDirectory(const fs::path& dir)
{
    std::vector<std::string> contents;
    if (fs::exists(dir) && fs::is_directory(dir))
    {
        for (const auto& entry : fs::directory_iterator(dir))
        {
            contents.push_back(entry.path().filename().string());
        }
    }
    std::sort(contents.begin(), contents.end());
    return contents;
}
