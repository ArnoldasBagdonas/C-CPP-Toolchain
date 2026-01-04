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

