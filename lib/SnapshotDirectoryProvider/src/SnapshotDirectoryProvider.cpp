#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"

#include <utility>

SnapshotDirectoryProvider::SnapshotDirectoryProvider(const fs::path& historyRootPath, const TimestampProvider& timestampProvider)
    : _historyRootPath(historyRootPath), _timestampProvider(timestampProvider)
{
}

fs::path SnapshotDirectoryProvider::GetOrCreate()
{
    std::call_once(_snapshotFlag, [&]() {
        _snapshotPath = _historyRootPath / _timestampProvider.NowFilesystemSafe();
        fs::create_directories(_snapshotPath);
    });
    return _snapshotPath;
}
