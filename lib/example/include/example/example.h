#ifndef EXAMPLE_H
#define EXAMPLE_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Adds two integers.
 *
 * @param a First integer
 * @param b Second integer
 * @return Sum of a and b
 */
int add(int a, int b);

/**
 * @brief Clamp a value to the range [min, max].
 *
 * @param value Value to clamp
 * @param min   Lower bound
 * @param max   Upper bound
 * @return Clamped value
 */
int clamp(int value, int min, int max);

/**
 * @brief Classify an integer value.
 *
 * @param value Value to classify
 * @return -1 for negative, 0 for zero, 1 for small positive (1-100), 2 for large positive (>100)
 */
int classify(int value);

/**
 * @brief Compute factorial of n.
 *
 * @param n Non-negative integer
 * @return n! on success, -1 if n is negative
 */
int factorial(int n);

#ifdef __cplusplus
}
#endif

#endif // EXAMPLE_H
