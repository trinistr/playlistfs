#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--noexec mount" test_mount --noexec -f "$(fixture script.sh)"
subtest "File is not executable" test "$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "-rw-r--r--"

run_test "--noexec and --symlinks mount" test_mount --noexec --symlinks -f "$(fixture script.sh)"
subtest "Symlink reports full permissions" test "$(extract_mode "$TEST_MOUNT_POINT/script.sh")" = "lrwxrwxrwx"
