#pragma once

#include "TimestampProvider/TimestampProvider.hpp"

#include <filesystem>
#include <mutex>

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
    SnapshotDirectoryProvider(const std::filesystem::path& historyRootPath, const TimestampProvider& timestampProvider);

    /**
     * @brief Get or create the snapshot directory.
     *
     * @return Snapshot directory path
     */
    std::filesystem::path GetOrCreate();

  private:
    std::filesystem::path _historyRootPath;
    const TimestampProvider& _timestampProvider;
    std::once_flag _snapshotFlag;
    std::filesystem::path _snapshotPath;
};
