#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

cool_mount() {
    # cd to use a relative path
    (cd "$TEST_ROOT"; test_mount "$1" "$(fixture test.playlist)" -f "fixtures/fstab" -f "$(fixture script.sh)")
}

run_test "--no-relative mount" cool_mount --no-relative 
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "-n mount" cool_mount -n
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative-files mount" cool_mount --no-relative-files
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is respected" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative-lists mount" cool_mount --no-relative-lists
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is respected" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"
