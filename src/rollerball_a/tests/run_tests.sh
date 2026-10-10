#!/bin/sh
# Builds and runs the rollerball_a module's tests (rollerball_a_test.c) as a small guest image.
#
#   src/rollerball_a/tests/run_tests.sh [build dir]     (default: build-rollerball_a)
#
# Needs the rollerball_a objects compiled in that build directory (cmake --build <dir>, or ninja
# on those objects). rollerball_a_test.c is compiled with the command CMake uses for
# src/rollerball_a/RollerBall/unplaced_anim.c plus -DOPENFRENZY_ROLLERBALL_A_TEST, and linked with the
# openfrenzy_guest.elf command restricted to: the platform layer and runtime, the generated
# stubs and the rollerball_a objects; src/runtime/of_boot.c (the game's main) is replaced by the
# test, which also defines the other modules' globals that rollerball_a uses. So it runs even
# when other game modules do not build yet. Then it runs headless in the Darwin host.
# Not registered with ctest (the top-level CMakeLists.txt is the architect's).
# (Copied from src/carryon_a/tests/run_tests.sh.)
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-rollerball_a}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/rollerball_a/tests/rollerball_a_test.c"
out="$build/rollerball_a-tests"
image="$out/openfrenzy_rollerball_a_test.elf"
mkdir -p "$out/data" "$out/saves"

# the generated stubs and this module's objects must match the current .decls files
ninja -C "$guest" CMakeFiles/game_stubs.dir/gen/of_stubs.c.obj > /dev/null
ninja -C "$guest" $(ninja -C "$guest" -t targets all 2>/dev/null | grep -o "^[^:]*src/rollerball_a/RollerBall/[^:]*\.obj") > /dev/null
commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'src/rollerball_a/RollerBall/unplaced_anim.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no openfrenzy_guest commands in $guest (configure the project first)" >&2
	exit 2
fi
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_ROLLERBALL_A_TEST=1 -o $out/rollerball_a_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# keep everything but the game modules' objects (and the boot object), then add ours
filtered=""
for word in $(printf '%s\n' "$link"); do
	case "$word" in
	*of_boot.c.obj) word="$out/rollerball_a_test.o" ;;
	*/src/runtime/*|*/src/vfs/*|*/src/xkernel/*|*/src/d3d8/*|*/src/dsound/*|*/src/input/*) ;;
	*/src/rollerball_a/tests/*) continue ;;
	*/src/rollerball_a/*) ;;
	*/src/*.obj) continue ;;
	*openfrenzy_guest.elf.map) word="$out/openfrenzy_rollerball_a_test.elf.map" ;;
	*openfrenzy_guest.elf) word="$image" ;;
	esac
	filtered="$filtered $word"
done
(cd "$guest" && eval "$filtered --error-limit=0")

OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" SDL_VIDEO_DRIVER=dummy \
	"$build/openfrenzy" "$image" > "$out/log.txt" 2>&1 && status=0 || status=$?
cat "$out/log.txt"
if [ "$status" -ne 0 ] || ! grep -q 'Game exited (0)' "$out/log.txt"; then
	echo "run_tests.sh: FAILED (host exit status $status)" >&2
	exit 1
fi
echo "run_tests.sh: passed"
