#!/bin/sh
# Compile the tankedup_b objects in build-tankedup_b (configure it first, docs/BUILD.md) and
# show the diagnostics of this module only. Build helper of tankedup_b (copied from src/carryon_a/tools/build.sh).
# Usage: sh src/tankedup_b/tools/build.sh [file.c ...]   (default: every .c of the module)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
G="$ROOT/build-tankedup_b/guest-main"
cd "$G" || exit 1
cmake . > /dev/null 2>&1
ninja game_decls 2>&1 | grep -E "error" && exit 1
if [ $# -eq 0 ]; then
	set -- $(cd "$ROOT" && find src/tankedup_b -name "*.c" -not -path "*/tests/*" | sort)
fi
targets=""
for f in "$@"; do
	targets="$targets CMakeFiles/openfrenzy_guest.dir$ROOT/$f.obj"
done
ninja -k 0 $targets 2>&1 | grep -A3 -E "tankedup_b/.*(error|warning)" | grep -v "^--$"
ninja -k 0 $targets > /dev/null 2>&1 && echo "tankedup_b: compiled $# files" || { echo "tankedup_b: FAILED"; exit 1; }
