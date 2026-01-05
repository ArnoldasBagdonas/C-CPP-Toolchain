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
 * @brief Open a SQLite database connection.
 *
 * @param[in] databasePath Path to the database file
 * @param[out] database Pointer to receive the database handle
 * @return true if database opened successfully, false otherwise
 */
static bool OpenDatabase(const fs::path& databasePath, sqlite3** database)
{
    if (nullptr == database)
    {
        return false;
    }

    return (SQLITE_OK == sqlite3_open(databasePath.string().c_str(), database));
}

/**
 * @brief Close a SQLite database connection and release resources.
 *
 * @param[in] database Database handle to close
 */
static void CloseDatabase(sqlite3* database)
{
    if (nullptr != database)
    {
        sqlite3_close(database);
    }
}

/**
 * @brief Initialize the database schema for file tracking.
 *
 * Creates the files table if it does not exist, with columns for path,
 * hash, last_updated timestamp, and status.
 *
 * @param[in] database Database handle
 * @return true if schema initialized successfully, false otherwise
 */
static bool InitializeSchema(sqlite3* database)
{
    if (nullptr == database)
    {
        return false;
    }

    static const char* SQL_CREATE_TABLE = "CREATE TABLE IF NOT EXISTS files ("
                                          "path TEXT PRIMARY KEY,"
                                          "hash TEXT NOT NULL,"
                                          "last_updated TEXT NOT NULL,"
                                          "status TEXT NOT NULL);";

    return (SQLITE_OK == sqlite3_exec(database, SQL_CREATE_TABLE, nullptr, nullptr, nullptr));
}

/**
 * @brief Update or insert file state information in the database.
 *
 * Uses INSERT with ON CONFLICT to update existing records or create new ones.
 *
 * @param[in] database Database handle
 * @param[in] filePath Relative path of the file
 * @param[in] fileHash Hash value of the file content
 * @param[in] changeStatus Current change status of the file
 * @param[in] timestamp Last update timestamp
 * @return true if operation succeeded, false otherwise
 */
static bool UpdateStoredFileState(sqlite3* database, const std::string& filePath, const std::string& fileHash, ChangeType changeStatus,
                                  const std::string& timestamp)
{
    if (nullptr == database)
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;

    if (SQLITE_OK != sqlite3_prepare_v2(database,
                                        "INSERT INTO files(path, hash, status, last_updated) "
                                        "VALUES(?1, ?2, ?3, ?4) "
                                        "ON CONFLICT(path) DO UPDATE SET "
                                        "hash=excluded.hash, status=excluded.status, last_updated=excluded.last_updated;",
                                        -1, &statement, nullptr))
    {
        return false;
    }

    sqlite3_bind_text(statement, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, fileHash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, ChangeTypeToString(changeStatus), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, timestamp.c_str(), -1, SQLITE_TRANSIENT);

    bool operationSucceeded = (SQLITE_DONE == sqlite3_step(statement));
    sqlite3_finalize(statement);
    return operationSucceeded;
}

/**
 * @brief Retrieve stored file state information from the database.
 *
 * @param[in] database Database handle
 * @param[in] filePath Relative path of the file to query
 * @param[out] outputHash Retrieved hash value
 * @param[out] outputStatus Retrieved change status
 * @param[out] outputTimestamp Retrieved last update timestamp
 * @return true if file record was found, false otherwise
 */
static bool GetStoredFileState(sqlite3* database, const std::string& filePath, std::string& outputHash, ChangeType& outputStatus, std::string& outputTimestamp)
{
    if (nullptr == database)
    {
        return false;
    }

    sqlite3_stmt* statement = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(database, "SELECT hash, status, last_updated FROM files WHERE path=?1;", -1, &statement, nullptr))
    {
        return false;
    }

    sqlite3_bind_text(statement, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);

    bool recordFound = false;
    if (SQLITE_ROW == sqlite3_step(statement))
    {
        const char* hashText = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
        const char* statusText = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
        const char* timestampText = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));

        if ((nullptr != hashText) && (nullptr != statusText) && (nullptr != timestampText))
        {
            outputHash = hashText;
            outputStatus = StringToChangeType(statusText);
            outputTimestamp = timestampText;
            recordFound = true;
        }
    }

    sqlite3_finalize(statement);
    return recordFound;
}

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
 * @param[in] sourceRootPath Root directory of source files
 * @param[in] currentBackupDirectory Current backup directory path
 * @param[in] database Database handle
 * @param[in] historyFolderPath History root directory for snapshots
 * @param[in] onProgressCallback Optional progress callback function
 * @param[in/out] processedCount Counter for processed files
 * @param[in] createSnapshotOnce Lazy initialization function for snapshot creation
 * @return true if file processed successfully, false otherwise
 */
