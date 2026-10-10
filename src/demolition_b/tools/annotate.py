#!/usr/bin/env python3
"""Filter: replace call/jmp targets in disassembly with their of_decls.h prototype and
mark targets that are not in the function table (analysis helper of the demolition_b module)."""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
p = {}
cur = None
for line in open(os.path.join(ROOT, "build-demolition_b", "guest-main", "gen", "of_decls.h")):
    m = re.match(r"/\* 0x([0-9a-f]{8}) ", line)
    if m:
        cur = m.group(1)
        continue
    if cur and line.strip() and not line.startswith("#"):
        p[cur] = line.strip()
        cur = None
for line in sys.stdin:
    m = re.search(r"\b(call|jmp)\s+0x([0-9a-f]+)", line)
    if m:
        t = m.group(2).rjust(8, "0")
        if t in p:
            line = line[:m.start()] + m.group(1) + " " + p[t] + "\n"
        elif m.group(1) == "call":
            line = line.rstrip("\n") + "   <-- NOT IN TABLE\n"
    print(line, end="")
