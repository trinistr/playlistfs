#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
. "$TESTS_BASE/setup.sh"

test_mount -f "$(fixture fstab.playlist)" -f "$(fixture fstab)"
run_test "Only --file: fsname has no special name" grep "playlistfs" "/proc/mounts" -q

test_mount "$(fixture test.playlist)"
pending run_test "One playlist: fsname includes playlist name" grep "playlistfs-'test'" "/proc/mounts" -q

test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)"
pending run_test "Two playlists: fsname includes first playlist name" grep "playlistfs-'fstab'" "/proc/mounts" -q
