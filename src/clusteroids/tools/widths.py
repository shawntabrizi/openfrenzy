#!/usr/bin/env python3
"""List the access width of every absolute global operand in the x86 code of functions
(byte/word/dword/qword ptr [..+0x2xxxxx]), to check the C types of the globals
(analysis helper of clusteroids; Ghidra's "_DAT_" names often have the wrong size).

Usage: python3 -I src/clusteroids/tools/widths.py ADDR [ADDR ...]
"""
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
DIS = os.path.join(ROOT, "src", "carryon_a", "tools", "dis.py")


def main():
    for a in sys.argv[1:]:
        out = subprocess.check_output([sys.executable, "-I", DIS, a]).decode()
        seen = {}
        for line in out.splitlines():
            for m in re.finditer(r"(byte|word|dword|qword|tbyte) ptr \[([^\]]*?)0x(2[0-9a-f]{5})\]", line):
                key = (int(m.group(3), 16), m.group(1), "idx" if m.group(2) else "")
                seen.setdefault(key, 0)
                seen[key] += 1
        print("==", a)
        for (addr, w, idx), n in sorted(seen.items()):
            if w != "dword":
                print("  0x%08x %-5s %s x%d" % (addr, w, idx, n))


if __name__ == "__main__":
    main()
