#!/usr/bin/env python3
"""Find the vtables (runs of code pointers in data sections) that hold functions of a module
(analysis helper of the demolition_c module; copied from src/demolition_b/tools/vtables.py).

Usage: python3 -I src/demolition_b/tools/vtables.py MODULE
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    module = sys.argv[1]
    table = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        f = line.split("\t")
        table[int(f[0], 16)] = f[1]
    d, secs = xbe_data.load()
    text = [s for s in secs if s[0] == ".text"][0]
    tlo, thi = text[1], text[1] + text[2]
    for name, va, vsize, raw, rsize in secs:
        if name in (".text", "D3D", "DSOUND", "XGRPH", "XPP", "WMADEC", "WMADECXM", "DOLBY"):
            continue
        blob = d[raw:raw + rsize]
        i = 0
        while i + 4 <= len(blob):
            run = []
            j = i
            while j + 4 <= len(blob):
                v = struct.unpack_from("<I", blob, j)[0]
                if tlo <= v < thi:
                    run.append(v)
                    j += 4
                else:
                    break
            if len(run) >= 2 and any(table.get(v) == module for v in run):
                print("vtable 0x%08x (%d):" % (va + i, len(run)))
                for k, v in enumerate(run):
                    print("  +0x%02x %08x %s" % (k * 4, v, table.get(v, "?")))
            i = j + 4 if j > i else i + 4


main()
