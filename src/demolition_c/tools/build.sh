#!/bin/sh
# Configures build-demolition_c and compiles only the demolition_c objects (the whole game
# image may fail on other modules' files). Usage: sh src/demolition_c/tools/build.sh
set -e
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
B="$ROOT/build-demolition_c"
cmake -S "$ROOT" -B "$B" -G Ninja >/dev/null
cmake --build "$B" --target guest_main-configure >/dev/null 2>&1 || true
cd "$B/guest-main"
cmake . >/dev/null
T=$(ninja -t targets all | grep -o '^[^:]*src/demolition_c/[^:]*\.obj')
ninja -k 0 $T
