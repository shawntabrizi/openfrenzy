#!/usr/bin/env python3
"""Generate src/bumpercars_a/BumperCars/bca_data.c: the data blocks owned by bumpercars_a.

Usage (from the repository root):
    python3 -I src/bumpercars_a/tools/gen_bca_data.py [EXTRA_FUNCS_DIR]

Same design as src/classlib_fx/tools/gen_classlib_fx_data.py (from core's gen_core_data.py):
each block is defined once with the original layout and the XBE's initial values, and every
name of an address inside a block becomes an assembler alias "name = block + offset". The
names are the Ghidra names in export/funcs/*.c (and EXTRA_FUNCS_DIR/*.c, functions exported
later with ExportAt.java), plus DAT_<addr> for every word of a block.

Blocks:
  of_bca_data   [0x00290ee4, 0x00292940)  .data: BumperCars colours and planes (0x00290ee4),
                                          the generic debris types of ClassLib/Debris.cpp
                                          (sprite definitions 0x00290f00, sprite animations
                                          0x002910b0, stage tables, types 0x00291600-0x0029180c)
                                          and the BumperCars debris types (0x00291810-)
  of_bca_debris [0x002ed33c, 0x002ed3a0)  .bss of Debris.cpp: the ground test, flags, the
                                          debris system, four colours

Not here: the CBCar_Game object (0x002ed458, 0x4bf0 bytes) lies in bumpercars_b's .bss block
of_bcb_bss (see bumpercars_a.h).

A word of .data becomes a pointer when it points at a text string in .rdata, at a game
function start, or into the block itself. Any other value inside an XBE section stops the
generator (fail loud), except values in a code section that are not function starts:
the tables hold flags (0x00020000, ...) and text bytes that look like such addresses.
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

BLOCKS = [
    ("of_bca_data", 0x00290EE4, 0x00292940, True),
    ("of_bca_debris", 0x002ED33C, 0x002ED3A0, False),
]

NAME_RE = re.compile(r"\b((?:_?DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?|u_[A-Za-z0-9_]*?)_(00[23][0-9a-f]{5}))\b")


def ghidra_names(dirs):
    names = {}
    for funcs in dirs:
        for f in sorted(os.listdir(funcs)):
            if not f.endswith(".c"):
                continue
            for full, addr in NAME_RE.findall(open(os.path.join(funcs, f), errors="replace").read()):
                names.setdefault(int(addr, 16), set()).update((full, "DAT_" + addr))
    return names


def function_starts():
    starts = set()
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if not line.startswith("#"):
            starts.add(int(line.split("\t", 1)[0], 16))
    return starts


def c_string(raw):
    out = []
    for b in raw:
        c = chr(b)
        if c == "\\":
            out.append("\\\\")
        elif c == '"':
            out.append('\\"')
        elif 32 <= b < 127:
            out.append(c)
        else:
            out.append("\\%03o" % b)
    return '"' + "".join(out) + '"'


def main():
    dirs = [os.path.join(ROOT, "export", "funcs")] + sys.argv[1:]
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)

    def section(v):
        for n, va, vs, ra, rs in secs:
            if va <= v < va + vs:
                return n
        return None

    def ascii_at(v):
        raw = b""
        while len(raw) < 512:
            c = at(v + len(raw), 1)
            if c == b"\0":
                return raw if all(32 <= b < 127 or b in (9, 10) for b in raw) else None
            raw += c
        return None

    names = ghidra_names(dirs)
    starts = function_starts()
    strings = {}
    funcs = set()
    body = []
    aliases = []
    errors = []
    for name, start, end, init in BLOCKS:
        size = end - start
        body.append("/* 0x%08x-0x%08x */" % (start, end))
        if not init:
            body.append("__attribute__((nocommon, aligned(16))) unsigned char %s[0x%x];" % (name, size))
        else:
            words = []
            for a in range(start, end, 4):
                v = struct.unpack("<I", at(a, 4))[0]
                s = section(v) if v >= 0x10000 else None
                if s == ".rdata" and ascii_at(v) is not None:
                    strings[v] = ascii_at(v)
                    words.append("(uint32_t)str_%08x" % v)
                elif s == ".text" and v in starts:
                    funcs.add(v)
                    words.append("(uint32_t)FUN_%08x" % v)
                elif start <= v < end:
                    words.append("(uint32_t)((char *)%s + 0x%x)" % (name, v - start))
                else:
                    # flags and text bytes can look like addresses in a code section
                    if s is not None and s not in (".text", "D3D", "DSOUND", "XGRPH", "XPP",
                                                   "WMADEC", "WMADECXM", "DOLBY"):
                        errors.append("0x%08x holds 0x%08x (%s): review it" % (a, v, s))
                    words.append("0x%08x" % v)
            body.append("__attribute__((aligned(16))) uint32_t %s[0x%x] = {" % (name, size // 4))
            for i in range(0, len(words), 4):
                body.append("\t/* 0x%08x */ %s," % (start + i * 4, ", ".join(words[i:i + 4])))
            body.append("};")
        body.append("")
        block_names = {}
        for addr in range(start, end, 4):
            block_names.setdefault(addr, set()).add("DAT_%08x" % addr)
        for addr in names:
            if start <= addr < end:
                block_names.setdefault(addr, set()).update(names[addr])
        for addr in sorted(block_names):
            for n in sorted(block_names[addr]):
                aliases.append((n, name, addr - start, addr))
    if errors:
        for e in errors:
            print("gen_bca_data: error: " + e, file=sys.stderr)
        sys.exit(1)
    out = ["/* generated by src/bumpercars_a/tools/gen_bca_data.py from orig/default.xbe and",
           "   export/funcs; do not edit. Data blocks owned by bumpercars_a: see the generator docstring. */",
           "#include <stdint.h>", ""]
    for v in sorted(strings):
        out.append("static const char str_%08x[] = %s;  /* 0x%08x */" % (v, c_string(strings[v]), v))
    out.append("")
    if funcs:
        out.append("/* functions referenced by the tables */")
        for v in sorted(funcs):
            out.append("void FUN_%08x(void);" % v)
        out.append("")
    out.append("extern uint32_t of_bca_data[];")
    out += body
    out.append("/* names of addresses inside the blocks, for this and other modules */")
    out.append("__asm__(")
    for n, block, off, addr in aliases:
        # file-scope asm names are Mach-O names: asm_convert.py strips one leading
        # underscore, so the C name _DAT_x is written __DAT_x (integration round 1)
        sym = "_" + n if n.startswith("_") else n
        out.append('\t".globl %s\\n.set %s, %s + 0x%x\\n"  /* 0x%08x */' % (sym, sym, block, off, addr))
    out.append(");")
    path = os.path.join(ROOT, "src", "bumpercars_a", "BumperCars", "bca_data.c")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    print("gen_bca_data: %d aliases, %d strings, %d functions -> %s"
          % (len(aliases), len(strings), len(funcs), path))


if __name__ == "__main__":
    main()
