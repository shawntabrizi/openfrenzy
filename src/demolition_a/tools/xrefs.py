#!/usr/bin/env python3
"""Find references to addresses in orig/default.xbe: data words equal to the address and
x86 call/jmp rel32 instructions that target it (analysis helper of the demolition_a module,
after src/bumpercars_a/tools/xrefs.py).

Usage: python3 -I src/demolition_a/tools/xrefs.py ADDR [ADDR ...]
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
        raw = d[ra:ra + rs]
        for off in range(0, len(raw) - 3):
            w, = struct.unpack_from("<I", raw, off)
            if w in want and (off % 4 == 0 or name == ".text"):
                print("%08x word %-6s -> %08x" % (va + off, name, w))
            if name == ".text" and off >= 1 and raw[off - 1] in (0xe8, 0xe9):
                t = (va + off + 4 + struct.unpack_from("<i", raw, off)[0]) & 0xffffffff
                if t in want:
                    print("%08x %s  .text  -> %08x" % (va + off - 1, "call" if raw[off - 1] == 0xe8 else "jmp ", t))


if __name__ == "__main__":
    main()
