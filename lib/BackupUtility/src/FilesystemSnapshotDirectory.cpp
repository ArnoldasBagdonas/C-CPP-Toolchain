#include "BackupUtility/FilesystemSnapshotDirectory.hpp"

#include <filesystem>

FilesystemSnapshotDirectory::FilesystemSnapshotDirectory(const TimestampProvider& timestampProvider)
    : _timestampProvider(timestampProvider)
{
}

fs::path FilesystemSnapshotDirectory::Create(const fs::path& historyRootPath) const
{
    fs::path snapshotDirectory = historyRootPath / _timestampProvider.NowFilesystemSafe();
    fs::create_directories(snapshotDirectory);
    return snapshotDirectory;
}
