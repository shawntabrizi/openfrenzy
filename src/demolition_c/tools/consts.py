#!/usr/bin/env python3
"""Print the Ghidra C of functions with the DAT_ globals they use resolved: XBE section,
dword, float value (analysis helper of the demolition_c module).

Usage: python3 -I src/demolition_c/tools/consts.py ADDR [ADDR ...]
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    d, secs = xbe_data.load()

    def info(va):
        for name, sva, vs, ra, rs in secs:
            if sva <= va < sva + vs:
                off = va - sva
                if off + 4 <= rs:
                    w = struct.unpack_from("<I", d, ra + off)[0]
                    f = struct.unpack_from("<f", d, ra + off)[0]
                    return "%s 0x%08x %r" % (name, w, f)
                return "%s (bss)" % name
        return "?"
    for a in sys.argv[1:]:
        a = a.lower().replace("0x", "").zfill(8)
        text = open(os.path.join(ROOT, "export", "funcs", a + ".c")).read()
        print("=== %s" % a)
        print(text)
        seen = set()
        for m in re.finditer(r"_?DAT_([0-9a-f]{8})", text):
            v = int(m.group(1), 16)
            if v in seen:
                continue
            seen.add(v)
            print("   DAT_%08x  %s" % (v, info(v)))


if __name__ == "__main__":
    main()
