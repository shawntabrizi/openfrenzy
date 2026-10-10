#!/usr/bin/env python3
"""List the functions outside charlib that charlib functions call, with the table prototype
and any prototype other modules declared in their .decls files (analysis helper).

Usage: python3 -I src/charlib/tools/callees.py [ADDR ...]
"""
import glob
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    table = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        c = line.rstrip("\n").split("\t")
        table[c[0]] = c
    decls = {}
    for p in glob.glob(os.path.join(ROOT, "src", "*", "*.decls")):
        for line in open(p):
            if line.strip() and not line.startswith("#"):
                a, proto = line.split(" ", 1)
                decls[a.lower()] = proto.strip()
    funcs = {}
    for f in ("functions.jsonl", "functions_extra.jsonl"):
        p = os.path.join(ROOT, "export", f)
        if os.path.exists(p):
            for line in open(p):
                j = json.loads(line)
                funcs[j["addr"].lower().replace("0x", "").zfill(8)] = j
    mine = [a for a, c in table.items() if c[1] == "charlib"]
    want = [a.lower().replace("0x", "").zfill(8) for a in sys.argv[1:]] or mine
    seen = {}
    for a in want:
        for c in funcs.get(a, {}).get("callees", []):
            c = c.lower().replace("0x", "").zfill(8)
            if c in table and table[c][1] != "charlib":
                seen.setdefault(c, []).append(a)
    for c in sorted(seen):
        t = table[c]
        print("%s %-12s %-40s | %s" % (c, t[1], t[8][:60], decls.get(c, "-")))


if __name__ == "__main__":
    main()
