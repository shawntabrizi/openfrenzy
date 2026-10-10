#!/usr/bin/env python3
"""Generate src/drillers/Drillers/drillers_data.c: the Drillers data blocks.

Usage (from the repository root):
    python3 -I src/drillers/tools/gen_drillers_data.py

Copied from src/clusteroids/tools/gen_clust_data.py (same design as
src/core/tools/gen_core_data.py). Drillers code indexes its globals as tables (per
player 0x730 bytes, per map cell 0x2d8 bytes), so each range is one block with the
original layout, and the C code reaches a global through DR_VAR(type, original address)
(drillers_data.h).

Only Ghidra names that other modules use get an assembler alias "name = block + offset"
(.globl; e.g. DAT_002ff9f0, the CDrillers_Game object that core's static constructor
builds, and the static objects whose vtables core's static destructors write).

Blocks (the Drillers ranges; only core's static constructors/destructors and foxhunt's
0x00125090 name an address inside them):
  of_dr_data   [0x00298d50, 0x002ab26c)  .data (XBE values)
  of_dr_bss    [0x002ff9e0, 0x00302138)  .bss (zero)

A word of .data becomes a pointer when it points at a text string in .rdata, exactly at a
game function start (of_functions.tsv), into a block, or is listed in POINTERS;
every other value inside an XBE section is listed on stderr and kept as a number.
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

BLOCKS = [
    ("of_dr_data", 0x00298D50, 0x002AB26C, True),
    ("of_dr_bss", 0x002FF9E0, 0x00302138, False),
]

# reviewed words that look like addresses but are numbers
NOT_POINTERS = set()

# reviewed words that point at data with a symbol elsewhere (address -> C expression)
POINTERS = {
    0x00298DA0: "vtable_0026c294",  # the static objects of the Drillers files (one-slot vtables:
    0x00298E70: "vtable_0026c2ec",  # the deleting destructor; core's static destructors store
    0x002A54D8: "vtable_0026c310",  # the vtable again at exit)
    0x002AA830: "vtable_0026c568",
    0x002AA900: "vtable_0026c5a0",
    0x002AAAF8: "vtable_0026cb20",
    0x002AAB08: "vtable_0026cb7c",
    0x002AADF8: "vtable_0026cbac",
    0x002AAE60: "vtable_0026cc44",  # owned by foxhunt's table entries 0x00125070.. (weak in core)
}

MODULE = "drillers"

# names other modules' hand-written code uses although Ghidra's C of their functions does not
# (core's static constructor/destructor pass the object in ECX)
EXTRA_ALIASES = {0x002FF9F0: "DAT_002ff9f0", 0x00300C78: "DAT_00300c78"}

# Ghidra names inside the blocks that are not addresses (bkernel's 0x0004a360 fills memory
# with the constant 0x00300030)
NO_ALIAS = {0x00300030}


def module_names():
    """Ghidra names that functions of MODULE use."""
    used = set()
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        cols = line.split("\t")
        if cols[1] != MODULE:
            continue
        p = os.path.join(ROOT, "export", "funcs", cols[0] + ".c")
        if os.path.exists(p):
            for full, addr in NAME_RE.findall(open(p, errors="replace").read()):
                used.add(int(addr, 16))
    return used

NAME_RE = re.compile(r"\b((?:_?DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?|u_[A-Za-z0-9_]*?)_(00[23][0-9a-f]{5}))\b")


def ghidra_names():
    names = {}
    funcs = os.path.join(ROOT, "export", "funcs")
    for f in sorted(os.listdir(funcs)):
        if not f.endswith(".c"):
            continue
        for full, addr in NAME_RE.findall(open(os.path.join(funcs, f), errors="replace").read()):
            names.setdefault(int(addr, 16), set()).update((full, "DAT_" + addr))
    return names


def foreign_names():
    """addresses that functions of other modules name"""
    used = set()
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if line.startswith("#"):
            continue
        cols = line.split("\t")
        if cols[1] == MODULE:
            continue
        p = os.path.join(ROOT, "export", "funcs", cols[0] + ".c")
        if os.path.exists(p):
            for full, addr in NAME_RE.findall(open(p, errors="replace").read()):
                used.add(int(addr, 16))
    return used


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

    def utf16_at(v):
        units = []
        while len(units) < 512:
            u = struct.unpack("<H", at(v + 2 * len(units), 2))[0]
            if u == 0:
                return units if units and all(32 <= x < 127 or x in (9, 10) for x in units) else None
            units.append(u)
        return None

    names = ghidra_names()
    starts = function_starts()
    used = module_names()
    foreign = foreign_names()
    out = ["/* generated by src/drillers/tools/gen_drillers_data.py from orig/default.xbe and",
           "   export/funcs; do not edit. Drillers data blocks: see drillers_data.h. */",
           "#include <stdint.h>", "",
           ] + ["extern const void *const %s[];" % v for v in sorted(set(POINTERS.values()))]
    for bname, bstart, bend, binit in BLOCKS:
        out.append("extern %s %s[];" % ("uint32_t" if binit else "unsigned char", bname))
    out.append("")
    strings = {}
    wstrings = {}
    funcs = set()
    body = []
    aliases = []
    review = []
    for name, start, end, init in BLOCKS:
        size = end - start
        body.append("/* 0x%08x-0x%08x */" % (start, end))
        if not init:
            body.append("__attribute__((nocommon, aligned(16))) unsigned char %s[0x%x];" % (name, size))
        else:
            words = []
            for a in range(start, end, 4):
                v = struct.unpack("<I", at(a, 4))[0]
                s = section(v) if v >= 0x10000 and a not in NOT_POINTERS else None
                if a in POINTERS:
                    words.append("(uint32_t)" + POINTERS[a])
                    continue
                inner = [b for b in BLOCKS if b[1] <= v < b[2]] if a not in NOT_POINTERS else []
                if inner:
                    words.append("(uint32_t)((unsigned char *)%s + 0x%x)" % (inner[0][0], v - inner[0][1]))
                    continue
                wide = utf16_at(v) if s == ".rdata" else None
                if wide is not None and len(wide) > 1 and len(ascii_at(v) or b"") <= 1:
                    wstrings[v] = wide
                    words.append("(uint32_t)wstr_%08x" % v)
                elif s == ".rdata" and ascii_at(v) is not None:
                    strings[v] = ascii_at(v)
                    words.append("(uint32_t)str_%08x" % v)
                elif s == ".text" and v in starts:
                    funcs.add(v)
                    words.append("(uint32_t)FUN_%08x" % v)
                else:
                    if s is not None:
                        review.append("0x%08x holds 0x%08x (%s): kept as a number" % (a, v, s))
                    words.append("0x%08x" % v)
            body.append("__attribute__((aligned(16))) uint32_t %s[0x%x] = {" % (name, size // 4))
            for i in range(0, len(words), 4):
                body.append("\t/* 0x%08x */ %s," % (start + i * 4, ", ".join(words[i:i + 4])))
            body.append("};")
        body.append("")
        for addr, n in sorted(EXTRA_ALIASES.items()):
            if start <= addr < end:
                aliases.append((n, name, addr - start, addr))
        for addr in sorted(names):
            if start <= addr < end and addr in foreign and addr not in NO_ALIAS:
                for n in sorted(x for x in names[addr] if x.startswith("DAT_")):
                    aliases.append((n, name, addr - start, addr))
    for v in sorted(strings):
        out.append("static const char str_%08x[] = %s;  /* 0x%08x */" % (v, c_string(strings[v]), v))
    for v in sorted(wstrings):
        text = "".join(chr(u) for u in wstrings[v])
        out.append("static const __CHAR16_TYPE__ wstr_%08x[] = u%s;  /* 0x%08x */"
                   % (v, c_string(text.encode("latin1")), v))
    out.append("")
    if funcs:
        out.append("/* functions referenced by the tables (declared in of_decls.h) */")
        out.append("")
    out += body
    out.append("/* Ghidra names of addresses inside the blocks, for other modules */")
    out.append("__asm__(")
    for n, block, off, addr in aliases:
        bind = ".globl"
        out.append('\t"%s %s\\n.set %s, %s + 0x%x\\n"  /* 0x%08x */' % (bind, n, n, block, off, addr))
    out.append(");")
    path = os.path.join(ROOT, "src", "drillers", "Drillers", "drillers_data.c")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    for line in review:
        print("review: " + line, file=sys.stderr)
    print("gen_drillers_data: %d aliases, %d strings, %d wide strings, %d functions -> %s"
          % (len(aliases), len(strings), len(wstrings), len(funcs), path))


if __name__ == "__main__":
    main()
