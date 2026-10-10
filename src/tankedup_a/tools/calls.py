#!/usr/bin/env python3
"""List the calls of an original function with the ECX value and the pushed arguments
(analysis helper of the tankedup_a module). Ghidra drops `this` (ECX) when it does not know
that a callee is __thiscall; this shows it.

Usage: build-tankedup_a/venv/bin/python -I src/tankedup_a/tools/calls.py ADDR [ADDR...]
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    import capstone
    tab = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if not line.startswith("#"):
            f = line.rstrip("\n").split("\t")
            tab[int(f[0], 16)] = f
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for arg in sys.argv[1:]:
        addr = int(arg, 16)
        length = int(tab[addr][5]) if addr in tab else 0x200
        print("=== %08x" % addr)
        ecx = "?"
        pushes = []
        for i in md.disasm(at(addr, length), addr):
            if i.mnemonic in ("mov", "lea") and i.op_str.startswith("ecx,"):
                ecx = i.op_str[4:].strip()
            elif i.mnemonic == "push":
                pushes.append(i.op_str)
            elif i.mnemonic == "call":
                m = re.match(r"0x([0-9a-f]+)$", i.op_str)
                name = ""
                if m and int(m.group(1), 16) in tab:
                    name = tab[int(m.group(1), 16)][7] + "  [" + tab[int(m.group(1), 16)][1] + "]"
                print("%08x  call %-10s %-34s ecx=%-22s pushes(last first)=%s"
                      % (i.address, i.op_str, name, ecx, list(reversed(pushes[-6:]))))
                pushes = []
            elif i.mnemonic == "add" and i.op_str.startswith("esp"):
                pushes = []


if __name__ == "__main__":
    main()
