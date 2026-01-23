#include "FileIterator/FileIterator.hpp"

#include <filesystem>

void FileIterator::Iterate(const fs::path& path, const std::function<void(const fs::path&)>& onFile) const
{
    std::error_code ec;
    if (true == fs::is_regular_file(path, ec))
    {
        onFile(path);
        return;
    }

    if (true == fs::is_directory(path, ec))
    {
        for (auto& entry : fs::recursive_directory_iterator(path, ec))
        {
            if (!ec && entry.is_regular_file(ec))
            {
                onFile(entry.path());
            }
        }
    }
}
