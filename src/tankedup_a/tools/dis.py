#!/usr/bin/env python3
"""Disassemble original x86 code from orig/default.xbe with capstone (Intel syntax)
(analysis helper of the tankedup_a module; adapted from src/bumpercars_b/tools/dis.py).

Usage: build-tankedup_a/venv/bin/python -I src/tankedup_a/tools/dis.py ADDR [LENGTH]
  LENGTH defaults to the size in src/include/of_functions.tsv.
Call targets get their table name; float constants in .rdata get their value.
"""
import os
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def table():
    t = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        f = line.rstrip("\n").split("\t")
        t[int(f[0], 16)] = f
    return t


def main():
    import capstone
    import re
    tab = table()
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    for arg in sys.argv[1:2]:
        addr = int(arg, 16)
        length = int(sys.argv[2], 0) if len(sys.argv) > 2 else int(tab[addr][5]) if addr in tab else 0x100
        raw = at(addr, length)
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        for i in md.disasm(raw, addr):
            note = ""
            for m in re.findall(r"0x([0-9a-f]{5,8})", i.op_str):
                v = int(m, 16)
                if v in tab and i.mnemonic in ("call", "jmp", "push", "mov"):
                    note += " ; " + tab[v][7]
                elif 0x253360 <= v < 0x2817f4 and "dword ptr" in i.op_str and i.mnemonic.startswith("f"):
                    note += " ; %r" % struct.unpack("<f", at(v, 4))[0]
                elif 0x253360 <= v < 0x2817f4 and "qword ptr" in i.op_str:
                    note += " ; %r" % struct.unpack("<d", at(v, 8))[0]
            print("%08x  %-8s %s%s" % (i.address, i.mnemonic, i.op_str, note))


if __name__ == "__main__":
    main()
