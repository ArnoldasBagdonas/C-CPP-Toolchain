// file SQLiteSession.hpp:

#pragma once

#include <filesystem>
#include <memory> // Required for std::unique_ptr
#include <mutex>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

// Forward declaration
enum class ChangeType;
const char* ChangeTypeToString(ChangeType changeType);
ChangeType StringToChangeType(const std::string& stringValue);

/**
 * @brief Custom deleter for sqlite3 database connections.
 * Ensures sqlite3_close is called when a unique_ptr goes out of scope.
 */
struct SQLiteDBDeleter
{
    void operator()(sqlite3* db) const
    {
        if (nullptr != db)
        {
            sqlite3_close(db);
        }
    }
};

/**
 * @brief Custom deleter for sqlite3_stmt prepared statements.
 * Ensures sqlite3_finalize is called when a unique_ptr goes out of scope.
 */
struct SQLiteStmtDeleter
{
    void operator()(sqlite3_stmt* stmt) const
    {
        if (nullptr != stmt)
        {
            sqlite3_finalize(stmt);
        }
    }
};

/**
 * @brief Thread-safe SQLite wrapper that provides a per-thread connection.
 *
 * Each thread gets its own connection automatically.
 * Connections are properly managed and cleaned up.
 */
class SQLiteSession
{
  public:
    static constexpr int SQLITE_BUSY_TIMEOUT_MS = 5000;

    /**
     * @brief Construct a SQLite session for the specified database file.
     *
     * @param[in] databasePath Path to the SQLite database file
     */
    explicit SQLiteSession(const fs::path& databasePath) : _databasePath(databasePath)
    {
        // Enable serialized mode for SQLite (thread-safe)
        sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    }

    /**
     * @brief Destructor cleans up all thread connections.
     */
    ~SQLiteSession() = default; // unique_ptr will handle closing connections

    /**
     * @brief Get a SQLite connection for this thread.
     *
     * Opens the connection if not already opened for the calling thread.
     *
     * @return sqlite3* pointer to the connection
     */
    std::unique_ptr<sqlite3, SQLiteDBDeleter>& get()
    {
        std::thread::id threadId = std::this_thread::get_id();

        {
            std::lock_guard<std::mutex> lock(_connectionsMutex);
            auto iterator = _connections.find(threadId);
            if ((_connections.end() != iterator) && (nullptr != iterator->second))
            {
                return iterator->second;
            }
        }

        // Open a new connection for this thread
        std::unique_ptr<sqlite3, SQLiteDBDeleter> database = OpenConnection();

        {
            std::lock_guard<std::mutex> lock(_connectionsMutex);
            _connections[threadId] = std::move(database);
            return _connections[threadId];
        }
    }

    /**
     * @brief Execute a SQL statement that does not return results.
     *
     * @param[in] sqlStatement SQL statement to execute
     * @throws std::runtime_error on error
     */
    void Execute(const std::string& sqlStatement)
    {
        char* errorMessage = nullptr;
        sqlite3* db = get().get();
        if (SQLITE_OK != sqlite3_exec(db, sqlStatement.c_str(), nullptr, nullptr, &errorMessage))
        {
            std::string error = (nullptr != errorMessage) ? errorMessage : "Unknown SQLite error";
            sqlite3_free(errorMessage);
            throw std::runtime_error(error);
        }
    }

    /**
     * @brief Prepare a statement for repeated execution.
     *
     * @param[in] sqlStatement SQL statement to prepare
     * @return sqlite3_stmt* prepared statement
     * @throws std::runtime_error on error
     */
    std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> Prepare(const std::string& sqlStatement)
    {
        sqlite3_stmt* statement = nullptr;
        sqlite3* db = get().get();
        if (SQLITE_OK != sqlite3_prepare_v2(db, sqlStatement.c_str(), -1, &statement, nullptr))
        {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
        return std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter>(statement);
    }

    /**
     * @brief Initialize the database schema for file tracking.
     *
     * Creates the files table if it does not exist, with columns for path,
     * hash, last_updated timestamp, and status.
     *
     * @return true if schema initialized successfully, false otherwise
     */
    bool InitializeSchema()
    {
        try
        {
            sqlite3* database = get().get();
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
        catch (const std::runtime_error& e)
        {
            // TODO: Log error e.what()
            return false;
        }
    }

    /**
     * @brief Update or insert file state information in the database.
     *
     * Uses INSERT with ON CONFLICT to update existing records or create new ones.
     *
     * @param[in] filePath Relative path of the file
     * @param[in] fileHash Hash value of the file content
     * @param[in] changeStatus Current change status of the file
     * @param[in] timestamp Last update timestamp
     * @return true if operation succeeded, false otherwise
     */
    bool UpdateFileState(const std::string& filePath, const std::string& fileHash, ChangeType changeStatus, const std::string& timestamp)
    {
        sqlite3* database = get().get();
        if (nullptr == database)
        {
            return false;
        }

        sqlite3_stmt* stmtRaw = nullptr;
        if (SQLITE_OK != sqlite3_prepare_v2(database,
                                            "INSERT INTO files(path, hash, status, last_updated) "
                                            "VALUES(?1, ?2, ?3, ?4) "
                                            "ON CONFLICT(path) DO UPDATE SET "
                                            "hash=excluded.hash, status=excluded.status, last_updated=excluded.last_updated;",
                                            -1, &stmtRaw, nullptr))
        {
            return false;
        }
        std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> statement(stmtRaw);

        sqlite3_bind_text(statement.get(), 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 2, fileHash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 3, ChangeTypeToString(changeStatus), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 4, timestamp.c_str(), -1, SQLITE_TRANSIENT);

        bool operationSucceeded = (SQLITE_DONE == sqlite3_step(statement.get()));
        return operationSucceeded;
    }

    /**
     * @brief Retrieve stored file state information from the database.
     *
     * @param[in] filePath Relative path of the file to query
     * @param[out] outputHash Retrieved hash value
     * @param[out] outputStatus Retrieved change status
     * @param[out] outputTimestamp Retrieved last update timestamp
     * @return true if file record was found, false otherwise
     */
    bool GetFileState(const std::string& filePath, std::string& outputHash, ChangeType& outputStatus, std::string& outputTimestamp)
    {
        sqlite3* database = get().get();
        if (nullptr == database)
        {
            return false;
        }

        sqlite3_stmt* stmtRaw = nullptr;
        if (SQLITE_OK != sqlite3_prepare_v2(database, "SELECT hash, status, last_updated FROM files WHERE path=?1;", -1, &stmtRaw, nullptr))
        {
            return false;
        }
        std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> statement(stmtRaw);

        sqlite3_bind_text(statement.get(), 1, filePath.c_str(), -1, SQLITE_TRANSIENT);

        bool recordFound = false;
        if (SQLITE_ROW == sqlite3_step(statement.get()))
        {
            const char* hashText = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 0));
            const char* statusText = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 1));
            const char* timestampText = reinterpret_cast<const char*>(sqlite3_column_text(statement.get(), 2));

