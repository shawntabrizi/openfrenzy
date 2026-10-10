#!/bin/sh
# Compile only the scratchmatch objects of the guest image (analysis helper of the scratchmatch
# module; the whole image may not build while other modules are in progress). Prints the
# errors and the warnings of scratchmatch files (except the XDK headers' __stdcall warnings).
# Usage: sh src/scratchmatch/tools/build.sh [build-dir]   (default build-scratchmatch)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B=${1:-$ROOT/build-scratchmatch}
cd "$B/guest-main" || exit 1
cmake . > /dev/null 2>&1
T=$(ninja -t targets all 2>/dev/null | grep -o "^[^:]*src/scratchmatch/[^:]*\.obj")
ninja -k 0 $T > "$B/sm_build.log" 2>&1
grep -E "FAILED|src/scratchmatch/[^ ]*: (error|warning)" "$B/sm_build.log" | grep -v stdcall
echo "objects: $(echo $T | wc -w), built: $(grep -c 'Building C' "$B/sm_build.log")"
