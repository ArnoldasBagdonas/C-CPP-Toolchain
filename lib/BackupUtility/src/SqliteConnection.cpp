// file SQLiteConnection.cpp

#include "BackupUtility/SQLiteConnection.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <utility>

SQLiteConnection::SQLiteConnection(const fs::path& databasePath, int busyTimeoutMs) : _database(nullptr)
{
    if (SQLITE_OK != sqlite3_open(databasePath.string().c_str(), &_database))
    {
        throw std::runtime_error("Failed to open SQLite DB: " + databasePath.string());
    }

    if (SQLITE_OK != sqlite3_busy_timeout(_database, busyTimeoutMs))
    {
        throw std::runtime_error("Failed to set SQLite busy timeout.");
    }

    EnableWalMode();
}

SQLiteConnection::~SQLiteConnection()
{
    if (nullptr != _database)
    {
        sqlite3_close(_database);
    }
}

SQLiteConnection::SQLiteConnection(SQLiteConnection&& other) noexcept : _database(std::exchange(other._database, nullptr))
{
}

SQLiteConnection& SQLiteConnection::operator=(SQLiteConnection&& other) noexcept
{
    if (this != &other)
    {
        if (nullptr != _database)
        {
            sqlite3_close(_database);
        }
        _database = std::exchange(other._database, nullptr);
    }
    return *this;
}

void SQLiteConnection::Execute(const std::string& sqlStatement)
{
    char* errorMessage = nullptr;
    if (SQLITE_OK != sqlite3_exec(_database, sqlStatement.c_str(), nullptr, nullptr, &errorMessage))
    {
        std::string error = (nullptr != errorMessage) ? errorMessage : "Unknown SQLite error";
        sqlite3_free(errorMessage);
        throw std::runtime_error(error);
    }
}

SQLiteStatement SQLiteConnection::Prepare(const std::string& sqlStatement)
{
    sqlite3_stmt* statement = nullptr;
    if (SQLITE_OK != sqlite3_prepare_v2(_database, sqlStatement.c_str(), -1, &statement, nullptr))
    {
        throw std::runtime_error(sqlite3_errmsg(_database));
    }
    return SQLiteStatement(statement);
}

void SQLiteConnection::EnableWalMode()
{
    char* errorMessage = nullptr;
    if (SQLITE_OK != sqlite3_exec(_database, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &errorMessage))
    {
        std::string error = (nullptr != errorMessage) ? errorMessage : "Unknown SQLite error enabling WAL";
        sqlite3_free(errorMessage);
        throw std::runtime_error(error);
    }
}
