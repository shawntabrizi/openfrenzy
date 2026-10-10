#!/usr/bin/env python3
"""Disassemble a function of orig/default.xbe (analysis helper of the mousewhack module).

Usage: python3 -I src/mousewhack/tools/dis.py ADDR [SIZE]
SIZE defaults to the function's size in src/include/of_functions.tsv. Uses llvm-mc (llvm@22).
Call targets get their names from the generated of_decls.h of build-mousewhack.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

OBJCOPY = "/opt/homebrew/opt/llvm@22/bin/llvm-objcopy"
OBJDUMP = "/opt/homebrew/opt/llvm@22/bin/llvm-objdump"


def main():
    addr = int(sys.argv[1], 16)
    size = int(sys.argv[2], 0) if len(sys.argv) > 2 else None
    if size is None:
        for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
            f = line.split("\t")
            if f[0] == "%08x" % addr:
                size = int(f[5])
    names = {}
    decls = os.path.join(ROOT, "build-mousewhack", "guest-main", "gen", "of_decls.h")
    for line in open(decls):
        m = re.match(r"#define FUN_([0-9a-f]{8}) (\w+)", line)
        if m:
            names[int(m.group(1), 16)] = m.group(2)
    d, secs = xbe_data.load()
    rd = xbe_data.reader(d, secs)
    code = rd(addr, size)
    tmp = os.environ.get("TMPDIR", "/tmp")
    raw = os.path.join(tmp, "mw_dis_%08x.bin" % addr)
    elf = raw + ".o"
    open(raw, "wb").write(code)
    # objcopy the raw bytes into an i386 ELF section placed at the original address
    subprocess.run([OBJCOPY, "-I", "binary", "-O", "elf32-i386", "--rename-section",
                    ".data=.text,code", "--change-section-address", ".text=0x%x" % addr,
                    raw, elf], check=True)
    out = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", "-M", "intel",
                          elf], capture_output=True, text=True).stdout
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(.*)", line)
        if not m:
            continue
        pc = int(m.group(1), 16) + addr
        ins = m.group(2).strip()
        c = re.match(r"(call|jmp|j\w+)\s+0x([0-9a-f]+)", ins)
        if c:
            tgt = (int(c.group(2), 16) + addr) & 0xffffffff
            ins = "%s 0x%08x %s" % (c.group(1), tgt, names.get(tgt, ""))
        ins = re.sub(r"\b0x([0-9a-f]{5,6})\b", lambda m: "0x%08x" % int(m.group(1), 16), ins)
        print("%08x  %s" % (pc, ins))


if __name__ == "__main__":
    main()
