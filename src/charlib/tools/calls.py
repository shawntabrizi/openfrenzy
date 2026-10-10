#!/usr/bin/env python3
"""For each call in an original function, print the call and the instructions since the
previous call that set ECX/EDX or push arguments, to see thiscall/fastcall arguments that
Ghidra's C output drops (analysis helper of the charlib module).

Usage: python3 -I src/charlib/tools/calls.py ADDR [ADDR ...]
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    for a in sys.argv[1:]:
        out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "disasm.py"), a],
                                      text=True)
        print("== %s" % a)
        window = []
        for line in out.splitlines():
            ins = line[10:]
            if ins.startswith("call"):
                regs = [w for w in window if any(k in w for k in ("ecx,", "edx,", "push", "fstp", "fst\t"))]
                print("   " + " | ".join(regs[-8:]))
                print("  " + line)
                window = []
            else:
                window.append(ins.replace("\t", " ").replace("dword ptr ", ""))


if __name__ == "__main__":
    main()
