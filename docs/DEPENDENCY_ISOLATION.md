# Third-Party Dependency Isolation with CMake

## Goal

Speed up rebuild times by isolating third-party dependencies into a **separate build step**, so that changes in application source code do not trigger unnecessary checks or rebuilds of external libraries (e.g. GoogleTest, SQLite, xxHash).

---

## Problem

Using `FetchContent` directly in the main project causes:

* Full dependency graph integration
* Repeated file scanning (especially large libraries like GoogleTest)
* Slower incremental builds even when only app code changes

---

## Solution Overview

Split the build into two independent parts:

```
third_party build (built once)
 ├── sqlite3
 ├── xxhash
 └── gtest

main build (fast iteration)
 └── your application (links prebuilt libraries)
```

---

## Step 1 — Create Standalone `third_party` Project

Turn the `third_party` directory into an independent CMake project.

### Key Features

* Uses `FetchContent` internally
* Builds static libraries
* Installs headers and binaries
* Exports CMake targets for reuse

### Example Structure

```
third_party/
 ├── CMakeLists.txt
 ├── third_partyConfig.cmake.in
 └── downloads/
```

---

## Step 2 — Build and Install Dependencies

Run once:

```bash
cmake -S third_party -B build/third_party
cmake --build build/third_party --target install
```

### Output Layout

```
build/third_party/install/
 ├── include/
 ├── lib/
 └── lib/cmake/third_party/
```

---

## Step 3 — Integrate into Main Project

### Add install path

```cmake
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/build/third_party/install")
```

### Find package

```cmake
find_package(third_party REQUIRED)
```

---

## Step 4 — Link Dependencies

### Application

```cmake
target_link_libraries(my_app PRIVATE
    third_party::sqlite3
    third_party::xxhash
)
```

### Tests

```cmake
target_link_libraries(my_tests PRIVATE
    third_party::gtest
    third_party::gtest_main
)
```

---

## Benefits

### Faster Rebuilds

| Change              | Impact      |
| ------------------- | ----------- |
| Application `.cpp`  | Fast        |
| Application headers | Fast        |
| Third-party sources | Not checked |
| GoogleTest          | Not scanned |

---

### Clean Separation

* No third-party code in main build graph
* Clear ownership boundaries
* Easier debugging and profiling

---

### ♻️ Reusability

* Dependencies built once
* Can be reused across multiple projects
* CI-friendly (cacheable)

---

## Optional Improvements

### Global Dependency Cache

Install dependencies outside the project:

```
~/deps/third_party/install
```

Then:

```cmake
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/deps/third_party/install")
```

---

### Build Type Control

```bash
cmake -S third_party -B build/third_party -DCMAKE_BUILD_TYPE=Release
```

---

### CI Optimization

Cache:

```
build/third_party/install
```

---

## Common Pitfalls

### Avoid

* Using `FetchContent` in the main project
* Mixing third-party and application builds

### Forgetting

* `INSTALL_INTERFACE` include paths
* Exporting targets with `install(EXPORT ...)`

---

## Summary

This approach transforms your build into:

* **Deterministic**
* **Fast for iteration**
* **Scalable for large projects**

By isolating dependencies, you eliminate unnecessary rebuild overhead and gain full control over your build pipeline.

---

## When to Use

This setup is ideal when:

* You have large dependencies (e.g. GoogleTest)
* Build times are increasing
* You want CI-friendly caching
* You prefer explicit dependency control over package managers
