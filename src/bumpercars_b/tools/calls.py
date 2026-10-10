#!/usr/bin/env python3
"""List the calls of an original function with the last value given to ECX before each
call and the pushes since the previous call (analysis helper of the bumpercars_b module).

Usage: build-bumpercars_b/venv/bin/python -I src/bumpercars_b/tools/calls.py ADDR [FILTER]
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    import capstone
    addr = int(sys.argv[1], 16)
    filt = sys.argv[2] if len(sys.argv) > 2 else None
    size = 0x400
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        f = line.split("\t")
        if f[0] == "%08x" % addr:
            size = int(f[5])
    d, secs = xbe_data.load()
    code = xbe_data.reader(d, secs)(addr, size)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    ecx = "?"
    pushes = []
    for ins in md.disasm(code, addr):
        if ins.mnemonic in ("mov", "lea") and ins.op_str.startswith("ecx,"):
            ecx = ins.op_str[5:]
        if ins.mnemonic == "push":
            pushes.append(ins.op_str)
        if ins.mnemonic == "call":
            if filt is None or filt in ins.op_str:
                print("%08x call %-10s ecx=%-28s pushes(last first)=%s" % (
                    ins.address, ins.op_str, ecx, list(reversed(pushes))[:8]))
            pushes = []
            ecx = "?(after call)"
        if ins.mnemonic.startswith("j") or ins.mnemonic == "ret":
            pass


if __name__ == "__main__":
    main()
