#!/usr/bin/env python3
"""Print .decls lines for the functions defined in mousewhack C files (analysis helper).

Usage: python3 -I src/mousewhack/tools/decls_from_src.py FILE.c [...]
A function is a "/* 0x<addr> ..." comment followed (after the comment ends) by a
non-static definition line. Prints "<addr> <prototype>;" for each, in file order, and
marks with "#?" the addresses that are not mousewhack functions in of_functions.tsv.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))


def main():
    table = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        f = line.split("\t")
        if len(f) > 2 and not line.startswith("#"):
            table[f[0]] = f[1]
    for path in sys.argv[1:]:
        lines = open(path).read().split("\n")
        i = 0
        while i < len(lines):
            m = re.match(r"/\* 0x([0-9a-f]{8})\b", lines[i])
            if not m:
                i += 1
                continue
            addr = m.group(1)
            j = i
            while "*/" not in lines[j]:
                j += 1
            j += 1
            sig = []
            while j < len(lines) and lines[j].strip() and not lines[j].startswith("{"):
                sig.append(lines[j].strip())
                j += 1
            text = " ".join(sig)
            if "(" in text and not text.startswith("static") and not text.startswith("const") \
                    and not text.startswith("#") and "=" not in text.split("(")[0]:
                mark = "" if table.get(addr) == "mousewhack" else "#? "
                print("%s%s %s;" % (mark, addr, text))
            i = j


if __name__ == "__main__":
    main()
