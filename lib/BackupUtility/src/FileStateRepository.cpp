// file FileStateRepository.cpp

#include "FileStateRepository.hpp"

#include "SQLiteSession/SQLiteConnection.hpp"

#include <stdexcept>

namespace
{
constexpr const char* SqlCreateFilesTable = "CREATE TABLE IF NOT EXISTS files ("
                                            "path TEXT PRIMARY KEY,"
                                            "hash TEXT NOT NULL,"
                                            "last_updated TEXT NOT NULL,"
                                            "status TEXT NOT NULL);";
}

FileStateRepository::FileStateRepository(SQLiteSession& databaseSession) : _databaseSession(databaseSession)
{
}

bool FileStateRepository::InitializeSchema()
{
    try
    {
        _databaseSession.Acquire().Execute(SqlCreateFilesTable);
        return true;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}

bool FileStateRepository::UpdateFileState(const std::string& filePath, const std::string& fileHash, ChangeType changeStatus,
                                          const std::string& timestamp)
{
    try
    {
        auto& connection = _databaseSession.Acquire();
        auto statement = connection.Prepare("INSERT INTO files(path, hash, status, last_updated) "
                                            "VALUES(?1, ?2, ?3, ?4) "
                                            "ON CONFLICT(path) DO UPDATE SET "
                                            "hash=excluded.hash, status=excluded.status, last_updated=excluded.last_updated;");

        statement.BindText(1, filePath);
        statement.BindText(2, fileHash);
        statement.BindText(3, ChangeTypeToString(changeStatus));
        statement.BindText(4, timestamp);

        return statement.ExecuteStatement();
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}

bool FileStateRepository::GetFileState(const std::string& filePath, std::string& outputHash, ChangeType& outputStatus,
                                      std::string& outputTimestamp)
{
    try
    {
        auto& connection = _databaseSession.Acquire();
        auto statement = connection.Prepare("SELECT hash, status, last_updated FROM files WHERE path=?1;");

        statement.BindText(1, filePath);

        if (false == statement.FetchRow())
        {
            return false;
        }

        const std::string hashText = statement.ColumnText(0);
        const std::string statusText = statement.ColumnText(1);
        const std::string timestampText = statement.ColumnText(2);

        if (hashText.empty() || statusText.empty() || timestampText.empty())
        {
            return false;
        }

        outputHash = hashText;
        outputStatus = StringToChangeType(statusText);
        outputTimestamp = timestampText;
        return true;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}

std::vector<FileStatusEntry> FileStateRepository::GetAllFileStatuses()
{
    auto& connection = _databaseSession.Acquire();
    auto statement = connection.Prepare("SELECT path, status FROM files;");

    std::vector<FileStatusEntry> results;
    while (statement.FetchRow())
    {
        const std::string pathText = statement.ColumnText(0);
        const std::string statusText = statement.ColumnText(1);

        if (pathText.empty() || statusText.empty())
        {
            continue;
        }

        results.push_back({pathText, StringToChangeType(statusText)});
    }

    return results;
}

bool FileStateRepository::MarkFileAsDeleted(const std::string& filePath, const std::string& timestamp)
{
    try
    {
        auto& connection = _databaseSession.Acquire();
        auto statement = connection.Prepare("UPDATE files SET status=?1, last_updated=?2 WHERE path=?3;");

        statement.BindText(1, ChangeTypeToString(ChangeType::Deleted));
        statement.BindText(2, timestamp);
        statement.BindText(3, filePath);

        return statement.ExecuteStatement();
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}
