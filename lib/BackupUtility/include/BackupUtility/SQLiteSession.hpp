// file SQLiteSession.hpp:

#pragma once

#include "BackupUtility/SQLiteConnection.hpp"

#include <filesystem>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace fs = std::filesystem;

/**
 * @brief Manages per-thread SQLite connections for a database file.
 */
class SQLiteSession
{
  public:
    static constexpr int SqliteBusyTimeoutMs = 5000;

    /**
     * @brief Construct a SQLite session for the specified database file.
     *
     * @param[in] databasePath Path to the SQLite database file
     */
    explicit SQLiteSession(const fs::path& databasePath);

    SQLiteSession(const SQLiteSession&) = delete;
    SQLiteSession& operator=(const SQLiteSession&) = delete;

    /**
     * @brief Acquire a SQLite connection for the current thread.
     *
     * @return SQLiteConnection reference bound to the current thread
     */
    SQLiteConnection& Acquire();

  private:
    SQLiteConnection CreateConnection();

    fs::path _databasePath;
    std::mutex _connectionsMutex;
    std::unordered_map<std::thread::id, std::unique_ptr<SQLiteConnection>> _connections;
};
