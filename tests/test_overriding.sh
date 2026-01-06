#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "Mounting with two lists" test_mount "$(fixture test.playlist)" "$(fixture fstab.playlist)"
subtest "File override works" grep "MY FSTAB CONTENT" "$TEST_MOUNT_POINT/fstab" -q
subtest "Unique file not overriden" compare_file_info "$TEST_MOUNT_POINT/hosts" "/etc/hosts"

run_test "Mounting in reverse order" test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)"
subtest "Reverse override works" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "Mounting with --file before list" test_mount -f "$(fixture fstab)" "$(fixture test.playlist)"
subtest "Override via --file works" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"

echo "This is my fstab" > "$TEST_TMP/fstab"
echo "fstab" > "$TEST_TMP/fstabbiest.playlist"
run_test "Mounting with three lists!" test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)" "$TEST_TMP/fstabbiest.playlist"
subtest "Three lists override each other in order" compare_file_info "$TEST_MOUNT_POINT/fstab" "$TEST_TMP/fstab"
