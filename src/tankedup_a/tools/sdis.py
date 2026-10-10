#!/usr/bin/env python3
"""Disassemble with string and float annotations (analysis helper of the tankedup_a module).

Usage: build-tankedup_a/venv/bin/python -I src/tankedup_a/tools/sdis.py ADDR [LENGTH]
Immediates that point at a text string in .rdata/.data are shown as the string.
"""
import os
import re
import struct
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

    def string(v):
        try:
            raw = b""
            while len(raw) < 80:
                c = at(v + len(raw), 1)
                if c == b"\0":
                    break
                raw += c
            if len(raw) >= 2 and all(32 <= b < 127 or b in (9, 10) for b in raw):
                return raw.decode().replace("\n", "\\n")
        except SystemExit:
            pass
        return None

    addr = int(sys.argv[1], 16)
    length = int(sys.argv[2], 0) if len(sys.argv) > 2 else int(tab[addr][5])
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for i in md.disasm(at(addr, length), addr):
        note = ""
        for m in re.findall(r"0x([0-9a-f]{5,8})", i.op_str):
            v = int(m, 16)
            if v in tab:
                note += " ; " + tab[v][7]
            elif 0x253360 <= v < 0x2b9278:
                s = string(v)
                if s is not None and "ptr" not in i.op_str:
                    note += ' ; "%s"' % s
                elif "dword ptr" in i.op_str and i.mnemonic.startswith("f"):
                    note += " ; %r" % struct.unpack("<f", at(v, 4))[0]
                elif "qword ptr" in i.op_str:
                    note += " ; %r" % struct.unpack("<d", at(v, 8))[0]
            if i.mnemonic == "mov" and "dword ptr" in i.op_str and v > 0x3f000000:
                note += " ; %r" % struct.unpack("<f", struct.pack("<I", v))[0]
        print("%08x  %-8s %s%s" % (i.address, i.mnemonic, i.op_str, note))


if __name__ == "__main__":
    main()
