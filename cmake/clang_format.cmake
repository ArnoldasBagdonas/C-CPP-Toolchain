# ----------------------------------------
# Clang-Format Integration
# ----------------------------------------

## Clang Format Configurator

# The clang-format-configurator provides an easy way to create or modify existing `.clang-format`
# files based on your preferred style. You can interactively adjust various formatting options and
# generate the file to match your project's requirements:
#
# Website https://zed0.co.uk/clang-format-configurator/
# Try to find clang-format
find_program(CLANG_FORMAT_EXECUTABLE clang-format)

if(NOT CLANG_FORMAT_EXECUTABLE)
    message(WARNING "clang-format not found. 'format' and 'format-check' targets will not be available.")
    return()
endif()

# ----------------------------------------
# File extensions to check
# ----------------------------------------
set(CLANG_FORMAT_CXX_FILE_EXTENSIONS
    *.c *.cpp *.cxx *.cc *.h *.hpp *.hxx *.ipp *.inl *.tpp
)

# ----------------------------------------
# Whitelist of source directories & files to format
# ----------------------------------------
set(CLANG_FORMAT_WHITELIST_DIRS
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/lib
)
set(CLANG_FORMAT_WHITELIST_FILES
    ${CMAKE_SOURCE_DIR}/psoc/Firmware.cydsn/main.c
)

# Collect source files from whitelist
set(CLANG_FORMAT_SOURCE_FILES "")

foreach(dir ${CLANG_FORMAT_WHITELIST_DIRS})
    foreach(ext ${CLANG_FORMAT_CXX_FILE_EXTENSIONS})
        file(GLOB_RECURSE tmp_files "${dir}/${ext}")
        list(APPEND CLANG_FORMAT_SOURCE_FILES ${tmp_files})
    endforeach()
endforeach()

# Add explicit files
list(APPEND CLANG_FORMAT_SOURCE_FILES ${CLANG_FORMAT_WHITELIST_FILES})

# Remove duplicates just in case
list(REMOVE_DUPLICATES CLANG_FORMAT_SOURCE_FILES)

# Print files being formatted
message(STATUS "clang-format source files: ${CLANG_FORMAT_SOURCE_FILES}")

# ----------------------------------------
# Format target
# ----------------------------------------
add_custom_target(format
    COMMENT "Running clang-format on all whitelisted source files..."
)

foreach(file ${CLANG_FORMAT_SOURCE_FILES})
    add_custom_command(TARGET format POST_BUILD
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${file}
    )
endforeach()

# ----------------------------------------
# Format check target
# ----------------------------------------
add_custom_target(format-check
    COMMENT "Checking formatting of all whitelisted source files..."
)

foreach(file ${CLANG_FORMAT_SOURCE_FILES})
    add_custom_command(TARGET format-check POST_BUILD
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run ${file}
    )
endforeach()
