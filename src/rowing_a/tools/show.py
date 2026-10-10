#!/usr/bin/env python3
"""Show one Rowing function for decompilation work (analysis helper of the rowing_a module).

Usage: python3 -I src/rowing_a/tools/show.py ADDR [--build DIR]

Copied from src/drillers/tools/show.py. Prints the Ghidra C from export/funcs/<addr>.c, then:
  - every global it names: where it lives (rowing .rdata/.data/.bss block, .rdata, other) and,
    for .rdata, its value as float and as hex;
  - every function it calls, with the prototype that callers compile against
    (build-rowing_a/guest-main/gen/of_decls.h).
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

BLOCKS = [("rdata", 0x00274E74, 0x00276764), ("data", 0x002B5474, 0x002B5B60), ("bss", 0x0030EF08, 0x0030FE90)]


def main():
    addr = sys.argv[1].lower().replace("0x", "").rjust(8, "0")
    build = "build-rowing_a"
    if "--build" in sys.argv:
        build = sys.argv[sys.argv.index("--build") + 1]
    src = open(os.path.join(ROOT, "export", "funcs", addr + ".c")).read()
    print(src)
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)

    def section(v):
        for n, va, vs, ra, rs in secs:
            if va <= v < va + vs:
                return n
        return None

    print("/* ---- globals")
    seen = set()
    for m in re.finditer(r"\b(_?(?:DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?|u_[A-Za-z0-9_]*?))_(00[0-9a-f]{6})\b", src):
        v = int(m.group(2), 16)
        if v in seen:
            continue
        seen.add(v)
        where = None
        for n, lo, hi in BLOCKS:
            if lo <= v < hi:
                where = "rowing %s +0x%x" % (n, v - lo)
        if where is None:
            s = section(v)
            where = s or "?"
            if s in (".rdata", ".data") and not where.startswith("rowing"):
                raw = at(v, 8)
                f = struct.unpack("<f", raw[:4])[0]
                dd = struct.unpack("<d", raw)[0]
                where += "  f=%r  x=0x%08x  d=%r" % (f, struct.unpack("<I", raw[:4])[0], dd)
        print("   0x%08x %-28s %s" % (v, m.group(0), where))
    print("*/")
    decls = open(os.path.join(ROOT, build, "guest-main", "gen", "of_decls.h")).read()
    print("/* ---- callees")
    for c in sorted(set(re.findall(r"\b(?:FUN|func)_(?:0x)?(00[0-9a-f]{6})\b", src))):
        if c == addr:
            continue
        m = re.search(r"/\* 0x%s ([^\n]*) \*/\n([^\n]*)\n(#define[^\n]*\n)?" % c, decls)
        if m:
            print("   %s [%s] %s" % (c, m.group(1), m.group(2)))
        else:
            print("   %s (not in the table)" % c)
    print("*/")


if __name__ == "__main__":
    main()
