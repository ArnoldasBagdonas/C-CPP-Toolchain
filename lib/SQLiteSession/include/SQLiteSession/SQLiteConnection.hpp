// file SQLiteConnection.hpp:

#pragma once

#include "SQLiteSession/SQLiteStatement.hpp"

#include <filesystem>
#include <string>

struct sqlite3;

namespace fs = std::filesystem;

/**
 * @brief RAII wrapper for a SQLite database connection.
 */
class SQLiteConnection
{
  public:
    SQLiteConnection(const fs::path& databasePath, int busyTimeoutMs);
    ~SQLiteConnection();

    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    SQLiteConnection(SQLiteConnection&& other) noexcept;
    SQLiteConnection& operator=(SQLiteConnection&& other) noexcept;

    void Execute(const std::string& sqlStatement);
    SQLiteStatement Prepare(const std::string& sqlStatement);

  private:
    void EnableWalMode();

    sqlite3* _database;
};
