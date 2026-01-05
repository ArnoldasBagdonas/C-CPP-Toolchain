// file SQLiteSession.hpp:

#pragma once

#include <filesystem>
#include <mutex>
#include <sqlite3.h>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>

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
    /**
     * @brief Construct a SQLite session for the specified database file.
     *
     * @param[in] databasePath Path to the SQLite database file
     */
    explicit SQLiteSession(const fs::path& databasePath) : m_databasePath(databasePath)
    {
        // Enable serialized mode for SQLite (thread-safe)
        sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    }

    /**
     * @brief Destructor cleans up all thread connections.
     */
    ~SQLiteSession()
    {
        // Clean up all thread connections
        std::lock_guard<std::mutex> lock(m_connectionsMutex);
        for (auto& connectionPair : m_connections)
        {
            if (nullptr != connectionPair.second)
            {
                sqlite3_close(connectionPair.second);
            }
        }
        m_connections.clear();
    }

    /**
     * @brief Get a SQLite connection for this thread.
     *
     * Opens the connection if not already opened for the calling thread.
     *
     * @return sqlite3* pointer to the connection
     */
    sqlite3* get()
    {
        std::thread::id threadId = std::this_thread::get_id();

        {
            std::lock_guard<std::mutex> lock(m_connectionsMutex);
            auto iterator = m_connections.find(threadId);
            if ((m_connections.end() != iterator) && (nullptr != iterator->second))
            {
                return iterator->second;
            }
        }

        // Open a new connection for this thread
        sqlite3* database = OpenConnection();

        {
            std::lock_guard<std::mutex> lock(m_connectionsMutex);
            m_connections[threadId] = database;
        }

        return database;
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
        if (SQLITE_OK != sqlite3_exec(get(), sqlStatement.c_str(), nullptr, nullptr, &errorMessage))
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
    sqlite3_stmt* Prepare(const std::string& sqlStatement)
    {
        sqlite3_stmt* statement = nullptr;
        if (SQLITE_OK != sqlite3_prepare_v2(get(), sqlStatement.c_str(), -1, &statement, nullptr))
        {
            throw std::runtime_error(sqlite3_errmsg(get()));
        }
        return statement;
    }

  private:
    /**
     * @brief Open a new SQLite connection for the current thread.
     *
     * @return sqlite3* pointer to the opened database connection
     * @throws std::runtime_error if connection fails
     */
    sqlite3* OpenConnection()
    {
        sqlite3* database = nullptr;
        if (SQLITE_OK != sqlite3_open(m_databasePath.string().c_str(), &database))
        {
            throw std::runtime_error("Failed to open SQLite DB: " + m_databasePath.string());
        }

        // Set busy timeout to handle concurrent access
        sqlite3_busy_timeout(database, 5000);

        // Enable WAL mode on this connection
        char* errorMessage = nullptr;
        if (SQLITE_OK != sqlite3_exec(database, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errorMessage))
        {
            std::string error = (nullptr != errorMessage) ? errorMessage : "Unknown SQLite error enabling WAL";
            sqlite3_free(errorMessage);
            sqlite3_close(database);
            throw std::runtime_error(error);
        }

        return database;
    }

    fs::path m_databasePath;
    std::mutex m_connectionsMutex;
    std::unordered_map<std::thread::id, sqlite3*> m_connections;
};
