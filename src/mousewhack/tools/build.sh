#!/bin/sh
# Compile only the mousewhack objects of the guest image (analysis helper of the mousewhack
# module; the whole image may not build while other modules are in progress).
# Usage: sh src/mousewhack/tools/build.sh [build-dir]   (default build-mousewhack)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B=${1:-$ROOT/build-mousewhack}
cd "$B/guest-main" || exit 1
cmake . > /dev/null 2>&1
T=$(ninja -t targets all 2>/dev/null | grep -o "^[^:]*src/mousewhack/[^:]*\.obj")
ninja -k 0 $T > "$B/mw_build.log" 2>&1; grep -E "error|FAILED" "$B/mw_build.log" | grep -v "^ninja"; grep -c "warning:" "$B/mw_build.log"
echo "exit: done"
