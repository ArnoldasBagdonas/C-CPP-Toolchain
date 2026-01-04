#include "gtest/gtest.h"
#include "BackupUtility/BackupUtility.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

class E2EBackupLifecycleTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "e2e_backup_test";
        fs::remove_all(test_dir_); // Ensure clean state
        fs::create_directories(test_dir_);

        source_dir_ = test_dir_ / "source";
        backup_dir_ = test_dir_ / "backup";
        fs::create_directories(source_dir_);
        fs::create_directories(backup_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    void createFile(const fs::path& path, const std::string& content) {
        fs::create_directories(path.parent_path());
        std::ofstream ofs(path);
        ofs << content;
    }

    std::string readFile(const fs::path& path) {
        std::ifstream ifs(path);
        return std::string((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    }

    // Helper to count items in a directory
    size_t countItemsInDir(const fs::path& dir_path) {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            return 0;
        }
        auto it = fs::directory_iterator(dir_path);
        return std::distance(fs::begin(it), fs::end(it));
    }

    // Helper to recursively count files in a directory
    size_t countFilesRecursive(const fs::path& dir_path) {
        if (!fs::exists(dir_path) || !fs::is_directory(dir_path)) {
            return 0;
        }
        size_t count = 0;
        for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
            if (entry.is_regular_file()) {
                count++;
            }
        }
        return count;
    }

    fs::path test_dir_;
    fs::path source_dir_;
    fs::path backup_dir_;
};

TEST_F(E2EBackupLifecycleTest, InitialBackup) {
    // Arrange
    createFile(source_dir_ / "file1.txt", "content1");
    createFile(source_dir_ / "subdir" / "file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = source_dir_;
    cfg.backupRoot = backup_dir_;
    cfg.databaseFile = backup_dir_ / "backup.db";

    // Act
    ASSERT_TRUE(RunBackup(cfg));

    // Assert
    fs::path live_backup_dir = backup_dir_ / "backup";
    EXPECT_TRUE(fs::exists(live_backup_dir / "file1.txt"));
    EXPECT_TRUE(fs::exists(live_backup_dir / "subdir" / "file2.txt"));
    EXPECT_EQ(readFile(live_backup_dir / "file1.txt"), "content1");
    EXPECT_EQ(readFile(live_backup_dir / "subdir" / "file2.txt"), "content2");

    EXPECT_TRUE(fs::exists(cfg.databaseFile));
    EXPECT_EQ(countFilesRecursive(live_backup_dir), 2);

    // Assert: No files should be in the deleted folder
    EXPECT_TRUE(fs::exists(backup_dir_ / "deleted"));
    EXPECT_EQ(countItemsInDir(backup_dir_ / "deleted"), 0);
}

TEST_F(E2EBackupLifecycleTest, IncrementalBackup) {
    // Arrange: Initial backup
    createFile(source_dir_ / "file1.txt", "content1");
    createFile(source_dir_ / "file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = source_dir_;
    cfg.backupRoot = backup_dir_;
    cfg.databaseFile = backup_dir_ / "backup.db";

    ASSERT_TRUE(RunBackup(cfg));

    // Arrange: Modify a file, add a file, remove a file
    createFile(source_dir_ / "file1.txt", "modified content");
    createFile(source_dir_ / "file3.txt", "new file");
    fs::remove(source_dir_ / "file2.txt");

    // Act
    ASSERT_TRUE(RunBackup(cfg));

    // Assert: Check backup state
    fs::path live_backup_dir = backup_dir_ / "backup";
    EXPECT_EQ(readFile(live_backup_dir / "file1.txt"), "modified content");
    EXPECT_EQ(readFile(live_backup_dir / "file3.txt"), "new file");
    EXPECT_FALSE(fs::exists(live_backup_dir / "file2.txt"));
    EXPECT_EQ(countFilesRecursive(live_backup_dir), 2);

    // Assert: Check history folder state
    fs::path deleted_dir = backup_dir_ / "deleted";
    EXPECT_EQ(countItemsInDir(deleted_dir), 1) << "There should be exactly one snapshot directory in deleted/";
    
    fs::path snapshot_dir;
    for(const auto& entry : fs::directory_iterator(deleted_dir)) {
        if (entry.is_directory()) {
            snapshot_dir = entry.path();
        }
    }
    ASSERT_FALSE(snapshot_dir.empty());

    EXPECT_EQ(countItemsInDir(snapshot_dir), 2) << "Snapshot directory should contain exactly two items.";
    
    EXPECT_TRUE(fs::exists(snapshot_dir / "file1.txt"));
    EXPECT_EQ(readFile(snapshot_dir / "file1.txt"), "content1");

    EXPECT_TRUE(fs::exists(snapshot_dir / "file2.txt"));
    EXPECT_EQ(readFile(snapshot_dir / "file2.txt"), "content2");
}

