#!/bin/sh
# Compiles only the rowing_b objects of the game image (rowing_b helper; adapted from
# src/demolition_b/tools/build.sh). Prints errors, and warnings in src/rowing_b only.
# Usage: src/rowing_b/tools/build.sh [build dir]   (default build-rowing_b)
root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-$root/build-rowing_b}
guest="$build/guest-main"
cmake --build "$guest" -t game_decls 2>&1 | grep -i "error" && exit 1
targets=$(ninja -C "$guest" -t targets all | grep -o "[^ :]*src/rowing_b/[^ :]*\.c\.obj" | sort -u)
out=$(ninja -k 0 -C "$guest" $targets 2>&1)
status=$?
printf '%s\n' "$out" | grep -E "error|FAILED|src/rowing_b/[^:]*:[0-9]+:[0-9]+: warning" | grep -v "^In file included"
exit $status
