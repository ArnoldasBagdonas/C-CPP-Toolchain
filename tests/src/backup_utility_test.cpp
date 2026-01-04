#include "gtest/gtest.h"
#include "BackupUtility/BackupUtility.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

class BackupUtilityTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = fs::temp_directory_path() / "backup_utility_test";
        fs::create_directories(test_dir_);

        source_dir_ = test_dir_ / "source";
        backup_dir_ = test_dir_ / "backup";
        fs::create_directory(source_dir_);
    }

    void TearDown() override {
        fs::remove_all(test_dir_);
    }

    fs::path test_dir_;
    fs::path source_dir_;
    fs::path backup_dir_;
};

TEST_F(BackupUtilityTest, TestDummyPlaceholder) {
    EXPECT_TRUE(true);
}
