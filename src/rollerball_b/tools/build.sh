#!/bin/sh
# Compile only the rollerball_b objects of the guest image (analysis helper of the rollerball_b
# module, based on src/mousewhack/tools/build.sh; the whole image may not build while other
# modules are in progress). Prints the errors and the warnings of rollerball_b files.
# Usage: sh src/rollerball_b/tools/build.sh [build-dir]   (default build-rollerball_b)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B=${1:-$ROOT/build-rollerball_b}
cd "$B/guest-main" || exit 1
cmake . > /dev/null 2>&1
T=$(ninja -t targets all 2>/dev/null | grep -o "^[^:]*src/rollerball_b/[^:]*\.obj")
ninja -k 0 $T > "$B/rbb_build.log" 2>&1
grep -E "FAILED|error:" "$B/rbb_build.log" | grep -v "^ninja"
grep -E "src/rollerball_b/.*warning:" "$B/rbb_build.log"
echo "objects: $(echo $T | wc -w)  failed: $(grep -c FAILED "$B/rbb_build.log")"
