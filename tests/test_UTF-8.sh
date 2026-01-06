#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "Mounting playlist with a UTF-8 name" test_mount "$(fixture ëñ.m3u8)"
subtest "UTF-8 file from playlist is present" compare_file_info "$TEST_MOUNT_POINT/ëñ" "$(fixture ëñ)"

run_test "Mounting with a UTF-8 --file" test_mount -f "$(fixture ëñ)"
subtest "UTF-8 file is present" compare_file_info "$TEST_MOUNT_POINT/ëñ" "$(fixture ëñ)"
