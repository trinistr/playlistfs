#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
source "$TESTS_BASE/setup.sh"

run_test "--symlink mount" test_mount --symlink "$(fixture test.playlist)"
run_test "File is a symlink" test "q$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "qlrwxrwxrwx"
pending run_test "Symlink is created today" test "q$(extract_file_info "$TEST_MOUNT_POINT/test.playlist" | cut -d' ' -f6)" = "q$(date -I)"

run_test "-s mount" test_mount -s "$(fixture test.playlist)"
run_test "File is a symlink" test "q$(extract_mode "$TEST_MOUNT_POINT/test.playlist")" = "qlrwxrwxrwx"
