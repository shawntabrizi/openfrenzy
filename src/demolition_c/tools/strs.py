#!/usr/bin/env python3
"""Print N pointers at ADDR and the strings they point to (demolition_c analysis helper).

Usage: python3 -I src/demolition_c/tools/strs.py ADDR N
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    d, secs = xbe_data.load()

    def off(va):
        for name, sva, vs, ra, rs in secs:
            if sva <= va < sva + vs and va - sva < rs:
                return ra + va - sva
        return None
    a = int(sys.argv[1], 16)
    for i in range(int(sys.argv[2])):
        p = struct.unpack_from("<I", d, off(a + 4 * i))[0]
        o = off(p)
        s = d[o:d.index(b"\0", o)].decode("latin-1") if o is not None else "?"
        print("0x%08x [%d] 0x%08x %r" % (a + 4 * i, i, p, s))


if __name__ == "__main__":
    main()
