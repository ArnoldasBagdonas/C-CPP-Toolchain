set(COVERAGE_HTML_DIR "${CMAKE_BINARY_DIR}/coverage_html")
set(COVERAGE_DATA_CLANG "${CMAKE_BINARY_DIR}/coverage.profdata")

add_custom_target(coverage_report
    COMMENT "Generating Clang/LLVM code coverage report..."
)

# Clean old data
add_custom_command(
    TARGET coverage_report PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_HTML_DIR}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${COVERAGE_DATA_CLANG}
    COMMAND /bin/sh -c "find ${CMAKE_BINARY_DIR} -name '*.profraw' -exec rm -f {} +"
    COMMENT "Cleaning LLVM coverage data..."
)

# Run tests with LLVM profiling
add_custom_command(
    TARGET coverage_report POST_BUILD
    COMMAND
        LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage_%m.profraw
        ${CMAKE_CTEST_COMMAND} --output-on-failure
    COMMENT "Running tests with LLVM profiling..."
)

# Generate report
add_custom_command(
    TARGET coverage_report POST_BUILD
    COMMAND llvm-profdata merge -sparse ${CMAKE_BINARY_DIR}/coverage_*.profraw -o ${COVERAGE_DATA_CLANG}
    COMMAND llvm-cov show
        $<TARGET_FILE:unit_tests>
        -instr-profile=${COVERAGE_DATA_CLANG}
        -format=html
        -output-dir=${COVERAGE_HTML_DIR}
        -ignore-filename-regex='/usr/.*|.*googletest.*'
    COMMENT "Generating LLVM HTML coverage report..."
)

add_dependencies(coverage_report unit_tests)
