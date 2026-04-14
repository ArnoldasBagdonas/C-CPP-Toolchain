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
        src/example.c
)

# Public include directory (for <example/example.h>)
target_include_directories(libexample
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(COMMAND set_target_flags)
set_target_flags(libexample)
endif()

# Optional dependencies
# target_link_libraries(libexample
#     PUBLIC
#         example::TimestampProvider
# )
