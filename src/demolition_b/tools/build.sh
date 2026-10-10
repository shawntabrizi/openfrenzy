#!/bin/sh
# Compiles only the demolition_b objects of the game image (demolition_b helper).
# Usage: src/demolition_b/tools/build.sh [build dir]   (default build-demolition_b)
root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-$root/build-demolition_b}
guest="$build/guest-main"
cmake --build "$guest" -t game_decls >/dev/null || exit 1
targets=$(ninja -C "$guest" -t targets all | grep -o "[^ :]*src/demolition_b/[^ :]*\.c\.obj" | sort -u)
ninja -C "$guest" $targets 2>&1 | grep -v "^ninja: Entering\|^\[" 
