#!/usr/bin/env python3
"""Print export/funcs/<addr>.c with readable names (analysis helper of the mousewhack module).

Usage: python3 -I src/mousewhack/tools/show.py ADDR [ADDR...]   [--decls build-dir/.../of_decls.h]

- FUN_<addr> calls get the name and prototype from the generated of_decls.h (listed once at
  the top of each function).
- _DAT_<addr> constants and 0x3f800000-like hex numbers get their float value as a comment.
"""
import os
import re
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402


def load_decls(path):
    names, protos = {}, {}
    last = None
    for line in open(path):
        m = re.match(r"#define FUN_([0-9a-f]{8}) (\w+)", line)
        if m:
            names[m.group(1)] = m.group(2)
            continue
        m = re.match(r"/\* 0x([0-9a-f]{8}) (\S+) ?(.*)\*/", line)
        if m:
            last = (m.group(1), m.group(2) + " " + m.group(3).strip())
            continue
        if last and line.strip() and not line.startswith("#"):
            protos[last[0]] = (line.strip(), last[1])
            last = None
    return names, protos


def nice_float(bits):
    f = struct.unpack("<f", struct.pack("<I", bits))[0]
    if f != f or f in (float("inf"), float("-inf")):
        return None
    if f == 0 or not (1e-7 < abs(f) < 1e9):
        return None
    return "%.9g" % f


def main():
    args = sys.argv[1:]
    decls = os.path.join(ROOT, "build-mousewhack", "guest-main", "gen", "of_decls.h")
    if "--decls" in args:
        i = args.index("--decls")
        decls = args[i + 1]
        del args[i:i + 2]
    names, protos = load_decls(decls)
    d, secs = xbe_data.load()
    rd = xbe_data.reader(d, secs)

    def dword(a):
        try:
            return struct.unpack("<I", rd(a, 4))[0]
        except Exception:
            return None

    for a in args:
        a = a.lower().replace("0x", "").zfill(8)
        p = os.path.join(ROOT, "export", "funcs", a + ".c")
        if not os.path.exists(p):
            p2 = os.path.join(ROOT, "build-mousewhack", "ghidra_extra", a + ".c")
            p = p2 if os.path.exists(p2) else p
        src = open(p).read()
        called = sorted(set(re.findall(r"FUN_([0-9a-f]{8})", src)))
        print("/* ===== %s  (%s) */" % (a, protos.get(a, ("?", ""))[1]))
        for c in called:
            if c == a:
                continue
            proto, where = protos.get(c, ("?", ""))
            print("//  %s %s : %s   [%s]" % (c, names.get(c, ""), proto, where))

        def fun(m):
            n = names.get(m.group(1))
            return "%s/*%s*/" % (n, m.group(1)) if n else m.group(0)

        def dat(m):
            v = dword(int(m.group(1), 16))
            fl = ("0" if v == 0 else nice_float(v)) if v is not None else None
            if fl == "0":
                try:
                    dv = struct.unpack("<d", rd(int(m.group(1), 16), 8))[0]
                    if 1e-7 < abs(dv) < 1e9:
                        return "%s/*0f or %.9g double*/" % (m.group(0), dv)
                except Exception:
                    pass
            if fl:
                return "%s/*%sf*/" % (m.group(0), fl)
            try:
                dv = struct.unpack("<d", rd(int(m.group(1), 16), 8))[0]
                if 1e-7 < abs(dv) < 1e9:
                    return "%s/*%.9g double*/" % (m.group(0), dv)
            except Exception:
                pass
            return m.group(0)

        def hexf(m):
            v = int(m.group(0), 16)
            if m.string[m.start() - 1] == "-":
                v = (-v) & 0xffffffff
            fl = nice_float(v)
            if fl and (v & 0xffff) == 0 or (fl and v & 0xff000000 in (0x3f000000, 0x3e000000, 0x40000000, 0x41000000, 0x42000000, 0x43000000, 0x44000000, 0x45000000, 0xbf000000, 0xc0000000, 0xc1000000, 0xc2000000, 0xc3000000, 0xc4000000, 0xc5000000)):
                return "%s/*%sf*/" % (m.group(0), fl)
            return m.group(0)

        src = re.sub(r"FUN_([0-9a-f]{8})", fun, src)
        src = re.sub(r"_DAT_([0-9a-f]{8})", dat, src)
        src = re.sub(r"0x[0-9a-f]{8}\b", hexf, src)
        print(src)


if __name__ == "__main__":
    main()
