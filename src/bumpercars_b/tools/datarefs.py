#!/usr/bin/env python3
"""List the global data (DAT_<addr>) that the functions of a module reference in Ghidra's C,
with the section, the referencing functions, and the functions of other modules that
reference the same address (analysis helper of the bumpercars_b module).

Usage: python3 -I src/bumpercars_b/tools/datarefs.py MODULE [LO HI]
"""
import collections
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    module = sys.argv[1]
    lo = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
    hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 0xffffffff
    table = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        f = line.rstrip("\n").split("\t")
        table[f[0]] = f[1]
    d, secs = xbe_data.load()
    refs = collections.defaultdict(set)
    others = collections.defaultdict(set)
    rx = re.compile(r"(?:_?DAT|PTR_DAT|PTR)_([0-9a-f]{8})")
    for name in os.listdir(os.path.join(ROOT, "export", "funcs")):
        a = name[:-2]
        text = open(os.path.join(ROOT, "export", "funcs", name)).read()
        for m in set(rx.findall(text)):
            v = int(m, 16)
            if not lo <= v < hi:
                continue
            if table.get(a) == module:
                refs[v].add(a)
            else:
                others[v].add("%s:%s" % (table.get(a, "?"), a))
    for v in sorted(refs):
        sec = next((s[0] for s in secs if s[1] <= v < s[1] + s[2]), "?")
        print("%08x %-6s %2d  %s  | %s" % (v, sec, len(refs[v]), " ".join(sorted(refs[v]))[:60],
                                         " ".join(sorted(others[v]))[:120]))


if __name__ == "__main__":
    main()
