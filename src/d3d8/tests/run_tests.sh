#!/bin/sh
# Builds and runs the d3d8 module's tests (src/d3d8/tests/d3d8_test.c) as a guest image, with the
# same compile and link commands as the project's smoke test (taken from the build's ninja files).
#
#   src/d3d8/tests/run_tests.sh [BUILD_DIR] [--window]
#
# BUILD_DIR defaults to build-d3d8 (configure and build it first: docs/BUILD.md). Without --window
# the run is headless (SDL_VIDEO_DRIVER=dummy) and the drawing tests are skipped; with --window a
# window opens and the device draws (needs the window server: run it outside the sandbox).
# Exit status 0 when every check passed.
set -eu

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
BUILD="$ROOT/build-d3d8"
WINDOW=0
for argument in "$@"; do
	case "$argument" in
	--window) WINDOW=1 ;;
	*) BUILD=$(cd "$argument" && pwd) ;;
	esac
done
GUEST="$BUILD/guest-main"
TEST_SOURCE="$ROOT/src/d3d8/tests/d3d8_test.c"
OUT="$BUILD/d3d8-test"
mkdir -p "$OUT/data" "$OUT/saves"

cmake --build "$GUEST" --target openfrenzy_smoke >/dev/null

cd "$GUEST"
SMOKE_SOURCE="$ROOT/port/macos/tests/smoke/smoke.c"
COMPILE=$(ninja -t commands openfrenzy_smoke.elf | grep -F -- "-c $SMOKE_SOURCE" | head -n 1)
LINK=$(ninja -t commands openfrenzy_smoke.elf | grep -F -- "ld.lld" | head -n 1)
if [ -z "$COMPILE" ] || [ -z "$LINK" ]; then
	echo "run_tests.sh: cannot find the smoke test's commands in $GUEST" >&2
	exit 2
fi
SMOKE_OBJECT=$(echo "$COMPILE" | sed -E 's/.* -o ([^ ]+) -c .*/\1/')
COMPILE=$(echo "$COMPILE" | sed -e "s| -MD -MT [^ ]* -MF [^ ]*||" -e "s|-o $SMOKE_OBJECT|-o $OUT/d3d8_test.o|" \
	-e "s|-c $SMOKE_SOURCE|-c $TEST_SOURCE|")
LINK=$(echo "$LINK" | sed -e "s|$SMOKE_OBJECT|$OUT/d3d8_test.o|" -e "s|-Map [^ ]*||" \
	-e "s|-o [^ ]*openfrenzy_smoke.elf|-o $OUT/d3d8_test.elf|")
sh -c "$COMPILE"
sh -c "$LINK"

export OPENFRENZY_DATA_ROOT="$OUT/data" OPENFRENZY_SAVE_ROOT="$OUT/saves"
if [ "$WINDOW" = 1 ]; then
	export OPENFRENZY_WINDOWED=1
	unset SDL_VIDEO_DRIVER || true
else
	export SDL_VIDEO_DRIVER=dummy
fi
set +e
"$BUILD/openfrenzy" "$OUT/d3d8_test.elf" >"$OUT/log.txt" 2>&1
STATUS=$?
set -e
cat "$OUT/log.txt"
if [ "$STATUS" -ne 0 ] || ! grep -q "Game exited (0)" "$OUT/log.txt"; then
	echo "run_tests.sh: FAILED (host status $STATUS)" >&2
	exit 1
fi
echo "run_tests.sh: passed"
