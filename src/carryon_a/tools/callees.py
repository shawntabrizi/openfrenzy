#!/usr/bin/env python3
"""Print the current prototype (generated of_decls.h) of every function that the given
functions call, as named in Ghidra's C. Analysis helper of carryon_a.

Usage: python3 -I src/carryon_a/tools/callees.py BUILD_DIR ADDR [ADDR ...]
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    decls = open(os.path.join(sys.argv[1], "guest-main", "gen", "of_decls.h")).read().splitlines()
    proto = {}
    for i, line in enumerate(decls):
        m = re.match(r"/\* 0x([0-9a-f]{8}) (\S+)", line)
        if m:
            j = i + 1
            body = []
            while j < len(decls) and not decls[j].startswith("/* 0x"):
                body.append(decls[j])
                j += 1
            proto[m.group(1)] = (m.group(2), " ".join(body))
    for a in sys.argv[2:]:
        a = "%08x" % int(a, 16)
        text = open(os.path.join(ROOT, "export", "funcs", a + ".c")).read()
        seen = []
        for c in re.findall(r"FUN_([0-9a-f]{8})", text):
            if c != a and c not in seen:
                seen.append(c)
        print("==", a)
        for c in seen:
            mod, p = proto.get(c, ("?", "?"))
            print("  %s %-12s %s" % (c, mod, p))


if __name__ == "__main__":
    main()
