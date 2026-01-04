
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

class E2ERunBackupTest : public ::testing::Test
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

    void createFile(const fs::path& path, const std::string& content)
    {
        fs::create_directories(path.parent_path());
        std::ofstream ofs(path, std::ios::binary);
        ASSERT_TRUE(ofs.good()) << "Failed to create file: " << path;
        ofs << content;
    }

    std::string readFile(const fs::path& path)
    {
        std::ifstream ifs(path, std::ios::binary);
        EXPECT_TRUE(ifs.good()) << "Failed to open file: " << path;
        std::stringstream buffer;
        buffer << ifs.rdbuf();
        return buffer.str();
    }
};

/* ============================================================================
 * ERROR HANDLING
 * ========================================================================== */

TEST_F(E2ERunBackupTest, RunBackup_WithNonExistentSource_ReturnsFalse)
{
    // Arrange
    BackupConfig cfg;
    cfg.sourceDir = "non_existent_dir";
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    bool result = RunBackup(cfg);

    // Assert
    ASSERT_FALSE(result) << "Backup should fail when source directory does not exist";

    fs::path liveBackupDir = backupRoot / "backup";
    if (fs::exists(liveBackupDir))
    {
        EXPECT_TRUE(GetDirectoryContents(liveBackupDir).empty()) << "No files should be backed up when source does not exist.";
    }

    fs::path deletedDir = backupRoot / "deleted";
    if (fs::exists(deletedDir))
    {
        EXPECT_TRUE(GetDirectoryContents(deletedDir).empty()) << "No deleted snapshots should be created on failure.";
    }
}

TEST_F(E2ERunBackupTest, RunBackup_WithEmptySource_CreatesEmptyBackup)
{
    // Arrange
    BackupConfig cfg;
    cfg.sourceDir = sourceDir; // empty
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    bool result = RunBackup(cfg);

    // Assert
    ASSERT_TRUE(result) << "Backup should succeed for empty source directory";

    fs::path liveBackupDir = backupRoot / "backup";
    ASSERT_TRUE(fs::exists(liveBackupDir)) << "Backup directory should exist";
    EXPECT_TRUE(GetDirectoryContents(liveBackupDir).empty()) << "Backup directory should be empty for empty source";

    fs::path deletedDir = backupRoot / "deleted";
    ASSERT_TRUE(fs::exists(deletedDir)) << "Deleted directory should exist";
    EXPECT_TRUE(GetDirectoryContents(deletedDir).empty()) << "Deleted directory should be empty for empty source";

    ASSERT_TRUE(fs::exists(dbPath)) << "Database file should be created even for empty backup";
}

/* ============================================================================
 * INITIAL BACKUP
 * ========================================================================== */

