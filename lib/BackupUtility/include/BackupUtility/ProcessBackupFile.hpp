#pragma once

#include "BackupUtility/BackupUtility.hpp"
#include "BackupUtility/TimestampProvider.hpp"
#include "BackupUtility/FileStateRepository.hpp"
#include "BackupUtility/SnapshotDirectoryProvider.hpp"
#include "BackupUtility/FileHasher.hpp"

#include <atomic>
#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief Application component for processing a single file during backup.
 */
class ProcessBackupFile
{
  public:
    ProcessBackupFile(const fs::path& sourceRoot, const fs::path& backupRoot, SnapshotDirectoryProvider& snapshotDirectory,
              FileStateRepository& fileStateRepository, const FileHasher& fileHasher,
              const TimestampProvider& timestampProvider, const std::function<void(const BackupProgress&)>& onProgress,
                      std::atomic<bool>& success, std::atomic<std::size_t>& processedCount);

    void Execute(const fs::path& file);

  private:
    const fs::path& _sourceRoot;
    const fs::path& _backupRoot;
    SnapshotDirectoryProvider& _snapshotDirectory;
    FileStateRepository& _fileStateRepository;
    const FileHasher& _fileHasher;
    const TimestampProvider& _timestampProvider;
    std::function<void(const BackupProgress&)> _onProgress;
    std::atomic<bool>& _success;
    std::atomic<std::size_t>& _processedCount;
};
