/**
 * @file diagnostic_e2e_tests.cpp
 * @brief End-to-end tests
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <example/add.h>

class HostE2E : public ::testing::Test {
protected:
    void SetUp() override {
        // Optional: setup code
    }
    void TearDown() override {
        // Optional: teardown code
    }
};

TEST_F(HostE2E, DummyE2ETest)
{
    // Arrange
    int expected = 5;

    // Act
    int actual = add(2, 3);

    // Assert
    ASSERT_EQ(expected, actual);
}
