BIN="$(realpath "$TESTS_BASE/../dist/bin/playlistfs")"
TEST_TMP="$TESTS_BASE/tmp"
TEST_MOUNT_POINT="$TESTS_BASE/tmp/mount_point"

test_mount() {
    cleanup
    mkdir -p "$TEST_MOUNT_POINT"
    "$BIN" "$@" "$TEST_MOUNT_POINT"
}

cleanup() {
    fusermount -u "$TEST_MOUNT_POINT" 2>/dev/null || true
}

trap cleanup EXIT INT TERM

run_test() {
    local test_name="$1"
    shift
    printf -- "$test_name: "
    if (1>/dev/null 2>&1 "$@"); then
        echo "✓"
    else
        echo "✗"
        exit 1
    fi
}

pending() {
    printf "(pending) "
    ("$@")
    if [ $? -eq 0 ]; then
        echo "unexpected pass!"
        exit 1
    fi
}

fixture() {
    printf "$TESTS_BASE/fixtures/$1"
}

extract_file_info() {
    LC_ALL=C \ls -lh --full-time "$1" | awk '{print $1, $2, $3, $4, $5, $6}'
}

extract_mode() {
    LC_ALL=C \stat -c %A "$1"
}

compare_file_info() {
    test "q$(extract_file_info "$1")" = "q$(extract_file_info "$2")"
}
