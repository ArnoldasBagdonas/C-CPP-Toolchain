#pragma once

#include "BackupUtility/FilesystemSnapshotDirectory.hpp"

#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component that creates a single snapshot directory once.
 */
class SnapshotDirectoryProvider
{
  public:
    SnapshotDirectoryProvider(const fs::path& historyRootPath, const FilesystemSnapshotDirectory& snapshotDirectory);

    fs::path GetOrCreate();

  private:
    fs::path _historyRootPath;
    const FilesystemSnapshotDirectory& _snapshotDirectory;
    std::once_flag _snapshotFlag;
    fs::path _snapshotPath;
};
