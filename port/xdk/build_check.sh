#!/bin/sh
# Compiles the XDK replacement layer (port/xdk/src) with the guest ABI
# (ILP32 AArch64, arm64_32, musl headers), as OpenCE's tools/android_build.py
# does for its platform layer. Objects only: no rebase pass, no link.
# Usage: port/xdk/build_check.sh [build-dir]   (default build-fix-platform-layer)
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
B=${1:-build-fix-platform-layer}
case $B in /*) ;; *) B=$ROOT/$B ;; esac
LLVM=${OPENFRENZY_LLVM_BIN:-/opt/homebrew/opt/llvm/bin}
SDL=${OPENFRENZY_SDL_INCLUDE:-/opt/homebrew/opt/sdl3/include}
MUSL_VERSION=1.2.5
TP=$B/third_party
M=$TP/musl-$MUSL_VERSION
ARCH=$ROOT/port/guest/libc/arch/arm64_32
X=$ROOT/port/xdk
mkdir -p "$TP" "$B/libc_include/bits" "$B/obj"

# musl headers (public sources; fetched once)
if [ ! -d "$M" ]; then
	curl -sSfL "https://musl.libc.org/releases/musl-$MUSL_VERSION.tar.gz" | tar xz -C "$TP"
fi
sed -f "$M/tools/mkalltypes.sed" "$ARCH/bits/alltypes.h.in" "$M/include/alltypes.h.in" > "$B/libc_include/bits/alltypes.h"
cp "$ARCH/bits/syscall.h.in" "$B/libc_include/bits/syscall.h"
sed -n -e s/__NR_/SYS_/p < "$ARCH/bits/syscall.h.in" >> "$B/libc_include/bits/syscall.h"

# Khronos OpenGL ES headers (C declarations only)
G=$TP/gles
if [ ! -f "$G/KHR/khrplatform.h" ]; then
	mkdir -p "$G/GLES2" "$G/GLES3" "$G/KHR"
	K=https://raw.githubusercontent.com/KhronosGroup
	for f in GLES3/gl32.h GLES3/gl3platform.h GLES2/gl2ext.h GLES2/gl2platform.h GLES2/gl2.h; do
		curl -sSfL -o "$G/$f" "$K/OpenGL-Registry/main/api/$f"
	done
	curl -sSfL -o "$G/KHR/khrplatform.h" "$K/EGL-Registry/main/api/KHR/khrplatform.h"
fi

# MSVC linkage semantics for the XDK headers' inline functions
python3 "$ROOT/scripts/linux_msvc_semantics.py" --output "$B/platform_msvc_semantics.h" --inlines "$X/include/xdk"

ABI="--target=arm64_32-apple-watchos -U__APPLE__ -U__MACH__ -fno-define-target-os-macros
 -D__linux__=1 -D__unix__=1 -DHALO_ARM64_GUEST=1 -DHALO_GLES=1 -DOPENFRENZY_GUEST=1
 -mcpu=cortex-a53 -nostdinc -fshort-wchar -fno-stack-protector -fno-unwind-tables
 -fno-asynchronous-unwind-tables -femulated-tls -ffp-contract=off -O2"
CODE="-fms-extensions -fcommon -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks
 -fno-omit-frame-pointer"
for n in wcslen wcsnlen wcschr wcsrchr wcscmp wcsncmp wcscpy wcsncpy wcscat wcsncat wmemchr wmemcmp wmemcpy wmemmove wmemset; do
	CODE="$CODE -fno-builtin-$n"
done
CLANG_INC=$("$LLVM/clang" -print-resource-dir)/include
FLAGS="$ABI $CODE -std=gnu11 -D_GNU_SOURCE -DHALO_LINUX_PLATFORM_LAYER -w
 -include $X/include/linux/halo_linux_prefix.h -include $B/platform_msvc_semantics.h
 -I$X/src -I$X/compat -I$X/include/linux -I$X/include -I$X/include/xdk -I$SDL -I$G
 -isystem $B/libc_include -isystem $ARCH -isystem $M/arch/generic -isystem $M/include -isystem $CLANG_INC
 ${OPENFRENZY_EXTRA_CFLAGS:-}"

fail=0
compile() {
	obj=$B/obj/$(basename "$1" .c).o
	if "$LLVM/clang" $FLAGS $2 -c "$1" -o "$obj" 2> "$obj.log"; then
		echo "ok   $(basename "$1")"
	else
		echo "FAIL $(basename "$1") (see $obj.log)"; fail=1
	fi
}
rm -f "$B"/obj/*.o
for src in "$X"/src/*.c "$X"/compat/*.c; do
	compile "$src" ""
done

# the entry-point table: regenerated from the symbols the HLE objects define
# when the (git-ignored) Ghidra export is present, else the committed .inc
for obj in "$B"/obj/*.o; do
	"$LLVM/llvm-nm" -j --defined-only --extern-only "$obj" | sed "s|^_||;s|\$| $(basename "$obj" .o)|"
done > "$B/hle_symbols.txt"
if [ -f "$ROOT/export/functions.jsonl" ]; then
	python3 "$ROOT/scripts/gen_xdk_entry_points.py" "$ROOT/export/functions.jsonl" "$B/hle_symbols.txt" \
		"$X/entry/xdk_entry_points.inc"
fi
compile "$X/entry/xdk_entry_points.c" "-I$X/entry"
exit $fail
