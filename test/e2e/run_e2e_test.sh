#!/bin/bash
# run_e2e_test.sh - QLang e2e 测试 runner
# 用法: run_e2e_test.sh <driver> <ql_file> <runtime_lib> [expected_output] [expected_exit]

set -e

DRIVER="$1"
QL_FILE="$2"
RUNTIME_LIB="$3"
EXPECTED_OUT="${4:-}"
EXPECTED_EXIT="${5:-0}"

if [ -z "$DRIVER" ] || [ -z "$QL_FILE" ] || [ -z "$RUNTIME_LIB" ]; then
    echo "Usage: $0 <driver> <ql_file> <runtime_lib> [expected_output] [expected_exit]"
    exit 1
fi

# Locate llc binary (handles llc-18, llc-17, etc.)
LLC=""
for candidate in llc llc-18 llc-17 llc-16; do
    if command -v "$candidate" &>/dev/null; then
        LLC="$candidate"
        break
    fi
done
if [ -z "$LLC" ]; then
    echo "FAIL: llc not found in PATH"
    exit 1
fi

# Locate clang binary (handles clang-18, clang-17, etc.)
CLANG=""
for candidate in clang clang-18 clang-17 clang-16; do
    if command -v "$candidate" &>/dev/null; then
        CLANG="$candidate"
        break
    fi
done
if [ -z "$CLANG" ]; then
    echo "FAIL: clang not found in PATH"
    exit 1
fi

TMPDIR_WORK=$(mktemp -d)
trap "rm -rf $TMPDIR_WORK" EXIT

LL_FILE="$TMPDIR_WORK/test.ll"
OBJ_FILE="$TMPDIR_WORK/test.o"
EXE_FILE="$TMPDIR_WORK/test_exe"

# Step 1: compile .ql → .ll
if ! "$DRIVER" "$QL_FILE" -S -o "$LL_FILE" 2>/dev/null; then
    # Compilation failed
    if [ "$EXPECTED_EXIT" = "nonzero" ]; then
        # Compile error is expected for abort tests
        echo "PASS: $(basename $QL_FILE)"
        exit 0
    else
        echo "FAIL: qlang compilation failed for $QL_FILE"
        exit 1
    fi
fi

# Step 2: compile .ll → .o
if ! "$LLC" "$LL_FILE" --filetype=obj -o "$OBJ_FILE" 2>/dev/null; then
    echo "FAIL: llc compilation failed"
    exit 1
fi

# Step 3: link
if ! "$CLANG" -no-pie "$OBJ_FILE" "$RUNTIME_LIB" -o "$EXE_FILE" 2>/dev/null; then
    echo "FAIL: linking failed"
    exit 1
fi

# Step 4: run (disable set -e temporarily to capture non-zero exit codes)
set +e
"$EXE_FILE" > "$TMPDIR_WORK/actual_output.txt" 2>/dev/null
actual_exit=$?
actual_output=$(cat "$TMPDIR_WORK/actual_output.txt")
set -e

# Step 5: verify exit code
if [ "$EXPECTED_EXIT" = "nonzero" ]; then
    if [ "$actual_exit" -eq 0 ]; then
        echo "FAIL: expected non-zero exit, got 0"
        exit 1
    fi
else
    if [ "$actual_exit" != "$EXPECTED_EXIT" ]; then
        echo "FAIL: expected exit $EXPECTED_EXIT, got $actual_exit"
        exit 1
    fi
fi

# Step 6: verify output if provided
if [ -n "$EXPECTED_OUT" ] && [ -f "$EXPECTED_OUT" ]; then
    expected=$(cat "$EXPECTED_OUT")
    if [ "$actual_output" != "$expected" ]; then
        echo "FAIL: output mismatch for $QL_FILE"
        echo "Expected: $(cat $EXPECTED_OUT | cat -A)"
        echo "Got:      $(echo "$actual_output" | cat -A)"
        exit 1
    fi
fi

echo "PASS: $(basename $QL_FILE)"
exit 0
