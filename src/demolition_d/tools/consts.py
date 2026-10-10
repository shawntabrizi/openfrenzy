#!/usr/bin/env python3
"""List the data constants (_DAT_xxxxxxxx) that the Ghidra C of the given functions reads,
with their float and double values (analysis helper of the demolition_d module).

Usage: python3 -I src/demolition_d/tools/consts.py ADDR [ADDR ...]   (or @file of addresses)
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    args = []
    for a in sys.argv[1:]:
        args += open(a[1:]).read().split() if a.startswith("@") else [a]
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    seen = {}
    for a in args:
        a = a.lower().replace("0x", "").zfill(8)
        p = os.path.join(ROOT, "export", "funcs", a + ".c")
        if not os.path.exists(p):
            continue
        for m in re.finditer(r"_?DAT_([0-9a-f]{8})", open(p).read()):
            seen.setdefault(int(m.group(1), 16), set()).add(a)
    for va in sorted(seen):
        try:
            raw = at(va, 8)
        except SystemExit:
            continue
        f, = struct.unpack_from("<f", raw)
        dv, = struct.unpack_from("<d", raw)
        u, = struct.unpack_from("<I", raw)
        sec = [s[0] for s in secs if s[1] <= va < s[1] + s[2]]
        print("0x%08x %-6s f=%-14.9g d=%-14.9g u=0x%08x  in %s" % (va, sec[0] if sec else "?", f, dv, u, ",".join(sorted(seen[va]))[:60]))


if __name__ == "__main__":
    main()
