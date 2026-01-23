// file SQLiteSession.cpp

#include "SQLiteSession/SQLiteSession.hpp"

#include "SQLiteSession/SQLiteConnection.hpp"

#include <sqlite3.h>

SQLiteSession::SQLiteSession(const fs::path& databasePath) : _databasePath(databasePath)
{
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
}

SQLiteSession::~SQLiteSession() = default;

SQLiteConnection& SQLiteSession::Acquire()
{
    std::thread::id threadId = std::this_thread::get_id();

    {
        std::lock_guard<std::mutex> lock(_connectionsMutex);

        auto iterator = _connections.find(threadId);
        if ((_connections.end() != iterator) && (nullptr != iterator->second))
        {
            return *iterator->second;
        }

        auto connection = std::make_unique<SQLiteConnection>(CreateConnection());
        auto result = _connections.emplace(threadId, std::move(connection));
        return *result.first->second;
    }
}

/**
 * @brief Create a new SQLite connection for the current session.
 *
 * @return New SQLiteConnection instance
 */
SQLiteConnection SQLiteSession::CreateConnection()
{
    return SQLiteConnection(_databasePath, SqliteBusyTimeoutMs);
}
