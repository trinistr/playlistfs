#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "Mounting with a list" test_mount "$TEST_ROOT/fixtures/test.playlist"
subtest "Absolute paths exist" test -f "$TEST_MOUNT_POINT/hosts" -a -f "$TEST_MOUNT_POINT/fstab"
subtest "Relative paths exist" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "File info: user-owned" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "$TEST_ROOT/fixtures/test.playlist"
subtest "File info: root-owned" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

cleanup
run_test "Fails without arguments" ! "$BIN"
subtest "Prints an error that no mount point is specified" sh -c "'$BIN' 2>&1 \
    | grep -Fq 'error: no target mount point'"

run_test "Succeeds mounting with only a mount point" test_mount
subtest "Warns about empty mount" sh -c "'$BIN' '$TEST_MOUNT_POINT' 2>&1 \
    | grep -Fq 'warning: no lists or files specified, mounting empty filesystem'"
