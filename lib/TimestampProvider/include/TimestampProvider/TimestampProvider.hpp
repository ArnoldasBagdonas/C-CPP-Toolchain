#pragma once

#include <string>

/**
 * @brief Infrastructure component providing timestamp strings using C time APIs.
 */
class TimestampProvider
{
  public:
    /**
     * @brief Get a filesystem-safe timestamp string.
     *
     * @return Timestamp string formatted for file names
     */
    std::string NowFilesystemSafe() const;
};
