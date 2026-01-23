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
    /**
     * @brief Construct a snapshot directory provider.
     *
     * @param[in] historyRootPath Root path for snapshot history
     * @param[in] timestampProvider Timestamp provider
     */
    SnapshotDirectoryProvider(const fs::path& historyRootPath, const TimestampProvider& timestampProvider);

    /**
     * @brief Get or create the snapshot directory.
     *
     * @return Snapshot directory path
     */
    fs::path GetOrCreate();

  private:
    fs::path _historyRootPath;
    const TimestampProvider& _timestampProvider;
    std::once_flag _snapshotFlag;
    fs::path _snapshotPath;
};
