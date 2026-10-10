#!/usr/bin/env python3
"""Print data from orig/default.xbe as C, for decompiled code that needs the original tables.

Usage: python3 -I scripts/xbe_data.py <command> <addr> [count]
  vtable  ADDR [N]   function-pointer table: one FUN_<addr> per slot, until a dword is not a
                     function start (or N slots). Uses src/include/of_functions.tsv for names.
  dwords  ADDR N     N 32-bit words (hex)
  floats  ADDR N     N 32-bit floats
  bytes   ADDR N     N bytes (hex)
  string  ADDR       a NUL-terminated string
  section ADDR       which XBE section holds ADDR

Addresses are XBE virtual addresses (hex, with or without 0x). Pointers inside the data are
original addresses: never copy them into the port as numbers; turn them into symbols.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.join(HERE, "..")


def load():
    d = open(os.path.join(ROOT, "orig", "default.xbe"), "rb").read()
    base, = struct.unpack_from("<I", d, 0x104)
    nsec, secaddr = struct.unpack_from("<II", d, 0x11C)
    secs = []
    for i in range(nsec):
        _, va, vs, ra, rs, na = struct.unpack_from("<IIIIII", d, secaddr - base + i * 56)
        name = d[na - base:d.index(b"\0", na - base)].decode()
        secs.append((name, va, vs, ra, rs))
    return d, secs


def reader(d, secs):
    def at(va, size):
        for name, v, vs, ra, rs in secs:
            if v <= va and va + size <= v + vs:
                off = va - v
                if off + size <= rs:
                    return d[ra + off:ra + off + size]
                return bytes(size)  # zero-filled (bss part of the section)
        raise SystemExit("0x%08x is not in any section" % va)
    return at


def function_names():
    names = {}
    path = os.path.join(ROOT, "src", "include", "of_functions.tsv")
    for line in open(path):
        if line.startswith("#"):
            continue
        cols = line.rstrip("\n").split("\t")
        names[int(cols[0], 16)] = (cols[1], cols[2] if cols[2] != "-" else cols[3])
    return names


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    cmd, addr = sys.argv[1], int(sys.argv[2], 16)
    count = int(sys.argv[3]) if len(sys.argv) > 3 else None
    d, secs = load()
    at = reader(d, secs)
    if cmd == "section":
        for name, v, vs, ra, rs in secs:
            if v <= addr < v + vs:
                print("%s [0x%08x, 0x%08x)" % (name, v, v + vs))
        return 0
    if cmd == "vtable":
        names = function_names()
        print("/* 0x%08x vtable */" % addr)
        print("static const void *const vtable_%08x[] = {" % addr)
        slot = 0
        while count is None or slot < count:
            v, = struct.unpack("<I", at(addr + slot * 4, 4))
            if v not in names:
                if count is None:
                    break
                print("\t(const void *)0, /* [%d] 0x%08x: not a game/engine function */" % (slot, v))
            else:
                module, where = names[v]
                print("\t(const void *)FUN_%08x, /* [%d] +0x%02x %s %s */" % (v, slot, slot * 4, module, where))
            slot += 1
        print("};")
        return 0
    if cmd == "string":
        out = b""
        while True:
            c = at(addr + len(out), 1)
            if c == b"\0":
                break
            out += c
        print('/* 0x%08x */ "%s"' % (addr, out.decode("latin-1").encode("unicode_escape").decode()))
        return 0
    n = count or 1
    if cmd == "dwords":
        vals = struct.unpack("<%dI" % n, at(addr, 4 * n))
        print("/* 0x%08x */ { %s }" % (addr, ", ".join("0x%08x" % v for v in vals)))
    elif cmd == "floats":
        vals = struct.unpack("<%df" % n, at(addr, 4 * n))
        print("/* 0x%08x */ { %s }" % (addr, ", ".join("%.9gf" % v for v in vals)))
    elif cmd == "bytes":
        print("/* 0x%08x */ { %s }" % (addr, ", ".join("0x%02x" % b for b in at(addr, n))))
    else:
        print(__doc__)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
