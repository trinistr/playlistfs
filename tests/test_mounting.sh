#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
BIN="$(realpath "$TESTS_BASE/../dist/bin/playlistfs")"
TEST_MOUNT_POINT="$TESTS_BASE/tmp/mount_point"

mkdir -p "$TEST_MOUNT_POINT"

"$BIN" "$TESTS_BASE/fixtures/test.playlist" "$TEST_MOUNT_POINT"

if [ -f "$TEST_MOUNT_POINT/hosts" -a -f "$TEST_MOUNT_POINT/fstab" -a -f "$TEST_MOUNT_POINT/test.playlist" ]; then
    echo "Mounting successful - all files created"
    STATUS=0
else
    echo "Mounting failed - files not created"
    STATUS=1
fi

fusermount -u "$TEST_MOUNT_POINT"

exit $STATUS
