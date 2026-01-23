// file FileStateRepository.hpp:

#pragma once

#include "BackupUtility/BackupUtility.hpp"
#include "SQLiteSession/SQLiteSession.hpp"

#include <string>
#include <vector>

/**
 * @brief Lightweight file status entry for repository iteration.
 */
struct FileStatusEntry
{
    std::string path;
    ChangeType status;
};

/**
 * @brief Adapter for persisting file state using SQLite.
 */
class FileStateRepository
{
  public:
    explicit FileStateRepository(SQLiteSession& databaseSession);

    bool InitializeSchema();

    bool UpdateFileState(const std::string& filePath, const std::string& fileHash, ChangeType changeStatus, const std::string& timestamp);

    bool GetFileState(const std::string& filePath, std::string& outputHash, ChangeType& outputStatus, std::string& outputTimestamp);

    std::vector<FileStatusEntry> GetAllFileStatuses();

    bool MarkFileAsDeleted(const std::string& filePath, const std::string& timestamp);

  private:
    SQLiteSession& _databaseSession;
};
