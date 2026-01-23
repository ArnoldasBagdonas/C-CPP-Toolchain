# -----------------------------------------------------------------------------
# lib/FileHasher/CMakeLists.txt
# Build FileHasher as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(FileHasher STATIC
    src/FileHasher.cpp
)

set_target_flags(FileHasher)

target_include_directories(FileHasher
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(FileHasher
    PUBLIC
        xxhash_static
)

add_library(rdemo_backup::FileHasher ALIAS FileHasher)
