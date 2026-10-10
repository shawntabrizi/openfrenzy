#!/bin/sh
# Compile only the hopper objects of the guest image (analysis helper of the hopper
# module; the whole image may not build while other modules are in progress).
# Usage: sh src/hopper/tools/build.sh [build-dir]   (default build-hopper)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B=${1:-$ROOT/build-hopper}
cd "$B/guest-main" || exit 1
cmake . > /dev/null 2>&1
T=$(ninja -t targets all 2>/dev/null | grep -o "^[^:]*src/hopper/[^:]*\.obj")
ninja -k 0 $T > "$B/hop_build.log" 2>&1; grep -E "error|FAILED" "$B/hop_build.log" | grep -v "^ninja"; grep -c "warning:" "$B/hop_build.log"
echo "exit: done"
