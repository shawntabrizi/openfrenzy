#!/bin/sh
# Compile only the foxhunt objects of the guest image (analysis helper of the foxhunt
# module; the whole image may not build while other modules are in progress).
# Usage: sh src/foxhunt/tools/build.sh [build-dir]   (default build-foxhunt)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B=${1:-$ROOT/build-foxhunt}
cd "$B/guest-main" || exit 1
cmake . > /dev/null 2>&1
T=$(ninja -t targets all 2>/dev/null | grep -o "^[^:]*src/foxhunt/[^:]*\.obj")
ninja -k 0 $T > "$B/fh_build.log" 2>&1; grep -E "error|FAILED" "$B/fh_build.log" | grep -v "^ninja"; grep "src/foxhunt.*warning:" "$B/fh_build.log" | sort -u
echo "exit: done"
