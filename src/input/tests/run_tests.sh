#!/bin/sh
# Builds and runs the input module's tests (xinput_test.c) as a guest image.
#
#   src/input/tests/run_tests.sh [build dir]     (default: build-input)
#
# Needs a finished project build in that directory (cmake --build <dir>). The test is compiled
# and linked with the exact commands CMake uses for openfrenzy_smoke.elf (platform flags,
# libofplatform.a, the guest runtime, musl), with smoke.c replaced by xinput_test.c, then run
# headless in the Darwin host. The top-level CMakeLists.txt is the architect's, so this test is
# not registered with ctest yet.
set -eu

root=$(cd "$(dirname "$0")/../../.." && pwd)
build=${1:-build-input}
case "$build" in /*) ;; *) build="$root/$build" ;; esac
guest="$build/guest-main"
test_src="$root/src/input/tests/xinput_test.c"
out="$build/input-tests"
image="$out/openfrenzy_xinput_test.elf"
mkdir -p "$out/data" "$out/saves"

commands=$(ninja -C "$guest" -t commands openfrenzy_smoke)
compile=$(printf '%s\n' "$commands" | grep -F 'tests/smoke/smoke.c' | grep -F ' -c ' | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F 'ld.lld' | tail -n 1)
if [ -z "$compile" ] || [ -z "$link" ]; then
	echo "run_tests.sh: no openfrenzy_smoke commands in $guest (build the project first)" >&2
	exit 2
fi
smoke_obj=$(printf '%s\n' "$compile" | sed -e 's/.* -o \([^ ]*\) .*/\1/')

# compile: the smoke file's command with our source and object (dependency file dropped)
compile=$(printf '%s\n' "$compile" | sed \
	-e 's/ -MD -MT [^ ]* -MF [^ ]*//' \
	-e "s# -o [^ ]* # -o $out/xinput_test.o #" \
	-e "s# -c [^ ]*\$# -c $test_src#")
(cd "$guest" && eval "$compile")

# link: the smoke image's command with our object and image
link=$(printf '%s\n' "$link" | sed \
	-e "s#$smoke_obj#$out/xinput_test.o#" \
	-e "s#-o [^ ]*openfrenzy_smoke.elf#-o $image#" \
	-e "s#-Map [^ ]*#-Map $out/openfrenzy_xinput_test.elf.map#")
(cd "$guest" && eval "$link")

OPENFRENZY_DATA_ROOT="$out/data" OPENFRENZY_SAVE_ROOT="$out/saves" SDL_VIDEO_DRIVER=dummy \
	"$build/openfrenzy" "$image" > "$out/log.txt" 2>&1 && status=0 || status=$?
cat "$out/log.txt"
if [ "$status" -ne 0 ] || ! grep -q 'Game exited (0)' "$out/log.txt"; then
	echo "run_tests.sh: FAILED (host exit status $status)" >&2
	exit 1
fi
echo "run_tests.sh: passed"
