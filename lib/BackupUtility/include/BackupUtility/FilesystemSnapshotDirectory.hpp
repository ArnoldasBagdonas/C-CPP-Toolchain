#pragma once

#include "BackupUtility/TimestampProvider.hpp"

#include <filesystem>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component creating snapshot directories on the filesystem.
 */
class FilesystemSnapshotDirectory
{
  public:
    explicit FilesystemSnapshotDirectory(const TimestampProvider& timestampProvider);

    fs::path Create(const fs::path& historyRootPath) const;

  private:
    const TimestampProvider& _timestampProvider;
};
