#!/bin/sh
# Builds and runs the bkernel module's tests (bkernel_test.c) as one guest image.
#
#   src/bkernel/tests/run_tests.sh [build dir]     (default: build-bkernel)
#
# Needs a finished project build in that directory (cmake --build <dir>). The test is
# compiled with the game build's command for a bkernel file (game flags, of_game.h and the
# generated declarations) plus -DOPENFRENZY_BKERNEL_TEST, and linked with the game build's
# objects of src/bkernel, the generated weak stubs (every other game function), the guest
# runtime, the platform layer and musl. The top-level CMakeLists.txt is the architect's, so
# the test is not registered with ctest.
#
# The package tests need the disc image (OPENFRENZY_ISO, default orig/game.xiso.iso): they
# read Packages/bgeffects.xbp from it (bkFopen, and CreateFileA in the background loader).
# Without the disc image they are skipped, and the script says so.
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-bkernel}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/bkernel/tests/bkernel_test.c"
out="$build/bkernel-tests"
image="$out/bkernel_test.elf"
mkdir -p "$out/data" "$out/saves"

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'bkernel/bKernel/xbDebug.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no game build commands in $guest (build the project first)" >&2
	exit 2
fi

# compile: the xbDebug.c command with the test source and object
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_BKERNEL_TEST=1 -o $out/bkernel_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: the guest runtime, the stubs, the bkernel objects (not the test's own empty object),
# the test, and the libraries of the game image's command
objects=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.obj$' |
	grep -E 'guest_runtime\.dir|game_stubs\.dir|/src/bkernel/' | grep -v '/tests/')
libs=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.a$')
lld=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E 'ld\.lld$' | head -n 1)
script=$(printf '%s\n' "$link" | sed -e 's/.* -T \([^ ]*\) .*/\1/')
(cd "$guest" && $lld -m aarch64linux -static -nostdlib -T "$script" $objects "$out/bkernel_test.o" \
	$libs -o "$image")

iso=${OPENFRENZY_ISO:-$root/orig/game.xiso.iso}
if [ -f "$iso" ]; then
	export OPENFRENZY_ISO="$iso" BKERNEL_TEST_DISC=1
else
	echo "run_tests.sh: no disc image ($iso): the package tests are SKIPPED" >&2
fi
export OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" OPENFRENZY_WINDOWED=1
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER-dummy}"

set +e
perl -e 'alarm shift; exec @ARGV or die' "${OF_TEST_TIMEOUT:-60}" "$build/openfrenzy" "$image" \
	> "$out/test.log" 2>&1
status=$?
set -e
grep -E 'FAIL|SKIP|bkernel tests|Game exited' "$out/test.log" || true
if [ $status -eq 0 ] && grep -q "Game exited (0)" "$out/test.log"; then
	echo "PASS bkernel_test"
	exit 0
fi
echo "FAIL bkernel_test (exit $status, log $out/test.log)"
exit 1
