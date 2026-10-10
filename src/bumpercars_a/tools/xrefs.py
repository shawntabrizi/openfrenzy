#!/usr/bin/env python3
"""List data words in orig/default.xbe that hold the address of a bumpercars_a function
(vtable slots, callback tables) and code immediates (push/mov imm32) that do
(analysis helper of the bumpercars_a module).

Usage: python3 -I src/bumpercars_a/tools/xrefs.py [ADDR ...]
Without ADDR: every function of the module (src/include/of_functions.tsv).
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    names = xbe_data.function_names()
    if len(sys.argv) > 1:
        want = {int(a, 16) for a in sys.argv[1:]}
    else:
        want = {a for a, (m, _) in names.items() if m == "bumpercars_a"}
    d, secs = xbe_data.load()
    for name, va, vs, ra, rs in secs:
        raw = d[ra:ra + rs]
        for off in range(0, len(raw) - 3):
            w, = struct.unpack_from("<I", raw, off)
            if w in want:
                kind = "code" if name in (".text",) else "data"
                if kind == "data" and off % 4:
                    continue
                print("%08x %s %-6s -> FUN_%08x" % (va + off, kind, name, w))


if __name__ == "__main__":
    main()
