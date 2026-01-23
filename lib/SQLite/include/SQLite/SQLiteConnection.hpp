// file SQLiteConnection.hpp:

#pragma once

#include "SQLite/SQLiteStatement.hpp"

#include <filesystem>
#include <string>

struct sqlite3;

/**
 * @brief RAII wrapper for a SQLite database connection.
 */
class SQLiteConnection
{
  public:
    /**
     * @brief Open a SQLite connection to the specified database file.
     *
     * @param[in] databasePath Path to the SQLite database file
     * @param[in] busyTimeoutMs Busy timeout in milliseconds
     */
    SQLiteConnection(const std::filesystem::path& databasePath, int busyTimeoutMs);
    /**
     * @brief Close the SQLite connection.
     */
    ~SQLiteConnection();

    SQLiteConnection(const SQLiteConnection&) = delete;
    SQLiteConnection& operator=(const SQLiteConnection&) = delete;

    /**
     * @brief Move-construct a SQLite connection.
     *
     * @param[in] other Connection to move from
     */
    SQLiteConnection(SQLiteConnection&& other) noexcept;
    /**
     * @brief Move-assign a SQLite connection.
     *
     * @param[in] other Connection to move from
     * @return Reference to this connection
     */
    SQLiteConnection& operator=(SQLiteConnection&& other) noexcept;

    /**
     * @brief Execute a SQL statement without results.
     *
     * @param[in] sqlStatement SQL statement to execute
     */
    void Execute(const std::string& sqlStatement);
    /**
     * @brief Prepare a SQL statement.
     *
     * @param[in] sqlStatement SQL statement to prepare
     * @return Prepared SQLite statement
     */
    SQLiteStatement Prepare(const std::string& sqlStatement);

  private:
    void EnableWriteAheadLoggingMode();

    sqlite3* _database;
};
