#pragma once

#include "BackupUtility/BackupUtility.hpp"
#include "FileStateRepository.hpp"
#include "FileHasher/FileHasher.hpp"
#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"
#include "TimestampProvider/TimestampProvider.hpp"

#include <atomic>
#include <filesystem>
#include <functional>

/**
 * @brief Application component for processing a single file during backup.
 */
class ProcessBackupFile
{
  public:
    /**
     * @brief Construct a processor for individual backup files.
     *
     * @param[in] sourceRoot Root path of the source directory
     * @param[in] backupRoot Root path of the backup directory
     * @param[in] snapshotDirectory Provider for snapshot directories
     * @param[in] fileStateRepository Repository for file state tracking
     * @param[in] fileHasher File hashing utility
     * @param[in] timestampProvider Timestamp provider
     * @param[in] onProgress Progress callback
     * @param[in/out] success Shared success flag for the operation
     * @param[in/out] processedCount Shared processed counter
     */
    ProcessBackupFile(const std::filesystem::path& sourceRoot, const std::filesystem::path& backupRoot,
              SnapshotDirectoryProvider& snapshotDirectory,
                      FileStateRepository& fileStateRepository, const FileHasher& fileHasher,
                      const TimestampProvider& timestampProvider, const std::function<void(const BackupProgress&)>& onProgress,
                      std::atomic<bool>& success, std::atomic<std::size_t>& processedCount);

    /**
     * @brief Process a single file for backup and state tracking.
     *
     * @param[in] file File path to process
     */
    void Execute(const std::filesystem::path& file);

  private:
    const std::filesystem::path& _sourceRoot;
    const std::filesystem::path& _backupRoot;
    SnapshotDirectoryProvider& _snapshotDirectory;
    FileStateRepository& _fileStateRepository;
    const FileHasher& _fileHasher;
    const TimestampProvider& _timestampProvider;
    std::function<void(const BackupProgress&)> _onProgress;
    std::atomic<bool>& _success;
    std::atomic<std::size_t>& _processedCount;
};
