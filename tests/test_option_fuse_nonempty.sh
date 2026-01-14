#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

make_test_mount_point
echo "a file" > "$TEST_MOUNT_POINT/file"

if using_fuse3; then
    run_test "Mounting over a non-empty firectory is always allowed" "$BIN" "$TEST_MOUNT_POINT"
    fusermount3 -u "$TEST_MOUNT_POINT"
    run_test "Specifying --nonempty successfully does nothing" "$BIN" --nonempty "$TEST_MOUNT_POINT"
else
    run_test "Mounting over a non-empty firectory fails" ! "$BIN" "$TEST_MOUNT_POINT"
    run_test "Specifying --nonempty allows such mounts" "$BIN" --nonempty "$TEST_MOUNT_POINT"
fi