TEST_F(E2ERunBackupTest, RunBackup_InitialBackup_CopiesAllFiles)
{
    // Arrange
    createFile(sourceDir / "file1.txt", "content1");
    createFile(sourceDir / "subdir/file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    ASSERT_TRUE(RunBackup(cfg)) << "Initial backup should succeed";

    // Assert
    fs::path liveBackupDir = backupRoot / "backup";
    ASSERT_TRUE(fs::exists(liveBackupDir)) << "Backup directory must exist";

    EXPECT_TRUE(fs::exists(liveBackupDir / "file1.txt")) << "File1 should exist in backup";
    EXPECT_TRUE(fs::exists(liveBackupDir / "subdir/file2.txt")) << "File2 should exist in backup";

    EXPECT_EQ(readFile(liveBackupDir / "file1.txt"), "content1");
    EXPECT_EQ(readFile(liveBackupDir / "subdir/file2.txt"), "content2");

    auto backupContentsRaw = GetDirectoryContents(liveBackupDir);
    auto backupContents = NormalizePaths(backupContentsRaw);

    EXPECT_THAT(backupContents, testing::UnorderedElementsAre("file1.txt", "subdir", "subdir/file2.txt"))
        << "Backup directory should contain all expected files and subdirectories";

    fs::path deletedDir = backupRoot / "deleted";
    EXPECT_TRUE(fs::exists(deletedDir)) << "Deleted directory should always exist";
    EXPECT_TRUE(GetDirectoryContents(deletedDir).empty()) << "Deleted directory should be empty after initial backup";
}

/* ============================================================================
 * INCREMENTAL BACKUPS
 * ========================================================================== */

TEST_F(E2ERunBackupTest, RunBackup_IncrementalBackup_TracksChanges)
{
    // Arrange: initial state
    createFile(sourceDir / "file1.txt", "content1");
    createFile(sourceDir / "file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    ASSERT_TRUE(RunBackup(cfg)) << "Initial backup should succeed";

    // Arrange: modify/add/remove files
    createFile(sourceDir / "file1.txt", "modified content");
    createFile(sourceDir / "file3.txt", "new file");
    fs::remove(sourceDir / "file2.txt");

    // Act
    ASSERT_TRUE(RunBackup(cfg)) << "Incremental backup should succeed";

    // Assert: live backup state
    fs::path liveBackupDir = backupRoot / "backup";
    EXPECT_EQ(readFile(liveBackupDir / "file1.txt"), "modified content");
    EXPECT_EQ(readFile(liveBackupDir / "file3.txt"), "new file");
    EXPECT_FALSE(fs::exists(liveBackupDir / "file2.txt")) << "Deleted file should not exist in live backup";

    // Assert: deleted snapshot
    fs::path deletedDir = backupRoot / "deleted";
    auto snapshots = ListDirectory(deletedDir);
    ASSERT_THAT(snapshots, testing::SizeIs(1)) << "There should be exactly one snapshot";

    fs::path snapshotDir = deletedDir / snapshots[0];
    auto snapshotContents = GetDirectoryContents(snapshotDir);

    EXPECT_THAT(snapshotContents, testing::UnorderedElementsAre("file1.txt", "file2.txt"))
        << "Deleted snapshot should contain previous versions of changed/deleted files";

    EXPECT_EQ(readFile(snapshotDir / "file1.txt"), "content1");
    EXPECT_EQ(readFile(snapshotDir / "file2.txt"), "content2");
}

/* ============================================================================
 * UNCHANGED FILES
 * ========================================================================== */

TEST_F(E2ERunBackupTest, RunBackup_UnchangedFile_IsNotModified)
{
    // Arrange
    createFile(sourceDir / "test.txt", "initial content");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act: initial backup
    ASSERT_TRUE(RunBackup(cfg)) << "First backup should succeed";

    fs::path backupFile = backupRoot / "backup/test.txt";
    ASSERT_TRUE(fs::exists(backupFile));

    auto originalTime = fs::last_write_time(backupFile);

    // Wait to ensure file timestamp would change if modified
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Act: second backup (no changes)
    ASSERT_TRUE(RunBackup(cfg)) << "Second backup with no changes should succeed";

    auto newTime = fs::last_write_time(backupFile);

    // Assert
    EXPECT_EQ(originalTime, newTime) << "Unchanged file should not be rewritten";

    fs::path deletedDir = backupRoot / "deleted";
    EXPECT_TRUE(GetDirectoryContents(deletedDir).empty()) << "Deleted directory should remain empty for unchanged files";
}

/* ============================================================================
 * SINGLE FILE SOURCE
 * ========================================================================== */

TEST_F(E2ERunBackupTest, RunBackup_SingleFileSource_CreatesBackupFile)
{
    // Arrange
    fs::path filePath = sourceDir / "single.txt";
    createFile(filePath, "single file content");

    BackupConfig cfg;
    cfg.sourceDir = filePath;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    // Act
    ASSERT_TRUE(RunBackup(cfg)) << "Backup of a single file should succeed";

    // Assert
    fs::path backupFile = backupRoot / "backup/single.txt";
    ASSERT_TRUE(fs::exists(backupFile)) << "Backup file must exist for single file source";
    EXPECT_EQ(readFile(backupFile), "single file content");
}

/* ============================================================================
 * REPEATED DELETIONS
 * ========================================================================== */

TEST_F(E2ERunBackupTest, RunBackup_AlreadyDeletedFile_IsNotArchivedAgain)
{
    // Arrange
    fs::path filePath = sourceDir / "file.txt";
    createFile(filePath, "content");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    ASSERT_TRUE(RunBackup(cfg)) << "Initial backup should succeed";

    fs::remove(filePath);
    ASSERT_TRUE(RunBackup(cfg)) << "First deletion backup should succeed";

    // Act: run backup again after file already deleted
    ASSERT_TRUE(RunBackup(cfg)) << "Second backup should succeed and ignore already deleted files";

    // Assert
    fs::path deletedDir = backupRoot / "deleted";
    auto snapshots = ListDirectory(deletedDir);
    ASSERT_THAT(snapshots, testing::SizeIs(1)) << "Only one snapshot directory should exist";

    fs::path snapshotDir = deletedDir / snapshots[0];
    EXPECT_TRUE(fs::is_directory(snapshotDir)) << "Snapshot must be a directory";

    auto snapshotContents = GetDirectoryContents(snapshotDir);
    EXPECT_THAT(snapshotContents, testing::ElementsAre("file.txt")) << "Snapshot should contain the deleted file exactly once";
}
