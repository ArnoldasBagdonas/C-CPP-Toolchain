// file main.cpp:

#include "BackupUtility/BackupUtility.hpp"
#include "cxxopts.hpp"

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    cxxopts::Options options("rdemo-backup", "Demo Backup Utility");

    // clang-format off

    options.add_options()
        ("s,source",  "Source directory", cxxopts::value<std::string>())
        ("b,backup",  "Backup directory", cxxopts::value<std::string>())
        ("v,verbose", "Verbose output")
        ("h,help",    "Print help");

    // clang-format on

    auto parseResult = options.parse(argc, argv);

    if ((parseResult.count("help")) || (!parseResult.count("source")) || (!parseResult.count("backup")))
    {
        std::cout << options.help() << '\n';
        return 0;
    }

    BackupConfig backupConfiguration;

    backupConfiguration.sourceDir = fs::path(parseResult["source"].as<std::string>());
    backupConfiguration.backupRoot = fs::path(parseResult["backup"].as<std::string>());
    backupConfiguration.verbose = (0 < parseResult.count("verbose"));

    backupConfiguration.databaseFile = backupConfiguration.backupRoot / "backup.db";

    std::error_code errorCode;
    backupConfiguration.sourceDir = fs::canonical(backupConfiguration.sourceDir, errorCode);
    if ((errorCode) || (!fs::is_directory(backupConfiguration.sourceDir)))
    {
        std::cerr << "Invalid source directory\n";
        return 1;
    }

    fs::create_directories(backupConfiguration.backupRoot, errorCode);
    if (errorCode)
    {
        std::cerr << "Failed to create backup directory\n";
        return 1;
    }

    if (backupConfiguration.verbose)
    {
        backupConfiguration.onProgress = [](const BackupProgress& progress)
        { std::cout << "[" << progress.stage << "] " << progress.processed << "/" << progress.total << " : " << progress.file << '\n'; };
    }

    if (!RunBackup(backupConfiguration))
    {
        std::cerr << "Backup failed\n";
        return 1;
    }

    if (backupConfiguration.verbose)
    {
        std::cout << "Backup completed successfully\n";
    }

    return 0;
}
