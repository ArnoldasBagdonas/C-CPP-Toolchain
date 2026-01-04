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

    bool dryRun  = false;
    bool verbose = false;

    std::function<void(const BackupProgress&)> onProgress;
};

bool RunBackup(const BackupConfig& cfg);
