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

namespace
{
// Constants for file processing
constexpr std::size_t FILE_READ_BUFFER_SIZE = 8192;
constexpr unsigned int MAX_QUEUE_SIZE_MULTIPLIER = 4;

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
bool ComputeFileHash(const fs::path& filePath, std::string& outputHash)
{
    std::ifstream inputStream(filePath, std::ios::binary);
    if (false == inputStream.is_open()) // Explicit comparison
    {
        return false;
    }

    XXH64_state_t* hashState = XXH64_createState();
    if (nullptr == hashState)
    {
        return false;
    }

    XXH64_reset(hashState, 0);

    char buffer[FILE_READ_BUFFER_SIZE];
    while (inputStream.read(buffer, sizeof(buffer)))
    {
        XXH64_update(hashState, buffer, sizeof(buffer));
    }

    if (inputStream.gcount() > 0)
    {
        XXH64_update(hashState, buffer, static_cast<size_t>(inputStream.gcount())); // Static cast for safety
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
std::string GetCurrentTimestamp()
{
    std::time_t currentTime = std::time(nullptr);
    std::tm timeStruct{};

#ifdef _WIN32
    _localtime64_s(&timeStruct, &currentTime);
#else
    localtime_r(&currentTime, &timeStruct);
#endif

    char buffer[32]; // Sufficient for "YYYY-MM-DD_HH-MM-SS" plus null terminator
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
fs::path CreateSnapshotDirectory(const fs::path& historyRootPath)
{
    fs::path snapshotDirectory = historyRootPath / GetCurrentTimestamp();
    fs::create_directories(snapshotDirectory);
    return snapshotDirectory;
}

class DeletedFilesProcessor
{
  public:
    DeletedFilesProcessor(const fs::path& sourceFolderPath, const fs::path& backupFolderPath, std::function<fs::path()> createSnapshotPathOnce,
                          SQLiteSession& databaseSession, std::function<void(const BackupProgress&)> onProgressCallback)
        : _sourceFolderPath(sourceFolderPath), _backupFolderPath(backupFolderPath), _createSnapshotPathOnce(createSnapshotPathOnce),
          _databaseSession(databaseSession), _onProgressCallback(onProgressCallback)
    {
    }

        bool ProcessDeletedFiles()
        {
            try
            {
                std::error_code errorCode;
    
                std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> statement;
                try
                {
                    statement = _databaseSession.GetAllFiles();
                }
                catch (const std::runtime_error& e)
                {
                    // TODO: Log error e.what()
                    return false;
                }
    
                bool operationSucceeded = true;
                while (SQLITE_ROW == sqlite3_step(statement.get()))
                {
                    const char* databasePathCString = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 0));
                    const char* statusCString = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1));
    
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
    
                    fs::path sourceFilePath = _sourceFolderPath / databasePath;
                    fs::path currentFilePath = _backupFolderPath / databasePath;
    
                    if (false == fs::exists(sourceFilePath, errorCode)) // Explicit comparison
                    {
                        fs::path snapshotPath;
                        try
                        {
                            snapshotPath = _createSnapshotPathOnce();
                        }
                        catch (const std::runtime_error& e)
                        {
                            // TODO: Log error e.what()
                            operationSucceeded = false;
                            break;
                        }
    
                        if (true == fs::exists(currentFilePath, errorCode)) // Explicit comparison
                        {
                            fs::path archivedPath = snapshotPath / databasePath;
                            fs::create_directories(archivedPath.parent_path(), errorCode);
                            fs::copy_file(currentFilePath, archivedPath, fs::copy_options::overwrite_existing, errorCode);
                        }
    
                        fs::remove(currentFilePath, errorCode);
    
                        try
                        {
                            if (false == _databaseSession.MarkFileAsDeleted(databasePath, GetCurrentTimestamp())) // Explicit comparison
                            {
                                operationSucceeded = false;
                                break;
                            }
                        }
                        catch (const std::runtime_error& e)
                        {
                            // TODO: Log error e.what()
                            operationSucceeded = false;
                            break;
                        }
    
                        if (nullptr != _onProgressCallback)
                        {
                            _onProgressCallback({"deleted", 0, 0, databasePath});
                        }
                    }
                }
    
                return operationSucceeded;
            }
            catch (const std::runtime_error& e)
            {
                // TODO: Log error e.what()
                return false;
            }
        }
  private:
    const fs::path& _sourceFolderPath;
    const fs::path& _backupFolderPath;
    std::function<fs::path()> _createSnapshotPathOnce;
    SQLiteSession& _databaseSession;
    std::function<void(const BackupProgress&)> _onProgressCallback;
};

class FileProcessor
{
  public:
    FileProcessor(const fs::path& sourceRoot, const fs::path& backupRoot, std::function<fs::path()> createSnapshotOnce,
                  SQLiteSession& database, std::function<void(const BackupProgress&)> threadSafeProgress,
                  std::atomic<bool>& success, std::atomic<std::size_t>& processedCount)
        : _sourceRoot(sourceRoot), _backupRoot(backupRoot), _createSnapshotOnce(createSnapshotOnce),
          _database(database), _threadSafeProgress(threadSafeProgress),
          _success(success), _processedCount(processedCount)
    {
    }