            if ((nullptr != hashText) && (nullptr != statusText) && (nullptr != timestampText))
            {
                outputHash = hashText;
                outputStatus = StringToChangeType(statusText);
                outputTimestamp = timestampText;
                recordFound = true;
            }
        }

        return recordFound;
    }

    /**
     * @brief Get all files from the database.
     *
     * @return std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> a prepared statement
     * @throws std::runtime_error if query preparation fails
     */
    std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> GetAllFiles()
    {
        sqlite3* database = get().get();
        if (nullptr == database)
        {
            throw std::runtime_error("Database connection is null.");
        }

        sqlite3_stmt* stmtRaw = nullptr;
        if (SQLITE_OK != sqlite3_prepare_v2(database, "SELECT path, status FROM files;", -1, &stmtRaw, nullptr))
        {
            throw std::runtime_error(sqlite3_errmsg(database));
        }
        return std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter>(stmtRaw);
    }

    /**
     * @brief Update file status to deleted.
     *
     * @param[in] filePath Relative path of the file
     * @param[in] timestamp Deletion timestamp
     * @return true if operation succeeded, false otherwise
     */
    bool MarkFileAsDeleted(const std::string& filePath, const std::string& timestamp)
    {
        sqlite3* database = get().get();
        if (nullptr == database)
        {
            return false;
        }

        sqlite3_stmt* stmtRaw = nullptr;
        if (SQLITE_OK != sqlite3_prepare_v2(database, "UPDATE files SET status=?1, last_updated=?2 WHERE path=?3;", -1, &stmtRaw, nullptr))
        {
            return false;
        }
        std::unique_ptr<sqlite3_stmt, SQLiteStmtDeleter> statement(stmtRaw);

        sqlite3_bind_text(statement.get(), 1, ChangeTypeToString(ChangeType::Deleted), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 2, timestamp.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement.get(), 3, filePath.c_str(), -1, SQLITE_TRANSIENT);

        bool operationSucceeded = (SQLITE_DONE == sqlite3_step(statement.get()));
        return operationSucceeded;
    }

  private:
    /**
     * @brief Open a new SQLite connection for the current thread.
     *
     * @return sqlite3* pointer to the opened database connection
     * @throws std::runtime_error if connection fails
     */
    std::unique_ptr<sqlite3, SQLiteDBDeleter> OpenConnection()
    {
        sqlite3* databaseRaw = nullptr;
        if (SQLITE_OK != sqlite3_open(_databasePath.string().c_str(), &databaseRaw))
        {
            throw std::runtime_error("Failed to open SQLite DB: " + _databasePath.string());
        }
        std::unique_ptr<sqlite3, SQLiteDBDeleter> database(databaseRaw);

        // Set busy timeout to handle concurrent access
        sqlite3_busy_timeout(database.get(), SQLITE_BUSY_TIMEOUT_MS);

        // Enable WAL mode on this connection
        char* errorMessage = nullptr;
        if (SQLITE_OK != sqlite3_exec(database.get(), "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errorMessage))
        {
            std::string error = (nullptr != errorMessage) ? errorMessage : "Unknown SQLite error enabling WAL";
            sqlite3_free(errorMessage);
            throw std::runtime_error(error);
        }

        return database;
    }

    fs::path _databasePath;
    std::mutex _connectionsMutex;
    std::unordered_map<std::thread::id, std::unique_ptr<sqlite3, SQLiteDBDeleter>> _connections;
};
