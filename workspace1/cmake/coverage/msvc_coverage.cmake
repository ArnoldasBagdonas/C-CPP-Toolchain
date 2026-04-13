# -----------------------------------------------------------------------------
# MSVC coverage using OpenCppCoverage
# -----------------------------------------------------------------------------


# Function to add a MSVC coverage report target with custom test dependencies
function(add_msvc_coverage_report_target COVERAGE_TEST_TARGET)
    find_program(OPENCPPCOVERAGE_EXE OpenCppCoverage.exe)
    if(NOT OPENCPPCOVERAGE_EXE)
        message(FATAL_ERROR
            "OpenCppCoverage.exe not found. "
            "Install OpenCppCoverage and ensure it is in PATH. "
            "https://github.com/OpenCppCoverage/OpenCppCoverage"
        )
    endif()

    file(TO_NATIVE_PATH "${CMAKE_SOURCE_DIR}" COVERAGE_SOURCE_DIR_WIN)
    file(TO_NATIVE_PATH "${CMAKE_BINARY_DIR}/coverage_html" COVERAGE_HTML_DIR_WIN)

    # Use the first test target for coverage (assume main test binary)
    if(COVERAGE_TEST_TARGET)
        list(GET COVERAGE_TEST_TARGET 0 MAIN_TEST_TARGET)
    else()
        set(MAIN_TEST_TARGET unit_tests)
    endif()
    file(TO_NATIVE_PATH "$<TARGET_FILE:${MAIN_TEST_TARGET}>" UNIT_TESTS_EXE_WIN)

    # Collect any external source directories registered via the
    # COVERAGE_ADDITIONAL_SOURCE_DIRS global property so that
    # OpenCppCoverage instruments files outside CMAKE_SOURCE_DIR.
    get_property(_extra_src_dirs GLOBAL PROPERTY COVERAGE_ADDITIONAL_SOURCE_DIRS)
    set(_extra_sources_args "")
    foreach(_dir IN LISTS _extra_src_dirs)
        get_filename_component(_abs_dir "${_dir}" ABSOLUTE)
        file(TO_NATIVE_PATH "${_abs_dir}" _dir_win)
        list(APPEND _extra_sources_args "--sources=${_dir_win}")
    endforeach()

    add_custom_target(coverage_report
        COMMENT "Generating MSVC code coverage report..."
    )

    add_custom_command(
        TARGET coverage_report PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory "${COVERAGE_HTML_DIR_WIN}"
        COMMENT "Cleaning previous MSVC coverage data..."
    )

    add_custom_command(
        TARGET coverage_report POST_BUILD
        COMMAND
            "${OPENCPPCOVERAGE_EXE}"
                --export_type=html:"${COVERAGE_HTML_DIR_WIN}"
                --sources="${COVERAGE_SOURCE_DIR_WIN}"
                ${_extra_sources_args}
                --
                "${UNIT_TESTS_EXE_WIN}"
        COMMENT "Running unit tests with OpenCppCoverage..."
    )

    if(COVERAGE_TEST_TARGET)
        add_dependencies(coverage_report ${COVERAGE_TEST_TARGET})
    endif()
endfunction()