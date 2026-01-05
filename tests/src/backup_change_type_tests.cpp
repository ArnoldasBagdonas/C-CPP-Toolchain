#include "BackupUtility/BackupUtility.hpp"
#include "gtest/gtest.h"

#include <string>
#include <vector>

TEST(ChangeTypeConversionTest, ToStringAndBack)
{
    // Arrange
    const std::vector<ChangeType> allTypes = {ChangeType::Unchanged, ChangeType::Added, ChangeType::Modified, ChangeType::Deleted};

    for (const auto& changeType : allTypes)
    {
        // Act
        const char* stringRepresentation = ChangeTypeToString(changeType);
        ChangeType convertedType = StringToChangeType(stringRepresentation);

        // Assert
        ASSERT_EQ(changeType, convertedType);
    }
}
