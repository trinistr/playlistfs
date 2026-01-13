# --- Variables for tests ---

# Root directory for tests
: ${TEST_ROOT:?Define TEST_ROOT in the test script}
# playlistfs executable
if [ -z "$BIN" ]; then
    BIN="$(realpath "$TEST_ROOT/../dist/bin/playlistfs")"
else
    BIN="$(realpath "$BIN")"
fi

# Current test file name
TEST_FILE="$(basename "$0")"
# Temporary directory for tests
TEST_TMP="$TEST_ROOT/tmp"
# This is defined by `test_mount()` and `make_test_mount_point()`
TEST_MOUNT_POINT=

# --- Functions for tests ---

# Mount filesystem at a test mount point with specified parameters.
test_mount() {
    cleanup
    make_test_mount_point
    "$BIN" "$@" "$TEST_MOUNT_POINT"
}

# Perform cleanup before/after a test.
cleanup() {
    # Mount points may get into a glitchy state where they can't be tested as files,
    # so we can't rely on -d or -e, or anything else.
    if [ -n "$TEST_MOUNT_POINT" -a "$TEST_MOUNT_POINT" != "$TEST_TMP/MOUNT*" ]; then
        if using_fuse3; then
            fusermount3 -u "$TEST_MOUNT_POINT" 2>/dev/null || true
        else
            fusermount -u "$TEST_MOUNT_POINT" 2>/dev/null || true
        fi
        rmdir "$TEST_MOUNT_POINT" 2>/dev/null
    fi
}

# Create a temporary directory and set TEST_MOUNT_POINT to its name.
make_test_mount_point() {
    TEST_MOUNT_POINT="$(mktemp --tmpdir="$TEST_TMP" -dt "MOUNT.XXXXXX")"
}

# Run a test and check exit status.
# Example: `run_test "Contains /" grep "/" /etc/fstab -q`
#
# A test can be:
# 1) skipped with `skip`, in which case it will not be executed at all;
# 2) marked as an expected failure with `pending`, making it pass if it *fails* and `UNPEND` is not set.
#
# By default, status code 0 is expected. This can be changed with `EXPECTED` variable.
# If the first argument after test name is "!", status code must not be equal to expected value.
run_test() {
    local test_name="$1"
    local success
    local failure
    local expected_status=${EXPECTED:-0}
    local not_expected=0
    local result
    local passed
    local log_dir="$TEST_ROOT/logs/$TEST_FILE/"
    local log_stdout="$log_dir/$test_name.stdout.log"
    local log_stderr="$log_dir/$test_name.stderr.log"

    shift
    if [ "$1" = "!" ]; then
        not_expected=1
        shift
    fi
    if [ -n "$PENDING_TEST" ]; then
        print_blue "(pending) "
    fi
    if [ -n "$SKIP_TEST" ]; then
        print_yellow "(skipped) $test_name\n"
        return
    fi
    if [ -n "$PENDING_TEST" -a -z "$UNPEND" ]; then
        success=print_red
        failure=print_blue
    else
        success=print_green
        failure=print_red
    fi

    mkdir -p "$log_dir"
    { 1>"$log_stdout" 2>"$log_stderr" "$@"; }
    result=$?

    [ $result = $expected_status ] || [ $not_expected -eq 1 ]
    passed=$?

    if [ $passed -eq 0 ]; then
        $success "$test_name: ✓"
    else
        $failure "$test_name: ✗"
        if [ -s "$log_stdout" ]; then
            print_yellow "\nSTDOUT:\n"
            cat "$log_dir/$test_name.stdout.log"
        fi
        if [ -s "$log_stderr" ]; then
            print_yellow "\nSTDERR:\n"
            cat "$log_dir/$test_name.stderr.log"
        fi
    fi

    if [ -n "$PENDING_TEST" ]; then
        if [ $passed -eq 0 ]; then
            print_red " unexpected pass!\n"
            passed=127
        elif [ -z "$UNPEND" ]; then
            passed=0
        fi
    fi
    echo

    if [ $passed -ne 0 ]; then
        exit $passed
    fi
}

# Run a test exactly the same as `run_test`, but with an indent in output.
subtest() {
    printf "  "
    run_test "$@"
}

# Mark a test as an expected failure.
# This will make it pass if the command fails and fail if it succeeds.
# Defining `UNPEND` with a non-empty value will reverse this.
# Example: `pending run_test "Contains 1" sh -c "echo '246' | grep '1' -q"`
pending() {
    PENDING_TEST=1 "$@" || exit $?
}

# Skip execution of a test.
# Example: `skip run_test "Big oof" false`
skip() {
    SKIP_TEST=1 "$@" || exit $?
}

# --- Helper functions for tests ---

# Return 0 if running with FUSE 3, non-0 otherwise.
using_fuse3() {
    "$BIN" -V | grep -q "libfuse 3"
}

# Get a full path to a fixture file (printed to stdout).
fixture() {
    printf "$TEST_ROOT/fixtures/$1"
}

# Get file's mode (permissions) (printed to stdout).
extract_mode() {
    LC_ALL=C \stat -c %A "$1"
}

# Get file's metadata from `ls`, excluding its name (printed to stdout).
extract_file_info() {
    LC_ALL=C \ls -lh --full-time -- "$1" | awk '{print $1, $2, $3, $4, $5, $6}'
}

# Compare file metadata between two files to check if they are equivalent (as status code).
# Does not check file contents.
# Suitable for different filesystems.
compare_file_info() {
    local info_a="$(extract_file_info "$1")"
    local info_b="$(extract_file_info "$2")"
    echo "$1: '$info_a'"
    echo "$2: '$info_b'"
    test "$info_a" = "$info_b"
}

# Compare stat metadata between two files to check uf they are equivalent (as status code).
# Does not check file contents.
# Suitable when fiels reside on the same filesystem.
compare_stat_info() {
    local info_a="$(stat --terse -- "$1" | cut -d' ' -f2-)"
    local info_b="$(stat --terse -- "$2" | cut -d' ' -f2-)"
    echo "$1: '$info_a'"
    echo "$2: '$info_b'"
    test "$info_a" == "$info_b"
}

# --- General setup ---

# Set colors only if supported
if [ -t 1 ] && [ -n "$TERM" ] && [ "$TERM" != "dumb" ]; then
    print_red() { printf -- "\033[0;31m$1\033[0m"; }
    print_yellow() { printf -- "\033[0;33m$1\033[0m"; }
    print_green() { printf -- "\033[0;32m$1\033[0m"; }
    print_blue() { printf -- "\033[0;34m$1\033[0m"; }
else
    print_red() { printf -- "$1"; }
    print_yellow() { printf -- "$1"; }
    print_green() { printf -- "$1"; }
    print_blue() { printf -- "$1"; }
fi

full_cleanup() {
    for TEST_MOUNT_POINT in "$TEST_TMP"/MOUNT*; do
        cleanup
    done
    rm -rf "$TEST_TMP"/*
}

# Unmount filesystem on exit.
trap full_cleanup EXIT INT TERM

# Just making sure here!
full_cleanup
mkdir -p "$TEST_TMP"
