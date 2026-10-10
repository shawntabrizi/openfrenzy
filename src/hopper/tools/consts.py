#!/usr/bin/env python3
"""Print the read-only constants an original function reads (analysis helper of the hopper
module): every `dword/qword ptr [0x...]` operand inside .rdata, with its value as a float or
double. Usage: python3 -I src/hopper/tools/consts.py ADDR[:END] [...]
"""
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

DIS = os.path.join(ROOT, "src", "hopper", "tools", "dis.py")


def main():
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)

    def section(v):
        for n, va, vs, ra, rs in secs:
            if va <= v < va + vs:
                return n
        return None
    for a in sys.argv[1:]:
        out = subprocess.check_output([sys.executable, "-I", DIS] + a.split(":")).decode()
        seen = {}
        for m in re.finditer(r"(dword|qword|tbyte) ptr \[0x([0-9a-f]+)\]", out):
            size, addr = m.group(1), int(m.group(2), 16)
            if section(addr) != ".rdata":
                continue
            if size == "dword":
                v = struct.unpack("<f", at(addr, 4))[0]
                raw = "%08x" % struct.unpack("<I", at(addr, 4))[0]
            elif size == "qword":
                v = struct.unpack("<d", at(addr, 8))[0]
                raw = "%016x" % struct.unpack("<Q", at(addr, 8))[0]
            else:
                continue
            seen[(addr, size)] = (v, raw)
        print("==", a)
        for (addr, size), (v, raw) in sorted(seen.items()):
            print("  0x%08x %s %r (%s)" % (addr, size, v, raw))


if __name__ == "__main__":
    main()
