#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--symlink mount" test_mount --symlink "$(fixture test.playlist)"
subtest "File is a symlink" test "q$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "qlrwxrwxrwx"
pending subtest "Symlink is created today" test "q$(extract_file_info "$TEST_MOUNT_POINT/test.playlist" | cut -d' ' -f6)" = "q$(date -I)"

run_test "-s mount" test_mount -s "$(fixture test.playlist)"
subtest "File is a symlink" test "q$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "qlrwxrwxrwx"
