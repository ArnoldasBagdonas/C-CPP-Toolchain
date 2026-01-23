#include "FileIterator/FileIterator.hpp"

#include <filesystem>

void FileIterator::Iterate(const std::filesystem::path& path,
                           const std::function<void(const std::filesystem::path&)>& onFile) const
{
    std::error_code errorCode;
    const bool isRegularFile = std::filesystem::is_regular_file(path, errorCode);
    if ((0 == errorCode.value()) && (true == isRegularFile))
    {
        onFile(path);
        return;
    }

    errorCode.clear();
    const bool isDirectory = std::filesystem::is_directory(path, errorCode);
    if ((0 != errorCode.value()) || (false == isDirectory))
    {
        return;
    }

    errorCode.clear();
    for (auto& entry : std::filesystem::recursive_directory_iterator(path, errorCode))
    {
        if (0 != errorCode.value())
        {
            break;
        }

        const bool entryIsFile = entry.is_regular_file(errorCode);
        if ((0 == errorCode.value()) && (true == entryIsFile))
        {
            onFile(entry.path());
        }
    }
}
