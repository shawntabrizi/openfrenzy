#!/usr/bin/env python3
"""Generate src/meta_a/Meta/meta_a_data.c: the Meta .data block with the original layout.

Usage (from the repository root, needs orig/default.xbe and export/funcs):
    python3 -I src/meta_a/tools/gen_meta_a_data.py

Why: the Meta code (meta_a and meta_b) keeps tables in its .data: tweak values, the
CMetaPlayer state tables, the tile type table, sprite definitions, the debris types
(linked to each other), the puzzle keywords. One C variable per Ghidra name cannot keep
their layout or their pointers. So meta_a defines the block once:

  of_meta_data   [0x002b04e4, 0x002b2670)   initial values from orig/default.xbe

Every Ghidra name of an address inside the block (DAT_, _DAT_, PTR_..., s_..., LAB_ in
export/funcs/*.c of any module) becomes an assembler alias "name = block + offset", so
any module that uses the name reaches the same bytes (same design as
src/core/tools/gen_core_data.py). Weak or common definitions of those names elsewhere
lose to these aliases.

A dword whose value is an address inside an XBE section is a pointer and is relocated:
into the block (block + offset), to a NUL-terminated string in .rdata (a string
literal), or to a function start of src/include/of_functions.tsv (FUN_<addr>). Any other
pointer stops the generator (fail loud). Values below 0x11000, and the byte tables
listed in RAW, are numbers.
"""
import os
import re
import struct
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

OUT = os.path.join(ROOT, "src", "meta_a", "Meta", "meta_a_data.c")
START = 0x002B04E4
END = 0x002B2670
# ranges that hold plain numbers (byte tables) whose dwords can look like addresses
RAW = [
    (0x002B05F0, 0x002B06D4),  # CMetaPlayer state/condition priorities (signed bytes)
    (0x002B0910, 0x002B0930),  # orb and tile glow colours (RGBA bytes)
]
NAME_RE = re.compile(r"\b((?:_?DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?|LAB)_(002b[0-9a-f]{4}))\b")


def ghidra_names():
    names = {}
    funcs = os.path.join(ROOT, "export", "funcs")
    for f in sorted(os.listdir(funcs)):
        if f.endswith(".c"):
            text = open(os.path.join(funcs, f), errors="replace").read()
            for full, addr in NAME_RE.findall(text):
                names.setdefault(int(addr, 16), set()).add(full)
    return names


def meta_a_names():
    """DAT_<addr> spellings that meta_a's own sources use (Ghidra may spell them _DAT_ or LAB_)."""
    names = {}
    src = os.path.join(ROOT, "src", "meta_a")
    for dirpath, _, files in os.walk(src):
        for f in files:
            if f.endswith((".c", ".h")) and f != "meta_a_data.c":
                text = open(os.path.join(dirpath, f), errors="replace").read()
                for full, addr in NAME_RE.findall(text):
                    names.setdefault(int(addr, 16), set()).add(full)
    return names


def strong_elsewhere():
    """Names that another module defines strongly (a file-scope definition that is not
    extern and not weak, or an assembler alias) in the tracked sources: not aliased here,
    so the two modules never define the same symbol."""
    import subprocess
    files = subprocess.run(["git", "ls-files", "src"], cwd=ROOT, capture_output=True, text=True).stdout.split()
    defined = set()
    decl = re.compile(r"^(?!extern\b)(?!static\b)[A-Za-z_][^;=(]*?\b((?:_?DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?)_002b[0-9a-f]{4})\b")
    alias = re.compile(r"\.set ((?:_?DAT|PTR_[A-Za-z0-9_]*?|s_[A-Za-z0-9_]*?)_002b[0-9a-f]{4}),")
    for f in files:
        if f.startswith("src/meta_a/") or not f.endswith((".c", ".s", ".S")):
            continue
        for line in open(os.path.join(ROOT, f), errors="replace"):
            for m in alias.finditer(line):
                defined.add(m.group(1))
            m = decl.match(line)
            if m and "weak" not in line:
                defined.add(m.group(1))
    return defined


