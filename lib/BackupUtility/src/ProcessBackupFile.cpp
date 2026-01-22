#include "BackupUtility/ProcessBackupFile.hpp"

#include <filesystem>

ProcessBackupFile::ProcessBackupFile(const fs::path& sourceRoot, const fs::path& backupRoot, SnapshotDirectoryProvider& snapshotDirectory,
                                     FileStateRepository& fileStateRepository, const FileHasher& fileHasher,
                                     const TimestampProvider& timestampProvider, const std::function<void(const BackupProgress&)>& onProgress,
                                     std::atomic<bool>& success, std::atomic<std::size_t>& processedCount)
    : _sourceRoot(sourceRoot), _backupRoot(backupRoot), _snapshotDirectory(snapshotDirectory), _fileStateRepository(fileStateRepository),
      _fileHasher(fileHasher), _timestampProvider(timestampProvider), _onProgress(onProgress), _success(success),
      _processedCount(processedCount)
{
}

void ProcessBackupFile::Execute(const fs::path& file)
{
    std::error_code ec;
    fs::path relativePath = fs::relative(file, _sourceRoot, ec);
    if (ec)
    {
        _success.store(false);
        return;
    }
    if (std::string(".") == relativePath.string())
        relativePath = file.filename();

    fs::path backupFile = _backupRoot / relativePath;
    std::string newHash;
    if (false == _fileHasher.Compute(file, newHash))
    {
        _success.store(false);
        return;
    }

    std::string storedHash;
    std::string storedTimestamp;
    ChangeType storedStatus;
    bool hasRecord = false;
    try
    {
        hasRecord = _fileStateRepository.GetFileState(relativePath.string(), storedHash, storedStatus, storedTimestamp)
                    && (storedStatus != ChangeType::Deleted);
    }
    catch (const std::runtime_error&)
    {
        _success.store(false);
        return;
    }

    bool changed = (false == hasRecord) || (newHash != storedHash);
    ChangeType newStatus = ChangeType::Unchanged;
    std::string timestampValue = storedTimestamp;

    if (false == hasRecord)
    {
        newStatus = ChangeType::Added;
        timestampValue = _timestampProvider.NowFilesystemSafe();
        fs::create_directories(backupFile.parent_path(), ec);
        fs::copy_file(file, backupFile, fs::copy_options::overwrite_existing, ec);
    }
    else if (true == changed)
    {
        newStatus = ChangeType::Modified;
        timestampValue = _timestampProvider.NowFilesystemSafe();
        fs::path snapshotFile;
        try
        {
            snapshotFile = _snapshotDirectory.GetOrCreate() / relativePath;
        }
        catch (const std::runtime_error&)
        {
            _success.store(false);
            return;
        }
        fs::create_directories(snapshotFile.parent_path(), ec);
        fs::copy_file(backupFile, snapshotFile, fs::copy_options::overwrite_existing, ec);
        fs::copy_file(file, backupFile, fs::copy_options::overwrite_existing, ec);
    }

    try
    {
        if (false == _fileStateRepository.UpdateFileState(relativePath.string(), newHash, newStatus, timestampValue))
        {
            _success.store(false);
        }
    }
    catch (const std::runtime_error&)
    {
        _success.store(false);
        return;
    }

    if (nullptr != _onProgress)
    {
        _onProgress({"collecting", ++_processedCount, 0, file});
    }
}
