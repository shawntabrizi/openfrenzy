#!/usr/bin/env python3
"""Disassemble original x86 code from orig/default.xbe (analysis helper of the charlib module).

Usage (from the repository root):
    python3 -I src/charlib/tools/disasm.py ADDR [END]

ADDR/END are XBE virtual addresses (hex). Without END, disassembles the function at ADDR
(size from src/include/of_functions.tsv). Needs llvm-objcopy/llvm-objdump (Homebrew llvm@22).
Ghidra drops the argument of _ftol (FUN_001de998) and of x87 stack calls; read them here.
"""
import os
import subprocess
import sys
import tempfile

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

LLVM = "/opt/homebrew/opt/llvm@22/bin"


def main():
    start = int(sys.argv[1], 16)
    if len(sys.argv) > 2:
        end = int(sys.argv[2], 16)
    else:
        end = None
        for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
            if line.startswith("#"):
                continue
            f = line.split("\t")
            if int(f[0], 16) == start:
                end = start + int(f[5])
        if end is None:
            end = start + 0x100
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    raw = at(start, end - start)
    names = xbe_data.function_names()
    with tempfile.TemporaryDirectory() as tmp:
        b = os.path.join(tmp, "code.bin")
        e = os.path.join(tmp, "code.o")
        open(b, "wb").write(raw)
        subprocess.check_call([LLVM + "/llvm-objcopy", "-I", "binary", "-O", "elf32-i386",
                               "--rename-section", ".data=.text,code", b, e])
        out = subprocess.check_output([LLVM + "/llvm-objdump", "-d", "--no-show-raw-insn",
                                       "-M", "intel", e], text=True)
    import re
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s*(.*)$", line)
        if not m:
            continue
        off, ins = int(m.group(1), 16), m.group(2).split(" <")[0].strip()
        parts = ins.split()
        note = ""
        if len(parts) == 2 and parts[0].startswith(("call", "j", "loop")) and parts[1].startswith("0x"):
            t = (int(parts[1], 16) + start) & 0xffffffff
            ins = "%s 0x%08x" % (parts[0], t)
            if parts[0] == "call":
                n = names.get(t)
                note = "    ; FUN_%08x %s" % (t, n[1] if n else "")
        print("%08x  %s%s" % (start + off, ins, note))

if __name__ == "__main__":
    main()
