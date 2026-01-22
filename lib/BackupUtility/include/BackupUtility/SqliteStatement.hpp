// file SQLiteStatement.hpp:

#pragma once

#include <string>

struct sqlite3_stmt;

/**
 * @brief RAII wrapper for a prepared SQLite statement.
 */
class SQLiteStatement
{
  public:
    explicit SQLiteStatement(sqlite3_stmt* statement);
    ~SQLiteStatement();

    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;

    SQLiteStatement(SQLiteStatement&& other) noexcept;
    SQLiteStatement& operator=(SQLiteStatement&& other) noexcept;

    void BindText(int index, const std::string& value);
    void BindText(int index, const char* value);

    bool FetchRow();
    bool ExecuteStatement();

    std::string ColumnText(int index) const;

  private:
    void Finalize();

    sqlite3_stmt* _statement;
};
