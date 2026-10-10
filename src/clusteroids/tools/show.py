#!/usr/bin/env python3
"""Print, for each function: Ghidra's C (export/funcs/<addr>.c), the data it references
(section and XBE value), the ECX value before each call (x86 `this`), and the current
prototype of each callee from a generated of_decls.h (analysis helper of clusteroids).

Based on src/bumpercars_a/tools/show.py and src/carryon_a/tools/ecx.py.

Usage: python3 -I src/clusteroids/tools/show.py [--decls PATH] ADDR [ADDR ...]
PATH defaults to build-clusteroids/guest-main/gen/of_decls.h.
"""
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

DIS = os.path.join(ROOT, "src", "carryon_a", "tools", "dis.py")


def load_decls(path):
    protos = {}
    if not os.path.exists(path):
        return protos
    lines = open(path).read().splitlines()
    for i, l in enumerate(lines):
        m = re.match(r"/\* 0x([0-9a-f]{8}) (\S+)", l)
        if m and i + 1 < len(lines):
            protos[int(m.group(1), 16)] = (m.group(2), lines[i + 1])
    return protos


def ecx_lines(addr):
    try:
        out = subprocess.check_output([sys.executable, "-I", DIS, "%x" % addr],
                                      stderr=subprocess.DEVNULL).decode()
    except Exception:
        return []
    res = []
    last = "?"
    for line in out.splitlines():
        a, _, ins = line.partition("  ")
        if re.match(r"(mov|lea|pop|xor)\s+ecx\b", ins):
            last = ins
        if ins.startswith("call"):
            res.append("%s %s  ecx<- %s" % (a, ins, last))
            last = "(clobbered)"
        elif re.match(r"\w+\s+ecx\b", ins) and not re.match(r"(mov|lea|pop|xor|cmp|test|push)\s", ins):
            last = "(modified: %s)" % ins
    return res


def main():
    args = sys.argv[1:]
    decls = os.path.join(ROOT, "build-clusteroids", "guest-main", "gen", "of_decls.h")
    if args and args[0] == "--decls":
        decls = args[1]
        args = args[2:]
    protos = load_decls(decls)
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    for a in args:
        a = int(a, 16)
        path = os.path.join(ROOT, "export", "funcs", "%08x.c" % a)
        text = open(path).read()
        print("=" * 20, "%08x" % a)
        print(text.strip())
        seen = set()
        for m in re.finditer(r"(?:_?DAT|PTR_[A-Za-z0-9_]*?|PTR|s_[A-Za-z0-9_]*?)_([0-9a-f]{8})", text):
            v = int(m.group(1), 16)
            if v in seen:
                continue
            seen.add(v)
            sec = next((s for s in secs if s[1] <= v < s[1] + s[2]), None)
            name = sec[0] if sec else "?"
            if sec and v - sec[1] + 4 <= sec[4]:
                raw = at(v, 4)
                w, = struct.unpack("<I", raw)
                f, = struct.unpack("<f", raw)
                info = "dword 0x%08x float %g" % (w, f)
            else:
                info = "bss"
            print("  // %s %s %s" % (m.group(0), name, info))
        for c in sorted(set(int(x, 16) for x in re.findall(r"FUN_([0-9a-f]{8})", text)) - {a}):
            mod, p = protos.get(c, ("?", "?"))
            print("  // callee %08x [%s] %s" % (c, mod, p))
        for l in ecx_lines(a):
            if "ecx<- ?" in l or "clobbered" in l:
                continue
            print("  // " + l)


if __name__ == "__main__":
    main()
