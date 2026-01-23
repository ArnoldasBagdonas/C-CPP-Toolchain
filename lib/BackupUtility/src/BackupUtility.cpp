// file BackupUtility.cpp
#include "BackupUtility/BackupUtility.hpp"

#include "FileStateRepository.hpp"
#include "ProcessBackupFile.hpp"
#include "ProcessDeletedFiles.hpp"

#include "FileHasher/FileHasher.hpp"
#include "FileIterator/FileIterator.hpp"
#include "SnapshotDirectoryProvider/SnapshotDirectoryProvider.hpp"
#include "SQLiteSession/SQLiteSession.hpp"
#include "ThreadedFileQueue/ThreadedFileQueue.hpp"
#include "TimestampProvider/TimestampProvider.hpp"

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
constexpr unsigned int MinWorkerThreadCount = 1;
}

bool RunBackup(const BackupConfig& config)
{
    std::atomic<bool> success{true};
    std::error_code ec;

    fs::path sourceRoot = fs::is_regular_file(config.sourceDir, ec) ? config.sourceDir.parent_path() : config.sourceDir;
    if ((0 != ec.value()) || (false == fs::exists(sourceRoot)))
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
    SnapshotDirectoryProvider snapshotOnce(historyRoot, timestampProvider);
    FileHasher fileHasher;

    ProcessBackupFile processBackupFile(sourceRoot, backupRoot, snapshotOnce, fileStateRepository, fileHasher, timestampProvider,
                                        threadSafeProgress, success, processedCount);

    unsigned int threadCount = std::max(MinWorkerThreadCount, std::thread::hardware_concurrency());
    const std::size_t maxQueueSize = threadCount * MaxQueueSizeMultiplier;

    ThreadedFileQueue fileQueue(threadCount, maxQueueSize, [&](const fs::path& file) { processBackupFile.Execute(file); });

    FileIterator iterator;
    iterator.Iterate(config.sourceDir, [&](const fs::path& file) { fileQueue.Enqueue(file); });

    fileQueue.Finalize();

    if (true == success.load())
    {
        ProcessDeletedFiles processDeletedFiles(sourceRoot, backupRoot, snapshotOnce, fileStateRepository, timestampProvider,
                                                threadSafeProgress);
        success.store(processDeletedFiles.Execute());
    }

    return success.load();
}