#!/usr/bin/env python3
"""Guess the calling convention of original functions: whether ECX/EDX are read before
being written (thiscall/fastcall), the `ret N` (callee-popped bytes) and whether the result
is left on the x87 stack (analysis helper of the bumpercars_b module).

Usage: build-bumpercars_b/venv/bin/python -I src/bumpercars_b/tools/abi.py ADDR [ADDR ...]
"""
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def main():
    import capstone
    sizes = {}
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        f = line.split("\t")
        sizes[int(f[0], 16)] = (int(f[5]), f[1], f[7], f[8].strip())
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    md.detail = True
    for a in sys.argv[1:]:
        addr = int(a, 16)
        size, mod, name, proto = sizes.get(addr, (0x200, "?", "?", "?"))
        code = at(addr, size)
        ecx = edx = None
        rets = set()
        fld_before_ret = False
        last = None
        for ins in md.disasm(code, addr):
            regs_read, regs_write = ins.regs_access()
            rn = {ins.reg_name(r) for r in regs_read}
            wn = {ins.reg_name(r) for r in regs_write}
            if ecx is None:
                if "ecx" in rn or "cx" in rn or "cl" in rn:
                    ecx = "read"
                elif "ecx" in wn:
                    ecx = "written"
            if edx is None:
                if "edx" in rn or "dl" in rn or "dx" in rn:
                    edx = "read"
                elif "edx" in wn:
                    edx = "written"
            if ins.mnemonic == "ret":
                rets.add(ins.op_str or "0")
                if last is not None and last.mnemonic.startswith("fld"):
                    fld_before_ret = True
            last = ins
        print("%08x %-12s ecx:%-7s edx:%-7s ret:%-8s fldret:%d  %s" % (
            addr, mod, ecx, edx, ",".join(sorted(rets)), fld_before_ret, proto))


if __name__ == "__main__":
    main()
