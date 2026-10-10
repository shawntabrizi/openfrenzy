#!/usr/bin/env python3
"""List the globals (DAT_/PTR_ names in Ghidra's C) that carryon_a functions use, with their
section, whether they are .bss, the XBE value and which other modules use them
(analysis helper of the carryon_a module).

Usage: python3 -I src/carryon_a/tools/globals.py [MODULE]
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

PAT = re.compile(r"\b_?(?:DAT|PTR_[A-Za-z0-9_]*?|PTR)_([0-9a-f]{8})\b")


def main():
    mod = sys.argv[1] if len(sys.argv) > 1 else "carryon_a"
    names = xbe_data.function_names()
    d, secs = xbe_data.load()
    users = {}
    mine = {}
    for addr, (m, _) in names.items():
        p = os.path.join(ROOT, "export", "funcs", "%08x.c" % addr)
        if not os.path.exists(p):
            continue
        for g in set(int(x, 16) for x in PAT.findall(open(p).read())):
            users.setdefault(g, set()).add(m)
            if m == mod:
                mine.setdefault(g, set()).add(addr)
    for g in sorted(mine):
        sec = "?"
        val = ""
        for name, va, vs, ra, rs in secs:
            if va <= g < va + vs:
                sec = name
                if g - va + 4 <= rs:
                    w, = struct.unpack_from("<I", d, ra + g - va)
                    f, = struct.unpack_from("<f", d, ra + g - va)
                    val = "0x%08x %g" % (w, f)
                else:
                    val = "bss"
        others = sorted(users[g] - {mod})
        print("%08x %-7s %-24s users=%d others=%s" % (g, sec, val, len(mine[g]), ",".join(others)))


if __name__ == "__main__":
    main()
