#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--symlinks mount" test_mount --symlinks "$(fixture test.playlist)"
subtest "File is a symlink" test "$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "lrwxrwxrwx"

run_test "-S mount" test_mount -S "$(fixture test.playlist)"
subtest "File is a symlink" test "$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "lrwxrwxrwx"

# Also see test_symlinks.sh.
