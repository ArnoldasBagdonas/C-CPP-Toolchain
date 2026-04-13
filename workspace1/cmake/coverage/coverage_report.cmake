# coverage_report.cmake
# Helper function to set up coverage report for the selected compiler and test targets

function(add_coverage_report TEST_TARGETS)
    if(MSVC)
        include("${CMAKE_SOURCE_DIR}/cmake/coverage/msvc_coverage.cmake")
        add_msvc_coverage_report_target(${TEST_TARGETS})
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        include("${CMAKE_SOURCE_DIR}/cmake/coverage/clang_coverage.cmake")
        add_clang_coverage_report_target(${TEST_TARGETS})
    elseif(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        include("${CMAKE_SOURCE_DIR}/cmake/coverage/gcc_coverage.cmake")
        add_gcc_coverage_report_target(${TEST_TARGETS})
    endif()
endfunction()
