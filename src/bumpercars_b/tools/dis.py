#!/usr/bin/env python3
"""Disassemble original x86 code from orig/default.xbe with capstone (Intel syntax)
(analysis helper of the bumpercars_b module).

Usage: build-bumpercars_b/venv/bin/python -I src/bumpercars_b/tools/dis.py ADDR [LENGTH]   (LENGTH default: table size)
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402



def main():
    import capstone
    addr = int(sys.argv[1], 16)
    if len(sys.argv) > 2:
        length = int(sys.argv[2], 0)
    else:
        length = 0x100
        for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
            f = line.split("\t")
            if f[0] == "%08x" % addr:
                length = int(f[5])
    d, secs = xbe_data.load()
    raw = xbe_data.reader(d, secs)(addr, length)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for i in md.disasm(raw, addr):
        print("%08x  %-8s %s" % (i.address, i.mnemonic, i.op_str))


if __name__ == "__main__":
    main()
