#!/usr/bin/env bash

#NOTE: Make it executable: chmod +x run_valgrind.sh

VALGRIND_BIN="$1"
XML_FILE="$2"
UNIT_TEST_BIN="$3"
shift 3

"$VALGRIND_BIN" --tool=memcheck \
    --leak-check=full \
    --track-origins=yes \
    --show-leak-kinds=all \
    --gen-suppressions=all \
    --error-exitcode=0 \
    --xml=yes \
    --xml-file="$XML_FILE" \
    "$UNIT_TEST_BIN" "$@"

# Always exit 0 so CMake continues to parse XML
exit 0