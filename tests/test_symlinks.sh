#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

ln -s FAKE "$TEST_TMP/symlink"
ln -s "$(fixture script.sh)" "$TEST_TMP/script"
printf "symlink\nscript\n" > "$TEST_TMP/link_list"

run_test "Mounting with symlinks in a list" test_mount "$TEST_TMP/link_list"
subtest "Relative link is a link" test "$(extract_mode "$TEST_MOUNT_POINT/symlink")" = "lrwxrwxrwx"
subtest "Relative link is the exact same link" test "$(readlink "$TEST_MOUNT_POINT/symlink")" = "FAKE"
subtest "Relative link has original times" test "$(utils/times "$TEST_MOUNT_POINT/symlink")" = "$(utils/times "$TEST_TMP/symlink")"
subtest "Absolute link is a link" test "$(extract_mode "$TEST_MOUNT_POINT/script")" = "lrwxrwxrwx"
subtest "Absolute link points where it should" test "$(readlink "$TEST_MOUNT_POINT/script")" = "$(fixture script.sh)"
subtest "Absolute link has original times" test "$(utils/times "$TEST_MOUNT_POINT/script")" = "$(utils/times "$TEST_TMP/script")"

run_test "Mounting with symlink as a --file" test_mount -f "$TEST_TMP/symlink"
subtest "Link is a link" test "$(extract_mode "$TEST_MOUNT_POINT/symlink")" = "lrwxrwxrwx"
subtest "Link is the exact same link" test "$(readlink "$TEST_MOUNT_POINT/symlink")" = "FAKE"
subtest "Link has original times" test "$(utils/times "$TEST_MOUNT_POINT/symlink")" = "$(utils/times "$TEST_TMP/symlink")"

run_test "Creating a new symlink to a symlink" ln -s "$TEST_TMP/script" "$TEST_MOUNT_POINT/more_links"
subtest "Link is a link" test "$(extract_mode "$TEST_MOUNT_POINT/more_links")" = "lrwxrwxrwx"
subtest "Link points to the symlink" test "$(readlink "$TEST_MOUNT_POINT/more_links")" = "$TEST_TMP/script"
subtest "Link has its own times" test "$(utils/times "$TEST_MOUNT_POINT/more_links")" != "$(utils/times "$TEST_TMP/script")"
subtest "Link is created today" test "$(extract_file_info "$TEST_MOUNT_POINT/more_links" | cut -d' ' -f6)" = "$(date -I)"

run_test "Mounting with symlinks and --symlinks" test_mount "$TEST_TMP/link_list" --symlinks
subtest "Link points to the original link" test "$(readlink "$TEST_MOUNT_POINT/script")" = "$TEST_TMP/script"
subtest "Link has its own times" test "$(utils/times "$TEST_MOUNT_POINT/script")" != "$(utils/times "$TEST_TMP/script")"
subtest "Link is created today" test "$(extract_file_info "$TEST_MOUNT_POINT/script" | cut -d' ' -f6)" = "$(date -I)"
