#pragma once

#include <string>

/**
 * @brief Infrastructure component providing timestamp strings using C time APIs.
 */
class TimestampProvider
{
  public:
    std::string NowFilesystemSafe() const;
};
