# cmake/coverage/gcc_coverage.cmake

set(COVERAGE_HTML_DIR "${CMAKE_BINARY_DIR}/coverage_html")
set(COVERAGE_DATA_GCC "${CMAKE_BINARY_DIR}/coverage.info")

add_custom_target(coverage_report
    COMMENT "Generating GCC code coverage report..."
)

# Clean old data
add_custom_command(
    TARGET coverage_report PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_HTML_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${COVERAGE_DATA_GCC}
    COMMAND /bin/sh -c "find ${CMAKE_BINARY_DIR} -name '*.gcda' -exec rm -f {} +"
    COMMENT "Cleaning GCC coverage data..."
)

# Run tests
add_custom_command(
    TARGET coverage_report POST_BUILD
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running tests (GCC)..."
)

# Generate report
add_custom_command(
    TARGET coverage_report POST_BUILD
    COMMAND lcov --capture --directory ${CMAKE_BINARY_DIR} --output-file ${COVERAGE_DATA_GCC}
    COMMAND genhtml ${COVERAGE_DATA_GCC} --output-directory ${COVERAGE_HTML_DIR}
    COMMENT "Generating GCC HTML coverage report..."
)

add_dependencies(coverage_report unit_tests)