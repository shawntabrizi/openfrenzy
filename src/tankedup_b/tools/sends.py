#!/usr/bin/env python3
"""For each call to TankEnts_SendDelayedMessage (0x001d2880) in a function, print the x86
from the previous call up to it: the delay push before `sub esp, 0x18`, the six words the
`rep movsd` copies, and the type push. Analysis helper of tankedup_b (uses tools/dis.py).

Usage: python3 -I src/tankedup_b/tools/sends.py ADDR [ADDR ...]
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    for a in sys.argv[1:]:
        out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py"), a]).decode()
        window = []
        print("==", a)
        for line in out.splitlines():
            window.append(line)
            if "call" in line and "0x1d2880" in line:
                print("\n".join("    " + w for w in window[-28:]))
                print("    ----")
            if "call" in line:
                window = window[-1:]


if __name__ == "__main__":
    main()
