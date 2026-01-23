#pragma once

#include "BackupUtility/BackupUtility.hpp"
#include "FileStateRepository.hpp"
#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"
#include "TimestampProvider/TimestampProvider.hpp"

#include <filesystem>
#include <functional>

/**
 * @brief Application component for handling files deleted from the source directory.
 */
class ProcessDeletedFiles
{
  public:
    /**
     * @brief Construct a processor for deleted files.
     *
     * @param[in] sourceFolderPath Source root for file existence checks
     * @param[in] backupFolderPath Backup root for current file versions
     * @param[in] snapshotDirectory Provider for snapshot directories
     * @param[in] fileStateRepository Repository for file state tracking
     * @param[in] timestampProvider Timestamp provider
     * @param[in] onProgress Progress callback
     */
    ProcessDeletedFiles(const std::filesystem::path& sourceFolderPath, const std::filesystem::path& backupFolderPath,
              SnapshotDirectoryProvider& snapshotDirectory,
                        FileStateRepository& fileStateRepository, const TimestampProvider& timestampProvider,
                        const std::function<void(const BackupProgress&)>& onProgress);

    /**
     * @brief Process files that no longer exist in the source directory.
     *
     * @return true on success, false on error
     */
    bool Execute();

  private:
    const std::filesystem::path& _sourceFolderPath;
    const std::filesystem::path& _backupFolderPath;
    SnapshotDirectoryProvider& _snapshotDirectory;
    FileStateRepository& _fileStateRepository;
    const TimestampProvider& _timestampProvider;
    std::function<void(const BackupProgress&)> _onProgress;
};
