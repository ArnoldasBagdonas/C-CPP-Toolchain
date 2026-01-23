#include "FileHasher/FileHasher.hpp"

#include <fstream>
#include <sstream>

#include <xxhash.h>

namespace
{
constexpr std::size_t FileReadBufferSize = 8192;
constexpr XXH64_hash_t HashSeed = 0;
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

    XXH64_reset(hashState, HashSeed);

    char buffer[FileReadBufferSize];
    const std::streamsize bufferSize = static_cast<std::streamsize>(sizeof(buffer));
    while (true)
    {
        inputStream.read(buffer, bufferSize);
        const std::streamsize bytesRead = inputStream.gcount();
        if (0 == bytesRead)
        {
            break;
        }
        XXH64_update(hashState, buffer, static_cast<size_t>(bytesRead));
        if (bufferSize > bytesRead)
        {
            break;
        }
    }

    uint64_t hashValue = XXH64_digest(hashState);
    XXH64_freeState(hashState);

    std::ostringstream outputStream;
    outputStream << std::hex << hashValue;
    outputHash = outputStream.str();
    return true;
}
