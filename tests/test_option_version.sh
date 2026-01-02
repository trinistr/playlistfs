#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--version run" "$BIN" --version
run_test "Outputs current version with --version" sh -c "'$BIN' --version | grep -Pq 'playlistfs [0-9]+\.[0-9]+\.[0-9]+'"

run_test "-V run" "$BIN" -V
run_test "Outputs current version with -V" sh -c "'$BIN' -V | grep -Pq 'playlistfs [0-9]+\.[0-9]+\.[0-9]+'"

cleanup
run_test "--version can be combined with mounting" \
    sh -c "'$BIN' --version '$(fixture test.playlist)' '$TEST_MOUNT_POINT' \
    | grep -Pq 'playlistfs [0-9]+\.[0-9]+\.[0-9]+'"
run_test "Files added successfully to mount point" test -f "$TEST_MOUNT_POINT/fstab"
