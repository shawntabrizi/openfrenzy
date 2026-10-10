#!/usr/bin/env python3
"""Print probable function starts (16-byte aligned code after ret/jmp/padding) in address
ranges (analysis helper of the demolition_a module).

Usage: python3 -I src/demolition_a/tools/starts.py START-END [START-END ...]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))

for r in sys.argv[1:]:
    a, b = r.split("-")
    out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py"), a, b]).decode()
    prev = "ret"
    for line in out.splitlines():
        addr, _, ins = line.partition("  ")
        op = ins.split()[0] if ins.split() else ""
        if re.match(r"(ret|jmp|nop|int3)", prev) and op not in ("nop", "int3") and addr.endswith("0"):
            print("START", addr, ins)
        prev = op
