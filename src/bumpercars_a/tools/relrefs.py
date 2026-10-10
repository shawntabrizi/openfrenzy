#!/usr/bin/env python3
"""List rel32 call (E8) / jmp (E9) instructions in .text whose target is one of ADDR
(analysis helper of the bumpercars_a module; byte scan, so a hit may be a false positive
inside another instruction: check it with the disassembler).

Usage: python3 -I src/bumpercars_a/tools/relrefs.py ADDR [ADDR ...]
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    want = {int(a, 16) for a in sys.argv[1:]}
    d, secs = xbe_data.load()
    for name, va, vs, ra, rs in secs:
        if name != ".text":
            continue
        raw = d[ra:ra + rs]
        for off in range(len(raw) - 5):
            op = raw[off]
            if op in (0xE8, 0xE9):
                rel, = struct.unpack_from("<i", raw, off + 1)
                t = (va + off + 5 + rel) & 0xffffffff
                if t in want:
                    print("%08x %s -> %08x" % (va + off, "call" if op == 0xE8 else "jmp", t))


if __name__ == "__main__":
    main()
