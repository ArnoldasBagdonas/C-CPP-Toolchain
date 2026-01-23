// file FileStateRepository.hpp:

#pragma once

#include "BackupUtility/BackupUtility.hpp"
#include "SQLite/SQLiteSession.hpp"

#include <string>
#include <vector>

/**
 * @brief Lightweight file status entry for repository iteration.
 */
struct FileStatusEntry
{
  std::string path; /**< Repository-relative file path */
  ChangeType status; /**< Stored change status for the file */
};

/**
 * @brief Adapter for persisting file state using SQLite.
 */
class FileStateRepository
{
  public:
    /**
     * @brief Create a repository bound to a SQLite session.
     *
     * @param[in] databaseSession Active SQLite session for persistence
     */
    explicit FileStateRepository(SQLiteSession& databaseSession);

    /**
     * @brief Create required database schema if it does not exist.
     *
     * @return true on success, false on error
     */
    bool InitializeSchema();

    /**
     * @brief Insert or update file state in the database.
     *
     * @param[in] filePath Repository-relative file path
     * @param[in] fileHash Content hash for the file
     * @param[in] changeStatus Change status to store
     * @param[in] timestamp Timestamp string for last update
     * @return true on success, false on error
     */
    bool UpdateFileState(const std::string& filePath, const std::string& fileHash, ChangeType changeStatus, const std::string& timestamp);

    /**
     * @brief Retrieve file state from the database.
     *
     * @param[in] filePath Repository-relative file path
     * @param[out] outputHash Stored file hash
     * @param[out] outputStatus Stored change status
     * @param[out] outputTimestamp Stored timestamp string
     * @return true if a record is found, false otherwise
     */
    bool GetFileState(const std::string& filePath, std::string& outputHash, ChangeType& outputStatus, std::string& outputTimestamp);

    /**
     * @brief Retrieve all stored file status entries.
     *
     * @return Collection of file status entries
     */
    std::vector<FileStatusEntry> GetAllFileStatuses();

    /**
     * @brief Mark a file as deleted in the database.
     *
     * @param[in] filePath Repository-relative file path
     * @param[in] timestamp Timestamp string for deletion
     * @return true on success, false on error
     */
    bool MarkFileAsDeleted(const std::string& filePath, const std::string& timestamp);

  private:
    SQLiteSession& _databaseSession;
};
