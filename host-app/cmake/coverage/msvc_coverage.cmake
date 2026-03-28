# -----------------------------------------------------------------------------
# MSVC coverage using OpenCppCoverage
# -----------------------------------------------------------------------------

find_program(OPENCPPCOVERAGE_EXE OpenCppCoverage.exe)

if(NOT OPENCPPCOVERAGE_EXE)
    message(FATAL_ERROR
        "OpenCppCoverage.exe not found. "
        "Install OpenCppCoverage and ensure it is in PATH. "
        "https://github.com/OpenCppCoverage/OpenCppCoverage"
    )
endif()

# Convert paths to Windows-native format (IMPORTANT)
file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" COVERAGE_SOURCE_DIR_WIN)
file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/coverage_html" COVERAGE_HTML_DIR_WIN)
file(TO_NATIVE_PATH "$<TARGET_FILE:unit_tests>" UNIT_TESTS_EXE_WIN)

add_custom_target(coverage_report
    COMMENT "Generating MSVC code coverage report..."
)

# -----------------------------------------------------------------------------
# Clean previous coverage
# -----------------------------------------------------------------------------
add_custom_command(
    TARGET coverage_report PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory "${COVERAGE_HTML_DIR_WIN}"
    COMMENT "Cleaning previous MSVC coverage data..."
)

# -----------------------------------------------------------------------------
# Run tests under OpenCppCoverage
# -----------------------------------------------------------------------------
add_custom_command(
    TARGET coverage_report POST_BUILD
    COMMAND
        "${OPENCPPCOVERAGE_EXE}"
            --export_type=html:"${COVERAGE_HTML_DIR_WIN}"
            --sources="${COVERAGE_SOURCE_DIR_WIN}"
            --
            "${UNIT_TESTS_EXE_WIN}"
    COMMENT "Running unit tests with OpenCppCoverage..."
)

add_dependencies(coverage_report unit_tests)