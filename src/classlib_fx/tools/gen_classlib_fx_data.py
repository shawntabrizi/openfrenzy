#!/usr/bin/env python3
"""Generate src/classlib_fx/ClassLib/classlib_fx_data.c: the data blocks owned by classlib_fx.

Usage (from the repository root):
    python3 -I src/classlib_fx/tools/gen_classlib_fx_data.py

Adapted from src/classlib_ui/tools/gen_classlib_ui_data.py (itself from
src/core/tools/gen_core_data.py; same design, see their docstrings): each block is defined
once with the original layout and the XBE's initial values, and every name of an address
inside a block becomes an assembler alias "name = block + offset". The names are the Ghidra
names in export/funcs/*.c of any module, plus DAT_<addr> for every word of a block (this
module's code uses DAT_<addr> names for its tables), except in EXCLUDE.

Blocks:
  of_clf_data   [0x00290cc0, 0x00290ee4)  .data of CProjector.cpp / RefBot.cpp, the blend
                                          table, the actor debris model override
                                          (0x00290e8c) and the debris vertex buffer
                                          "debrisVB" (a CSpriteVB at 0x00290ea8, XBE values)
  of_clf_bss    [0x002ed3a0, 0x002ed454)  .bss of CProjector.cpp / RefBot.cpp (zero)
  of_clf_ray_data [0x00292c44, 0x00292c48)  .data of CRay.cpp (XBE values)
  of_clf_ray_bss  [0x002f27c8, 0x002f27d4)  .bss of CRay.cpp: the ray control list (zero)
  of_clf_token_data [0x00298d1c, 0x00298d4c)  .data of CToken.cpp: tunables, model table
  of_clf_token_bss  [0x002ff9c8, 0x002ff9e0)  .bss of CToken.cpp: shadow mode/texture, list
  of_clf_water_data [0x002ae578, 0x002ae5c0)  .data of CWaterField.cpp: default parameters, and
                                          two constants of the state handlers at 0x0013a650
  of_clf_water_bss  [0x00302340, 0x003023d4)  .bss: the two water materials, and two words of
                                          the state handlers (0x003023cc, 0x003023d0)
  of_clf_ai_data    [0x002ae7b4, 0x002ae814)  .data of the AI state handlers (0x00139e00-)
  of_clf_gull_data  [0x002b2670, 0x002b26b4)  .data of CSeagulls.cpp: default flock parameters

EXCLUDE: [0x00290d14, 0x00290d54) lies inside the ray type table (entry 1's colours) but
classlib_ui already defines those names (its block of_clu_data2). Two definitions of a name
cannot link, so this module emits no alias there; the bytes in of_clf_data are the ones the
projector code uses. See the module report: classlib_ui should drop of_clu_data2.

A word of .data becomes a pointer when it points at a text string in .rdata, at a game
function start, into its own block (the empty list heads of debrisVB), or at an address in
DATA_POINTERS (another module's data: emitted as the
address of DAT_<addr>, which that module defines or aliases; clf_extern.c has a tentative
definition). Every other value inside an XBE section stops the generator (fail loud).
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

BLOCKS = [
    ("of_clf_data", 0x00290CC0, 0x00290EE4, True),
    ("of_clf_bss", 0x002ED3A0, 0x002ED454, False),
    ("of_clf_ray_data", 0x00292C44, 0x00292C48, True),
    ("of_clf_ray_bss", 0x002F27C8, 0x002F27D4, False),
    ("of_clf_token_data", 0x00298D1C, 0x00298D4C, True),
    ("of_clf_token_bss", 0x002FF9C8, 0x002FF9E0, False),
    ("of_clf_water_data", 0x002AE578, 0x002AE5C0, True),
    ("of_clf_water_bss", 0x00302340, 0x003023D4, False),
    ("of_clf_ai_data", 0x002AE7B4, 0x002AE814, True),
    ("of_clf_gull_data", 0x002B2670, 0x002B26B4, True),
]

EXCLUDE = [(0x00290D14, 0x00290D54)]

# data addresses of other modules that this module's tables point at
DATA_POINTERS = {0x002916DC}

NAME_RE = re.compile(r"\b((?:_?DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?|u_[A-Za-z0-9_]*?)_(00[23][0-9a-f]{5}))\b")


def excluded(addr):
    return any(lo <= addr < hi for lo, hi in EXCLUDE)


def ghidra_names():
    names = {}
    funcs = os.path.join(ROOT, "export", "funcs")
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

    names = ghidra_names()
    starts = function_starts()
    out = ["/* generated by src/classlib_fx/tools/gen_classlib_fx_data.py from orig/default.xbe and",
           "   export/funcs; do not edit. Data blocks owned by classlib_fx: see the generator docstring. */",
           "#include <stdint.h>", ""]
    strings = {}
    funcs = set()
    datap = set()
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
                    words.append("(uint32_t)((unsigned char *)%s + 0x%x)" % (name, v - start))
                elif v in DATA_POINTERS:
                    datap.add(v)
                    words.append("(uint32_t)DAT_%08x" % v)
                else:
                    if s is not None:
                        errors.append("0x%08x holds 0x%08x (%s): add it to DATA_POINTERS or "
                                      "review it" % (a, v, s))
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
            if excluded(addr):
                continue
            for n in sorted(block_names[addr]):
                aliases.append((n, name, addr - start, addr))
    if errors:
        for e in errors:
            print("gen_classlib_fx_data: error: " + e, file=sys.stderr)
        sys.exit(1)
    for v in sorted(strings):
        out.append("static const char str_%08x[] = %s;  /* 0x%08x */" % (v, c_string(strings[v]), v))
    out.append("")
    if funcs:
        out.append("/* functions referenced by the tables (declared in of_decls.h) */")
        out.append("")
    if datap:
        out.append("/* other modules' data the tables point at (tentative definitions in clf_extern.c) */")
        for v in sorted(datap):
            out.append("extern int DAT_%08x[];" % v)
        out.append("")
    out += body
    out.append("/* names of addresses inside the blocks, for this and other modules */")
    out.append("__asm__(")
    for n, block, off, addr in aliases:
        # file-scope asm names are Mach-O names: asm_convert.py strips one leading
        # underscore, so the C name _DAT_x is written __DAT_x (integration round 1)
        sym = "_" + n if n.startswith("_") else n
        out.append('\t".globl %s\\n.set %s, %s + 0x%x\\n"  /* 0x%08x */' % (sym, sym, block, off, addr))
    out.append(");")
    path = os.path.join(ROOT, "src", "classlib_fx", "ClassLib", "classlib_fx_data.c")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    print("gen_classlib_fx_data: %d aliases, %d strings, %d functions, %d data pointers -> %s"
          % (len(aliases), len(strings), len(funcs), len(datap), path))


if __name__ == "__main__":
    main()
