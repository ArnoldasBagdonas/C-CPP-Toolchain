#include "BackupUtility/BackupUtility.hpp"
#include "gtest/gtest.h"

#include <string>
#include <vector>

TEST(ChangeTypeConversionTest, ToStringAndBack)
{
    // Arrange
    const std::vector<ChangeType> allTypes = {
        ChangeType::Unchanged, ChangeType::Added, ChangeType::Modified, ChangeType::Deleted};

    for (const auto &type : allTypes)
    {
        // Act
        const char *str = ChangeTypeToString(type);
        ChangeType convertedType = StringToChangeType(str);

        // Assert
        ASSERT_EQ(type, convertedType);
    }
}

TEST(ChangeTypeConversionTest, StringToChangeType_UnknownString_ReturnsUnchanged)
{
    // Arrange
    std::string unknown = "unknown_change_type";

    // Act
    ChangeType type = StringToChangeType(unknown);

    // Assert
    ASSERT_EQ(ChangeType::Unchanged, type);
}
