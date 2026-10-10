#!/usr/bin/env python3
"""Print Ghidra's C for functions (export/funcs/<addr>.c) followed by the data each one
references: section, and the float / dword value for read-only data
(analysis helper of the bumpercars_a module).

Usage: python3 -I src/bumpercars_a/tools/show.py ADDR [ADDR ...]
OF_EXTRA_FUNCS=<dir> also reads <dir>/<addr>.c (functions exported with ExportAt.java).
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
    at = xbe_data.reader(d, secs)
    for a in sys.argv[1:]:
        a = "%08x" % int(a, 16)
        path = os.path.join(ROOT, "export", "funcs", a + ".c")
        extra = os.environ.get("OF_EXTRA_FUNCS")
        if not os.path.exists(path) and extra:
            path = os.path.join(extra, a + ".c")
        text = open(path).read()
        print("=" * 20, a)
        print(text.strip())
        seen = set()
        for m in re.finditer(r"(?:_?DAT|PTR_DAT|PTR)_([0-9a-f]{8})", text):
            v = int(m.group(1), 16)
            if v in seen:
                continue
            seen.add(v)
            sec = next((s for s in secs if s[1] <= v < s[1] + s[2]), None)
            name = sec[0] if sec else "?"
            info = ""
            if sec and v - sec[1] + 4 <= sec[4]:
                raw = at(v, 4)
                w, = struct.unpack("<I", raw)
                f, = struct.unpack("<f", raw)
                info = "dword 0x%08x float %g" % (w, f)
                if name != ".rdata" and name != "D3D":
                    info += " (initial)"
            else:
                info = "bss"
            print("  // %s %s %s" % (m.group(0), name, info))


if __name__ == "__main__":
    main()
