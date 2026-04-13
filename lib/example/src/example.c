#include <example/example.h>

int add(int a, int b) {
    return a + b;
}

int clamp(int value, int min, int max) {
    if (value < min) {
        return min;
    }
    if (value > max) {
        return max;
    }
    return value;
}

int classify(int value) {
    if (value < 0) {
        return -1;
    }
    if (value == 0) {
        return 0;
    }
    if (value <= 100) {
        return 1;
    }
    return 2;
}

int factorial(int n) {
    if (n < 0) {
        return -1;
    }
    if (n == 0) {
        return 1;
    }
    return n * factorial(n - 1);
}
