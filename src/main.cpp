// file main.cpp:

#include "BackupUtility/BackupUtility.hpp"
#include "cxxopts.hpp"

#include <filesystem>
#include <iostream>
#include <optional> // Required for std::optional

namespace fs = std::filesystem;

namespace
{

/**
 * @brief Parses command-line arguments using cxxopts.
 *
 * @param[in] argc Argument count.
 * @param[in] argv Argument values.
 * @return std::optional<cxxopts::ParseResult> if parsing is successful and help is not requested,
 *         otherwise returns an empty optional (e.g., if help is shown or required args are missing).
 */
std::optional<cxxopts::ParseResult> ParseCommandLineOptions(int argc, char* argv[])
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

    if ((true == parseResult.count("help")) || (false == parseResult.count("source")) || (false == parseResult.count("backup")))
    {
        std::cout << options.help() << '\n';
        return std::nullopt;
    }

    return parseResult;
}

/**
 * @brief Sets up the BackupConfig based on parsed command-line options and validates directories.
 *
 * @param[in] parseResult The parsed command-line options.
 * @return std::optional<BackupConfig> if configuration is successful and directories are valid,
 *         otherwise returns an empty optional.
 */
std::optional<BackupConfig> SetupBackupConfiguration(const cxxopts::ParseResult& parseResult)
{
    BackupConfig config;

    config.sourceDir = fs::path(parseResult["source"].as<std::string>());
    config.backupRoot = fs::path(parseResult["backup"].as<std::string>());
    config.verbose = (0 < parseResult.count("verbose"));

    config.databaseFile = config.backupRoot / "backup.db";

    std::error_code errorCode;
    config.sourceDir = fs::canonical(config.sourceDir, errorCode);
    if ((true == errorCode.value()) || (false == fs::is_directory(config.sourceDir)))
    {
        std::cerr << "Invalid source directory\n";
        return std::nullopt;
    }

    fs::create_directories(config.backupRoot, errorCode);
    if (true == errorCode.value())
    {
        std::cerr << "Failed to create backup directory\n";
        return std::nullopt;
    }

    if (true == config.verbose)
    {
        config.onProgress = [](const BackupProgress& progress)
        { std::cout << "[" << progress.stage << "] " << progress.processed << "/" << progress.total << " : " << progress.file << '\n'; };
    }

    return config;
}

} // namespace

int main(int argc, char* argv[])
{
    std::optional<cxxopts::ParseResult> parseResult = ParseCommandLineOptions(argc, argv);

    if (false == parseResult.has_value())
    {
        return 0; // Help was shown or invalid arguments, exit gracefully.
    }

    std::optional<BackupConfig> backupConfiguration = SetupBackupConfiguration(parseResult.value());

    if (false == backupConfiguration.has_value())
    {
        return 1; // Configuration failed, error message already printed.
    }

    if (false == RunBackup(backupConfiguration.value()))
    {
        std::cerr << "Backup failed\n";
        return 1;
    }

    if (true == backupConfiguration.value().verbose)
    {
        std::cout << "Backup completed successfully\n";
    }

    return 0;
}
