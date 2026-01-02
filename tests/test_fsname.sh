#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

test_mount -f "$(fixture fstab.playlist)" -f "$(fixture fstab)"
run_test "Only --file: fsname is 'playlistfs'" grep "^playlistfs $TEST_MOUNT_POINT" "/proc/mounts" -q

test_mount -f "$(fixture fstab)" "$(fixture test.playlist)"
run_test "One playlist: fsname is playlist's basename" grep "^test.playlist $TEST_MOUNT_POINT" "/proc/mounts" -q

test_mount "$(fixture fstab.playlist)" -f "$(fixture fstab)" "$(fixture test.playlist)"
run_test "Two playlists: fsname is first playlist's basename" grep "^fstab.playlist $TEST_MOUNT_POINT" "/proc/mounts" -q
