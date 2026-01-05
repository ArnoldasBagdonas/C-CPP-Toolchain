// file BackupUtility.cpp:

#include "BackupUtility/BackupUtility.hpp"
#include "BackupUtility/SQLiteSession.hpp"

#include <atomic>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>

#include <sqlite3.h>
#include <xxhash.h>

namespace fs = std::filesystem;

/**
 * @brief Compute XXH64 hash of a file's content.
 *
 * Reads file in chunks and computes hash using xxHash algorithm.
 * Resources are properly released on both success and failure paths.
 *
 * @param[in] filePath Path to file to hash
 * @param[out] outputHash Hexadecimal string representation of computed hash
 * @return true if hash computed successfully, false if file cannot be opened or hash state allocation fails
 */
static bool ComputeFileHash(const fs::path& filePath, std::string& outputHash)
{
    std::ifstream inputStream(filePath, std::ios::binary);
    if (!inputStream)
    {
        return false;
    }

    XXH64_state_t* hashState = XXH64_createState();
    if (nullptr == hashState)
    {
        return false;
    }

    XXH64_reset(hashState, 0);

    char buffer[8192];
    while (inputStream.read(buffer, sizeof(buffer)))
    {
        XXH64_update(hashState, buffer, sizeof(buffer));
    }

    if (inputStream.gcount() > 0)
    {
        XXH64_update(hashState, buffer, inputStream.gcount());
    }

    uint64_t hashValue = XXH64_digest(hashState);
    XXH64_freeState(hashState);

    std::ostringstream outputStream;
    outputStream << std::hex << hashValue;
    outputHash = outputStream.str();
    return true;
}

/**
 * @brief Generate current timestamp string in filesystem-safe format.
 *
 * Format: YYYY-MM-DD_HH-MM-SS
 * Uses platform-specific thread-safe time conversion functions.
 *
 * @return Timestamp string
 */
static std::string GetCurrentTimestamp()
{
    std::time_t currentTime = std::time(nullptr);
    std::tm timeStruct{};

#ifdef _WIN32
    _localtime64_s(&timeStruct, &currentTime);
#else
    localtime_r(&currentTime, &timeStruct);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &timeStruct);
    return buffer;
}

/**
 * @brief Create a timestamped snapshot directory.
 *
 * Creates a subdirectory under the history root using current timestamp as name.
 *
 * @param[in] historyRootPath Root directory for snapshots
 * @return Path to newly created snapshot directory
 */
static fs::path CreateSnapshotDirectory(const fs::path& historyRootPath)
{
    fs::path snapshotDirectory = historyRootPath / GetCurrentTimestamp();
    fs::create_directories(snapshotDirectory);
    return snapshotDirectory;
}

/**
 * @brief Process a single file for backup.
 *
 * Computes file hash, compares with stored state, archives previous version if changed,
 * and updates the current backup directory and database.
 *
 * @param[in] filePath Absolute path to source file
 * @param[in] sourceFolderPath Root directory of source files
 * @param[in] backupFolderPath Current backup directory path
 * @param[in] createSnapshotPathOnce Lazy initialization function for snapshot creation
 * @param[in] databaseSession SQLite session for database operations
 * @param[in] onProgressCallback Optional progress callback function
 * @param[in/out] processedCount Counter for processed files
 * @return true if file processed successfully, false otherwise
 */
