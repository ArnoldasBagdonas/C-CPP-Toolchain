#include "gtest/gtest.h"
#include "BackupUtility/BackupUtility.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sqlite3.h>

namespace fs = std::filesystem;

class E2EBackupTest : public ::testing::Test {
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
    
    int getFileCountFromDb(const fs::path& db_path) {
        sqlite3* db;
        if (sqlite3_open(db_path.string().c_str(), &db) != SQLITE_OK) {
            return -1;
        }

        sqlite3_stmt* stmt;
        const char* sql = "SELECT count(*) FROM files WHERE status != 'Deleted';";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            sqlite3_close(db);
            return -1;
        }

        int count = 0;
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            count = sqlite3_column_int(stmt, 0);
        }

        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return count;
    }

    fs::path test_dir_;
    fs::path source_dir_;
    fs::path backup_dir_;
};

TEST_F(E2EBackupTest, InitialBackup) {
    // Arrange
    createFile(source_dir_ / "file1.txt", "content1");
    createFile(source_dir_ / "subdir" / "file2.txt", "content2");

    BackupConfig cfg;
    cfg.sourceDir = source_dir_;
    cfg.backupRoot = backup_dir_;
    cfg.databaseFile = backup_dir_ / "backup.db";

    // Act
    EXPECT_TRUE(RunBackup(cfg));

    // Assert
    EXPECT_TRUE(fs::exists(backup_dir_ / "backup" / "file1.txt"));
    EXPECT_TRUE(fs::exists(backup_dir_ / "backup" / "subdir" / "file2.txt"));
    EXPECT_EQ(readFile(backup_dir_ / "backup" / "file1.txt"), "content1");
    EXPECT_EQ(readFile(backup_dir_ / "backup" / "subdir" / "file2.txt"), "content2");
    
    EXPECT_TRUE(fs::exists(cfg.databaseFile));
    EXPECT_EQ(getFileCountFromDb(cfg.databaseFile), 2);
}

TEST_F(E2EBackupTest, IncrementalBackup) {
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
    EXPECT_EQ(readFile(backup_dir_ / "backup" / "file1.txt"), "modified content");
    EXPECT_EQ(readFile(backup_dir_ / "backup" / "file3.txt"), "new file");
    EXPECT_FALSE(fs::exists(backup_dir_ / "backup" / "file2.txt"));

    // Assert: Check history for file1's old version
    bool old_file1_found = false;
    for (const auto& entry : fs::directory_iterator(backup_dir_ / "deleted")) {
        if (fs::is_directory(entry)) {
            if (fs::exists(entry.path() / "file1.txt")) {
                EXPECT_EQ(readFile(entry.path() / "file1.txt"), "content1");
                old_file1_found = true;
            }
        }
    }
    EXPECT_TRUE(old_file1_found);
    
    // Assert: Check history for deleted file2
    bool deleted_file2_found = false;
    for (const auto& entry : fs::directory_iterator(backup_dir_ / "deleted")) {
        if (fs::is_directory(entry)) {
            if (fs::exists(entry.path() / "file2.txt")) {
                 EXPECT_EQ(readFile(entry.path() / "file2.txt"), "content2");
                deleted_file2_found = true;
            }
        }
    }
    EXPECT_TRUE(deleted_file2_found);
    
    EXPECT_EQ(getFileCountFromDb(cfg.databaseFile), 2);
}
