#!/usr/bin/env python3
"""Generate src/carryon_a/CarryOn/carryon_a_data.c: the CarryOn data blocks.

Usage (from the repository root):
    python3 -I src/carryon_a/tools/gen_carryon_data.py

Copied from src/charlib/tools/gen_charlib_data.py (same design, see
src/core/tools/gen_core_data.py): CarryOn code indexes its globals as tables (per variant,
per player), and the CFire_Game object at 0x00293ad8 is a .data object whose fields have
their own Ghidra names (0x00293ae4, 0x00293b28, ...). Each block is defined once with the
original layout, and every Ghidra name inside a block (from export/funcs/*.c of any module)
becomes an assembler alias "name = block + offset".

Aliases of names that carryon_a functions use are strong (.globl). Names that only other
modules use (carryon_b) are weak (.weak_definition, which port/macos/tools/asm_convert.py
turns into ELF .weak), so their owner may still define them.

Blocks (the CarryOn ranges that src/charlib/charlib_data.h leaves out):
  of_co_data    [0x00293a50, 0x00293e80)  .data (XBE values)
  of_co_data2   [0x00293f14, 0x00294afc)  .data (XBE values)
  of_co_bss     [0x002f6b80, 0x002f6c10)  .bss (zero)
  of_co_bss2    [0x002f6d20, 0x002f7a20)  .bss (zero)

A word of .data becomes a pointer when it points at a text string in .rdata, exactly at a
game function start (of_functions.tsv), or is listed in POINTERS (reviewed: vtables);
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
    ("of_co_data", 0x00293A50, 0x00293E80, True),
    ("of_co_data2", 0x00293F14, 0x00294AFC, True),
    ("of_co_bss", 0x002F6B80, 0x002F6C10, False),
    ("of_co_bss2", 0x002F6D20, 0x002F7A20, False),
]

# reviewed words that look like addresses but are numbers (flag words next to name
# pointers in the tables at 0x002941b0 and 0x00294990)
NOT_POINTERS = {0x002941DC, 0x002941E4, 0x002941EC, 0x002942F4, 0x00294998, 0x00294A58}

# reviewed words that point at data with a symbol in carryon_a (address -> C expression)
POINTERS = {
    0x00293AD8: "vtable_00267468",  # the CFire_Game object's vtable
}

MODULE = "carryon_a"


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
    used.update(header_only_names())
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
    for addr in header_only_names():
        names.setdefault(addr, set()).add("DAT_%08x" % addr)
    return names


def header_only_names():
    """Addresses that carryon_a_data.h names but no Ghidra output spells exactly (a table
    reached through an offset, e.g. DAT_00293fd0 as &DAT_00293fd4 - 4). Their aliases are
    strong: only this module uses those names."""
    seen = set()
    funcs = os.path.join(ROOT, "export", "funcs")
    for f in os.listdir(funcs):
        if f.endswith(".c"):
            for full, addr in NAME_RE.findall(open(os.path.join(funcs, f), errors="replace").read()):
                seen.add(int(addr, 16))
    header = open(os.path.join(ROOT, "src", "carryon_a", "carryon_a_data.h")).read()
    return {int(a, 16) for a in re.findall(r"\bDAT_(00[23][0-9a-f]{5})\b", header)} - seen


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
    out = ["/* generated by src/carryon_a/tools/gen_carryon_data.py from orig/default.xbe and",
           "   export/funcs; do not edit. CarryOn data blocks: see carryon_a_data.h. */",
           "#include <stdint.h>", "",
           "extern const void *const vtable_00267468[];"]
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
        for addr in sorted(names):
            if start <= addr < end:
                for n in sorted(names[addr]):
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
        bind = ".globl" if addr in used else ".weak_definition"
        out.append('\t"%s %s\\n.set %s, %s + 0x%x\\n"  /* 0x%08x */' % (bind, n, n, block, off, addr))
    out.append(");")
    path = os.path.join(ROOT, "src", "carryon_a", "CarryOn", "carryon_a_data.c")
    with open(path, "w") as f:
        f.write("\n".join(out) + "\n")
    for line in review:
        print("review: " + line, file=sys.stderr)
    print("gen_carryon_data: %d aliases, %d strings, %d wide strings, %d functions -> %s"
          % (len(aliases), len(strings), len(wstrings), len(funcs), path))


if __name__ == "__main__":
    main()
