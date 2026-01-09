// file BackupUtility.cpp
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

/** Run backup with multithreaded file processing */
bool RunBackup(const BackupConfig& config)
{
    std::atomic<bool> success{true};
    std::error_code ec;

    // Determine source root
    fs::path sourceRoot = fs::is_regular_file(config.sourceDir) ? config.sourceDir.parent_path() : config.sourceDir;
    if (!fs::exists(sourceRoot))
        return false;

    fs::path backupRoot = config.backupRoot / "backup";
    fs::path historyRoot = config.backupRoot / "deleted";
    fs::create_directories(backupRoot, ec);
    fs::create_directories(historyRoot, ec);

    // Snapshot creation (once)
    std::once_flag snapshotFlag;
    fs::path snapshotPath;
    auto CreateSnapshotOnce = [&]() -> fs::path
    {
        std::call_once(snapshotFlag, [&]() { snapshotPath = CreateSnapshotDirectory(historyRoot); });
        return snapshotPath;
    };

    // Initialize database
    SQLiteSession database(config.databaseFile);
    if (!database.InitializeSchema())
        return false;

    // Thread-safe progress callback
    std::mutex progressMutex;
    auto threadSafeProgress = [&](const BackupProgress& prog)
    {
        if (!config.onProgress)
            return;
        std::lock_guard lock(progressMutex);
        config.onProgress(prog);
    };

    // Queue for files (bounded)
    std::atomic<std::size_t> processedCount{0};
    std::mutex queueMutex;
    std::condition_variable queueCV;
    std::queue<fs::path> fileQueue;
    bool doneWalking = false;

    unsigned int threadCount = std::max(1u, std::thread::hardware_concurrency());
    const std::size_t maxQueueSize = threadCount * 4; // bounded size

    // Helper: process a single file
    auto ProcessFile = [&](const fs::path& file)
    {
        fs::path relativePath = fs::relative(file, sourceRoot, ec);
        if (ec)
        {
            success.store(false);
            return;
        }
        if (relativePath == ".")
            relativePath = file.filename();

        fs::path backupFile = backupRoot / relativePath;
        std::string newHash;
        if (!ComputeFileHash(file, newHash))
        {
            success.store(false);
            return;
        }

        std::string storedHash, storedTimestamp;
        ChangeType storedStatus;
        bool hasRecord = database.GetFileState(relativePath.string(), storedHash, storedStatus, storedTimestamp) && storedStatus != ChangeType::Deleted;

        bool changed = (!hasRecord) || (newHash != storedHash);
        ChangeType newStatus = ChangeType::Unchanged;
        std::string timestampValue = storedTimestamp;

        if (!hasRecord)
        {
            newStatus = ChangeType::Added;
            timestampValue = GetCurrentTimestamp();
            fs::create_directories(backupFile.parent_path(), ec);
            fs::copy_file(file, backupFile, fs::copy_options::overwrite_existing, ec);
        }
        else if (changed)
        {
            newStatus = ChangeType::Modified;
            timestampValue = GetCurrentTimestamp();
            fs::path snapshotFile = CreateSnapshotOnce() / relativePath;
            fs::create_directories(snapshotFile.parent_path(), ec);
            fs::copy_file(backupFile, snapshotFile, fs::copy_options::overwrite_existing, ec);
            fs::copy_file(file, backupFile, fs::copy_options::overwrite_existing, ec);
        }

        if (!database.UpdateFileState(relativePath.string(), newHash, newStatus, timestampValue))
            success.store(false);

        threadSafeProgress({"collecting", ++processedCount, 0, file});
    };

    // Worker thread: consume files from the queue
    auto WorkerThread = [&]()
    {
        while (true)
        {
            fs::path file;
            // Lock scope
            {
                std::unique_lock lock(queueMutex);

                // Wait until there is work or we are done
                queueCV.wait(lock, [&]() { return doneWalking || !fileQueue.empty(); });

                if (fileQueue.empty())
                    return;

                // Dequeue file for processing
                file = std::move(fileQueue.front());
                fileQueue.pop();

                // Notify producer in case it was waiting for space in the queue
                queueCV.notify_all();
            }

            // Process the file outside the lock
            ProcessFile(file);
        }
    };

    // Start worker threads
    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < threadCount; ++i)
        workers.emplace_back(WorkerThread);

    
    auto EnqueueFile = [&](const fs::path& file)
    {
        std::unique_lock lock(queueMutex);
        queueCV.wait(lock, [&]() { return fileQueue.size() < maxQueueSize; });
        fileQueue.push(file);
        queueCV.notify_all(); // notify workers
    };
    
    // Enqueue initial files
    const fs::path path = config.sourceDir;
    if (fs::is_regular_file(path))
    {
        EnqueueFile(path);
    }
    else if (fs::is_directory(path))
    {
        for (auto& entry : fs::recursive_directory_iterator(path, ec))
        {
            if (!ec && entry.is_regular_file())
                EnqueueFile(entry.path());
        }
    }

    // Signal done
    {
        std::lock_guard lock(queueMutex);
        doneWalking = true;
    }
    queueCV.notify_all();

    // Join threads
    for (auto& t : workers)
        t.join();

    // Handle deleted files
    if (success.load())
        success.store(DetectDeletedFiles(sourceRoot, backupRoot, CreateSnapshotOnce, database, threadSafeProgress));

    return success.load();
}
