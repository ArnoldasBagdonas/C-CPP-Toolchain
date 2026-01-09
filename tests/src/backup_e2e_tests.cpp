/**
 * @file backup_e2e_tests.cpp
 * @brief End-to-end tests for the backup utility.
 */
#include "BackupUtility/BackupUtility.hpp"
#include "helpers/TestHelpers.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

class RunE2ETests : public ::testing::Test
{
  protected:
    fs::path sourceDir;
    fs::path backupRoot;
    fs::path dbPath;

    void SetUp() override
    {
        const auto* testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
        const std::string testName = testInfo->name();

        sourceDir = fs::temp_directory_path() / ("source_" + testName);
        backupRoot = fs::temp_directory_path() / ("backup_" + testName);
        dbPath = backupRoot / "backup.db";

        fs::remove_all(sourceDir);
        fs::remove_all(backupRoot);

        fs::create_directories(sourceDir);
        fs::create_directories(backupRoot);
    }

    void TearDown() override
    {
        std::error_code ec;
        fs::remove_all(sourceDir, ec);
        fs::remove_all(backupRoot, ec);
    }

    void CreateFile(const fs::path& filePath, const std::string& content)
    {
        fs::create_directories(filePath.parent_path());
        std::ofstream outputStream(filePath, std::ios::binary);
        ASSERT_TRUE(outputStream.good()) << "Failed to create file: " << filePath;
        outputStream << content;
    }

    std::string ReadFile(const fs::path& filePath)
    {
        std::ifstream inputStream(filePath, std::ios::binary);
        std::stringstream buffer;
        buffer << inputStream.rdbuf();
        return buffer.str();
    }
};

/* ============================================================================ */
/* ERROR HANDLING */
/* ============================================================================ */

TEST_F(RunE2ETests, RunBackup_WithNonExistentSource_ReturnsFalse)
{
    // Arrange
    BackupConfig configuration;
    configuration.sourceDir = "non_existent_dir";
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    // Act
    bool backupResult = RunBackup(configuration);

    // Assert
    ASSERT_FALSE(backupResult);

    fs::path liveBackupDir = backupRoot / "backup";
    if (fs::exists(liveBackupDir))
    {
        auto liveContents = GetDirectoryContents(liveBackupDir);
        ASSERT_THAT(liveContents, testing::IsEmpty()) << "Live backup directory should be empty for failed backup";
    }

    fs::path deletedDir = backupRoot / "deleted";
    if (fs::exists(deletedDir))
    {
        auto deletedContents = GetDirectoryContents(deletedDir);
        ASSERT_THAT(deletedContents, testing::IsEmpty()) << "Deleted directory should be empty for failed backup";
    }
}

TEST_F(RunE2ETests, RunBackup_WithEmptySource_CreatesEmptyBackup)
{
    // Arrange
    BackupConfig configuration;
    configuration.sourceDir = sourceDir;
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    // Act
    bool backupResult = RunBackup(configuration);

    // Assert
    ASSERT_TRUE(backupResult);

    fs::path liveBackupDir = backupRoot / "backup";
    bool liveBackupExists = fs::exists(liveBackupDir);
    ASSERT_TRUE(liveBackupExists);
    {
        auto liveContents = GetDirectoryContents(liveBackupDir);
        ASSERT_THAT(liveContents, testing::IsEmpty()) << "Live backup directory should be empty for empty source";
    }

    fs::path deletedDir = backupRoot / "deleted";
    bool deletedDirExists = fs::exists(deletedDir);
    ASSERT_TRUE(deletedDirExists);
    {
        auto deletedContents = GetDirectoryContents(deletedDir);
        ASSERT_THAT(deletedContents, testing::IsEmpty()) << "Deleted directory should be empty for initial backup";
    }

    bool databaseExists = fs::exists(dbPath);
    ASSERT_TRUE(databaseExists);
}

/* ============================================================================ */
/* INITIAL BACKUP */
/* ============================================================================ */

