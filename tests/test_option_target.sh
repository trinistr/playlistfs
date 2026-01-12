#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

cleanup
make_test_mount_point
run_test "--target mount" "$BIN" --target "$TEST_MOUNT_POINT" "$(fixture test.playlist)"
subtest "Files added successfully to mount point" test -f "$TEST_MOUNT_POINT/fstab"

cleanup
make_test_mount_point
run_test "-t mount" "$BIN" -t "$TEST_MOUNT_POINT" "$(fixture test.playlist)"
subtest "Files added successfully to mount point" test -f "$TEST_MOUNT_POINT/fstab"
