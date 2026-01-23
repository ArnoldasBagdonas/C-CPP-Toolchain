#include "ProcessDeletedFiles.hpp"

#include <vector>

namespace
{
constexpr std::size_t UnknownProcessedCount = 0;
constexpr std::size_t UnknownTotalCount = 0;
}

ProcessDeletedFiles::ProcessDeletedFiles(const std::filesystem::path& sourceFolderPath, const std::filesystem::path& backupFolderPath,
                                         SnapshotDirectoryProvider& snapshotDirectory, FileStateRepository& fileStateRepository,
                                         const TimestampProvider& timestampProvider,
                                         const std::function<void(const BackupProgress&)>& onProgress)
    : _sourceFolderPath(sourceFolderPath), _backupFolderPath(backupFolderPath), _snapshotDirectory(snapshotDirectory),
      _fileStateRepository(fileStateRepository), _timestampProvider(timestampProvider), _onProgress(onProgress)
{
}

bool ProcessDeletedFiles::Execute()
{
    try
    {
        std::error_code errorCode;

        std::vector<FileStatusEntry> fileEntries;
        try
        {
            fileEntries = _fileStateRepository.GetAllFileStatuses();
        }
        catch (const std::runtime_error&)
        {
            return false;
        }

        bool operationSucceeded = true;
        for (const auto& entry : fileEntries)
        {
            const std::string& databasePath = entry.path;
            ChangeType fileStatus = entry.status;

            if (ChangeType::Deleted == fileStatus)
            {
                continue;
            }

            std::filesystem::path sourceFilePath = _sourceFolderPath / databasePath;
            std::filesystem::path currentFilePath = _backupFolderPath / databasePath;

            errorCode.clear();
            const bool sourceExists = std::filesystem::exists(sourceFilePath, errorCode);
            if ((0 != errorCode.value()) || (false == sourceExists))
            {
                std::filesystem::path snapshotPath;
                try
                {
                    snapshotPath = _snapshotDirectory.GetOrCreate();
                }
                catch (const std::runtime_error&)
                {
                    operationSucceeded = false;
                    break;
                }

                errorCode.clear();
                const bool currentExists = std::filesystem::exists(currentFilePath, errorCode);
                if ((0 == errorCode.value()) && (true == currentExists))
                {
                    std::filesystem::path archivedPath = snapshotPath / databasePath;
                    std::filesystem::create_directories(archivedPath.parent_path(), errorCode);
                    std::filesystem::copy_file(currentFilePath, archivedPath, std::filesystem::copy_options::overwrite_existing, errorCode);
                }

                std::filesystem::remove(currentFilePath, errorCode);

                try
                {
                    if (false == _fileStateRepository.MarkFileAsDeleted(databasePath, _timestampProvider.NowFilesystemSafe()))
                    {
                        operationSucceeded = false;
                        break;
                    }
                }
                catch (const std::runtime_error&)
                {
                    operationSucceeded = false;
                    break;
                }

                if (nullptr != _onProgress)
                {
                    _onProgress({"deleted", UnknownProcessedCount, UnknownTotalCount, databasePath});
                }
            }
        }

        return operationSucceeded;
    }
    catch (const std::runtime_error&)
    {
        return false;
    }
}
