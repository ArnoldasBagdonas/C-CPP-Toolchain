#include "BackupUtility/SnapshotDirectoryProvider.hpp"

#include <utility>

SnapshotDirectoryProvider::SnapshotDirectoryProvider(const fs::path& historyRootPath, const FilesystemSnapshotDirectory& snapshotDirectory)
    : _historyRootPath(historyRootPath), _snapshotDirectory(snapshotDirectory)
{
}

fs::path SnapshotDirectoryProvider::GetOrCreate()
{
    std::call_once(_snapshotFlag, [&]() { _snapshotPath = _snapshotDirectory.Create(_historyRootPath); });
    return _snapshotPath;
}
