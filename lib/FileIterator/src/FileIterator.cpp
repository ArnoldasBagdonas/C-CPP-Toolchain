#include "FileIterator/FileIterator.hpp"

#include <filesystem>

void FileIterator::Iterate(const fs::path& path, const std::function<void(const fs::path&)>& onFile) const
{
    std::error_code errorCode;
    const bool isRegularFile = fs::is_regular_file(path, errorCode);
    if ((0 == errorCode.value()) && (true == isRegularFile))
    {
        onFile(path);
        return;
    }

    errorCode.clear();
    const bool isDirectory = fs::is_directory(path, errorCode);
    if ((0 != errorCode.value()) || (false == isDirectory))
    {
        return;
    }

    errorCode.clear();
    for (auto& entry : fs::recursive_directory_iterator(path, errorCode))
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
