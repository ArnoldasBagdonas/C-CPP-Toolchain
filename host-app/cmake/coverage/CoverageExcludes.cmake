# Exclude:
#  - test sources
#  - googletest
#  - system headers
set(COVERAGE_EXCLUDE_REGEX
    "/usr/.*|.*googletest.*|.*[/\\\\]test[/\\\\].*|.*_test\\.(c|cc|cpp)$"
)
