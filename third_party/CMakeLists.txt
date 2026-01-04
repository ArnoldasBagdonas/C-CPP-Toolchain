# third_party/CMakeLists.txt
# Clone third party repositories into third_party folder (manually or via Git submodule)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# ----------------------------------------------------------------------------- 
# SQLite3
# -----------------------------------------------------------------------------

# Create SQLite3 as an STATIC library
add_library(sqlite3 STATIC
    sqlite3/sqlite3.c
)

# Apply compiler flags (Debug/Release/Coverage/Valgrind)
set_target_flags(sqlite3)

# Include directories
target_include_directories(sqlite3 PUBLIC
    sqlite3
)

# -----------------------------------------------------------------------------
# cxxopts (header-only, v3.3.1)
# -----------------------------------------------------------------------------
FetchContent_Declare(
    cxxopts
    URL https://github.com/jarro2783/cxxopts/archive/refs/tags/v3.3.1.tar.gz
)
FetchContent_MakeAvailable(cxxopts)

# -----------------------------------------------------------------------------
# xxHash (v0.8.3)
# -----------------------------------------------------------------------------
FetchContent_Declare(
    xxhash
    URL https://github.com/Cyan4973/xxHash/archive/refs/tags/v0.8.3.tar.gz
)
FetchContent_MakeAvailable(xxhash)

add_library(xxhash_static STATIC ${xxhash_SOURCE_DIR}/xxhash.c)
set_target_flags(xxhash_static)
target_include_directories(xxhash_static
    PUBLIC
        ${xxhash_SOURCE_DIR}
)

