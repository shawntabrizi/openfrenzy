#!/bin/sh
# Decodes every music stream of the disc with our WMA decoder and with FFmpeg, and compares
# the samples (src/dsound/tests/pcm_compare.c). Needs ffmpeg on PATH (brew install ffmpeg).
#
#   src/dsound/tests/compare_ffmpeg.sh [ISO] [PACKAGE]
#
# Pass: every stream matches FFmpeg to 1 LSB up to its first continuation-only superframe
# (a superframe in which no frame ends). FFmpeg decodes those wrongly (see wma_decoder.c,
# "No frame ends in this superframe"): from there on it differs, with or without errors in
# its log. A difference before that point is a failure.
set -eu
root=$(cd "$(dirname "$0")/../../.." && pwd)
iso=${1:-$root/orig/game.xiso.iso}
package=${2:-Packages/Music.xbp}
work=${TMPDIR:-/tmp}/openfrenzy-wma-compare
offset=4096
mkdir -p "$work"
cc -std=gnu11 -O2 -I"$root/src/dsound" -I"$root/src/vfs" "$root/src/dsound/tests/test_wma.c" \
	"$root/src/dsound/wma_decoder.c" "$root/src/vfs/xiso_fs.c" -lm -o "$work/test_wma"
cc -std=gnu11 -O2 "$root/src/dsound/tests/pcm_compare.c" -lm -o "$work/pcm_compare"
count=$("$work/test_wma" "$iso" --package "$package" | sed -n 's/^\([0-9]*\) streams.*/\1/p')
matched=0 after_continuation=0 failed=0
i=0
while [ "$i" -lt "$count" ]; do
	info=$("$work/test_wma" "$iso" --package "$package" --stream "$i" --dump-asf "$work/s$i.wma" \
		--dump-pcm "$work/s$i.raw" | grep "^stream")
	continuation=$(echo "$info" | sed -n 's/.*(first at frame \([0-9]*\)).*/\1/p')
	ffmpeg -hide_banner -nostdin -v quiet -i "$work/s$i.wma" -f s16le -acodec pcm_s16le -y "$work/r$i.raw" || true
	if result=$("$work/pcm_compare" "$work/s$i.raw" "$work/r$i.raw" "$offset"); then
		matched=$((matched + 1))
		echo "stream $i: match; $result"
		i=$((i + 1))
		continue
	fi
	first=$(echo "$result" | sed -n 's/.*(first at frame \([0-9]*\) = .*/\1/p')
	# pcm_compare counts frames of the reference; ours are $offset ahead
	if [ -n "$continuation" ] && [ -n "$first" ] && [ $((first + offset + 2048)) -ge "$continuation" ]; then
		after_continuation=$((after_continuation + 1))
		echo "stream $i: match up to the first continuation superframe (frame $continuation); $result"
	else
		failed=$((failed + 1))
		echo "stream $i: MISMATCH (continuation superframe at '${continuation:-none}'); $result"
	fi
	i=$((i + 1))
done
echo "$count streams: $matched match FFmpeg, $after_continuation match up to FFmpeg's continuation bug, $failed mismatch"
[ "$failed" -eq 0 ]
