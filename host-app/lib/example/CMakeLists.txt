# -----------------------------------------------------------------------------
# lib/example/CMakeLists.txt
# Example C static library
# -----------------------------------------------------------------------------

# Static library (explicit as requested)
add_library(libexample STATIC)

# Optional alias (still useful even in small projects)
add_library(example::libexample ALIAS libexample)

# Sources
target_sources(libexample
    PRIVATE
        src/add.c
)

# Public include directory (for <example/add.h>)
target_include_directories(libexample
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Apply compiler flags for build type (Debug/Release/Coverage/Valgrind)
set_target_flags(libexample)

# Optional dependencies
# target_link_libraries(libexample
#     PUBLIC
#         example::TimestampProvider
# )