TEST_F(RunE2ETests, RunBackup_InitialBackup_CopiesAllFiles)
{
    // Arrange
    CreateFile(sourceDir / "file1.txt", "content1");
    CreateFile(sourceDir / "subdir" / "file2.txt", "content2");

    BackupConfig configuration;
    configuration.sourceDir = sourceDir;
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    // Act
    bool backupResult = RunBackup(configuration);

    // Assert
    ASSERT_TRUE(backupResult);

    fs::path liveBackupDir = backupRoot / "backup";
    bool liveBackupExists = fs::exists(liveBackupDir);
    ASSERT_TRUE(liveBackupExists);

    {
        auto liveContents = GetDirectoryContents(liveBackupDir);
        ASSERT_THAT(liveContents, testing::UnorderedElementsAreArray({"file1.txt", "subdir", "subdir/file2.txt"}))
            << "Backup directory should contain all expected files and subdirectories";

        std::string file1Content = ReadFile(liveBackupDir / "file1.txt");
        ASSERT_EQ(file1Content, "content1");

        std::string file2Content = ReadFile(liveBackupDir / "subdir" / "file2.txt");
        ASSERT_EQ(file2Content, "content2");
    }

    fs::path deletedDir = backupRoot / "deleted";
    bool deletedDirExists = fs::exists(deletedDir);
    ASSERT_TRUE(deletedDirExists);
    {
        auto deletedContents = GetDirectoryContents(deletedDir);
        ASSERT_THAT(deletedContents, testing::IsEmpty()) << "Deleted directory should be empty for initial backup";
    }
}

/* ============================================================================ */
/* INCREMENTAL BACKUPS */
/* ============================================================================ */

TEST_F(RunE2ETests, RunBackup_IncrementalBackup_TracksChanges)
{
    // Arrange
    CreateFile(sourceDir / "file1.txt", "content1");
    CreateFile(sourceDir / "file2.txt", "content2");

    BackupConfig configuration;
    configuration.sourceDir = sourceDir;
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    bool initialBackupResult = RunBackup(configuration);
    ASSERT_TRUE(initialBackupResult);

    CreateFile(sourceDir / "file1.txt", "modified content");
    CreateFile(sourceDir / "file3.txt", "new file");
    fs::remove(sourceDir / "file2.txt");

    // Act
    bool incrementalBackupResult = RunBackup(configuration);

    // Assert
    ASSERT_TRUE(incrementalBackupResult);

    fs::path liveBackupDir = backupRoot / "backup";
    {
        auto liveContents = GetDirectoryContents(liveBackupDir);
        ASSERT_THAT(liveContents, testing::UnorderedElementsAreArray({"file1.txt", "file3.txt"})) << "Live backup should contain current files only";

        std::string file1Content = ReadFile(liveBackupDir / "file1.txt");
        ASSERT_EQ(file1Content, "modified content");

        std::string file3Content = ReadFile(liveBackupDir / "file3.txt");
        ASSERT_EQ(file3Content, "new file");
    }

    fs::path deletedDir = backupRoot / "deleted";
    auto snapshotDirectories = ListDirectory(deletedDir);
    ASSERT_THAT(snapshotDirectories, testing::SizeIs(1)) << "Deleted directory should contain one snapshot";

    fs::path snapshotDir = deletedDir / snapshotDirectories[0];
    {
        auto snapshotContents = GetDirectoryContents(snapshotDir);
        ASSERT_THAT(snapshotContents, testing::UnorderedElementsAreArray({"file1.txt", "file2.txt"}))
            << "Snapshot should contain previous versions of modified and deleted files";

        std::string archivedFile1Content = ReadFile(snapshotDir / "file1.txt");
        ASSERT_EQ(archivedFile1Content, "content1");

        std::string archivedFile2Content = ReadFile(snapshotDir / "file2.txt");
        ASSERT_EQ(archivedFile2Content, "content2");
    }
}

/* ============================================================================ */
/* UNCHANGED FILES */
/* ============================================================================ */

