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

    void ExpectBackupContents(const fs::path& dir, std::initializer_list<std::string> expected)
    {
        auto rawContents = GetDirectoryContents(dir); // returns vector<fs::path> or vector<string>
        auto contents = NormalizePaths(rawContents);   // normalize all paths to /
        EXPECT_THAT(contents, testing::UnorderedElementsAreArray(expected)) << "Backup contents mismatch in " << dir;
    }
};

/* ============================================================================ */
/* ERROR HANDLING */
/* ============================================================================ */

TEST_F(E2ERunBackupTest, RunBackup_WithNonExistentSource_ReturnsFalse)
{
    BackupConfig cfg;
    cfg.sourceDir = "non_existent_dir";
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    bool result = RunBackup(cfg);
    ASSERT_FALSE(result);

    fs::path liveBackupDir = backupRoot / "backup";
    if (fs::exists(liveBackupDir))
        ExpectBackupContents(liveBackupDir, {});

    fs::path deletedDir = backupRoot / "deleted";
    if (fs::exists(deletedDir))
        ExpectBackupContents(deletedDir, {});
}

TEST_F(E2ERunBackupTest, RunBackup_WithEmptySource_CreatesEmptyBackup)
{
    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    bool result = RunBackup(cfg);
    ASSERT_TRUE(result);

    fs::path liveBackupDir = backupRoot / "backup";
    ASSERT_TRUE(fs::exists(liveBackupDir));
    ExpectBackupContents(liveBackupDir, {});

    fs::path deletedDir = backupRoot / "deleted";
    ASSERT_TRUE(fs::exists(deletedDir));
    ExpectBackupContents(deletedDir, {});

    ASSERT_TRUE(fs::exists(dbPath));
}

/* ============================================================================ */
/* INITIAL BACKUP */
/* ============================================================================ */

TEST_F(E2ERunBackupTest, RunBackup_InitialBackup_CopiesAllFiles)
{
    createFile(sourceDir / "file1.txt", "content1");
    createFile(sourceDir / "subdir" / "file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    bool result = RunBackup(cfg);
    ASSERT_TRUE(result);

    fs::path liveBackupDir = backupRoot / "backup";
    ASSERT_TRUE(fs::exists(liveBackupDir));

    ExpectBackupContents(liveBackupDir, {"file1.txt", "subdir", "subdir/file2.txt"});

    EXPECT_EQ(readFile(liveBackupDir / "file1.txt"), "content1");
    EXPECT_EQ(readFile(liveBackupDir / "subdir" / "file2.txt"), "content2");

    fs::path deletedDir = backupRoot / "deleted";
    ASSERT_TRUE(fs::exists(deletedDir));
    ExpectBackupContents(deletedDir, {});
}

/* ============================================================================ */
/* INCREMENTAL BACKUPS */
/* ============================================================================ */

TEST_F(E2ERunBackupTest, RunBackup_IncrementalBackup_TracksChanges)
{
    createFile(sourceDir / "file1.txt", "content1");
    createFile(sourceDir / "file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    ASSERT_TRUE(RunBackup(cfg));

    createFile(sourceDir / "file1.txt", "modified content");
    createFile(sourceDir / "file3.txt", "new file");
    fs::remove(sourceDir / "file2.txt");

    bool result = RunBackup(cfg);
    ASSERT_TRUE(result);

    fs::path liveBackupDir = backupRoot / "backup";
    ExpectBackupContents(liveBackupDir, {"file1.txt", "file3.txt"});

    EXPECT_EQ(readFile(liveBackupDir / "file1.txt"), "modified content");
    EXPECT_EQ(readFile(liveBackupDir / "file3.txt"), "new file");

    fs::path deletedDir = backupRoot / "deleted";
    auto snapshots = ListDirectory(deletedDir);
    ASSERT_THAT(snapshots, testing::SizeIs(1));

    fs::path snapshotDir = deletedDir / snapshots[0];
    ExpectBackupContents(snapshotDir, {"file1.txt", "file2.txt"});

    EXPECT_EQ(readFile(snapshotDir / "file1.txt"), "content1");
    EXPECT_EQ(readFile(snapshotDir / "file2.txt"), "content2");
}

/* ============================================================================ */
/* UNCHANGED FILES */
/* ============================================================================ */

TEST_F(E2ERunBackupTest, RunBackup_UnchangedFile_IsNotModified)
{
    createFile(sourceDir / "test.txt", "initial content");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    ASSERT_TRUE(RunBackup(cfg));

    fs::path backupFile = backupRoot / "backup" / "test.txt";
    ASSERT_TRUE(fs::exists(backupFile));

    auto originalTime = fs::last_write_time(backupFile);
    std::this_thread::sleep_for(std::chrono::seconds(2));

    bool result = RunBackup(cfg);
    ASSERT_TRUE(result);

    auto newTime = fs::last_write_time(backupFile);
    EXPECT_EQ(originalTime, newTime);

    fs::path deletedDir = backupRoot / "deleted";
    ExpectBackupContents(deletedDir, {});
}

/* ============================================================================ */
/* SINGLE FILE SOURCE */
/* ============================================================================ */

TEST_F(E2ERunBackupTest, RunBackup_SingleFileSource_CreatesBackupFile)
{
    fs::path filePath = sourceDir / "single.txt";
    createFile(filePath, "single file content");

    BackupConfig cfg;
    cfg.sourceDir = filePath;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    bool result = RunBackup(cfg);
    ASSERT_TRUE(result);

    fs::path backupFile = backupRoot / "backup" / "single.txt";
    ExpectBackupContents(backupRoot / "backup", {"single.txt"});
    EXPECT_EQ(readFile(backupFile), "single file content");
}

/* ============================================================================ */
/* REPEATED DELETIONS */
/* ============================================================================ */

TEST_F(E2ERunBackupTest, RunBackup_AlreadyDeletedFile_IsNotArchivedAgain)
{
    fs::path filePath = sourceDir / "file.txt";
    createFile(filePath, "content");

    BackupConfig cfg;
    cfg.sourceDir = sourceDir;
    cfg.backupRoot = backupRoot;
    cfg.databaseFile = dbPath;

    ASSERT_TRUE(RunBackup(cfg));

    fs::remove(filePath);
    ASSERT_TRUE(RunBackup(cfg));

    bool result = RunBackup(cfg);
    ASSERT_TRUE(result);

    fs::path deletedDir = backupRoot / "deleted";
    auto snapshots = ListDirectory(deletedDir);
    ASSERT_THAT(snapshots, testing::SizeIs(1));

    fs::path snapshotDir = deletedDir / snapshots[0];
    ASSERT_TRUE(fs::is_directory(snapshotDir));
    ExpectBackupContents(snapshotDir, {"file.txt"});

    fs::path liveBackupDir = backupRoot / "backup";
    ASSERT_TRUE(fs::exists(liveBackupDir));
    ExpectBackupContents(liveBackupDir, {});
}
