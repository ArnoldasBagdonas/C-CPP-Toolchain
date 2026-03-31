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
 └── gtest / gmock

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
 ├── downloads/          ← cached archive tarballs
 └── src/                ← extracted / tracked source trees
```

### Source Management Strategy

The `third_party/` directory uses a **two-tier source management** approach:

| Directory                | Purpose                                                        | Tracked in Git?  |
| ------------------------ | -------------------------------------------------------------- | ---------------- |
| `third_party/src/`       | Extracted source trees — allows local modifications and review | Yes              |
| `third_party/downloads/` | Original archive tarballs used as a fallback                   | Yes              |

**How it works:**

1. If `third_party/src/<name>-src/` exists, the build uses it directly.
2. If the source directory is missing, the build extracts it from the cached
   archive in `third_party/downloads/`.
3. If the archive is also missing, it is downloaded from the upstream URL.

**Why track sources in git?**

* **Resilience** — if an upstream repository or release is temporarily or
  permanently unavailable, the project still builds.
* **Reproducibility** — the exact source used is pinned in version control.
* **Modifiability** — local patches to third-party code are tracked and
  reviewable in the normal git workflow.

---

## Step 2 — Build and Install Dependencies

Run once from the project root:

```bash
rm -rf build/third_party
cmake -S third_party -B build/third_party -G Ninja \
      -DCMAKE_INSTALL_PREFIX=$PWD/build/third_party/install
cmake --build build/third_party --target install
```

> **Important:** Always pass `-DCMAKE_INSTALL_PREFIX` to control where the
> libraries are installed. Without it, CMake will attempt to install to a
> system-wide path (e.g. `/usr/local`), which is almost never what you want.

### Output Layout

```
build/third_party/install/
 ├── include/
 ├── lib/
 └── lib/cmake/third_party/
```

---

## Step 3 — Integrate into Main Project

Add the install path to the CMake search path, then find the package:

```cmake
# Default location of prebuilt third_party
set(THIRD_PARTY_INSTALL_DIR
    "${CMAKE_SOURCE_DIR}/build/third_party/install"
    CACHE PATH "Path to third_party install directory"
)

# Add to search path
list(APPEND CMAKE_PREFIX_PATH "${THIRD_PARTY_INSTALL_DIR}")

# Find the package
find_package(third_party REQUIRED)
```

Using a `CACHE PATH` variable lets users override the install location at
configure time without editing CMakeLists:

```bash
cmake -S . -B build -DTHIRD_PARTY_INSTALL_DIR=/custom/path
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
    third_party::gmock
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

### Reusability

* Dependencies built once
* Can be reused across multiple projects
* CI-friendly (cacheable)

---

## Sanitizer Builds and Third-Party Dependencies

### The Problem

Sanitizers (ASan, TSan, MSan, UBSan) work by instrumenting code at compile
time. For correct operation, **all code linked into the final binary** must be
compiled with the same sanitizer flags — including third-party libraries.

If the main project is compiled with `-fsanitize=address` but links against a
GoogleTest built without it, you may get:

* False negatives (bugs in third-party call paths go undetected)
* Missing stack traces
* Linker errors or runtime crashes
* Massive false positives (especially with MemorySanitizer)

### Which Build Types Need Matching Third-Party Flags?

| Build Type                   | Third-party needs same flags? | Why                                                    |
| ---------------------------- | ----------------------------- | ------------------------------------------------------ |
| Debug / Release              | ❌ No                         | Standard flags — ABI compatible                        |
| Coverage                     | ❌ No                         | Coverage excludes third-party code by design           |
| Valgrind                     | ❌ No                         | Valgrind instruments at runtime, not compile time      |
| **AddressSanitizer**         | ✅ Yes                        | All code must be instrumented for reliable detection   |
| **ThreadSanitizer**          | ✅ Yes                        | Uninstrumented code causes false negatives             |
| **MemorySanitizer**          | ✅ **Critical**               | Uninstrumented code causes massive false positives     |
| **UndefinedBehaviorSanitizer** | ⚠️ Recommended              | UB in third-party code won't be caught otherwise       |

### Solution: Per-Variant Third-Party Builds

Build third_party with separate install prefixes — one for default builds, and
one for each sanitizer:

