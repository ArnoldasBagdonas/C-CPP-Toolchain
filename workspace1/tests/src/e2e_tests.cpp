/**
 * @file diagnostic_e2e_tests.cpp
 * @brief End-to-end tests
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <example/example.h>

class HostE2E : public ::testing::Test {
protected:
    void SetUp() override {
        // Optional: setup code
    }
    void TearDown() override {
        // Optional: teardown code
    }
};

TEST_F(HostE2E, ClampThenClassify)
{
    int clamped = clamp(999, 0, 50);
    ASSERT_EQ(1, classify(clamped));
}

TEST_F(HostE2E, FactorialOfAdd)
{
    int sum = add(2, 3);
    ASSERT_EQ(120, factorial(sum));
}

TEST_F(HostE2E, ClampNegativeThenClassify)
{
    int clamped = clamp(-10, -5, 5);
    ASSERT_EQ(-1, classify(clamped));
}
