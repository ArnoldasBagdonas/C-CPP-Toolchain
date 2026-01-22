// file BackupUtility.cpp
#include "BackupUtility/BackupUtility.hpp"

#include "BackupUtility/TimestampProvider.hpp"
#include "BackupUtility/FileStateRepository.hpp"
#include "BackupUtility/FilesystemFileEnumerator.hpp"
#include "BackupUtility/FilesystemSnapshotDirectory.hpp"
#include "BackupUtility/ProcessBackupFile.hpp"
#include "BackupUtility/ProcessDeletedFiles.hpp"
#include "BackupUtility/SQLiteSession.hpp"
#include "BackupUtility/SnapshotDirectoryProvider.hpp"
#include "BackupUtility/ThreadedFileQueue.hpp"
#include "BackupUtility/FileHasher.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>

namespace fs = std::filesystem;

namespace
{
constexpr unsigned int MaxQueueSizeMultiplier = 4;
}

/** Run backup with multithreaded file processing */
bool RunBackup(const BackupConfig& config)
{
    std::atomic<bool> success{true};
    std::error_code ec;

    fs::path sourceRoot = fs::is_regular_file(config.sourceDir, ec) ? config.sourceDir.parent_path() : config.sourceDir;
    if (ec || !fs::exists(sourceRoot))
    {
        return false;
    }

    fs::path backupRoot = config.backupRoot / "backup";
    fs::path historyRoot = config.backupRoot / "deleted";
    fs::create_directories(backupRoot, ec);
    fs::create_directories(historyRoot, ec);

    SQLiteSession databaseSession(config.databaseFile);
    FileStateRepository fileStateRepository(databaseSession);
    if (false == fileStateRepository.InitializeSchema())
    {
        return false;
    }

    std::mutex progressMutex;
    auto threadSafeProgress = [&](const BackupProgress& prog)
    {
        if (nullptr == config.onProgress)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(progressMutex);
        config.onProgress(prog);
    };

    std::atomic<std::size_t> processedCount{0};

    TimestampProvider timestampProvider;
    FilesystemSnapshotDirectory snapshotDirectory(timestampProvider);
    SnapshotDirectoryProvider snapshotOnce(historyRoot, snapshotDirectory);
    FileHasher fileHasher;

    ProcessBackupFile processBackupFile(sourceRoot, backupRoot, snapshotOnce, fileStateRepository, fileHasher, timestampProvider,
                                        threadSafeProgress, success, processedCount);

    unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
    const std::size_t maxQueueSize = threadCount * MaxQueueSizeMultiplier;

    ThreadedFileQueue fileQueue(threadCount, maxQueueSize, [&](const fs::path& file) { processBackupFile.Execute(file); });

    FilesystemFileEnumerator enumerator;
    enumerator.Enumerate(config.sourceDir, [&](const fs::path& file) { fileQueue.Enqueue(file); });

    fileQueue.Finalize();

    if (true == success.load())
    {
        ProcessDeletedFiles processDeletedFiles(sourceRoot, backupRoot, snapshotOnce, fileStateRepository, timestampProvider,
                                                threadSafeProgress);
        success.store(processDeletedFiles.Execute());
    }

    return success.load();
}