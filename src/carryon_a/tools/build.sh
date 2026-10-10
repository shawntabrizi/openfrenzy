#!/bin/sh
# Compile the carryon_a objects in build-carryon_a (configure it first, docs/BUILD.md) and
# show the diagnostics of this module only. Analysis/build helper of carryon_a.
# Usage: sh src/carryon_a/tools/build.sh [file.c ...]   (default: every .c of the module)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
G="$ROOT/build-carryon_a/guest-main"
cd "$G" || exit 1
cmake . > /dev/null 2>&1
ninja game_decls 2>&1 | grep -E "error" && exit 1
if [ $# -eq 0 ]; then
	set -- $(cd "$ROOT" && find src/carryon_a -name '*.c' | sort)
fi
targets=""
for f in "$@"; do
	targets="$targets CMakeFiles/openfrenzy_guest.dir$ROOT/$f.obj"
done
ninja -k 0 $targets 2>&1 | grep -A3 -E "carryon_a/.*(error|warning)" | grep -v "^--$"
ninja -k 0 $targets > /dev/null 2>&1 && echo "carryon_a: compiled $# files" || { echo "carryon_a: FAILED"; exit 1; }
