#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
. "$TESTS_BASE/setup.sh"

run_test "Mounting" test_mount "$TESTS_BASE/fixtures/test.playlist"
run_test "Absolute paths" test -f "$TEST_MOUNT_POINT/hosts" -a -f "$TEST_MOUNT_POINT/fstab"
run_test "Relative paths" test -f "$TEST_MOUNT_POINT/test.playlist"
run_test "File info: user-owned" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "$TESTS_BASE/fixtures/test.playlist"
run_test "File info: root-owned" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

cleanup
run_test "Fails without arguments" sh -c '! "$BIN"'
run_test "Prints an error" sh -c "'$BIN' 2>&1 \
    | grep -Fq 'error: no target mount point'"

cleanup
run_test "Succeeds mounting with only a mount point" "$BIN" "$TEST_MOUNT_POINT"
run_test "Warns about empty mount" sh -c "'$BIN' '$TEST_MOUNT_POINT' 2>&1 \
    | grep -Fq 'warning: no lists or files specified, mounting empty filesystem'"