```
build/
 ├── third_party/install/          ← Debug, Release, Coverage, Valgrind
 ├── third_party-asan/install/     ← AddressSanitizer
 ├── third_party-tsan/install/     ← ThreadSanitizer
 ├── third_party-msan/install/     ← MemorySanitizer
 └── third_party-ubsan/install/    ← UndefinedBehaviorSanitizer
```

Example — building third_party with AddressSanitizer:

```bash
cmake -S third_party -B build/third_party-asan -G Ninja \
      -DCMAKE_INSTALL_PREFIX=$PWD/build/third_party-asan/install \
      -DCMAKE_C_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
      -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address" \
      -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address"
cmake --build build/third_party-asan --target install
```

Then point the main project at the matching install:

```bash
cmake -S . -B build/asan -G Ninja \
      -DCMAKE_BUILD_TYPE=AddressSanitizer \
      -DTHIRD_PARTY_INSTALL_DIR=$PWD/build/third_party-asan/install
```

### Automation with Make

A `Makefile` in the project root automates the full dependency chain. Each
application target depends on the matching third_party variant — Make only
rebuilds third_party when the stamp file is missing:

```
make test-asan
  └── asan                         (build app with ASan)
       └── third-party-asan        (build third_party with ASan, if needed)
```

Common commands:

```bash
make debug          # Build debug (auto-builds default third_party)
make asan           # Build ASan  (auto-builds ASan third_party)
make test-debug     # Build + run debug tests
make test-asan      # Build + run ASan tests
make clean          # Remove all build artifacts
make help           # Show all available targets
```

See `Makefile` for the full list of targets and configuration options.

---

## Optional Improvements

### Global Dependency Cache

Install dependencies outside the project:

```
# Linux / macOS
~/deps/third_party/install

# Windows
%USERPROFILE%\deps\third_party\install
```

Then pass the path at configure time:

```bash
# Linux / macOS
cmake -S . -B build -DTHIRD_PARTY_INSTALL_DIR="$HOME/deps/third_party/install"

# Windows (cmd)
cmake -S . -B build -DTHIRD_PARTY_INSTALL_DIR="%USERPROFILE%\deps\third_party\install"
```

---

### Build Type Control

```bash
cmake -S third_party -B build/third_party -G Ninja \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=$PWD/build/third_party/install
```

---

### CI Optimization

Cache this directory between CI runs:

```
build/third_party/install
```

---

## Common Pitfalls

### Build Type Mismatch

The `third_party` build and the main project **must** use a compatible build
type. Building third_party as `Release` while the main project uses `Debug` can
cause ODR violations, ABI mismatches, or subtle runtime errors — especially with
C++ libraries like GoogleTest.

```bash
# Good — both use the same build type
cmake -S third_party -B build/third_party -DCMAKE_BUILD_TYPE=Debug ...
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
```

### Generator Mismatch

Use the same CMake generator (e.g. Ninja, Unix Makefiles) for both the
`third_party` build and the main project. Mixing generators can produce
incompatible library formats or link errors.

### Missing Install Prefix

Forgetting `-DCMAKE_INSTALL_PREFIX` causes `cmake --build ... --target install`
to write to a system path. Always specify it explicitly.

### Missing Headers

When adding a new dependency, remember to install **all** header directories it
exposes. For example, GoogleTest ships both `gtest/` and `gmock/` include trees
— both must be installed for downstream consumers.

### Stale Install After Version Bump

When you change a dependency version, **wipe** the install directory and rebuild
from scratch:

```bash
rm -rf build/third_party
cmake -S third_party -B build/third_party -G Ninja \
      -DCMAKE_INSTALL_PREFIX=$PWD/build/third_party/install
cmake --build build/third_party --target install
```

There is no automatic staleness detection — the cached archive filename acts as
a simple version tag, but nothing prevents manual replacement. Consider adding
`EXPECTED_HASH SHA256=...` to `file(DOWNLOAD ...)` calls for integrity
verification.

### Avoid

* Using `FetchContent` in the main project
* Mixing third-party and application builds

### Don't Forget

* `INSTALL_INTERFACE` include paths on exported targets
* Exporting targets with `install(EXPORT ...)`
* Installing headers for **all** sub-libraries (e.g. both gtest and gmock)

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
