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
        ("d,dry-run", "Dry run only")
        ("v,verbose", "Verbose output")
        ("h,help",    "Print help");
    
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") || !result.count("source") || !result.count("backup"))
    {
        std::cout << options.help() << '\n';
        return 0;
    }

    BackupConfig cfg;

    cfg.sourceDir = fs::path(result["source"].as<std::string>());
    cfg.backupRoot = fs::path(result["backup"].as<std::string>());
    cfg.verbose = result.count("verbose") > 0;
    cfg.dryRun = result.count("dry-run") > 0;

    // Database is always fixed inside backup folder
    cfg.databaseFile = cfg.backupRoot / "backup.db";

    // Validate source directory
    std::error_code ec;
    cfg.sourceDir = fs::canonical(cfg.sourceDir, ec);
    if (ec || !fs::is_directory(cfg.sourceDir))
    {
        std::cerr << "Invalid source directory\n";
        return 1;
    }

    // Ensure backup root exists
    fs::create_directories(cfg.backupRoot, ec);
    if (ec)
    {
        std::cerr << "Failed to create backup directory\n";
        return 1;
    }

    // Optional verbose progress
    if (cfg.verbose)
    {
        cfg.onProgress = [](const BackupProgress& p)
        { std::cout << "[" << p.stage << "] " << p.processed << "/" << p.total << " : " << p.file << '\n'; };
    }

    if (!RunBackup(cfg))
    {
        std::cerr << "Backup failed\n";
        return 1;
    }

    if (cfg.verbose)
        std::cout << "Backup completed successfully\n";

    return 0;
}
