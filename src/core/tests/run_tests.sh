#!/bin/sh
# Builds and runs the core module's tests (core_test.c) as a guest image.
#
#   src/core/tests/run_tests.sh [build dir]     (default: build-core)
#
# Needs a finished project build in that directory (cmake --build <dir>). core_test.c is
# compiled with the exact command CMake uses for src/core/Core/core.c (game flags) plus
# -DOPENFRENZY_CORE_TEST, and linked with the openfrenzy_guest.elf command, with
# src/runtime/of_boot.c (the game's main) replaced by the test. Then it runs headless in the
# Darwin host. Not registered with ctest (the top-level CMakeLists.txt is the architect's).
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-core}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/core/tests/core_test.c"
out="$build/core-tests"
image="$out/openfrenzy_core_test.elf"
mkdir -p "$out/data" "$out/saves"

commands=$(ninja -C "$guest" -t commands openfrenzy_guest)
compile=$(printf '%s\n' "$commands" | grep -F 'src/core/Core/core.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no openfrenzy_guest commands in $guest (build the project first)" >&2
	exit 2
fi
core_obj=$(printf '%s\n' "$compile" | sed -e 's/.* -o \([^ ]*\) .*/\1/')
boot_obj=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -F 'of_boot.c' | head -n 1)
test_obj_in_build=$(printf '%s\n' "$link" | tr ' ' '\n' | grep -F 'core_test.c' | head -n 1 || true)

compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -DOPENFRENZY_CORE_TEST=1 -o $out/core_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

link=$(printf '%s\n' "$link" | sed \
	-e "s#$boot_obj#$out/core_test.o#" \
	-e "s#-o [^ ]*openfrenzy_guest.elf#-o $image#" \
	-e "s#-Map [^ ]*#-Map $out/openfrenzy_core_test.elf.map#")
if [ -n "$test_obj_in_build" ]; then
	link=$(printf '%s\n' "$link" | sed -e "s#$test_obj_in_build##")
fi
(cd "$guest" && eval "$link")

OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" SDL_VIDEO_DRIVER=dummy \
	"$build/openfrenzy" "$image" > "$out/log.txt" 2>&1 && status=0 || status=$?
cat "$out/log.txt"
if [ "$status" -ne 0 ] || ! grep -q 'Game exited (0)' "$out/log.txt"; then
	echo "run_tests.sh: FAILED (host exit status $status)" >&2
	exit 1
fi
echo "run_tests.sh: passed"
