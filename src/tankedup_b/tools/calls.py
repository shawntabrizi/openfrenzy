#!/usr/bin/env python3
"""For each call in a function, print the pushes and the ECX/EDX writes since the
previous call (the call's stack arguments, last push = first argument, and its
register arguments). Analysis helper of tankedup_b (uses tools/dis.py).

Usage: python3 -I src/tankedup_b/tools/calls.py ADDR [ADDR ...]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    for a in sys.argv[1:]:
        out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py"), a]).decode()
        window = []
        print("==", a)
        for line in out.splitlines():
            addr, _, ins = line.partition("  ")
            ins = ins.replace("\t", " ")
            if ins.startswith("call"):
                pushes = [w for w in window if w.startswith("push")]
                regs = [w for w in window if re.match(r"(mov|lea|xor|pop) (ecx|edx)\b", w)]
                print("  %s %s\n      args: %s\n      regs: %s" % (
                    addr, ins, " | ".join(reversed(pushes)), " | ".join(regs[-2:])))
                window = []
            else:
                window.append(ins)


if __name__ == "__main__":
    main()
