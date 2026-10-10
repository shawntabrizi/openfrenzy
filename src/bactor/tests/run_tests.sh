#!/bin/sh
# Builds and runs the bactor module's tests (bactor_test.inc) as a guest image.
# (The source is not named .c: the game image compiles every .c under src/bactor.)
#
#   src/bactor/tests/run_tests.sh [build dir]     (default: build-bactor)
#
# Needs a finished project build in that directory (cmake --build <dir>). The test is compiled
# with the exact command CMake uses for a bactor source (game flags, of_game.h) and linked with
# the openfrenzy_guest.elf command, keeping only the BACTOR objects, the weak stubs of every
# other game function, the guest runtime and the libraries (no other game code, no of_boot.c:
# the test has its own main). It runs headless in the Darwin host. The top-level CMakeLists.txt
# is the architect's, so this test is not registered with ctest yet.
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-bactor}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
out="$build/bactor-tests"
image="$out/openfrenzy_bactor_test.elf"
mkdir -p "$out/data" "$out/saves"
test_src="$out/bactor_test.c"
cp "$root/src/bactor/tests/bactor_test.inc" "$test_src"

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'src/bactor/BACTOR/actor.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no openfrenzy_guest commands in $guest (build the project first)" >&2
	exit 2
fi

# compile: actor.c's command (the guest_cc.py pipeline) with our source and object
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]*actor\.c\.obj# -o $out/bactor_test.obj#" \
	-e "s# -c [^ ]*actor\.c# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: drop the game objects of other modules, of_boot.c and the static constructor list
objects=""
for word in $link; do
	case "$word" in
	*.obj)
		case "$word" in
		*/src/bactor/*|*game_stubs.dir*|*guest_runtime.dir*) objects="$objects $word" ;;
		esac
		;;
	esac
done
libs=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E '\.a$' | tr '\n' ' ')
lld=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -E 'ld\.lld$' | head -n 1)
script=$(printf '%s\n' "$link" | sed -e 's/.* -T \([^ ]*\) .*/\1/')
(cd "$guest" && "$lld" -m aarch64linux -static -nostdlib -T "$script" -Map "$out/openfrenzy_bactor_test.elf.map" \
	$objects "$out/bactor_test.obj" $libs -o "$image")

OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" SDL_VIDEO_DRIVER=dummy \
	"$build/openfrenzy" "$image" > "$out/log.txt" 2>&1 && status=0 || status=$?
cat "$out/log.txt"
if [ "$status" -ne 0 ] || ! grep -q 'Game exited (0)' "$out/log.txt"; then
	echo "run_tests.sh: FAILED (host exit status $status)" >&2
	exit 1
fi
echo "run_tests.sh: passed"
