#!/bin/sh
# dsound module tests (docs/BUILD.md: build-dsound). Runs:
#   1. tests/test_wma.c natively (Apple clang): every music stream on the disc decodes;
#   2. tests/test_dsound_guest.c as a guest image in the host: DirectSound buffers and the
#      WMA media object as the game drives them; its music must match the native decode (frames, RMS).
# The guest image is compiled and linked with the exact commands of the smoke image
# (ninja -t commands), so it follows the build's flags. ctest does not run these yet: the
# top-level CMakeLists.txt (architect) would have to register them.
#
#   src/dsound/tests/run_tests.sh [BUILD_DIR] [ISO]
set -eu
root=$(cd "$(dirname "$0")/../../.." && pwd)
build=$(cd "${1:-$root/build-dsound}" && pwd)
iso=${2:-$root/orig/game.xiso.iso}
work=$build/dsound-tests
mkdir -p "$work/data" "$work/saves"
tests=$root/src/dsound/tests

echo "== native WMA decoder"
cc -std=gnu11 -O2 -Wall -Wextra -Wno-unused-parameter -I"$root/src/dsound" -I"$root/src/vfs" \
	"$tests/test_wma.c" "$root/src/dsound/wma_decoder.c" "$root/src/vfs/xiso_fs.c" -lm -o "$work/test_wma"
"$work/test_wma" "$iso" | tail -1
"$work/test_wma" "$iso" --package Packages/ScratchMusic.xbp | tail -1
"$work/test_wma" "$iso" --stream 0 --dump-pcm "$work/native_stream0.raw" >/dev/null
cc -std=gnu11 -O2 "$tests/pcm_compare.c" -lm -o "$work/pcm_compare"

echo "== guest DirectSound test"
cmake --build "$build/guest-main" --target ofplatform openfrenzy_smoke >/dev/null
cd "$build/guest-main"
compile=$(ninja -t commands openfrenzy_smoke | grep -- "-c .*tests/smoke/smoke.c$")
link=$(ninja -t commands openfrenzy_smoke | grep "ld.lld")
smoke_object=$(echo "$compile" | sed -n 's/.* -o \([^ ]*smoke.c.obj\) .*/\1/p')
object=$work/test_dsound_guest.o
image=$work/test_dsound_guest.elf
# the compile command with our source and object; the link with our object and image
eval "$(echo "$compile" | sed -e "s| -MD -MT [^ ]* -MF [^ ]*||" -e "s|-o $smoke_object|-o $object|" \
	-e "s|-c [^ ]*tests/smoke/smoke.c$|-c $tests/test_dsound_guest.c|")"
eval "$(echo "$link" | sed -e "s|: && ||" -e "s| && :||" -e "s|$smoke_object|$object|" \
	-e "s|-Map [^ ]*|-Map $image.map|" -e "s|-o [^ ]*openfrenzy_smoke.elf|-o $image|")"
cd "$root"
log=$work/test_dsound_guest.log
OPENFRENZY_DATA_ROOT=$work/data OPENFRENZY_SAVE_ROOT=$work/saves OPENFRENZY_ISO=$iso OPENFRENZY_DSOUND_TEST_PCM=$work/guest_stream0.raw \
	OPENFRENZY_AUDIO_ENABLED=false SDL_VIDEO_DRIVER=dummy \
	perl -e 'alarm shift; exec @ARGV' 120 "$build/openfrenzy" "$image" > "$log" 2>&1 || true
grep -E "^check|^  |dsound guest test|Game exited" "$log" || cat "$log"
grep -q "Game exited (0)" "$log" || { echo "FAIL: the guest test did not exit 0"; exit 1; }
# the guest's decode against the native one, sample by sample: equal length, no sample
# off by more than 1 (musl's cos and lrintf are not Apple's in the last bit)
"$work/pcm_compare" "$work/guest_stream0.raw" "$work/native_stream0.raw" 0 || {
	echo "FAIL: the guest's music differs from the native decoder's"; exit 1; }
echo "dsound tests passed"
