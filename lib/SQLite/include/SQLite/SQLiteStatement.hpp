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
    /**
     * @brief Construct a SQLite statement wrapper.
     *
     * @param[in] statement Raw SQLite statement pointer
     */
    explicit SQLiteStatement(sqlite3_stmt* statement);
    /**
     * @brief Finalize the SQLite statement.
     */
    ~SQLiteStatement();

    SQLiteStatement(const SQLiteStatement&) = delete;
    SQLiteStatement& operator=(const SQLiteStatement&) = delete;

    /**
     * @brief Move-construct a SQLite statement.
     *
     * @param[in] other Statement to move from
     */
    SQLiteStatement(SQLiteStatement&& other) noexcept;
    /**
     * @brief Move-assign a SQLite statement.
     *
     * @param[in] other Statement to move from
     * @return Reference to this statement
     */
    SQLiteStatement& operator=(SQLiteStatement&& other) noexcept;

    /**
     * @brief Bind a string parameter.
     *
     * @param[in] index 1-based parameter index
     * @param[in] value String value to bind
     */
    void BindText(int index, const std::string& value);
    /**
     * @brief Bind a C-string parameter.
     *
     * @param[in] index 1-based parameter index
     * @param[in] value C-string value to bind
     */
    void BindText(int index, const char* value);

    /**
     * @brief Fetch the next row from the statement.
     *
     * @return true if a row is available, false if done
     */
    bool FetchRow();
    /**
     * @brief Execute the statement to completion.
     *
     * @return true if execution completes, false if a row is returned
     */
    bool ExecuteStatement();

    /**
     * @brief Read a text column value from the current row.
     *
     * @param[in] index Zero-based column index
     * @return Column text value or empty string if null
     */
    std::string ColumnText(int index) const;

  private:
    void Finalize();

    sqlite3_stmt* _statement;
};
