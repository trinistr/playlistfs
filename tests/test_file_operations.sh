#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

# Deletion

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Deleting a file" rm "$TEST_MOUNT_POINT/test.playlist"
subtest "File no longer exists" test ! -f "$TEST_MOUNT_POINT/test.playlist"
subtest "There are less files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 2"
subtest "Filesystem reports expected less number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 2"

# Linking

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Linking a file" ln "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
subtest "Old name exists" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "New name exists" test -f "$TEST_MOUNT_POINT/test"
subtest "New name refers to the same file" compare_stat_info "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
subtest "There are more files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 4"
subtest "Filesystem reports expected greater number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 4"

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Symlinking a file" ln -s "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
subtest "File exists" test -f "$TEST_MOUNT_POINT/test.playlist"
subtest "Link exists" test -f "$TEST_MOUNT_POINT/test"
subtest "Link links to the file" test "$(readlink "$TEST_MOUNT_POINT/test")" = "$TEST_MOUNT_POINT/test.playlist"
subtest "There are more files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 4"
subtest "Filesystem reports expected greater number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 4"

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Symlinking a file outside" ln -s "$(fixture ëñ)" "$TEST_MOUNT_POINT/test"
subtest "Link exists" test -f "$TEST_MOUNT_POINT/test"
subtest "Link links to the file" test "$(readlink "$TEST_MOUNT_POINT/test")" = "$(fixture ëñ)"
subtest "There are more files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 4"
subtest "Filesystem reports expected greater number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 4"

run_test "Linking a symlink" ln "$TEST_MOUNT_POINT/test" "$TEST_MOUNT_POINT/testy"
subtest "Both links are are the same symlink" compare_stat_info "$TEST_MOUNT_POINT/test" "$TEST_MOUNT_POINT/testy"
subtest "Link links to the file" test "$(readlink "$TEST_MOUNT_POINT/testy")" = "$(fixture ëñ)"
subtest "There are more files now" sh -c "test \$(ls '$TEST_MOUNT_POINT' | wc -l) = 5"
subtest "Filesystem reports expected greater number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 5"

# Renaming

test_mount "$TEST_ROOT/fixtures/test.playlist"
run_test "Renaming a file" mv "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/fstab2"
subtest "Old name no longer exists" test ! -f "$TEST_MOUNT_POINT/fstab"
subtest "New name exists" test -f "$TEST_MOUNT_POINT/fstab2"
subtest "New name refers to the same file" compare_file_info "$TEST_MOUNT_POINT/fstab2" "/etc/fstab"
subtest "Filesystem reports expected number of files" sh -c "test \$(stat --file-system --format=%c '$TEST_MOUNT_POINT') = 3"

# From rename(2):
# If oldpath and newpath are existing hard links referring to the same file, then rename() does nothing, and returns a success status.
# `mv` fails in this situation, to show that nothing was done, so we use a utility.
test_mount "$TEST_ROOT/fixtures/test.playlist"
ln "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
run_test "rename(2) successfully does nothing when renaming a file to a different link" utils/rename "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"
subtest "Both names refer to the same file" compare_stat_info "$TEST_MOUNT_POINT/test.playlist" "$TEST_MOUNT_POINT/test"

# rename flags are supported on FUSE 3, but not 2.
# Not all versions of `mv` support `--exchange` either, and `--update=none` is supported in userspace, so we use utilities.
if using_fuse3; then
    test_mount "$TEST_ROOT/fixtures/test.playlist"
    run_test "Renaming with RENAME_EXCHANGE" utils/rename_exchange "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/test.playlist"
    subtest "New name refers to renamed file" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "/etc/fstab"
    subtest "Old name refers to the other file" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture test.playlist)"

    test_mount "$TEST_ROOT/fixtures/test.playlist"
    run_test "Renaming a file with RENAME_NOREPLACE with unused name" utils/rename_noreplace "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/test"
    subtest "Old name has disappeared" test ! -f "$TEST_MOUNT_POINT/fstab"
    subtest "New name refers to renamed file" compare_file_info "$TEST_MOUNT_POINT/test" "/etc/fstab"

    test_mount "$TEST_ROOT/fixtures/test.playlist"
    run_test "Renaming a file with RENAME_NOREPLACE when name exists fails" ! utils/rename_noreplace "$TEST_MOUNT_POINT/fstab" "$TEST_MOUNT_POINT/test.playlist"
    subtest "Old name is still present" test -f "$TEST_MOUNT_POINT/fstab"
    subtest "New name still refers to original file" compare_file_info "$TEST_MOUNT_POINT/test.playlist" "$(fixture test.playlist)"
fi
