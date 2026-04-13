
# Function to add a Clang/LLVM coverage report target with custom test dependencies
function(add_clang_coverage_report_target COVERAGE_TEST_TARGET)
    set(COVERAGE_HTML_DIR "${CMAKE_BINARY_DIR}/coverage_html")
    set(COVERAGE_DATA_CLANG "${CMAKE_BINARY_DIR}/coverage.profdata")

    add_custom_target(coverage_report
        COMMENT "Generating Clang/LLVM code coverage report..."
    )

    add_custom_command(
        TARGET coverage_report PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory ${COVERAGE_HTML_DIR}
        COMMAND ${CMAKE_COMMAND} -E remove -f ${COVERAGE_DATA_CLANG}
        COMMAND /bin/sh -c "find ${CMAKE_BINARY_DIR} -name '*.profraw' -exec rm -f {} +"
        COMMENT "Cleaning LLVM coverage data..."
    )

    add_custom_command(
        TARGET coverage_report POST_BUILD
        COMMAND
            LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/coverage_%m.profraw
            ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMENT "Running tests with LLVM profiling..."
    )

    # Use the first test target for llvm-cov (assume main test binary)
    if(COVERAGE_TEST_TARGET)
        list(GET COVERAGE_TEST_TARGET 0 MAIN_TEST_TARGET)
    else()
        set(MAIN_TEST_TARGET unit_tests)
    endif()

    add_custom_command(
        TARGET coverage_report POST_BUILD
        COMMAND llvm-profdata merge -sparse ${CMAKE_BINARY_DIR}/coverage_*.profraw -o ${COVERAGE_DATA_CLANG}
        COMMAND llvm-cov show
            $<TARGET_FILE:${MAIN_TEST_TARGET}>
            -instr-profile=${COVERAGE_DATA_CLANG}
            -format=html
            -output-dir=${COVERAGE_HTML_DIR}
            -ignore-filename-regex='/usr/.*|.*googletest.*'
        COMMENT "Generating LLVM HTML coverage report..."
    )

    if(COVERAGE_TEST_TARGET)
        add_dependencies(coverage_report ${COVERAGE_TEST_TARGET})
    endif()
endfunction()
