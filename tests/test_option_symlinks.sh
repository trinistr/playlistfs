#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--symlinks mount" test_mount --symlinks "$(fixture test.playlist)"
subtest "File is a symlink" test "$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "lrwxrwxrwx"

run_test "-s mount" test_mount -s "$(fixture test.playlist)"
subtest "File is a symlink" test "$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "lrwxrwxrwx"
