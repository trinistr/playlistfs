#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
source "$TESTS_BASE/setup.sh"

cleanup
run_test "--target mount" "$BIN" --target "$TEST_MOUNT_POINT" "$(fixture test.playlist)"
run_test "Files added successfully to mount point" test -f "$TEST_MOUNT_POINT/fstab"

cleanup
run_test "-t mount" "$BIN" -t "$TEST_MOUNT_POINT" "$(fixture test.playlist)"
run_test "Files added successfully to mount point" test -f "$TEST_MOUNT_POINT/fstab"
