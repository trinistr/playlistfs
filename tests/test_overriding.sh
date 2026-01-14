#!/bin/sh

TEST_ROOT="$(dirname "$(realpath "$0")")"
. "$TEST_ROOT/setup.sh"

run_test "Mounting with two lists" test_mount "$(fixture test.playlist)" "$(fixture fstab.playlist)"
subtest "File override works" grep "MY FSTAB CONTENT" "$TEST_MOUNT_POINT/fstab" -q
subtest "Unique file not overriden" compare_file_info "$TEST_MOUNT_POINT/hosts" "/etc/hosts"

run_test "Mounting in reverse order" test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)"
subtest "Reverse override works" compare_file_info "$TEST_MOUNT_POINT/fstab" "/etc/fstab"

echo "This is my fstab" > "$TEST_TMP/fstab"
echo "fstab" > "$TEST_TMP/fstabbiest.playlist"
run_test "Mounting with three lists!" test_mount "$(fixture fstab.playlist)" "$(fixture test.playlist)" "$TEST_TMP/fstabbiest.playlist"
subtest "Three lists override each other in order" compare_file_info "$TEST_MOUNT_POINT/fstab" "$TEST_TMP/fstab"

run_test "Mounting with --file before list" test_mount --file "$(fixture fstab)" "$(fixture test.playlist)"
subtest "Override via --file works" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"

run_test "Mounting with --symlink before list" test_mount --symlink "$(fixture fstab)" "$(fixture test.playlist)"
subtest "Override via --symlink works" test "$(readlink "$TEST_MOUNT_POINT/fstab")" = "$(fixture fstab)"

run_test "Mounting with --file and --symlink" test_mount \
    --file "$(fixture fstab.playlist)" --symlink "../../tmp" \
    --file "$(fixture test.playlist)" --symlink "../test.playlist" \
    --symlink "/etc/fstab" --file "$(fixture fstab)"
subtest "--file after --symlink wins" compare_file_info "$TEST_MOUNT_POINT/fstab" "$(fixture fstab)"
subtest "--symlink after --file wins" test "$(readlink "$TEST_MOUNT_POINT/test.playlist")" = "../test.playlist"
subtest "Non-overriden file exists" compare_file_info "$TEST_MOUNT_POINT/fstab.playlist" "$(fixture fstab.playlist)"
subtest "Non-overriden symlink exists" test "$(readlink "$TEST_MOUNT_POINT/tmp")" = "../../tmp"
