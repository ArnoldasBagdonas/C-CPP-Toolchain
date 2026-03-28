/**
 * @file diagnostic_unit_tests.cpp
 * @brief Unit tests
 */

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include <example/add.h>

class HostUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Optional: setup code
    }
    void TearDown() override {
        // Optional: teardown code
    }
};

TEST_F(HostUnitTest, DummyTest)
{
    // Arrange
    int expected = 5;

    // Act
    int actual = add(2, 3);

    // Assert
    ASSERT_EQ(expected, actual);
}
