#!/usr/bin/env python3
"""List the code addresses whose bytes hold a 32-bit value in [LO, HI) (for example vtable
stores), with the enclosing function from src/include/of_functions.tsv
(analysis helper of the demolition_b module).

Usage: python3 -I src/demolition_b/tools/immrefs.py LO HI
"""
import bisect
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
    funcs = []
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        f = line.split("\t")
        funcs.append((int(f[0], 16), f[1]))
    funcs.sort()
    starts = [a for a, _ in funcs]
    d, secs = xbe_data.load()
    name, va, vsize, raw, rsize = [s for s in secs if s[0] == ".text"][0]
    blob = d[raw:raw + rsize]
    for i in range(len(blob) - 3):
        v = struct.unpack_from("<I", blob, i)[0]
        if lo <= v < hi:
            k = bisect.bisect_right(starts, va + i) - 1
            fa, mod = funcs[k] if k >= 0 else (0, "?")
            print("%08x -> %08x  in %08x %s" % (va + i, v, fa, mod))


main()