static bool ProcessFile(const fs::path& filePath, const fs::path& sourceRootPath, const fs::path& currentBackupDirectory, sqlite3* database,
                        const fs::path& historyFolderPath, std::function<void(const BackupProgress&)> onProgressCallback,
                        std::atomic<std::size_t>& processedCount, std::function<fs::path()> createSnapshotOnce)
{
    std::error_code errorCode;
    fs::path relativeFilePath = fs::relative(filePath, sourceRootPath, errorCode);
    if (errorCode)
    {
        return false;
    }

    // Handle single-file-in-root case
    if ("." == relativeFilePath)
    {
        relativeFilePath = filePath.filename();
    }

    fs::path currentFilePath = currentBackupDirectory / relativeFilePath;

    std::string newHash;
    if (!ComputeFileHash(filePath, newHash))
    {
        return false;
    }

    std::string storedHash;
    ChangeType storedStatus;
    std::string storedTimestamp;
    bool hadPreviousRecord =
        GetStoredFileState(database, relativeFilePath.string(), storedHash, storedStatus, storedTimestamp) && (ChangeType::Deleted != storedStatus);
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
        auto snapshotPath = createSnapshotOnce();
        fs::path archivedPath = snapshotPath / relativeFilePath;
        fs::create_directories(archivedPath.parent_path(), errorCode);
        fs::copy_file(currentFilePath, archivedPath, fs::copy_options::overwrite_existing, errorCode);
        fs::copy_file(filePath, currentFilePath, fs::copy_options::overwrite_existing, errorCode);
    }
    else
    {
        newStatus = ChangeType::Unchanged;
    }

    if (nullptr != database)
    {
        std::string timestampValue;
        if (ChangeType::Unchanged != newStatus)
        {
            timestampValue = GetCurrentTimestamp();
        }
        else
        {
            timestampValue = storedTimestamp;
        }

        if (!UpdateStoredFileState(database, relativeFilePath.string(), newHash, newStatus, timestampValue))
        {
            return false;
        }
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
 * @param[in] database Database handle
 * @param[in] sourceDirectoryPath Source directory path
 * @param[in] currentBackupDirectory Current backup directory path
 * @param[in] historyFolderPath History root directory for snapshots
 * @param[in] onProgressCallback Optional progress callback function
 * @param[in] createSnapshotOnce Lazy initialization function for snapshot creation
 * @return true if operation succeeded, false otherwise
 */
static bool DetectDeletedFiles(sqlite3* database, const fs::path& sourceDirectoryPath, const fs::path& currentBackupDirectory,
                               const fs::path& historyFolderPath, std::function<void(const BackupProgress&)> onProgressCallback,
                               std::function<fs::path()> createSnapshotOnce)
{
    std::error_code errorCode;
    if (nullptr == database)
    {
        return true;
    }

    sqlite3_stmt* statement = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(database, "SELECT path, status FROM files;", -1, &statement, nullptr))
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

        fs::path sourceFilePath = sourceDirectoryPath / databasePath;
        fs::path currentFilePath = currentBackupDirectory / databasePath;

        if (!fs::exists(sourceFilePath, errorCode))
        {
            auto snapshotPath = createSnapshotOnce();

            if (fs::exists(currentFilePath, errorCode))
            {
                fs::path archivedPath = snapshotPath / databasePath;
                fs::create_directories(archivedPath.parent_path(), errorCode);
                fs::copy_file(currentFilePath, archivedPath, fs::copy_options::overwrite_existing, errorCode);
            }

            fs::remove(currentFilePath, errorCode);

            sqlite3_stmt* updateStatement = nullptr;
            if (SQLITE_OK != sqlite3_prepare_v2(database, "UPDATE files SET status=?1, last_updated=?2 WHERE path=?3;", -1, &updateStatement, nullptr))
            {
                operationSucceeded = false;
                break;
            }

            sqlite3_bind_text(updateStatement, 1, ChangeTypeToString(ChangeType::Deleted), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(updateStatement, 2, GetCurrentTimestamp().c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(updateStatement, 3, databasePath.c_str(), -1, SQLITE_TRANSIENT);

            if (SQLITE_DONE != sqlite3_step(updateStatement))
            {
                sqlite3_finalize(updateStatement);
                operationSucceeded = false;
                break;
            }
            sqlite3_finalize(updateStatement);

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

    const fs::path sourceFolderPath = configuration.sourceDir;
    if (!fs::exists(sourceFolderPath))
    {
        return false;
    }

    const fs::path currentBackupDirectory = configuration.backupRoot / "backup";
    const fs::path historyFolderPath = configuration.backupRoot / "deleted";

    fs::create_directories(currentBackupDirectory, errorCode);
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
    sqlite3* database = databaseSession.get();

    if (!InitializeSchema(database))
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
                sqlite3* threadDatabase = databaseSession.get();
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

                    ProcessFile(currentFile, sourceFolderPath, currentBackupDirectory, threadDatabase, historyFolderPath, threadSafeProgressCallback,
                                processedCount, CreateSnapshotOnce);
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
    fs::path actualSourceRootPath = sourceFolderPath;
    if (fs::is_directory(sourceFolderPath))
    {
        for (auto& entry : fs::recursive_directory_iterator(sourceFolderPath, errorCode))
        {
            if ((!errorCode) && (entry.is_regular_file()))
            {
                EnqueueFile(entry.path());
            }
        }
    }
    else if (fs::is_regular_file(sourceFolderPath))
    {
        EnqueueFile(sourceFolderPath);
        // Use parent directory as root for deletion detection
        actualSourceRootPath = sourceFolderPath.parent_path();
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
        if (!DetectDeletedFiles(database, actualSourceRootPath, currentBackupDirectory, historyFolderPath, threadSafeProgressCallback, CreateSnapshotOnce))
        {
            operationSucceeded = false;
        }
    }

    return operationSucceeded;
}
