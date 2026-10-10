#!/usr/bin/env python3
"""For each call in a function, print the last instruction before it that wrote ECX
(the x86 `this` of __thiscall/__fastcall callees). Analysis helper (copied from src/carryon_b/tools/ecx.py).

Usage: python3 -I src/tankedup_b/tools/ecx.py ADDR [ADDR ...]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    for a in sys.argv[1:]:
        out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py"), a]).decode()
        last = "?"
        print("==", a)
        for line in out.splitlines():
            addr, _, ins = line.partition("  ")
            if re.match(r"(mov|lea|pop|xor)\s+ecx,", ins) or re.match(r"(mov|lea|pop|xor)\s+ecx\b", ins):
                last = "%s %s" % (addr, ins)
            if ins.startswith("call") or (ins.startswith("jmp") and "FUN_" in ins):
                print("  %s %-50s ecx<- %s" % (addr, ins, last))
            if ins.startswith("call"):
                last = "(clobbered by call)"
            if re.match(r"\w+\s+[^,]*\becx\b,", ins) and not re.match(r"(mov|lea|pop|xor|cmp|test)\s", ins):
                last = "(modified: %s)" % ins


if __name__ == "__main__":
    main()
