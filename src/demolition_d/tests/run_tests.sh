#!/bin/sh
# Builds and runs the demolition_d module's tests (demolition_d_test.c) as one guest image.
#
#   src/demolition_d/tests/run_tests.sh [build dir]     (default: build-demolition_d)
#
# Needs a configured project build in that directory (cmake -S . -B <dir>). Builds only what
# the test needs (the demolition_d objects, the weak stubs, the guest runtime, the platform
# layer, musl), so other modules' build errors do not stop it. The test is compiled with the
# game build's command for a demolition_d file plus -DOPENFRENZY_DEMOLITION_D_TEST, and linked
# with the demolition_d objects, the generated weak stubs (every other game function), the
# guest runtime, the platform layer and musl. A link error here means demolition_d uses a
# symbol nobody defines. (Adapted from src/demolition_b/tests/run_tests.sh.)
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-demolition_d}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/demolition_d/tests/demolition_d_test.c"
out="$build/demolition_d-tests"
image="$out/demolition_d_test.elf"
mkdir -p "$out/data" "$out/saves"

cmake --build "$guest" -t game_decls >/dev/null
objects_all=$(ninja -C "$guest" -t targets all | grep -o "[^ :]*src/demolition_d/[^ :]*\.c\.obj" | sort -u)
ninja -C "$guest" $objects_all game_stubs guest_runtime ofplatform guestc >/dev/null

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'demolition_d/Demolition/CDemolition_Crate.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no game build commands in $guest (configure the project first)" >&2
	exit 2
fi

# compile: the CDemolition_Crate.c command with the test source and object
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_DEMOLITION_D_TEST=1 -o $out/demolition_d_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: the guest runtime, the stubs, the demolition_d objects (not the test's own empty
# object), the test, and the libraries of the game image's command
objects=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.obj$' |
	grep -E 'guest_runtime\.dir|game_stubs\.dir|/src/demolition_d/' | grep -v '/tests/')
libs=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.a$')
lld=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E 'ld\.lld$' | head -n 1)
script=$(printf '%s\n' "$link" | sed -e 's/.* -T \([^ ]*\) .*/\1/')
(cd "$guest" && $lld -m aarch64linux -static -nostdlib -T "$script" $objects "$out/demolition_d_test.o" \
	--whole-archive $(printf '%s\n' $libs | grep ofplatform) --no-whole-archive \
	$(printf '%s\n' $libs | grep -v ofplatform) -o "$image")

export OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" OPENFRENZY_WINDOWED=1
export SDL_VIDEO_DRIVER="${SDL_VIDEO_DRIVER-dummy}"

set +e
perl -e 'alarm shift; exec @ARGV or die' "${OF_TEST_TIMEOUT:-60}" "$build/openfrenzy" "$image" \
	> "$out/test.log" 2>&1
status=$?
set -e
grep -E 'FAIL|SKIP|demolition_d tests|Game exited' "$out/test.log" || true
if [ $status -eq 0 ] && grep -q "Game exited (0)" "$out/test.log"; then
	echo "PASS demolition_d_test"
	exit 0
fi
echo "FAIL demolition_d_test (exit $status, log $out/test.log)"
exit 1
