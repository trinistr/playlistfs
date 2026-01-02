#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--fsname mount without lists" test_mount --fsname "MY NAME" -f "$(fixture ëñ)"
run_test "Filesystem is named as specified, with space replaced with \\\\040" grep "^MY\\\\040NAME $TEST_MOUNT_POINT" "/proc/mounts" -q 

run_test "--fsname mount with lists" test_mount --fsname "ëñ" "$(fixture test.playlist)" -f "$(fixture fstab)"
run_test "Filesystem is named as specified" grep "^ëñ $TEST_MOUNT_POINT" "/proc/mounts" -q 