def functions():
    starts = set()
    for line in open(os.path.join(ROOT, "src", "include", "of_functions.tsv")):
        if not line.startswith("#"):
            starts.add(int(line.split("\t")[0], 16))
    return starts


def c_string(raw):
    out = []
    for b in raw:
        c = chr(b)
        if c == "\\":
            out.append("\\\\")
        elif c == '"':
            out.append('\\"')
        elif 0x20 <= b < 0x7f:
            out.append(c)
        elif b == 10:
            out.append("\\n")
        elif b == 13:
            out.append("\\r")
        elif b == 9:
            out.append("\\t")
        else:
            out.append("\\%03o" % b)
    return "".join(out)


def main():
    data, secs = xbe_data.load()
    at = xbe_data.reader(data, secs)
    funcs = functions()

    def section(va):
        for name, v, vs, ra, rs in secs:
            if v <= va < v + vs:
                return name
        return None

    def string_at(va):
        raw = b""
        while True:
            b = at(va + len(raw), 1)
            if b == b"\0":
                return raw
            if b[0] not in (9, 10, 13) and not 0x20 <= b[0] < 0x7f:
                return None
            raw += b
            if len(raw) > 512:
                return None

    strings = {}
    words = []
    for va in range(START, END, 4):
        v, = struct.unpack("<I", at(va, 4))
        raw = any(lo <= va < hi for lo, hi in RAW)
        sec = section(v) if v >= 0x11000 and not raw else None
        if sec is None:
            words.append("0x%08x" % v)
        elif START <= v < END:
            words.append("META_SELF(0x%x)" % (v - START))
        elif v in funcs:
            words.append("(uint32_t)FUN_%08x" % v)
        elif sec == ".rdata" and string_at(v) is not None:
            strings[v] = string_at(v)
            words.append("(uint32_t)str_%08x" % v)
        else:
            raise SystemExit("gen_meta_a_data: 0x%08x = 0x%08x (%s): unknown pointer" % (va, v, sec))

    names = ghidra_names()
    for addr, spellings in meta_a_names().items():
        names.setdefault(addr, set()).update(spellings)
    for addr in list(names):
        names[addr].add("DAT_%08x" % addr)
    with open(OUT, "w") as f:
        f.write("/* generated by src/meta_a/tools/gen_meta_a_data.py from orig/default.xbe and\n"
                "   export/funcs; do not edit. The Meta .data block: see the generator. */\n")
        f.write("#include <stdint.h>\n\n")
        # (FUN_<addr> are declared by of_decls.h, which every guest file includes)
        for va in sorted(strings):
            f.write('static const char str_%08x[] = "%s";  /* 0x%08x */\n' % (va, c_string(strings[va]), va))
        f.write("\nextern uint32_t of_meta_data[];\n")
        f.write("#define META_SELF(off) ((uint32_t)((char *)of_meta_data + (off)))\n\n")
        f.write("/* 0x%08x-0x%08x */\n" % (START, END))
        f.write("__attribute__((aligned(16))) uint32_t of_meta_data[0x%x] = {\n" % len(words))
        for i in range(0, len(words), 4):
            f.write("\t/* 0x%08x */ %s,\n" % (START + i * 4, ", ".join(words[i:i + 4])))
        skipped = strong_elsewhere()
        f.write("};\n\n/* not aliased (another module defines them): %s */\n"
                % (" ".join(sorted(n for a in names if START <= a < END for n in names[a] if n in skipped)) or "-"))
        f.write("__asm__(\n")
        count = 0
        for addr in sorted(names):
            if START <= addr < END:
                # The guest's assembly is Mach-O first (C names get a leading underscore),
                # then tools/asm_convert.py strips one underscore: write "_" + the C name.
                for name in sorted(n for n in names[addr] if n not in skipped):
                    f.write('\t".globl _%s\\n.set _%s, _of_meta_data + 0x%x\\n"  /* 0x%08x */\n'
                            % (name, name, addr - START, addr))
                    count += 1
        f.write(");\n")
    print("gen_meta_a_data: %d words, %d strings, %d aliases" % (len(words), len(strings), count))


if __name__ == "__main__":
    main()
