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

# The platform library mixes every platform module, and another module's work in progress may
# not compile. So: this module's objects must build (fail loud); the rest is built as far as it
# goes, and the test links the objects (a module whose newest source fails keeps its last object).
cmake --build "$BUILD" --target openfrenzy >/dev/null
cmake --build "$GUEST" --target guestc guest_runtime >/dev/null
cd "$GUEST"
D3D8_OBJECTS=$(ninja -t targets all | sed -n 's|^\(CMakeFiles/ofplatform.dir/[^:]*/src/d3d8/[^:]*\.obj\):.*|\1|p')
if [ -z "$D3D8_OBJECTS" ]; then
	echo "run_tests.sh: no d3d8 objects in $GUEST" >&2
	exit 2
fi
ninja $D3D8_OBJECTS >"$OUT/build.log" 2>&1 || { cat "$OUT/build.log"; echo "run_tests.sh: d3d8 build FAILED" >&2; exit 1; }
ninja -k 0 ofplatform >"$OUT/build-others.log" 2>&1 || \
	echo "run_tests.sh: note: other platform modules do not build now (see $OUT/build-others.log); their last objects are used"
PLATFORM_OBJECTS=$(find CMakeFiles/ofplatform.dir -name '*.obj' | sort)
rm -f "$OUT/libofplatform_test.a"
/opt/homebrew/opt/llvm@22/bin/llvm-ar rcs "$OUT/libofplatform_test.a" $PLATFORM_OBJECTS

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
	-e "s|-o [^ ]*openfrenzy_smoke.elf|-o $OUT/d3d8_test.elf|" -e "s| libofplatform.a | $OUT/libofplatform_test.a |")
sh -c "$COMPILE"
sh -c "$LINK"

export OPENFRENZY_DATA_ROOT="$OUT/data" OPENFRENZY_SAVE_ROOT="$OUT/saves"
# the generated GLSL goes here, and is compiled below with glslangValidator (brew install glslang)
rm -rf "$OUT/glsl"
mkdir -p "$OUT/glsl"
export OPENFRENZY_D3D8_TEST_GLSL="$OUT/glsl"
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
if command -v glslangValidator >/dev/null 2>&1; then
	BAD=0
	COUNT=0
	for SHADER in "$OUT"/glsl/*.vert "$OUT"/glsl/*.frag; do
		[ -f "$SHADER" ] || continue
		COUNT=$((COUNT + 1))
		if ! glslangValidator "$SHADER" >"$SHADER.log" 2>&1; then
			BAD=$((BAD + 1))
			echo "GLSL FAIL $SHADER"
			cat "$SHADER.log"
		fi
	done
	echo "run_tests.sh: $COUNT generated shaders compiled by glslangValidator, $BAD failed"
	if [ "$COUNT" -eq 0 ] || [ "$BAD" -ne 0 ]; then
		echo "run_tests.sh: FAILED (GLSL)" >&2
		exit 1
	fi
else
	echo "run_tests.sh: glslangValidator not found: the generated GLSL was NOT compiled (brew install glslang)"
fi
echo "run_tests.sh: passed"
