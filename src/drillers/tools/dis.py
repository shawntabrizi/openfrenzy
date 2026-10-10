#!/usr/bin/env python3
"""Disassemble original x86 code from orig/default.xbe with LLVM 22's llvm-objdump
(analysis helper of the drillers module).

Usage: python3 -I src/carryon_a/tools/dis.py ADDR [END]
Without END the size comes from src/include/of_functions.tsv.
The .text section is converted once to an ELF object in $TMPDIR (llvm-objcopy).
"""
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

LLVM = "/opt/homebrew/opt/llvm@22/bin"


def main():
    d, secs = xbe_data.load()
    start = int(sys.argv[1], 16)
    if len(sys.argv) > 2:
        end = int(sys.argv[2], 16)
    else:
        # up to the next function start (the table sizes can miss a final ret)
        starts = sorted(int(line.split("\t")[0], 16)
                        for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv"))
                        if not line.startswith("#"))
        later = [a for a in starts if a > start]
        end = later[0] if later else start + 0x100
    for name, va, vs, ra, rs in secs:
        if va <= start < va + vs:
            break
    tmp = os.environ.get("TMPDIR", "/tmp")
    obj = os.path.join(tmp, "of_%s_%08x.o" % (name.strip("."), va))
    if not os.path.exists(obj):
        raw = os.path.join(tmp, "of_sec.bin")
        open(raw, "wb").write(d[ra:ra + rs])
        subprocess.check_call([LLVM + "/llvm-objcopy", "-I", "binary", "-O", "elf32-i386",
                               "--rename-section", ".data=.text,code", raw, obj])
    out = subprocess.check_output([LLVM + "/llvm-objdump", "-d", "--no-show-raw-insn",
                                   "--x86-asm-syntax=intel",
                                   "--start-address=0x%x" % (start - va),
                                   "--stop-address=0x%x" % (end - va),
                                   obj]).decode()
    names = xbe_data.function_names()
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s*(.*)$", line)
        if not m:
            continue
        ins = re.sub(r"<[^>]*>", "", m.group(2))

        def fix(t):
            v = int(t.group(1), 16) + va
            n = names.get(v)
            return "0x%x%s" % (v, " FUN_%08x(%s)" % (v, n[0]) if n else "")
        if re.match(r"(call|j\w+)\s+0x", ins):
            ins = re.sub(r"0x([0-9a-f]+)", fix, ins, count=1)
        print("%08x  %s" % (int(m.group(1), 16) + va, ins))


if __name__ == "__main__":
    main()
