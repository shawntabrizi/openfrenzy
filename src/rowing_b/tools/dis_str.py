#!/usr/bin/env python3
"""rowing_b analysis helper: dis.py output with every immediate that points at a text
string in .rdata annotated with the string (and .rdata floats with their value).

Usage: python3 -I src/rowing_b/tools/dis_str.py ADDR [END]
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)

    def text(v):
        for n, va, vs, ra, rs in secs:
            if va <= v < va + vs and n in (".rdata", ".data"):
                raw = b""
                while len(raw) < 80:
                    c = at(v + len(raw), 1)
                    if c == b"\0":
                        break
                    raw += c
                if len(raw) >= 2 and all(32 <= b < 127 or b in (9, 10) for b in raw):
                    return '"%s"' % raw.decode()
                if n == ".rdata":
                    return "f=%.9g" % struct.unpack("<f", at(v, 4))[0]
        return None
    out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py")] + sys.argv[1:]).decode()
    for line in out.splitlines():
        notes = []
        for m in re.finditer(r"0x(2[0-9a-f]{5})\b", line):
            t = text(int(m.group(1), 16))
            if t:
                notes.append(t)
        print(line + ("    ; " + " ".join(notes) if notes else ""))


if __name__ == "__main__":
    main()
