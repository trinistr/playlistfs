#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "--version run" "$BIN" --version
subtest "Outputs current version with --version" sh -c "'$BIN' --version | grep -Eq 'PlaylistFS [0-9]+\.[0-9]+\.[0-9]+'"

run_test "-V run" "$BIN" -V
subtest "Outputs current version with -V" sh -c "'$BIN' -V | grep -Eq 'PlaylistFS [0-9]+\.[0-9]+\.[0-9]+'"

cleanup
run_test "--version can be combined with mounting" \
    sh -c "'$BIN' --version '$(fixture test.playlist)' '$TEST_MOUNT_POINT' \
    | grep -Eq 'PlaylistFS [0-9]+\.[0-9]+\.[0-9]+'"
subtest "Files added successfully to mount point" test -f "$TEST_MOUNT_POINT/fstab"
