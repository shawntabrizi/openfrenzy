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
native_line=$("$work/test_wma" "$iso" --stream 0 | grep "^stream")
native_frames=$(echo "$native_line" | sed -n 's/.* \([0-9]*\) frames = .*/\1/p')
native_rms=$(echo "$native_line" | sed -n 's/.*rms \([0-9.]*\),.*/\1/p')

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
OPENFRENZY_DATA_ROOT=$work/data OPENFRENZY_SAVE_ROOT=$work/saves OPENFRENZY_ISO=$iso \
	OPENFRENZY_AUDIO_ENABLED=false SDL_VIDEO_DRIVER=dummy \
	perl -e 'alarm shift; exec @ARGV' 120 "$build/openfrenzy" "$image" > "$log" 2>&1 || true
grep -E "^check|^  |dsound guest test|Game exited" "$log" || cat "$log"
guest_frames=$(sed -n 's/.*music: \([0-9]*\) frames.*/\1/p' "$log")
guest_rms=$(sed -n 's/.*music: .* rms \([0-9.]*\),.*/\1/p' "$log")
grep -q "Game exited (0)" "$log" || { echo "FAIL: the guest test did not exit 0"; exit 1; }
# The guest's musl and the host's libm differ in the last bit of cos/pow/lrintf, so a few
# samples may differ by 1 LSB: the frame count must be equal, the RMS within 0.1%.
if [ -z "$native_frames" ] || [ "$native_frames" != "$guest_frames" ] || ! awk -v a="$native_rms" -v b="$guest_rms" \
	'BEGIN { d = a - b; if (d < 0) d = -d; exit !(a > 0 && d <= a * 0.001) }'; then
	echo "FAIL: guest music ($guest_frames frames, rms $guest_rms) differs from native ($native_frames, $native_rms)"
	exit 1
fi
echo "music: guest and native decoders agree ($guest_frames frames, rms $guest_rms vs $native_rms)"
echo "dsound tests passed"
