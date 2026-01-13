#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--read-only mount" test_mount --read-only -f "$(fixture script.sh)"
subtest "File is read-only" test "$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "-r-xr-xr-x"

run_test "-r mount" test_mount -r -f "$(fixture script.sh)"
subtest "File is read-only" test "$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "-r-xr-xr-x"

run_test "--read-only and --symlinks mount" test_mount --read-only --symlinks -f "$(fixture script.sh)"
subtest "Symlink reports full permissions" test "$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "lrwxrwxrwx"
