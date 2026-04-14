# -----------------------------------------------------------------------------
# lib/example/CMakeLists.txt
# Static C library: libexample
# -----------------------------------------------------------------------------

# Define static library target
add_library(libexample STATIC)

# Namespaced alias for consumers
add_library(example::libexample ALIAS libexample)

# Source files
target_sources(libexample
    PRIVATE
        src/example.c
)

# Public headers (used as <example/example.h>)
target_include_directories(libexample
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Apply project-specific compiler flags if available (Debug/Release/Coverage/Valgrind)
if(COMMAND set_target_flags)
    set_target_flags(libexample)
endif()

# Optional dependencies
# target_link_libraries(libexample
#     PUBLIC
#         example::TimestampProvider
# )
