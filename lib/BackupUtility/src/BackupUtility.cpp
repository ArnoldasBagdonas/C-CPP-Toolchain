// file BackupUtility.cpp
#include "BackupUtility/BackupUtility.hpp"

#include "FileStateRepository.hpp"
#include "ProcessBackupFile.hpp"
#include "ProcessDeletedFiles.hpp"

#include "FileHasher/FileHasher.hpp"
#include "FileIterator/FileIterator.hpp"
#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"
#include "SQLite/SQLiteSession.hpp"
#include "ThreadedFileQueue/ThreadedFileQueue.hpp"
#include "TimestampProvider/TimestampProvider.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>

namespace
{
constexpr unsigned int MaxQueueSizeMultiplier = 4;
constexpr unsigned int MinWorkerThreadCount = 1;
}

bool RunBackup(const BackupConfig& config)
{
    std::atomic<bool> success{true};
    std::error_code ec;

    std::filesystem::path sourceRoot = std::filesystem::is_regular_file(config.sourceDir, ec) ? config.sourceDir.parent_path()
                                                                                               : config.sourceDir;
    if ((0 != ec.value()) || (false == std::filesystem::exists(sourceRoot)))
    {
        return false;
    }

    std::filesystem::path backupRoot = config.backupRoot / "backup";
    std::filesystem::path historyRoot = config.backupRoot / "deleted";
    std::filesystem::create_directories(backupRoot, ec);
    std::filesystem::create_directories(historyRoot, ec);

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
    SnapshotDirectoryProvider snapshotOnce(historyRoot, timestampProvider);
    FileHasher fileHasher;

    ProcessBackupFile processBackupFile(sourceRoot, backupRoot, snapshotOnce, fileStateRepository, fileHasher, timestampProvider,
                                        threadSafeProgress, success, processedCount);

    unsigned int threadCount = std::max(MinWorkerThreadCount, std::thread::hardware_concurrency());
    const std::size_t maxQueueSize = threadCount * MaxQueueSizeMultiplier;

    ThreadedFileQueue fileQueue(threadCount, maxQueueSize,
                                [&](const std::filesystem::path& file) { processBackupFile.Execute(file); });

    FileIterator iterator;
    iterator.Iterate(config.sourceDir, [&](const std::filesystem::path& file) { fileQueue.Enqueue(file); });

    fileQueue.Finalize();

    if (true == success.load())
    {
        ProcessDeletedFiles processDeletedFiles(sourceRoot, backupRoot, snapshotOnce, fileStateRepository, timestampProvider,
                                                threadSafeProgress);
        success.store(processDeletedFiles.Execute());
    }

    return success.load();
}