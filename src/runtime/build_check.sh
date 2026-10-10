#!/bin/sh
# Regenerates src/runtime/entry/xdk_entry_points.inc from the symbols that the platform layer
# (src/{runtime,vfs,xkernel,d3d8,dsound,input}) really defines in a CMake build.
#
# The first version of this script compiled the layer itself without the rebase pass and
# downloaded musl and the GLES headers. The CMake build (port/guest) now compiles and links
# the same files through the rebase pass, so this script reads its libofplatform.a instead.
# No network is needed.
#
# Usage: src/runtime/build_check.sh [build-dir]    (default build-runtime)
#   1. builds the platform layer      (cmake --build <build-dir>; configure it first)
#   2. lists the defined symbols      (llvm-nm of guest-main/libofplatform.a)
#   3. regenerates the table          (scripts/gen_xdk_entry_points.py, needs export/functions.jsonl)
#   4. rebuilds, so the new table is compiled and linked (openfrenzy_platform_check.elf)
# Exit status: 0 when every step passed.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
B=${1:-build-runtime}
case $B in /*) ;; *) B=$ROOT/$B ;; esac
LLVM=${OPENFRENZY_LLVM_BIN:-/opt/homebrew/opt/llvm@22/bin}
INC=$ROOT/src/runtime/entry/xdk_entry_points.inc
LIB=$B/guest-main/libofplatform.a

if [ ! -f "$B/build.ninja" ] && [ ! -f "$B/Makefile" ]; then
	echo "build_check: $B is not configured; run: cmake -S $ROOT -B $B -G Ninja" >&2
	exit 1
fi
cmake --build "$B" --target guest_main
if [ ! -f "$LIB" ]; then
	echo "build_check: $LIB is missing after the build" >&2
	exit 1
fi

# one defined external symbol per line (weak ones too, as the HLE layer's inline fallbacks are weak)
"$LLVM/llvm-nm" -j --defined-only --extern-only "$LIB" | grep -v -e ':$' -e '^$' | sort -u > "$B/hle_symbols.txt"
echo "build_check: $(wc -l < "$B/hle_symbols.txt" | tr -d ' ') symbols defined by the platform layer"

if [ ! -f "$ROOT/export/functions.jsonl" ]; then
	echo "build_check: export/functions.jsonl is missing; the committed table is kept" >&2
	exit 1
fi
# Merge export/functions_extra.jsonl (functions reached only through tables, found by the second
# Ghidra pass). Their calls into the libraries are missing from the library functions' "callers"
# lists, because Ghidra had no function at the call site then: add them from their "callees".
python3 -I - "$ROOT/export" "$B/functions_all.jsonl" <<'EOF'
import json, os, sys
export, output = sys.argv[1:3]
functions = {}
for name in ("functions.jsonl", "functions_extra.jsonl"):
    path = os.path.join(export, name)
    if os.path.exists(path):
        with open(path, encoding="utf-8") as handle:
            for line in handle:
                entry = json.loads(line)
                functions.setdefault(entry["addr"], entry)
for address, entry in functions.items():
    for callee in entry.get("callees", []):
        target = functions.get(callee)
        if target is not None and address not in target["callers"]:
            target["callers"].append(address)
with open(output, "w", encoding="utf-8") as out:
    for address in sorted(functions):
        out.write(json.dumps(functions[address]) + "\n")
EOF
python3 -I "$ROOT/scripts/gen_xdk_entry_points.py" "$B/functions_all.jsonl" "$B/hle_symbols.txt" "$INC"
sed -n 2p "$INC"
cmake --build "$B" --target guest_main
echo "build_check: ok"
