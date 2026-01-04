// file BackupUtility.hpp:

#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace fs = std::filesystem;

enum class ChangeType
{
    Unchanged,
    Added,
    Modified,
    Deleted
};

inline const char* ChangeTypeToString(ChangeType type)
{
    switch (type)
    {
    case ChangeType::Unchanged:
        return "Unchanged";
    case ChangeType::Added:
        return "Added";
    case ChangeType::Modified:
        return "Modified";
    case ChangeType::Deleted:
        return "Deleted";
    }
    return "Unknown";
}

inline ChangeType StringToChangeType(const std::string& s)
{
    if (s == "Unchanged")
        return ChangeType::Unchanged;
    if (s == "Added")
        return ChangeType::Added;
    if (s == "Modified")
        return ChangeType::Modified;
    if (s == "Deleted")
        return ChangeType::Deleted;
    return ChangeType::Unchanged;
}

struct BackupProgress
{
    const char* stage;
    std::size_t processed;
    std::size_t total;
    fs::path file;
};

struct BackupConfig
{
    fs::path sourceDir;
    fs::path backupRoot;
    fs::path databaseFile;

    bool verbose = false;

    std::function<void(const BackupProgress&)> onProgress;
};

bool RunBackup(const BackupConfig& cfg);
