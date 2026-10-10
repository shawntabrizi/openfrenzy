#!/bin/sh
# Compile the drillers objects in build-drillers (configure it first, docs/BUILD.md) and
# show the diagnostics of this module only. Analysis/build helper of drillers.
# Usage: sh src/drillers/tools/build.sh [file.c ...]   (default: every .c of the module)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
G="$ROOT/build-drillers/guest-main"
cd "$G" || exit 1
cmake . > /dev/null 2>&1
ninja game_decls 2>&1 | grep -E "error" && exit 1
if [ $# -eq 0 ]; then
	set -- $(cd "$ROOT" && find src/drillers -name '*.c' | sort)
fi
targets=""
for f in "$@"; do
	targets="$targets CMakeFiles/openfrenzy_guest.dir$ROOT/$f.obj"
done
ninja -k 0 $targets 2>&1 | grep -A3 -E "drillers/.*(error|warning)" | grep -v "^--$"
ninja -k 0 $targets > /dev/null 2>&1 && echo "drillers: compiled $# files" || { echo "drillers: FAILED"; exit 1; }
