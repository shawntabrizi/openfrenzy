#!/usr/bin/env python3
"""Annotate Ghidra C of the scratchmatch functions for hand decompilation (analysis helper of
the scratchmatch module): calls get the names other modules gave them (from the generated
of_decls.h), and .rdata constants get their float/double value.

Usage: python3 -I src/scratchmatch/tools/annotate.py BUILD_DIR ADDR... > out.c
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    build = sys.argv[1]
    decls = open(os.path.join(build, "guest-main", "gen", "of_decls.h")).read()
    names = dict(re.findall(r"#define FUN_([0-9a-f]{8}) (\w+)", decls))
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)

    def rdata(m):
        a = int(m.group(2), 16)
        if not (0x253360 <= a < 0x281734):
            return m.group(0)
        f, = struct.unpack("<f", at(a, 4))
        dd, = struct.unpack("<d", at(a, 8))
        return "%s/*f=%.9g d=%.9g*/" % (m.group(0), f, dd)

    def fun(m):
        n = names.get(m.group(1))
        return "%s/*%s*/" % (n, m.group(1)) if n else m.group(0)

    for addr in sys.argv[2:]:
        text = open(os.path.join(ROOT, "export", "funcs", "%08x.c" % int(addr, 16))).read()
        text = re.sub(r"FUN_([0-9a-f]{8})", fun, text)
        text = re.sub(r"(_?DAT_|FLOAT_|DOUBLE_)([0-9a-f]{8})", rdata, text)
        print("//==== %s" % addr)
        print(text)


if __name__ == "__main__":
    main()
