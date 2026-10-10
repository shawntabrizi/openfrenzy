#!/bin/sh
# Compile only the rollerball_a objects of the guest image (analysis helper of the
# rollerball_a module, copied from src/mousewhack/tools/build.sh; the whole image may not
# build while other modules are in progress).
# Usage: sh src/rollerball_a/tools/build.sh [build-dir]   (default build-rollerball_a)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B=${1:-$ROOT/build-rollerball_a}
cd "$B/guest-main" || exit 1
cmake . > /dev/null 2>&1
T=$(ninja -t targets all 2>/dev/null | grep -o "^[^:]*src/rollerball_a/[^:]*\.obj")
ninja -k 0 $T > "$B/rb_build.log" 2>&1; grep -E "error|FAILED" "$B/rb_build.log" | grep -v "^ninja"; echo "warnings: $(grep -c "warning:" "$B/rb_build.log")"
echo "exit: done"
