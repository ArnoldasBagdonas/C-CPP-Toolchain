#include "gtest/gtest.h"
#include <thread>
#include <vector>
#include <limits>
#include <cstring> // for memset

/* ========================================================================== */
/* ========================== AddressSanitizer ============================== */
/* ========================================================================== */

// Use-after-free
TEST(ASan, UseAfterFree) {
    int* ptr = new int(42);
    delete ptr;
    *ptr = 0; // Use-after-free
}

TEST(ASan, UseAfterFreeAlias) {
    int* a = new int(7);
    int* b = a;
    delete a;
    *b = 42; // Use-after-free, usually silent
}

// Heap buffer overflow
TEST(ASan, HeapBufferOverflow) {
    int* arr = new int[5];
    arr[5] = 100; // Heap buffer overflow
    delete[] arr;
}

// Stack buffer overflow
TEST(ASan, StackBufferOverflow) {
    int arr[5];
    for (int i = 0; i <= 5; ++i) { // Stack buffer overflow off-by-one
        arr[i] = i;
    }
}

TEST(ASan, StackOverflowMemcpy) {
    char src[17] = "0123456789ABCDEF";
    char dst[9];
    memcpy(dst, src, sizeof(src)); // overflow, often unnoticed
}

TEST(ASan, VectorDataOverflow) {
    std::vector<int> v(4);
    int* p = v.data();
    p[4] = 123; // out-of-bounds, no bounds checking
}

/* ========================================================================== */
/* ========================== ThreadSanitizer =============================== */
/* ========================================================================== */

// Simple data race
TEST(TSan, DataRaceSimple) {
    int shared = 0;

    auto writer = [&shared]() {
        for(int i = 0; i < 1000; ++i) shared++;
    };
    auto reader = [&shared]() {
        for(int i = 0; i < 1000; ++i) shared--;
    };

    std::thread t1(writer);
    std::thread t2(reader);
    t1.join();
    t2.join();
}

TEST(TSan, ReferenceRace) {
    int x = 0;
    std::thread t1([&]() { x++; });
    std::thread t2([&]() { x++; });
    t1.join();
    t2.join();
}

// Another data race
TEST(TSan, DataRaceVector) {
    std::vector<int> vec(1);
    auto t1 = std::thread([&vec]() { vec[0]++; });
    auto t2 = std::thread([&vec]() { vec[0]++; });
    t1.join();
    t2.join();
}

// Data race with boolean
TEST(TSan, DataRaceBool) {
    bool flag = false;
    auto t1 = std::thread([&flag]() { flag = true; });
    auto t2 = std::thread([&flag]() { flag = false; });
    t1.join();
    t2.join();
}

TEST(TSan, LazyInitRace) {
    static int* ptr = nullptr;

    auto init = []() {
        if (!ptr)
            ptr = new int(42); // data race
    };

    std::thread t1(init);
    std::thread t2(init);
    t1.join();
    t2.join();
}

/* ========================================================================== */
/* ========================== MemorySanitizer =============================== */
/* ========================================================================== */

// Uninitialized read
TEST(MSan, UninitializedRead) {
    int x;
    int y = x + 1; // Undefined behavior: read uninitialized
    (void)y;
}

// Another uninitialized read in array
TEST(MSan, UninitializedArray) {
    int arr[3];
    int sum = arr[0] + arr[1]; // Undefined behavior
    (void)sum;
}

// Pointer uninitialized
TEST(MSan, UninitializedPointer) {
    int* ptr;
    *ptr = 42; // Undefined behavior
}


/* ========================================================================== */
/* ========================== UndefinedBehaviorSanitizer ==================== */
/* ========================================================================== */

// Signed integer overflow
TEST(UBSan, SignedIntegerOverflow) {
    int x = std::numeric_limits<int>::max();
    int y = x + 10;
    (void)y;
}

// Signed integer overflow
TEST(UBSan, SignedOverflowLoop) {
    int x = std::numeric_limits<int>::max();
    for (int i = 0; i < 10; ++i) {
        x++; // Undefined behavior
    }
}

// NOTE: x86 allows unaligned memory access (hardware level), unaligned loads/stores are legal:
// (executes successfully in hardware, if the CPU happily executes the instruction, UBSan often lets it pass.)
//
TEST(UBSan, MisalignedAccess) {
    char buf[sizeof(int) + 1];
    int* p = reinterpret_cast<int*>(buf + 1);
    *p = 42; // Undefined behavior on many architectures
}

enum Color { RED, GREEN, BLUE };

TEST(UBSan, InvalidEnum) {
    Color c = static_cast<Color>(42);
    switch (c) {
        case RED:
        case GREEN:
        case BLUE:
        default:
            break;
    }
}

// Invalid shift
TEST(UBSan, InvalidShift) {
    int x = 1;
    int y = 32;
    volatile int z = x << y; // UB: shift by >= bitwidth
    (void)z;
}
