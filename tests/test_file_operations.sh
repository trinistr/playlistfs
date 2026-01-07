#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Deleting a file" rm "$TEST_MOUNT_POINT/test.playlist"
subtest "File no longer exists" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "There are less files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 2"
subtest "Filesystem reports expected less number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 2"

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Linking a file" ln "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
subtest "Old name exists" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "New name exists" test -f "$TEST_MOUNT_POINT/test"
# FUSE 3 allows to set timeouts to 0, which fixes the caching problem.
# But really, we need to look for a better solution (probably it's inode numbers again).
if using_fuse3; then
    subtest "New name refers to the same file" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
    subtest "There are more files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 4"
fi
subtest "Filesystem reports expected greater number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 4"

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Renaming a file" mv "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/fstab2"
subtest "Old name no longer exists" test ! -f "$TEST_MOUNT_POINT/fstab"
subtest "New name exists" test -f "$TEST_MOUNT_POINT/fstab2"
subtest "New name refers to the same file" compare_file_info "$TEST_MOUNT_POINT/fstab2" "/etc/fstab"
subtest "Filesystem reports expected number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 3"

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Try renaming a file with --no-clobber" mv --no-clobber "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/test.playlist"
subtest "Old name is still present" test -f "$TEST_MOUNT_POINT/fstab"
subtest "New name still refers to original file" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "$(fixture test.playlist)"

# exchange rename is supported on 3, but not 2
if using_fuse3; then
    test_mount "$TEST_ROOT/fixtures/test.playlist"
    run_test "Renaming with --exchange" mv --exchange "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/test.playlist"
    subtest "New name refers to renamed file" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "/etc/fstab"
    subtest "Old name refers to the other file" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture test.playlist)"
fi

# From rename(2):
# If oldpath and newpath are existing hard links referring to the same file, then rename() does nothing, and returns a success status.
test_mount "$TEST_ROOT/fixtures/test.playlist"
ln "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
run_test "Renaming file to a different link to the same file" mv "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
pending subtest "Old name is still present" test -f "$TEST_MOUNT_POINT/test.playlist"
skip subtest "New name is still present" test -f "$TEST_MOUNT_POINT/test"
skip subtest "Both refer to the same file" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
