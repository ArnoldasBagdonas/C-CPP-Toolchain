// file SQLiteStatement.cpp

#include "SQLiteSession/SQLiteStatement.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <utility>

namespace
{
std::runtime_error MakeSqliteError(sqlite3_stmt* statement, const std::string& prefix)
{
    sqlite3* database = sqlite3_db_handle(statement);
    const char* message = (nullptr != database) ? sqlite3_errmsg(database) : "Unknown SQLite error";
    return std::runtime_error(prefix + message);
}
}

SQLiteStatement::SQLiteStatement(sqlite3_stmt* statement) : _statement(statement)
{
    if (nullptr == _statement)
    {
        throw std::runtime_error("SQLite statement is null.");
    }
}

SQLiteStatement::~SQLiteStatement()
{
    Finalize();
}

SQLiteStatement::SQLiteStatement(SQLiteStatement&& other) noexcept : _statement(std::exchange(other._statement, nullptr))
{
}

SQLiteStatement& SQLiteStatement::operator=(SQLiteStatement&& other) noexcept
{
    if (this != &other)
    {
        Finalize();
        _statement = std::exchange(other._statement, nullptr);
    }
    return *this;
}

void SQLiteStatement::BindText(int index, const std::string& value)
{
    if (SQLITE_OK != sqlite3_bind_text(_statement, index, value.c_str(), -1, SQLITE_TRANSIENT))
    {
        throw MakeSqliteError(_statement, "Failed to bind text parameter: ");
    }
}

void SQLiteStatement::BindText(int index, const char* value)
{
    if (SQLITE_OK != sqlite3_bind_text(_statement, index, value, -1, SQLITE_TRANSIENT))
    {
        throw MakeSqliteError(_statement, "Failed to bind text parameter: ");
    }
}

bool SQLiteStatement::FetchRow()
{
    int result = sqlite3_step(_statement);
    if (SQLITE_ROW == result)
    {
        return true;
    }
    if (SQLITE_DONE == result)
    {
        return false;
    }
    throw MakeSqliteError(_statement, "Failed to step statement: ");
}

bool SQLiteStatement::ExecuteStatement()
{
    int result = sqlite3_step(_statement);
    if (SQLITE_DONE == result)
    {
        return true;
    }
    if (SQLITE_ROW == result)
    {
        return false;
    }
    throw MakeSqliteError(_statement, "Failed to execute statement: ");
}

std::string SQLiteStatement::ColumnText(int index) const
{
    const unsigned char* value = sqlite3_column_text(_statement, index);
    if (nullptr == value)
    {
        return {};
    }
    return reinterpret_cast<const char*>(value);
}

void SQLiteStatement::Finalize()
{
    if (nullptr != _statement)
    {
        sqlite3_finalize(_statement);
        _statement = nullptr;
    }
}
