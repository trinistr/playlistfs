#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
source "$TESTS_BASE/setup.sh"

run_test "--noexec mount" test_mount --noexec -f "$(fixture script.sh)"
run_test "File is not executable" test "q$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "q-rw-r--r--"
