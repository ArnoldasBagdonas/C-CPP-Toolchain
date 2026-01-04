// file SQLiteSession.hpp:

#pragma once

#include <filesystem>
#include <sqlite3.h>
#include <string>
#include <mutex>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <thread>

namespace fs = std::filesystem;

/**
 * @brief Thread-safe SQLite wrapper that provides a per-thread connection.
 *
 * Each thread gets its own connection automatically.
 * Connections are properly managed and cleaned up.
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

    ~SQLiteSession()
    {
        // Clean up all thread connections
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        for (auto& pair : m_connections)
        {
            if (pair.second)
            {
                sqlite3_close(pair.second);
            }
        }
        m_connections.clear();
    }

    /**
     * @brief Get a SQLite connection for this thread.
     * Opens the connection if not already opened.
     *
     * @return sqlite3* pointer to the connection
     */
    sqlite3* get()
    {
        std::thread::id threadId = std::this_thread::get_id();
        
        {
            std::lock_guard<std::mutex> lock(m_connectionsMutex);
            auto it = m_connections.find(threadId);
            if (it != m_connections.end() && it->second != nullptr)
            {
                return it->second;
            }
        }
        
        // Open a new connection for this thread
        sqlite3* db = open();
        
        {
            std::lock_guard<std::mutex> lock(m_connectionsMutex);
            m_connections[threadId] = db;
        }
        
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

        // Set busy timeout to handle concurrent access
        sqlite3_busy_timeout(db, 5000); // 5 seconds

        // Enable WAL mode on this connection
        char* errMsg = nullptr;
        if (sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errMsg) != SQLITE_OK)
        {
            std::string error = errMsg ? errMsg : "Unknown SQLite error enabling WAL";
            sqlite3_free(errMsg);
            sqlite3_close(db);
            throw std::runtime_error(error);
        }

        return db;
    }

    fs::path m_dbPath;
    std::mutex m_connectionsMutex;
    std::unordered_map<std::thread::id, sqlite3*> m_connections;
};
