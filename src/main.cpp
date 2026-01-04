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

    auto result = options.parse(argc, argv);

    if ((result.count("help")) || (!result.count("source")) || (!result.count("backup")))
    {
        std::cout << options.help() << '\n';
        return 0;
    }

    BackupConfig configuration;

    configuration.sourceDir = fs::path(result["source"].as<std::string>());
    configuration.backupRoot = fs::path(result["backup"].as<std::string>());
    configuration.verbose = (result.count("verbose") > 0);

    configuration.databaseFile = configuration.backupRoot / "backup.db";

    std::error_code errorCode;
    configuration.sourceDir = fs::canonical(configuration.sourceDir, errorCode);
    if ((errorCode) || (!fs::is_directory(configuration.sourceDir)))
    {
        std::cerr << "Invalid source directory\n";
        return 1;
    }

    fs::create_directories(configuration.backupRoot, errorCode);
    if (errorCode)
    {
        std::cerr << "Failed to create backup directory\n";
        return 1;
    }

    if (configuration.verbose)
    {
        configuration.onProgress = [](const BackupProgress& progress)
        {
            std::cout << "[" << progress.stage << "] " << progress.processed << "/" << progress.total << " : " << progress.file << '\n';
        };
    }

    if (!RunBackup(configuration))
    {
        std::cerr << "Backup failed\n";
        return 1;
    }

    if (configuration.verbose)
    {
        std::cout << "Backup completed successfully\n";
    }

    return 0;
}
