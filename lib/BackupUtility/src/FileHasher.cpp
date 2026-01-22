#include "BackupUtility/FileHasher.hpp"

#include <fstream>
#include <sstream>

#include <xxhash.h>

namespace
{
constexpr std::size_t FileReadBufferSize = 8192;
}

bool FileHasher::Compute(const fs::path& filePath, std::string& outputHash) const
{
    std::ifstream inputStream(filePath, std::ios::binary);
    if (false == inputStream.is_open())
    {
        return false;
    }

    XXH64_state_t* hashState = XXH64_createState();
    if (nullptr == hashState)
    {
        return false;
    }

    XXH64_reset(hashState, 0);

    char buffer[FileReadBufferSize];
    while (inputStream.read(buffer, sizeof(buffer)))
    {
        XXH64_update(hashState, buffer, sizeof(buffer));
    }

    if (inputStream.gcount() > 0)
    {
        XXH64_update(hashState, buffer, static_cast<size_t>(inputStream.gcount()));
    }

    uint64_t hashValue = XXH64_digest(hashState);
    XXH64_freeState(hashState);

    std::ostringstream outputStream;
    outputStream << std::hex << hashValue;
    outputHash = outputStream.str();
    return true;
}