    void ProcessSingleFile(const fs::path& file)
    {
        std::error_code ec;
        fs::path relativePath = fs::relative(file, _sourceRoot, ec);
        if (ec)
        {
            _success.store(false);
            return;
        }
        if (std::string(".") == relativePath.string()) // Explicit comparison
            relativePath = file.filename();

        fs::path backupFile = _backupRoot / relativePath;
        std::string newHash;
        if (false == ComputeFileHash(file, newHash)) // Explicit comparison
        {
            _success.store(false);
            return;
        }

        std::string storedHash, storedTimestamp;
        ChangeType storedStatus;
        bool hasRecord = false;
        try
        {
            hasRecord = _database.GetFileState(relativePath.string(), storedHash, storedStatus, storedTimestamp) && (storedStatus != ChangeType::Deleted);
        }
        catch (const std::runtime_error& e)
        {
            // TODO: Log error e.what()
            _success.store(false);
            return;
        }

        bool changed = (false == hasRecord) || (newHash != storedHash); // Explicit comparison
        ChangeType newStatus = ChangeType::Unchanged;
        std::string timestampValue = storedTimestamp;

        if (false == hasRecord) // Explicit comparison
        {
            newStatus = ChangeType::Added;
            timestampValue = GetCurrentTimestamp();
            fs::create_directories(backupFile.parent_path(), ec);
            fs::copy_file(file, backupFile, fs::copy_options::overwrite_existing, ec);
        }
        else if (true == changed) // Explicit comparison
        {
            newStatus = ChangeType::Modified;
            timestampValue = GetCurrentTimestamp();
            fs::path snapshotFile;
            try
            {
                snapshotFile = _createSnapshotOnce() / relativePath;
            }
            catch (const std::runtime_error& e)
            {
                // TODO: Log error e.what()
                _success.store(false);
                return;
            }
            fs::create_directories(snapshotFile.parent_path(), ec);
            fs::copy_file(backupFile, snapshotFile, fs::copy_options::overwrite_existing, ec);
            fs::copy_file(file, backupFile, fs::copy_options::overwrite_existing, ec);
        }

        try
        {
            if (false == _database.UpdateFileState(relativePath.string(), newHash, newStatus, timestampValue)) // Explicit comparison
            {
                _success.store(false);
            }
        }
        catch (const std::runtime_error& e)
        {
            // TODO: Log error e.what()
            _success.store(false);
            return;
        }

        _threadSafeProgress({"collecting", ++_processedCount, 0, file});
    }

  private:
    const fs::path& _sourceRoot;
    const fs::path& _backupRoot;
    std::function<fs::path()> _createSnapshotOnce;
    SQLiteSession& _database;
    std::function<void(const BackupProgress&)> _threadSafeProgress;
    std::atomic<bool>& _success;
    std::atomic<std::size_t>& _processedCount;
};

} // namespace

/** Run backup with multithreaded file processing */
bool RunBackup(const BackupConfig& config)
{
    std::atomic<bool> success{true};
    std::error_code ec;

    // Determine source root
    fs::path sourceRoot = fs::is_regular_file(config.sourceDir, ec) ? config.sourceDir.parent_path() : config.sourceDir;
    if (ec || !fs::exists(sourceRoot))
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
    if (false == database.InitializeSchema()) // Explicit comparison
    {
        return false;
    }

    // Thread-safe progress callback
    std::mutex progressMutex;
    auto threadSafeProgress = [&](const BackupProgress& prog)
    {
        if (nullptr == config.onProgress) // Explicit comparison
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
    const std::size_t maxQueueSize = threadCount * MAX_QUEUE_SIZE_MULTIPLIER; // bounded size

    // Start worker threads
    std::vector<std::thread> workers;
    for (unsigned int i = 0; i < threadCount; ++i)
    {
        workers.emplace_back(std::thread([&]() {
            FileProcessor fileProcessor(sourceRoot, backupRoot, CreateSnapshotOnce, database, threadSafeProgress, success, processedCount);
            while (true)
            {
                fs::path file;
                {
                    std::unique_lock lock(queueMutex);
                    queueCV.wait(lock, [&]() { return doneWalking || false == fileQueue.empty(); });
                    if (true == fileQueue.empty() && true == doneWalking)
                        return;
                    if (true == fileQueue.empty())
                        continue;
                    file = std::move(fileQueue.front());
                    fileQueue.pop();
                    queueCV.notify_all();
                }
                fileProcessor.ProcessSingleFile(file);
            }
        }));
    }

    auto EnqueueFile = [&](const fs::path& file)
    {
        std::unique_lock lock(queueMutex);
        queueCV.wait(lock, [&]() { return fileQueue.size() < maxQueueSize; });
        fileQueue.push(file);
        queueCV.notify_all(); // notify workers
    };

    // Enqueue initial files
    const fs::path path = config.sourceDir;
    if (true == fs::is_regular_file(path, ec)) // Explicit comparison
    {
        EnqueueFile(path);
    }
    else if (true == fs::is_directory(path, ec)) // Explicit comparison
    {
        for (auto& entry : fs::recursive_directory_iterator(path, ec))
        {
            if (!ec && entry.is_regular_file(ec))
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
    if (true == success.load()) // Explicit comparison
    {
        DeletedFilesProcessor deletedFilesProcessor(sourceRoot, backupRoot, CreateSnapshotOnce, database, threadSafeProgress);
        success.store(deletedFilesProcessor.ProcessDeletedFiles());
    }

    return success.load();
}