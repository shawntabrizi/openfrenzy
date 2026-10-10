#!/usr/bin/env python3
"""Print the lines of a flow listing (tools/flow.py output) with addresses in [LO, HI).
Usage: python3 -I src/drillers/tools/rng.py FILE LO HI [LO HI ...]   (hex)"""
import sys

rs = [(int(sys.argv[i], 16), int(sys.argv[i + 1], 16)) for i in range(2, len(sys.argv) - 1, 2)]
for line in open(sys.argv[1]):
    if len(line) > 8 and all(c in "0123456789abcdef" for c in line[:8]):
        a = int(line[:8], 16)
        if any(lo <= a < hi for lo, hi in rs):
            print(line.rstrip("\n")[:170])
