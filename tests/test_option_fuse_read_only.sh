#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
. "$TESTS_BASE/setup.sh"

run_test "--read-only mount" test_mount --read-only -f "$(fixture script.sh)"
run_test "File is read-only" test "q$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "q-r-xr-xr-x"

run_test "-r mount" test_mount -r -f "$(fixture script.sh)"
run_test "File is read-only" test "q$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "q-r-xr-xr-x"
