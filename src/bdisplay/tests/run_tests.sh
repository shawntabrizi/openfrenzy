#!/bin/sh
# Builds and runs the bdisplay module's tests (bdisplay_test.c) as one guest image.
#
#   src/bdisplay/tests/run_tests.sh [build dir]     (default: build-bdisplay)
#
# Needs a finished project build in that directory (cmake --build <dir>). The test is
# compiled with the game build's command for a bdisplay file (game flags, of_game.h and the
# generated declarations) plus -DOPENFRENZY_BDISPLAY_TEST, and linked with the game build's
# objects of src/bdisplay and src/bkernel (the maths), the generated weak stubs (every other
# game function), the guest runtime, the platform layer and musl. No D3D device is created.
# The top-level CMakeLists.txt is the architect's, so the test is not registered with ctest.
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-bdisplay}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/bdisplay/tests/bdisplay_test.c"
out="$build/bdisplay-tests"
image="$out/bdisplay_test.elf"
mkdir -p "$out/data" "$out/saves"

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'bdisplay/BDISPLAY/xbData.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no game build commands in $guest (build the project first)" >&2
	exit 2
fi

# compile: the xbData.c command with the test source and object
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_BDISPLAY_TEST=1 -o $out/bdisplay_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: the guest runtime, the stubs, the bdisplay and bkernel objects (not the tests'
# own objects), the test, and the libraries of the game image's command
objects=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.obj$' |
	grep -E 'guest_runtime\.dir|game_stubs\.dir|/src/bdisplay/|/src/bkernel/' | grep -v '/tests/')
libs=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.a$')
lld=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E 'ld\.lld$' | head -n 1)
script=$(printf '%s\n' "$link" | sed -e 's/.* -T \([^ ]*\) .*/\1/')
(cd "$guest" && $lld -m aarch64linux -static -nostdlib -T "$script" $objects "$out/bdisplay_test.o" \
	$libs -o "$image")

export OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" OPENFRENZY_WINDOWED=1
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER-dummy}"

set +e
perl -e 'alarm shift; exec @ARGV or die' "${OF_TEST_TIMEOUT:-60}" "$build/openfrenzy" "$image" \
	> "$out/test.log" 2>&1
status=$?
set -e
grep -E 'FAIL|bdisplay tests|Game exited' "$out/test.log" || true
if [ $status -eq 0 ] && grep -q "Game exited (0)" "$out/test.log"; then
	echo "PASS bdisplay_test"
	exit 0
fi
echo "FAIL bdisplay_test (exit $status, log $out/test.log)"
exit 1
