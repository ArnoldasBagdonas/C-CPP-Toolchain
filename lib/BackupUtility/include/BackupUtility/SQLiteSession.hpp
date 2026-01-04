// file SQLiteSession.hpp:

#pragma once

#include <filesystem>
#include <sqlite3.h>
#include <string>
#include <mutex>
#include <stdexcept>

namespace fs = std::filesystem;

/**
 * @brief Thread-safe SQLite wrapper that provides a per-thread connection.
 *
 * Each thread gets its own connection automatically using `thread_local`.
 * Connections are cleaned up automatically when the thread exits.
 */
class SQLiteSession
{
public:
    explicit SQLiteSession(const fs::path& dbPath)
        : m_dbPath(dbPath)
    {
        // Enable serialized mode for SQLite (thread-safe)
        sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    }

    ~SQLiteSession() = default; // thread_local connections are closed automatically

    /**
     * @brief Get a SQLite connection for this thread.
     * Opens the connection if not already opened.
     *
     * @return sqlite3* pointer to the connection
     */
    sqlite3* get()
    {
        thread_local sqlite3* db = open();
        return db;
    }

    /**
     * @brief Execute a SQL statement that does not return results.
     *
     * @param sql SQL statement
     * @throws std::runtime_error on error
     */
    void execute(const std::string& sql)
    {
        char* errMsg = nullptr;
        if (sqlite3_exec(get(), sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
        {
            std::string error = errMsg ? errMsg : "Unknown SQLite error";
            sqlite3_free(errMsg);
            throw std::runtime_error(error);
        }
    }

    /**
     * @brief Prepare a statement for repeated execution.
     *
     * @param sql SQL statement
     * @return sqlite3_stmt* prepared statement
     * @throws std::runtime_error on error
     */
    sqlite3_stmt* prepare(const std::string& sql)
    {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(get(), sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        {
            throw std::runtime_error(sqlite3_errmsg(get()));
        }
        return stmt;
    }

private:
    sqlite3* open()
    {
        sqlite3* db = nullptr;
        if (sqlite3_open(m_dbPath.string().c_str(), &db) != SQLITE_OK)
        {
            throw std::runtime_error("Failed to open SQLite DB: " + m_dbPath.string());
        }
        return db;
    }

    fs::path m_dbPath;
};
