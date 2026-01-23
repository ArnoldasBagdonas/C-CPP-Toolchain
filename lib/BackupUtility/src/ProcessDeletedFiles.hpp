#pragma once

#include "BackupUtility/BackupUtility.hpp"
#include "FileStateRepository.hpp"
#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"
#include "TimestampProvider/TimestampProvider.hpp"

#include <filesystem>
#include <functional>

namespace fs = std::filesystem;

/**
 * @brief Application component for handling files deleted from the source directory.
 */
class ProcessDeletedFiles
{
  public:
    ProcessDeletedFiles(const fs::path& sourceFolderPath, const fs::path& backupFolderPath, SnapshotDirectoryProvider& snapshotDirectory,
                        FileStateRepository& fileStateRepository, const TimestampProvider& timestampProvider,
                        const std::function<void(const BackupProgress&)>& onProgress);

    bool Execute();

  private:
    const fs::path& _sourceFolderPath;
    const fs::path& _backupFolderPath;
    SnapshotDirectoryProvider& _snapshotDirectory;
    FileStateRepository& _fileStateRepository;
    const TimestampProvider& _timestampProvider;
    std::function<void(const BackupProgress&)> _onProgress;
};
