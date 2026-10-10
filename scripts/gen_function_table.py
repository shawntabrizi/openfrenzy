#!/usr/bin/env python3
"""Write src/include/of_functions.tsv: one row per game/engine .text function to decompile.

Run:  python3 -I scripts/gen_function_table.py   (after map_sources.py and modules.py)
In:   export/functions.jsonl, export/funcs/<addr>.c, export/source_map.json, export/modules.json,
      orig/default.xbe (static-constructor order)
Out:  src/include/of_functions.tsv (committed; the build reads it, so a build needs no export/)

Columns (tab-separated):
  addr       8 hex digits
  module     work module (export/modules.json); the module that owns the function
  file       original source file (relative to the game or engine root), or "-" when unknown
  dir        resolved original directory (or LIB:<region>)
  confidence tagged | bracketed | vtable | gap | leading | trailing | region
  size       bytes
  crt        index in the static-constructor table (.CRT$XC*) run by _cinit, or "-"
  ghidra     Ghidra's name
  proto      prototype with Ghidra type names, symbol FUN_<addr>; calling conventions dropped

Only functions of kind game/engine are listed. Library regions (xapi, libc, bink) are replaced
by the platform layer and are not listed; their callers use the standard C / XDK names.

Type normalisation (see src/include/of_game.h for the typedefs):
  pointers to unknown or library types -> void *   (FILE, __func, struct names)
  float10 (x87 return) -> float  (most game functions return float; check each one)
  unkbyte10 -> double, bool -> undefined1, code * -> code *
"""
import json
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")
EXPORT = os.path.join(ROOT, "export")
OUT = os.path.join(ROOT, "src", "include", "of_functions.tsv")

KNOWN = {"void", "int", "uint", "short", "ushort", "char", "byte", "float", "double", "long",
         "longlong", "ulonglong", "undefined", "undefined1", "undefined2", "undefined4",
         "undefined8", "dword", "size_t", "wchar_t", "code"}
RENAME = {"float10": "float", "unkbyte10": "double", "bool": "undefined1", "long": "int",
          "size_t": "uint", "wchar_t": "ushort", "dword": "uint"}


def norm_type(t):
    t = " ".join(t.replace("*", " * ").split()).replace(" *", "*")
    stars = t.count("*")
    base = t.replace("*", "").replace("const", "").strip()
    base = RENAME.get(base, base)
    if base not in KNOWN:
        return "void" + "*" * max(stars, 1) if stars else "undefined4"
    if base == "code" and not stars:
        return "undefined4"
    return base + "*" * stars


def parse_signature(path):
    text = open(path).read()
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    sig = " ".join(text[:text.find("{")].split())
    m = re.match(r"(.*?)\s*\b([\w:~]+)\s*\((.*)\)$", sig)
    if not m:
        return None
    ret, _, params = m.groups()
    ret = re.sub(r"\b__(thiscall|fastcall|stdcall|cdecl)\b", "", ret).strip()
    out = []
    if params.strip() not in ("void", ""):
        for p in params.split(","):
            p = p.strip()
            if p == "...":
                out.append("...")
                continue
            m2 = re.match(r"(.*?)(\w+)$", p)
            ptype = m2.group(1) if m2 and m2.group(1).strip() else p
            out.append(norm_type(ptype))
    return norm_type(ret) if ret else "void", out


# _cinit (0x340f8) runs the .CRT$XC* table [0x281750, 0x2818a0) in order, after the
# .CRT$XI table (LIBC only). Entries in other sections (D3D, XPP) are library constructors.
CRT_XC_TABLE = (0x281750, 0x2818a0)


def crt_order(xbe_path):
    """{addr: index} of the game's static constructors in _cinit order."""
    d = open(xbe_path, "rb").read()
    base, = struct.unpack_from("<I", d, 0x104)
    nsec, secaddr = struct.unpack_from("<II", d, 0x11C)
    secs = []
    for i in range(nsec):
        _, va, vs, ra, rs, na = struct.unpack_from("<IIIIII", d, secaddr - base + i * 56)
        secs.append((va, ra, rs))
    order = {}
    for addr in range(CRT_XC_TABLE[0], CRT_XC_TABLE[1], 4):
        for va, ra, rs in secs:
            if va <= addr < va + rs:
                v, = struct.unpack_from("<I", d, ra + addr - va)
                if v and v != 0xFFFFFFFF:
                    order.setdefault("%08x" % v, len(order))
    return order


def main():
    smap = json.load(open(os.path.join(EXPORT, "source_map.json")))["functions"]
    modules = json.load(open(os.path.join(EXPORT, "modules.json")))
    resolved = modules.pop("_resolved")
    meta = {}
    for name in ("functions.jsonl", "functions_extra.jsonl"):
        if os.path.exists(os.path.join(EXPORT, name)):
            for line in open(os.path.join(EXPORT, name)):
                f = json.loads(line)
                meta.setdefault(f["addr"], f)
    rows = []
    crt_addrs = [a for a, e in smap.items() if e["region"] == "crt_init"]
    order = crt_order(os.path.join(ROOT, "orig", "default.xbe"))
    unparsed = 0
    for mid, rec in modules.items():
        if rec["kind"] == "platform":
            continue
        for a in rec["functions"]:
            e = smap[a]
            sig = parse_signature(os.path.join(EXPORT, "funcs", a + ".c"))
            if sig is None:
                unparsed += 1
                sig = ("void", [])
            ret, params = sig
            proto = "%s FUN_%s(%s)" % (ret, a, ", ".join(params) or "void")
            rows.append((a, mid, e.get("file") or "-", resolved[a], e["confidence"], str(e["size"]),
                         str(order[a]) if a in order else "-", meta[a]["name"], proto))
    rows.sort()
    with open(OUT, "w") as fh:
        fh.write("# generated by scripts/gen_function_table.py; do not edit (see docs/ARCHITECTURE.md)\n")
        fh.write("#addr\tmodule\tfile\tdir\tconfidence\tsize\tcrt\tghidra\tproto\n")
        for r in rows:
            fh.write("\t".join(r) + "\n")
    in_table = sum(1 for a in crt_addrs if a in order)
    print("%d functions -> %s (%d signatures not parsed; %d of %d crt_init functions in the XC table)"
          % (len(rows), os.path.relpath(OUT, ROOT), unparsed, in_table, len(crt_addrs)))


if __name__ == "__main__":
    sys.exit(main())
