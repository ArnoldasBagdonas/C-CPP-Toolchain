// file BackupUtility.hpp:

#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace fs = std::filesystem;

/**
 * @brief Enumeration of possible file change states during backup operations.
 */
enum class ChangeType
{
    Unchanged,  /**< File has not changed since last backup */
    Added,      /**< File is new and was not present in previous backup */
    Modified,   /**< File exists but content has changed */
    Deleted     /**< File was present before but has been removed */
};

/**
 * @brief Convert a ChangeType enumeration value to its string representation.
 *
 * @param[in] changeType The change type to convert
 * @return String representation of the change type
 */
inline const char* ChangeTypeToString(ChangeType changeType)
{
    switch (changeType)
    {
    case ChangeType::Unchanged:
        return "Unchanged";
    case ChangeType::Added:
        return "Added";
    case ChangeType::Modified:
        return "Modified";
    case ChangeType::Deleted:
        return "Deleted";
    }
    return "Unknown";
}

/**
 * @brief Convert a string to its corresponding ChangeType enumeration value.
 *
 * @param[in] stringValue The string to convert
 * @return Corresponding ChangeType value, defaults to Unchanged if string is not recognized
 */
inline ChangeType StringToChangeType(const std::string& stringValue)
{
    if ("Unchanged" == stringValue)
    {
        return ChangeType::Unchanged;
    }
    if ("Added" == stringValue)
    {
        return ChangeType::Added;
    }
    if ("Modified" == stringValue)
    {
        return ChangeType::Modified;
    }
    if ("Deleted" == stringValue)
    {
        return ChangeType::Deleted;
    }
    return ChangeType::Unchanged;
}

/**
 * @brief Progress information for backup operations.
 */
struct BackupProgress
{
    const char* stage;          /**< Current stage of backup operation */
    std::size_t processed;      /**< Number of items processed so far */
    std::size_t total;          /**< Total number of items to process */
    fs::path file;              /**< Currently processing file path */
};

/**
 * @brief Configuration parameters for backup operations.
 */
struct BackupConfig
{
    fs::path sourceDir;         /**< Source directory to back up */
    fs::path backupRoot;        /**< Root directory for backup storage */
    fs::path databaseFile;      /**< Path to SQLite database file for tracking state */

    bool verbose;               /**< Enable verbose progress output */

    std::function<void(const BackupProgress&)> onProgress;  /**< Optional callback for progress notifications */

    /**
     * @brief Initialize configuration with default values.
     */
    BackupConfig()
        : verbose(false)
        , onProgress(nullptr)
    {
    }
};

/**
 * @brief Execute a backup operation based on provided configuration.
 *
 * This function performs an incremental backup of the source directory,
 * tracking file changes, archiving modified or deleted files, and maintaining
 * backup state in a SQLite database.
 *
 * @param[in] configuration Configuration parameters for the backup operation
 * @return true if backup completed successfully, false on error
 */
bool RunBackup(const BackupConfig& configuration);
