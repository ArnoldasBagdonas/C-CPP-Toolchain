# -----------------------------------------------------------------------------
# lib/ThreadedFileQueue/CMakeLists.txt
# Build ThreadedFileQueue as a STATIC library with modern CMake practices
# -----------------------------------------------------------------------------

add_library(ThreadedFileQueue STATIC
    src/ThreadedFileQueue.cpp
)

set_target_flags(ThreadedFileQueue)

target_include_directories(ThreadedFileQueue
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

add_library(rdemo_backup::ThreadedFileQueue ALIAS ThreadedFileQueue)
