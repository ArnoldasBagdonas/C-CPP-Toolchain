#include "TimestampProvider/TimestampProvider.hpp"

#include <ctime>

std::string TimestampProvider::NowFilesystemSafe() const
{
    std::time_t currentTime = std::time(nullptr);
    std::tm timeStruct{};

#ifdef _WIN32
    _localtime64_s(&timeStruct, &currentTime);
#else
    localtime_r(&currentTime, &timeStruct);
#endif

    char buffer[32];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", &timeStruct);
    return buffer;
}
