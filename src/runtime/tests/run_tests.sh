#!/bin/sh
# Runtime module tests. Each tests/test_*.c is one guest image: compiled with the platform
# layer's exact flags (copied from the CMake build's command for port/macos/tests/smoke/smoke.c),
# linked like openfrenzy_smoke.elf, and run in the host. A test passes when the guest exits 0.
#
# The CMake build has no hook for module tests (CMakeLists.txt belongs to the architect), so
# this script drives the already-configured build directory.
#
# Usage: src/runtime/tests/run_tests.sh [build-dir] [test-name ...]   (default build-runtime, all)
# Environment:
#   SDL_VIDEO_DRIVER   default dummy (headless). Set it empty (SDL_VIDEO_DRIVER=) to open real
#                      windows; that needs the window server, so not inside the Claude sandbox.
#   OPENFRENZY_ISO     disc image (default <root>/orig/game.xiso.iso); disc tests skip without it
set -eu
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
T=$ROOT/src/runtime/tests
B=${1:-build-runtime}
[ $# -gt 0 ] && shift
case $B in /*) ;; *) B=$ROOT/$B ;; esac
G=$B/guest-main
OUT=$B/runtime-tests
LLD=${OPENFRENZY_LLD:-/opt/homebrew/opt/lld@22/bin/ld.lld}
mkdir -p "$OUT"

# only what the tests link: the host, the platform layer, musl and the guest runtime (a broken
# game module elsewhere in src/ does not stop them)
{ cmake --build "$B" --target openfrenzy && cmake --build "$G" --target ofplatform guestc guest_runtime; } \
	> "$OUT/build.log" 2>&1 || { tail -30 "$OUT/build.log"; echo "run_tests: build failed" >&2; exit 1; }

# the smoke test's compile command, up to its dependency/output options
CC_LINE=$(ninja -C "$G" -t commands openfrenzy_smoke | grep -F 'tests/smoke/smoke.c' | head -1 | sed 's/ -MD .*//')
[ -n "$CC_LINE" ] || { echo "run_tests: cannot find the smoke compile command in $G" >&2; exit 1; }
RUNTIME_OBJECTS=$(ls "$G"/CMakeFiles/guest_runtime.dir/runtime/*.obj "$G"/CMakeFiles/guest_runtime.dir/gen/*.obj)

ISO=${OPENFRENZY_ISO:-$ROOT/orig/game.xiso.iso}
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER-dummy}"
export OPENFRENZY_DATA_ROOT="$OUT/data" OPENFRENZY_SAVE_ROOT="$OUT/saves"
mkdir -p "$OPENFRENZY_DATA_ROOT" "$OPENFRENZY_SAVE_ROOT"
[ -f "$ISO" ] && export OPENFRENZY_ISO="$ISO"

if [ $# -gt 0 ]; then NAMES="$*"; else NAMES=$(cd "$T" && ls test_*.c | sed 's/\.c$//'); fi
pass=0 fail=0 xfail=0
for name in $NAMES; do
	obj=$OUT/$name.o image=$OUT/$name.elf log=$OUT/$name.log
	if ! eval "$CC_LINE -I$T -o $obj -c $T/$name.c" > "$log" 2>&1; then
		echo "FAIL $name (compile, $log)"; cat "$log"; fail=$((fail + 1)); continue
	fi
	if ! "$LLD" -m aarch64linux -static -nostdlib -T "$ROOT/port/macos/guest.ld" $RUNTIME_OBJECTS "$obj" \
		"$G/libofplatform.a" "$G/libguestc.a" -o "$image" >> "$log" 2>&1; then
		echo "FAIL $name (link, $log)"; cat "$log"; fail=$((fail + 1)); continue
	fi
	set +e
	"$B/openfrenzy" "$image" >> "$log" 2>&1
	status=$?
	set -e
	known=$(grep -v '^#' "$T/known_failures.txt" 2>/dev/null | awk -v n="$name" '$1 == n { $1 = ""; print; exit }')
	if [ $status -eq 0 ] && grep -q "Game exited (0)" "$log"; then
		if [ -n "$known" ]; then
			echo "XPASS $name: passes now; remove it from known_failures.txt"
		else
			echo "ok   $name"
		fi
		pass=$((pass + 1))
	elif [ -n "$known" ]; then
		echo "XFAIL $name (exit $status):$known"; xfail=$((xfail + 1))
	else
		echo "FAIL $name (exit $status, $log)"; tail -40 "$log"; fail=$((fail + 1))
	fi
done
echo "run_tests: $pass passed, $fail failed, $xfail known failures (SDL_VIDEO_DRIVER=${SDL_VIDEO_DRIVER:-<real>})"
[ $fail -eq 0 ]
