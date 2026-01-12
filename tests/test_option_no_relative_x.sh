#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

cool_mount() {
    test_mount "$@" "$(fixture test.playlist)" -f "fixtures/fstab" -f "$(fixture script.sh)"
}

run_test "--no-relative mount" cool_mount --no-relative 
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "-N mount" cool_mount -N
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative-files mount" cool_mount --no-relative-files
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is respected" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative-paths mount" cool_mount --no-relative-paths
subtest "Absolute path in a list is respected" test -f "$TEST_MOUNT_POINT/hosts"
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Absolute path in --file is respected" test -f "$TEST_MOUNT_POINT/script.sh"
subtest "Relative path in --file is respected" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"

run_test "--no-relative --relative-files mount" cool_mount --no-relative --relative-files
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Relative path in --file is respected" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"

run_test "--no-relative --relative-paths mount" cool_mount --no-relative --relative-paths
subtest "Relative path in a list is respected" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "--no-relative --relative mount" cool_mount --no-relative --relative
subtest "Relative path in a list is respected" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Relative path in --file is respected" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"

run_test "--relative --no-relative mount" cool_mount --relative --no-relative
subtest "Relative path in a list is ignored" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Relative path in --file is ignored" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"
