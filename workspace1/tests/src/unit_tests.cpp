/**
 * @file diagnostic_unit_tests.cpp
 * @brief Unit tests
 */

#include "gtest/gtest.h"

#include <string>
#include <vector>

#include <example/example.h>

class HostUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Optional: setup code
    }
    void TearDown() override {
        // Optional: teardown code
    }
};

// ---------------------------------------------------------------------------
// add()
// ---------------------------------------------------------------------------
TEST_F(HostUnitTest, AddPositive)
{
    ASSERT_EQ(5, add(2, 3));
}

TEST_F(HostUnitTest, AddNegative)
{
    ASSERT_EQ(-1, add(2, -3));
}

// ---------------------------------------------------------------------------
// clamp() — three branches: below min, above max, in range
// ---------------------------------------------------------------------------
TEST_F(HostUnitTest, ClampBelowMin)
{
    ASSERT_EQ(0, clamp(-5, 0, 100));
}

TEST_F(HostUnitTest, ClampAboveMax)
{
    ASSERT_EQ(100, clamp(200, 0, 100));
}

TEST_F(HostUnitTest, ClampInRange)
{
    ASSERT_EQ(50, clamp(50, 0, 100));
}

// ---------------------------------------------------------------------------
// classify() — four branches: negative, zero, small positive, large positive
// ---------------------------------------------------------------------------
TEST_F(HostUnitTest, ClassifyNegative)
{
    ASSERT_EQ(-1, classify(-42));
}

TEST_F(HostUnitTest, ClassifyZero)
{
    ASSERT_EQ(0, classify(0));
}

TEST_F(HostUnitTest, ClassifySmallPositive)
{
    ASSERT_EQ(1, classify(50));
}

TEST_F(HostUnitTest, ClassifyLargePositive)
{
    ASSERT_EQ(2, classify(200));
}

// ---------------------------------------------------------------------------
// factorial() — three paths: negative input, base case, recursive case
// ---------------------------------------------------------------------------
TEST_F(HostUnitTest, FactorialNegative)
{
    ASSERT_EQ(-1, factorial(-1));
}

TEST_F(HostUnitTest, FactorialZero)
{
    ASSERT_EQ(1, factorial(0));
}

TEST_F(HostUnitTest, FactorialPositive)
{
    ASSERT_EQ(120, factorial(5));
}
