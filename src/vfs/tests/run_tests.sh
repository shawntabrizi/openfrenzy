#!/bin/sh
# vfs module tests (src/vfs). Builds src/vfs/tests/vfs_test.c as a guest image with the
# compile and link commands of openfrenzy_smoke.elf from an existing build, runs it in the
# host against the player's disc image, and checks its output against scripts/xiso_list.py
# (an independent XDVDFS reader).
#
#   src/vfs/tests/run_tests.sh [build dir]        (default: build-vfs; build it first)
#
# Needs orig/game.xiso.iso (or OPENFRENZY_ISO). Exit 0 when every check passed.
set -eu

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
BUILD=$(cd "${1:-$ROOT/build-vfs}" && pwd)
ISO=${OPENFRENZY_ISO:-$ROOT/orig/game.xiso.iso}
GUEST=$BUILD/guest-main
WORK=$BUILD/test-vfs
TEST_C=$ROOT/src/vfs/tests/vfs_test.c
SMOKE_C=$ROOT/port/macos/tests/smoke/smoke.c

[ -f "$ISO" ] || { echo "run_tests: no disc image at $ISO" >&2; exit 2; }
[ -x "$BUILD/openfrenzy" ] || { echo "run_tests: build $BUILD first" >&2; exit 2; }
rm -rf "$WORK"
mkdir -p "$WORK/data" "$WORK/saves"

# the smoke image's commands, retargeted to the test
commands=$(ninja -C "$GUEST" -t commands "$BUILD/openfrenzy_smoke.elf")
compile=$(printf '%s\n' "$commands" | grep -F -- "-c $SMOKE_C" | tail -n 1)
link=$(printf '%s\n' "$commands" | grep -F -- "-o $BUILD/openfrenzy_smoke.elf" | tail -n 1)
[ -n "$compile" ] && [ -n "$link" ] || { echo "run_tests: smoke commands not found" >&2; exit 2; }
smoke_object=$(printf '%s\n' "$compile" | sed -E 's/.* -o ([^ ]+) -c .*/\1/')
compile=$(printf '%s\n' "$compile" | sed -E 's/ -MD -MT [^ ]+ -MF [^ ]+//' |
	sed "s|-o $smoke_object -c $SMOKE_C|-o $WORK/vfs_test.o -c $TEST_C|")
link=$(printf '%s\n' "$link" | sed -e 's/^: && //' -e 's/ && :$//' |
	sed -e "s| $smoke_object | $WORK/vfs_test.o |" -e "s|-o $BUILD/openfrenzy_smoke.elf|-o $WORK/vfs_test.elf|" \
	    -e "s|-Map [^ ]*||")
(cd "$GUEST" && sh -c "$compile" && sh -c "$link")

status=0
OPENFRENZY_DATA_ROOT=$WORK/data OPENFRENZY_SAVE_ROOT=$WORK/saves OPENFRENZY_ISO=$ISO \
	SDL_VIDEO_DRIVER=dummy "$BUILD/openfrenzy" "$WORK/vfs_test.elf" >"$WORK/out.txt" 2>&1 || status=$?
cat "$WORK/out.txt"
grep -q "Game exited (0)" "$WORK/out.txt" || { echo "run_tests: FAIL vfs_test exit status $status"; exit 1; }

# cross-check against scripts/xiso_list.py
python3 -I "$ROOT/scripts/xiso_list.py" "$ISO" > "$WORK/xiso_list.txt"
python3 -I - "$WORK" "$ISO" "$ROOT" <<'EOF'
import os, subprocess, sys, zlib
work, iso, root = sys.argv[1:]
out = open(os.path.join(work, "out.txt"), encoding="latin-1").read().splitlines()
failed = 0

def check(ok, name):
    global failed
    print(("ok " if ok else "FAIL ") + name)
    failed += not ok

# d:\Packages\* from FindFirstFileA must be exactly the image's Packages/ entries
want = set()
for line in open(os.path.join(work, "xiso_list.txt"), encoding="latin-1"):
    size, path = line.split(None, 1)
    path = path.rstrip("\n")
    if not path.startswith("Packages/"):
        continue
    rest = path[len("Packages/"):]
    if size == "<dir>" and rest.count("/") == 1:
        want.add((rest[:-1], 0, "dir"))
    elif size != "<dir>" and "/" not in rest:
        want.add((rest, int(size), "file"))
got = set()
for line in out:
    if line.startswith("LIST "):
        _, name, size, kind = line.split(" ")
        got.add((name, int(size), kind))
check(got == want, "cross-check: d:\\Packages listing equals xiso_list.py (%d entries)" % len(want))
for entry in sorted(want ^ got):
    print("   differs:", entry)

# file contents through ReadFile must equal the image's bytes
for line in out:
    if not line.startswith("CRC "):
        continue
    _, path, crc, size = line.split(" ")
    disc = path[3:].replace("\\", "/")
    extracted = os.path.join(work, "extracted.bin")
    subprocess.run([sys.executable, "-I", os.path.join(root, "scripts", "xiso_list.py"), iso,
                    "--extract", disc, extracted], check=True, stdout=subprocess.DEVNULL)
    data = open(extracted, "rb").read()
    check(int(crc, 16) == zlib.crc32(data) and int(size) == len(data),
          "cross-check: %s contents equal xiso_list.py (%d bytes)" % (path, len(data)))
    os.remove(extracted)
    if disc.lower() == "default.xbe" and os.path.exists(os.path.join(root, "orig", "default.xbe")):
        check(data == open(os.path.join(root, "orig", "default.xbe"), "rb").read(),
              "cross-check: d:\\default.xbe equals orig/default.xbe")
sys.exit(1 if failed else 0)
EOF
echo "run_tests: vfs passed"
