#pragma once

#include "TimestampProvider/TimestampProvider.hpp"

#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;

/**
 * @brief Infrastructure component that creates a single snapshot directory once.
 */
class SnapshotDirectoryProvider
{
  public:
    SnapshotDirectoryProvider(const fs::path& historyRootPath, const TimestampProvider& timestampProvider);

    fs::path GetOrCreate();

  private:
    fs::path _historyRootPath;
    const TimestampProvider& _timestampProvider;
    std::once_flag _snapshotFlag;
    fs::path _snapshotPath;
};
