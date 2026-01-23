#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"

#include <utility>

SnapshotDirectoryProvider::SnapshotDirectoryProvider(const std::filesystem::path& historyRootPath,
                                                     const TimestampProvider& timestampProvider)
    : _historyRootPath(historyRootPath), _timestampProvider(timestampProvider)
{
}

std::filesystem::path SnapshotDirectoryProvider::GetOrCreate()
{
    std::call_once(_snapshotFlag, [&]() {
        _snapshotPath = _historyRootPath / _timestampProvider.NowFilesystemSafe();
        std::filesystem::create_directories(_snapshotPath);
    });
    return _snapshotPath;
}
