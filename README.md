# C-CPP Toolchain

The repository includes a small, self-contained backup utility inspired by [rdiff-backup](https://rdiff-backup.net/). The utility is intentionally simplified and exists to exercise the toolchain rather than to serve as a production-grade backup solution. `rdemo-backup` is a reference-quality C++17 project whose primary purpose is to demonstrate a modern, cross-platform C/C++ toolchain built with CMake. It targets professional developers working on Linux and Windows with GCC, Clang, or MSVC.

A Docker-based development environment is provided with tooling for static analysis, sanitizers, coverage, documentation, and diagram generation.

## Inspiration and backup model
This project is conceptually inspired by `rdiff-backup`, which tracks file history over time and allows recovery of previous versions. Unlike `rdiff-backup`, this utility **does not store deltas**.

Instead, it uses a **full-file snapshot model**:

- Files are copied in full when added or modified
- Previous versions of modified or deleted files are archived into timestamped snapshot directories
- An embedded SQLite database tracks file state across runs

This approach trades storage efficiency for simplicity, transparency, and direct filesystem access. Backed-up files remain readable and usable without requiring the utility itself or any proprietary format.

## Design highlights and techniques

### Incremental backups via content hashing

File changes are detected using the [xxHash](https://github.com/Cyan4973/xxHash) algorithm (`XXH64`) rather than timestamps or file sizes. This avoids false positives and keeps comparisons fast even for large files.

### Thread-per-core file processing with work queues

Files are streamed into a shared queue and processed by a worker pool sized to `std::thread::hardware_concurrency()`. This avoids pre-enumerating all files and keeps memory usage predictable.

### Lazy snapshot creation using `std::call_once`

Snapshot directories for modified or deleted files are created only when needed, using `std::once_flag` and `std::call_once`. This avoids unnecessary filesystem writes when no changes occur.

Reference: [https://en.cppreference.com/w/cpp/thread/call_once.html](https://en.cppreference.com/w/cpp/thread/call_once.html)

### Thread-safe SQLite access with per-thread connections

Each worker thread transparently receives its own SQLite connection, avoiding global locks while maintaining consistency via SQLite WAL mode.

References:

- [https://www.sqlite.org/threadsafe.html](https://www.sqlite.org/threadsafe.html)

- [https://www.sqlite.org/wal.html](https://www.sqlite.org/wal.html)

### Write-ahead logging (WAL) for concurrent writes

SQLite is configured with PRAGMA journal_mode=WAL to support concurrent readers and writers without blocking.

### Portable filesystem handling using `std::filesystem`

All path normalization, directory traversal, and file copying use the standard C++ filesystem library.

Reference: [https://en.cppreference.com/w/cpp/filesystem](https://en.cppreference.com/w/cpp/filesystem)

### Command-line parsing with a header-only library

CLI options are handled via [cxxopts](https://github.com/jarro2783/cxxopts), keeping the executable lightweight and dependency management simple.

### Builds with CMake

The project enforces C++17, disables compiler extensions, centralizes compiler flags, and cleanly separates third-party dependencies.

## Toolchain features demonstrated

The repository is designed to showcase a realistic C/C++ development setup, including:

- Multiple build types (Debug, Release, Coverage).

- Clang-format enforcement.

- Sanitizers:

    - AddressSanitizer (ASan).

    - ThreadSanitizer (TSan).

    - UndefinedBehaviorSanitizer (UBSan).

    - MemorySanitizer (MSan, Clang/Linux).

- Valgrind integration (Linux).

- Code coverage for GCC, Clang, and MSVC.

## Fonts included in the development container

The Docker environment installs fonts commonly used for documentation and diagrams:

- Cantarell – https://gitlab.gnome.org/GNOME/cantarell-fonts

- Roboto – https://fonts.google.com/specimen/Roboto

- GNU FreeFont (OTF) – https://www.gnu.org/software/freefont/

- Ubuntu Font Family – https://design.ubuntu.com/font

Microsoft Core Fonts are also installed manually for compatibility with legacy documents:
https://sourceforge.net/projects/corefonts/

## Project structure

```
c-cpp-toolchain/
├── .clang-format           # Configuration for code formatting
├── .copilot/               # Instructions and configurations for Copilot
├── .devcontainer/          # Configuration for development container environment (Dev Containers)
├── .git/                   # Git version control system files
├── .vscode/                # VS Code editor specific settings and configurations
├── build/                  # Default output directory for build artifacts (executables, libraries, etc.)
├── cmake/                  # Custom CMake modules and scripts for build configuration
├── lib/                    # Contains source code for reusable libraries, including BackupUtility
│   └── BackupUtility/      # The core backup utility library
│       ├── include/        # Public header files for BackupUtility
│       └── src/            # Source files for BackupUtility
├── scripts/                # Utility scripts (e.g., Valgrind integration)
├── src/                    # Main application source code (e.g., rdemo-backup entry point)
├── tests/                  # Unit and integration tests for the project
│   ├── helpers/            # Helper utilities for writing tests
│   ├── mocks/              # Mock objects for testing
│   ├── src/                # Test source files
│   └── stubs/              # Stub implementations for testing
└── third_party/            # External dependencies, managed by CMake's FetchContent
    └── sqlite3/            # SQLite3 library source
```

## Building `rdemo-backup`

This project uses CMake for its build system. To build the `rdemo-backup` utility, follow these standard steps:

1.  **Create a build directory:** It's recommended to build out-of-source.
    ```bash
    mkdir build
    cd build
    ```
2.  **Configure the project with CMake:** This generates the build files for your chosen generator (e.g., Makefiles, Visual Studio projects).
    ```bash
    cmake ..
    ```
    You can specify a build type (e.g., `Debug`, `Release`, `Coverage`) using the `-DCMAKE_BUILD_TYPE` option:
    ```bash
    cmake .. -DCMAKE_BUILD_TYPE=Release
    ```
3.  **Build the project:**
    ```bash
    cmake --build .
    ```
    This will compile the `rdemo-backup` executable and any associated libraries. The executable will typically be found in `build/`.

## Command Line Options

The `rdemo-backup` utility supports the following command-line options:

*   `-s, --source <path>`: Specifies the source directory to be backed up.
*   `-b, --backup <path>`: Specifies the destination directory where backups will be stored.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
