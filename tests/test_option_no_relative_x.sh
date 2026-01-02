#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

cool_mount() {
    # cd to use a relative path
    (cd "$TEST_ROOT"; test_mount "$1" "$(fixture test.playlist)" -f "fixtures/fstab" -f "$(fixture script.sh)")
}

run_test "--no-relative mount" cool_mount --no-relative 
run_test "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
run_test "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
run_test "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
run_test "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "-n mount" cool_mount -n
run_test "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
run_test "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
run_test "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
run_test "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative-files mount" cool_mount --no-relative-files
run_test "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
run_test "Relative path in a list is respected" test -f "$TEST_MOUNT_POINT/test.playlist"
run_test "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
run_test "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative-lists mount" cool_mount --no-relative-lists
run_test "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
run_test "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
run_test "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
run_test "Relative path in --file is respected" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"