TEST_F(RunE2ETests, RunBackup_UnchangedFile_IsNotModified)
{
    // Arrange
    CreateFile(sourceDir / "test.txt", "initial content");

    BackupConfig configuration;
    configuration.sourceDir = sourceDir;
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    bool initialBackupResult = RunBackup(configuration);
    ASSERT_TRUE(initialBackupResult);

    fs::path backupFile = backupRoot / "backup" / "test.txt";
    bool backupFileExists = fs::exists(backupFile);
    ASSERT_TRUE(backupFileExists);

    auto originalTime = fs::last_write_time(backupFile);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Act
    bool secondBackupResult = RunBackup(configuration);

    // Assert
    ASSERT_TRUE(secondBackupResult);

    auto newTime = fs::last_write_time(backupFile);
    ASSERT_EQ(originalTime, newTime);

    fs::path deletedDir = backupRoot / "deleted";
    {
        auto deletedContents = GetDirectoryContents(deletedDir);
        ASSERT_THAT(deletedContents, testing::IsEmpty()) << "Deleted directory should be empty when no files were deleted";
    }
}

/* ============================================================================ */
/* SINGLE FILE SOURCE */
/* ============================================================================ */

TEST_F(RunE2ETests, RunBackup_SingleFileSource_CreatesBackupFile)
{
    // Arrange
    fs::path sourceFilePath = sourceDir / "single.txt";
    CreateFile(sourceFilePath, "single file content");

    BackupConfig configuration;
    configuration.sourceDir = sourceFilePath;
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    // Act
    bool backupResult = RunBackup(configuration);

    // Assert
    ASSERT_TRUE(backupResult);

    fs::path backupFile = backupRoot / "backup" / "single.txt";
    {
        auto backupContents = GetDirectoryContents(backupRoot / "backup");
        ASSERT_THAT(backupContents, testing::UnorderedElementsAreArray({"single.txt"})) << "Backup directory should contain the single file";

        std::string fileContent = ReadFile(backupFile);
        ASSERT_EQ(fileContent, "single file content");
    }
}

/* ============================================================================ */
/* REPEATED DELETIONS */
/* ============================================================================ */

TEST_F(RunE2ETests, RunBackup_AlreadyDeletedFile_IsNotArchivedAgain)
{
    // Arrange
    fs::path sourceFilePath = sourceDir / "file.txt";
    CreateFile(sourceFilePath, "content");

    BackupConfig configuration;
    configuration.sourceDir = sourceDir;
    configuration.backupRoot = backupRoot;
    configuration.databaseFile = dbPath;

    bool initialBackupResult = RunBackup(configuration);
    ASSERT_TRUE(initialBackupResult);

    fs::remove(sourceFilePath);
    bool firstDeletionBackupResult = RunBackup(configuration);
    ASSERT_TRUE(firstDeletionBackupResult);

    // Act
    bool secondDeletionBackupResult = RunBackup(configuration);

    // Assert
    ASSERT_TRUE(secondDeletionBackupResult);

    fs::path deletedDir = backupRoot / "deleted";
    auto snapshotDirectories = ListDirectory(deletedDir);
    ASSERT_THAT(snapshotDirectories, testing::SizeIs(1)) << "Only one snapshot should exist for a single deletion event";

    fs::path snapshotDir = deletedDir / snapshotDirectories[0];
    bool snapshotIsDirectory = fs::is_directory(snapshotDir);
    ASSERT_TRUE(snapshotIsDirectory);
    {
        auto snapshotContents = GetDirectoryContents(snapshotDir);
        ASSERT_THAT(snapshotContents, testing::UnorderedElementsAreArray({"file.txt"})) << "Snapshot should contain the deleted file";
    }

    fs::path liveBackupDir = backupRoot / "backup";
    bool liveBackupExists = fs::exists(liveBackupDir);
    ASSERT_TRUE(liveBackupExists);
    {
        auto liveContents = GetDirectoryContents(liveBackupDir);
        ASSERT_THAT(liveContents, testing::IsEmpty()) << "Live backup directory should be empty after file deletion";
    }
}
