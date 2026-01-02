#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--noexec mount" test_mount --noexec -f "$(fixture script.sh)"
run_test "File is not executable" test "q$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "q-rw-r--r--"
