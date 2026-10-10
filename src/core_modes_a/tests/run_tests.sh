#!/bin/sh
# Builds and runs the core_modes_a module's tests (core_modes_a_test.c) as one guest image.
#
#   src/core_modes_a/tests/run_tests.sh [build dir]     (default: build-core_modes_a)
#
# Needs a finished project build in that directory (cmake --build <dir>). The test is
# compiled with the game build's command for a core_modes_a file (game flags, of_game.h and
# the generated declarations) plus -DOPENFRENZY_CORE_MODES_A_TEST, and linked with the game
# build's objects of src/core_modes_a, the generated weak stubs (every other game function),
# the guest runtime, the platform layer and musl. (Adapted from src/bkernel/tests/run_tests.sh.)
# The top-level CMakeLists.txt is the architect's, so the test is not registered with ctest.
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-core_modes_a}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/core_modes_a/tests/core_modes_a_test.c"
out="$build/core_modes_a-tests"
image="$out/core_modes_a_test.elf"
mkdir -p "$out/data" "$out/saves"

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'core_modes_a/Core/BG_Effects.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no game build commands in $guest (build the project first)" >&2
	exit 2
fi

# compile: the BG_Effects.c command with the test source and object
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_CORE_MODES_A_TEST=1 -o $out/core_modes_a_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: the guest runtime, the stubs, the core_modes_a objects (not the test's own empty object),
# the test, and the libraries of the game image's command
objects=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.obj$' |
	grep -E 'guest_runtime\.dir|game_stubs\.dir|/src/core_modes_a/' | grep -v '/tests/')
libs=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.a$')
lld=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E 'ld\.lld$' | head -n 1)
script=$(printf '%s\n' "$link" | sed -e 's/.* -T \([^ ]*\) .*/\1/')
(cd "$guest" && $lld -m aarch64linux -static -nostdlib -T "$script" $objects "$out/core_modes_a_test.o" \
	$libs -o "$image")

export OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" OPENFRENZY_WINDOWED=1
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER-dummy}"

set +e
perl -e 'alarm shift; exec @ARGV or die' "${OF_TEST_TIMEOUT:-60}" "$build/openfrenzy" "$image" \
	> "$out/test.log" 2>&1
status=$?
set -e
grep -E 'FAIL|SKIP|core_modes_a tests|Game exited' "$out/test.log" || true
if [ $status -eq 0 ] && grep -q "Game exited (0)" "$out/test.log"; then
	echo "PASS core_modes_a_test"
	exit 0
fi
echo "FAIL core_modes_a_test (exit $status, log $out/test.log)"
exit 1