static bool ProcessFile(const fs::path& filePath, const fs::path& sourceFolderPath, const fs::path& backupFolderPath,
                        std::function<fs::path()> createSnapshotPathOnce, SQLiteSession& databaseSession,
                        std::function<void(const BackupProgress&)> onProgressCallback, std::atomic<std::size_t>& processedCount)
{
    std::error_code errorCode;
    fs::path relativeFilePath = fs::relative(filePath, sourceFolderPath, errorCode);
    if (errorCode)
    {
        return false;
    }

    // Handle single-file-in-root case
    if ("." == relativeFilePath)
    {
        relativeFilePath = filePath.filename();
    }

    fs::path currentFilePath = backupFolderPath / relativeFilePath;

    std::string newHash;
    if (!ComputeFileHash(filePath, newHash))
    {
        return false;
    }

    std::string storedHash;
    ChangeType storedStatus;
    std::string storedTimestamp;
    bool hadPreviousRecord =
        databaseSession.GetFileState(relativeFilePath.string(), storedHash, storedStatus, storedTimestamp) && (ChangeType::Deleted != storedStatus);
    bool fileChanged = ((!hadPreviousRecord) || (newHash != storedHash));

    ChangeType newStatus;
    if (!hadPreviousRecord)
    {
        newStatus = ChangeType::Added;
        fs::create_directories(currentFilePath.parent_path(), errorCode);
        fs::copy_file(filePath, currentFilePath, fs::copy_options::overwrite_existing, errorCode);
    }
    else if (fileChanged)
    {
        newStatus = ChangeType::Modified;
        auto snapshotPath = createSnapshotPathOnce();
        fs::path archivedPath = snapshotPath / relativeFilePath;
        fs::create_directories(archivedPath.parent_path(), errorCode);
        fs::copy_file(currentFilePath, archivedPath, fs::copy_options::overwrite_existing, errorCode);
        fs::copy_file(filePath, currentFilePath, fs::copy_options::overwrite_existing, errorCode);
    }
    else
    {
        newStatus = ChangeType::Unchanged;
    }

    std::string timestampValue;
    if (ChangeType::Unchanged != newStatus)
    {
        timestampValue = GetCurrentTimestamp();
    }
    else
    {
        timestampValue = storedTimestamp;
    }

    if (!databaseSession.UpdateFileState(relativeFilePath.string(), newHash, newStatus, timestampValue))
    {
        return false;
    }

    if (nullptr != onProgressCallback)
    {
        onProgressCallback({"collecting", ++processedCount, 0, filePath});
    }

    return true;
}

/**
 * @brief Detect and process files that have been deleted from source.
 *
 * Scans database for tracked files, checks if they still exist in source,
 * archives deleted files to snapshot, and updates database status.
 *
 * @param[in] sourceFolderPath Source directory path
 * @param[in] backupFolderPath Current backup directory path
 * @param[in] historyFolderPath History root directory for snapshots
 * @param[in] createSnapshotPathOnce Lazy initialization function for snapshot creation
 * @param[in] databaseSession SQLite session for database operations
 * @param[in] onProgressCallback Optional progress callback function
 * @return true if operation succeeded, false otherwise
 */
static bool DetectDeletedFiles(const fs::path& sourceFolderPath, const fs::path& backupFolderPath, std::function<fs::path()> createSnapshotPathOnce,
                               SQLiteSession& databaseSession, std::function<void(const BackupProgress&)> onProgressCallback)
{
    std::error_code errorCode;

    sqlite3_stmt* statement = nullptr;
    if (!databaseSession.GetAllFiles(&statement))
    {
        return false;
    }

    bool operationSucceeded = true;
    while (SQLITE_ROW == sqlite3_step(statement))
    {
        const char* databasePathCString = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
        const char* statusCString = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));

        if ((nullptr == databasePathCString) || (nullptr == statusCString))
        {
            continue;
        }

        std::string databasePath = databasePathCString;
        ChangeType fileStatus = StringToChangeType(statusCString);

        if (ChangeType::Deleted == fileStatus)
        {
            continue;
        }

        fs::path sourceFilePath = sourceFolderPath / databasePath;
        fs::path currentFilePath = backupFolderPath / databasePath;

        if (!fs::exists(sourceFilePath, errorCode))
        {
            auto snapshotPath = createSnapshotPathOnce();

            if (fs::exists(currentFilePath, errorCode))
            {
                fs::path archivedPath = snapshotPath / databasePath;
                fs::create_directories(archivedPath.parent_path(), errorCode);
                fs::copy_file(currentFilePath, archivedPath, fs::copy_options::overwrite_existing, errorCode);
            }

            fs::remove(currentFilePath, errorCode);

            if (!databaseSession.MarkFileAsDeleted(databasePath, GetCurrentTimestamp()))
            {
                operationSucceeded = false;
                break;
            }

            if (nullptr != onProgressCallback)
            {
                onProgressCallback({"deleted", 0, 0, databasePath});
            }
        }
    }

    sqlite3_finalize(statement);
    return operationSucceeded;
}

