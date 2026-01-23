# -----------------------------------------------------------------------------
# lib/TimestampProvider/CMakeLists.txt
# Build TimestampProvider as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(TimestampProvider STATIC
    src/TimestampProvider.cpp
)

set_target_flags(TimestampProvider)

target_include_directories(TimestampProvider
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(rdemo_backup::TimestampProvider ALIAS TimestampProvider)
