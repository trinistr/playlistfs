#!/bin/sh

TESTS_BASE="$(dirname "$(realpath "$0")")"
source "$TESTS_BASE/setup.sh"

run_test "Mounting with two lists" test_mount "$(fixture test.playlist)" "$(fixture fstab.playlist)"
run_test "File override works" grep "MY FSTAB CONTENT" "$TEST_MOUNT_POINT/fstab" -q
run_test "Different file not overriden" compare_file_info "$TEST_MOUNT_POINT/hosts" "/etc/hosts"

run_test "Mounting in reverse order" test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)"
run_test "Reverse override works" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

run_test "Mounting with --file before list" test_mount -f "$(fixture fstab)" "$(fixture test.playlist)"
run_test "Override via --file works" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"

echo "This is my fstab" > "$TEST_TMP/fstab"
echo "fstab" > "$TEST_TMP/fstabbiest.playlist"
run_test "Mounting with three lists!" test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)" "$TEST_TMP/fstabbiest.playlist"
run_test "Three lists override each other in order" compare_file_info "$TEST_MOUNT_POINT/fstab" "$TEST_TMP/fstab"
rm "$TEST_TMP/fstab" "$TEST_TMP/fstabbiest.playlist"
