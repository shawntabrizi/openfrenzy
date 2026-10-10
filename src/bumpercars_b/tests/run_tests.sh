#!/bin/sh
# Builds and runs the bumpercars_b module's tests (bumpercars_b_test.c) as one guest image.
#
#   src/bumpercars_b/tests/run_tests.sh [build dir]     (default: build-bumpercars_b)
#
# Needs a configured project build in that directory (cmake -S . -B <dir>). Builds only what
# the test needs (the bumpercars_b objects, the weak stubs, the guest runtime, the platform
# layer, musl), so other modules' build errors do not stop it. The test is compiled with the
# game build's command for a bumpercars_b file plus -DOPENFRENZY_BUMPERCARS_B_TEST, and linked
# with the bumpercars_b objects, the generated weak stubs (every other game function), the
# guest runtime, the platform layer and musl. A link error here means bumpercars_b uses a
# symbol nobody defines. (Adapted from src/demolition_b/tests/run_tests.sh.)
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-bumpercars_b}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/bumpercars_b/tests/bumpercars_b_test.c"
out="$build/bumpercars_b-tests"
image="$out/bumpercars_b_test.elf"
mkdir -p "$out/data" "$out/saves"

cmake --build "$guest" -t game_decls >/dev/null
objects_all=$(ninja -C "$guest" -t targets all | grep -o "[^ :]*src/bumpercars_b/[^ :]*\.c\.obj" | sort -u)
ninja -C "$guest" $objects_all game_stubs guest_runtime ofplatform guestc >/dev/null

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'bumpercars_b/BumperCars/bcb_misc.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no game build commands in $guest (configure the project first)" >&2
	exit 2
fi

# compile: the bcb_misc.c command with the test source and object
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_BUMPERCARS_B_TEST=1 -o $out/bumpercars_b_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: the guest runtime, the stubs, the bumpercars_b objects (not the test's own empty
# object), the test, and the libraries of the game image's command
objects=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.obj$' |
	grep -E 'guest_runtime\.dir|game_stubs\.dir|/src/bumpercars_b/' | grep -v '/tests/')
libs=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.a$')
lld=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E 'ld\.lld$' | head -n 1)
script=$(printf '%s\n' "$link" | sed -e 's/.* -T \([^ ]*\) .*/\1/')
(cd "$guest" && $lld -m aarch64linux -static -nostdlib -T "$script" $objects "$out/bumpercars_b_test.o" \
	--whole-archive $(printf '%s\n' $libs | grep ofplatform) --no-whole-archive \
	$(printf '%s\n' $libs | grep -v ofplatform) -o "$image")

export OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" OPENFRENZY_WINDOWED=1
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER-dummy}"

set +e
perl -e 'alarm shift; exec @ARGV or die' "${OF_TEST_TIMEOUT:-60}" "$build/openfrenzy" "$image" \
	> "$out/test.log" 2>&1
status=$?
set -e
grep -E 'FAIL|SKIP|bumpercars_b tests|Game exited' "$out/test.log" || true
if [ $status -eq 0 ] && grep -q "Game exited (0)" "$out/test.log"; then
	echo "PASS bumpercars_b_test"
	exit 0
fi
echo "FAIL bumpercars_b_test (exit $status, log $out/test.log)"
exit 1
