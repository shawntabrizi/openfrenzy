#!/usr/bin/env python3
"""rowing_b analysis helper: for every function of the module, print the data words in the
XBE (outside .text) that point to it (vtable slots, tables), and the callers from
export/functions.jsonl.

Usage: python3 -I src/rowing_b/tools/refs.py
"""
import json
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    names = xbe_data.function_names()
    mine = sorted(a for a, (m, _) in names.items() if m == "rowing_b")
    mineset = set(mine)
    d, secs = xbe_data.load()
    data_refs = {}
    for name, va, vs, ra, rs in secs:
        if name == ".text":
            continue
        for off in range(0, rs - 3, 4):
            v, = struct.unpack_from("<I", d, ra + off)
            if v in mineset:
                data_refs.setdefault(v, []).append((name, va + off))
    callers = {}
    for fn in ("functions.jsonl", "functions_extra.jsonl"):
        p = os.path.join(ROOT, "export", fn)
        if not os.path.exists(p):
            continue
        for line in open(p):
            o = json.loads(line)
            a = int(o["addr"], 16) if isinstance(o["addr"], str) else o["addr"]
            if a in mineset:
                callers.setdefault(a, set()).update(o.get("callers") or [])
    for a in mine:
        cs = sorted(callers.get(a, []))
        cs2 = []
        for c in cs:
            ci = int(c, 16) if isinstance(c, str) else c
            mod = names.get(ci, ("?",))[0]
            cs2.append("%06x:%s" % (ci, mod))
        dr = ["%s:%08x" % r for r in data_refs.get(a, [])]
        print("%06x callers=[%s] data=[%s]" % (a, " ".join(cs2), " ".join(dr)))


if __name__ == "__main__":
    main()
