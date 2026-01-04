#include "BackupUtility/BackupUtility.hpp"
#include "gtest/gtest.h"
#include <gmock/gmock.h>
#include "helpers/TestHelpers.hpp"
#include <filesystem>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

class E2EBackupExtendedTest : public ::testing::Test
{
protected:
    fs::path sourceDir;
    fs::path backupRoot;
    fs::path dbPath;

    void SetUp() override
    {
        const auto* const test_info = ::testing::UnitTest::GetInstance()->current_test_info();
        const std::string test_name = test_info->name();

        sourceDir = fs::temp_directory_path() / ("source_more_" + test_name);
        backupRoot = fs::temp_directory_path() / ("backup_more_" + test_name);
        dbPath = backupRoot / "backup.db";

        // Clean up from previous runs if they failed
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

    void createFile(const fs::path &path, const std::string &content)
    {
        fs::create_directories(path.parent_path());
        std::ofstream(path) << content;
    }

    std::string readFile(const fs::path &path)
    {
        std::ifstream file(path);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
};

TEST_F(E2EBackupExtendedTest, RunBackup_WithNonExistentSource_ReturnsFalse)
{
    // Arrange
    BackupConfig cfg;
    cfg.sourceDir = "non_existent_dir";
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    bool result = RunBackup(cfg);

    // Assert
    ASSERT_FALSE(result);
}

#include <thread>
#include <chrono>

TEST_F(E2EBackupExtendedTest, RunBackup_UnchangedFile_DoesNotGetModified)
{
    // Arrange
    fs::path testFile = sourceDir / "test.txt";
    createFile(testFile, "initial content");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    ASSERT_TRUE(RunBackup(cfg)); // First backup

    fs::path backupFile = backupRoot / "backup" / "test.txt";
    ASSERT_TRUE(fs::exists(backupFile));
    auto lastWriteTime = fs::last_write_time(backupFile);

    // Make sure enough time passes for a different timestamp
    std::this_thread::sleep_for(std::chrono::seconds(2));

    ASSERT_TRUE(RunBackup(cfg)); // Second backup with no changes

    auto newLastWriteTime = fs::last_write_time(backupFile);

    // Assert
    fs::path historyDir = backupRoot / "deleted";
    ASSERT_TRUE(fs::exists(historyDir)); // The directory is always created.

    auto history_items = GetDirectoryContents(historyDir);
    EXPECT_THAT(history_items, testing::IsEmpty()) << "History directory should be empty for an unchanged file backup.";

    ASSERT_EQ(lastWriteTime, newLastWriteTime); // File should not have been touched
}

TEST_F(E2EBackupExtendedTest, RunBackup_AlreadyDeletedFile_IsIgnored)
{
    // Arrange
    fs::path testFile = sourceDir / "test.txt";
    createFile(testFile, "content");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    ASSERT_TRUE(RunBackup(cfg)); // Initial backup

    fs::remove(testFile);
    ASSERT_TRUE(RunBackup(cfg)); // Deletes the file from backup and archives it

    // Act
    ASSERT_TRUE(RunBackup(cfg)); // Should ignore the already deleted file

    // Assert
    fs::path historyDir = backupRoot / "deleted";
    auto snapshots = ListDirectory(historyDir);
    ASSERT_THAT(snapshots, testing::SizeIs(1)) << "There should be exactly one snapshot directory.";

    fs::path snapshotDir = historyDir / snapshots[0];
    ASSERT_TRUE(fs::is_directory(snapshotDir));

    auto snapshot_contents = GetDirectoryContents(snapshotDir);
    EXPECT_THAT(snapshot_contents, testing::ElementsAre("test.txt"))
        << "The snapshot directory should contain the deleted file.";
}

TEST_F(E2EBackupExtendedTest, RunBackup_SingleFileBackup_CreatesBackup)
{
    // Arrange
    fs::path testFile = sourceDir / "single_file.txt";
    createFile(testFile, "this is a single file");

    BackupConfig cfg;
    cfg.sourceDir = testFile;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    ASSERT_TRUE(RunBackup(cfg));

    // Assert
    fs::path backupFile = backupRoot / "backup" / "single_file.txt";
    ASSERT_TRUE(fs::exists(backupFile));
    ASSERT_EQ(readFile(backupFile), "this is a single file");
}
