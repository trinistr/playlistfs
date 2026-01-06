#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--read-only mount" test_mount --read-only -f "$(fixture script.sh)"
subtest "File is read-only" test "q$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "q-r-xr-xr-x"

run_test "-r mount" test_mount -r -f "$(fixture script.sh)"
subtest "File is read-only" test "q$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "q-r-xr-xr-x"