bool RunBackup(const BackupConfig& configuration)
{
    bool operationSucceeded = true;
    std::error_code errorCode;

    const fs::path sourceFilePath = configuration.sourceDir;
    const fs::path sourceFolderPath = fs::is_regular_file(configuration.sourceDir) ? configuration.sourceDir.parent_path() : configuration.sourceDir;

    if (!fs::exists(sourceFolderPath))
    {
        return false;
    }

    const fs::path backupFolderPath = configuration.backupRoot / "backup";
    const fs::path historyFolderPath = configuration.backupRoot / "deleted";

    fs::create_directories(backupFolderPath, errorCode);
    fs::create_directories(historyFolderPath, errorCode);

    std::once_flag snapshotOnceFlag;
    fs::path snapshotPath;

    // Lazy initialization pattern for snapshot creation
    auto CreateSnapshotOnce = [&]() -> fs::path
    {
        std::call_once(snapshotOnceFlag, [&]() { snapshotPath = CreateSnapshotDirectory(historyFolderPath); });
        return snapshotPath;
    };

    // Thread-safe SQLite wrapper
    SQLiteSession databaseSession(configuration.databaseFile);

    if (!databaseSession.InitializeSchema())
    {
        return false;
    }

    // Progress callback thread-safety
    std::mutex progressMutex;
    auto threadSafeProgressCallback = [&](const BackupProgress& progress)
    {
        if (!configuration.onProgress)
        {
            return;
        }
        std::lock_guard lock(progressMutex);
        configuration.onProgress(progress);
    };

    std::atomic<std::size_t> processedCount{0};
    std::mutex queueMutex;
    std::condition_variable queueCondition;
    std::queue<fs::path> fileQueue;
    bool doneWalking = false;

    // Thread pool
    std::vector<std::thread> workerThreads;
    unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());

    for (unsigned int threadIndex = 0; threadIndex < threadCount; ++threadIndex)
    {
        workerThreads.emplace_back(
            [&]()
            {
                while (true)
                {
                    fs::path currentFile;
                    {
                        std::unique_lock lock(queueMutex);
                        queueCondition.wait(lock, [&]() { return ((!fileQueue.empty()) || (doneWalking)); });

                        if (fileQueue.empty())
                        {
                            if (doneWalking)
                            {
                                return;
                            }
                            continue;
                        }

                        currentFile = fileQueue.front();
                        fileQueue.pop();
                    }

                    ProcessFile(currentFile, sourceFolderPath, backupFolderPath, CreateSnapshotOnce, databaseSession, threadSafeProgressCallback,
                                processedCount);
                }
            });
    }

    // Enqueue files lazily, streaming directly to threads
    auto EnqueueFile = [&](const fs::path& file)
    {
        {
            std::lock_guard lock(queueMutex);
            fileQueue.push(file);
        }
        queueCondition.notify_one();
    };

    // Stream files from directory or single file
    if (fs::is_directory(sourceFilePath))
    {
        for (auto& entry : fs::recursive_directory_iterator(sourceFolderPath, errorCode))
        {
            if ((!errorCode) && (entry.is_regular_file()))
            {
                EnqueueFile(entry.path());
            }
        }
    }
    else if (fs::is_regular_file(sourceFilePath))
    {
        EnqueueFile(sourceFilePath);
    }
    else
    {
        operationSucceeded = false;
    }

    // Signal threads no more work
    {
        std::lock_guard lock(queueMutex);
        doneWalking = true;
    }
    queueCondition.notify_all();

    // Wait for threads
    for (auto& workerThread : workerThreads)
    {
        workerThread.join();
    }

    if (operationSucceeded)
    {
        if (!DetectDeletedFiles(sourceFolderPath, backupFolderPath, CreateSnapshotOnce, databaseSession, threadSafeProgressCallback))
        {
            operationSucceeded = false;
        }
    }

    return operationSucceeded;
}
