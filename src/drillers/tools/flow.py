#!/usr/bin/env python3
"""Disassemble one Drillers function by following its control flow (analysis helper of the
drillers module). The Drillers code is split into chunks far from the entry, so a linear
listing (tools/dis.py) does not show a whole function.

Usage: python3 -I src/drillers/tools/flow.py ADDR [--build DIR]

From ADDR, follows fall-through, conditional and unconditional jumps and jump tables, and stops
at `ret`, at an indirect jump, and at a jump to another function entry (tools/entries.txt,
printed as a tail call). Prints the blocks in address order. Every call shows the last
instruction that wrote ECX (`this` of a __thiscall/__fastcall callee) and the callee's
prototype from <build>/guest-main/gen/of_decls.h.
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ROOT = os.path.abspath(os.path.join(ROOT, ".."))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import xbe_data  # noqa: E402

LO, HI = 0x0010F830, 0x00125070


def listing():
    tmp = os.environ.get("TMPDIR", "/tmp")
    path = os.path.join(tmp, "of_drillers_all.s")
    if not os.path.exists(path):
        out = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py"),
                                       "%x" % LO, "%x" % HI]).decode()
        open(path, "w").write(out)
    ins = {}
    order = []
    for line in open(path):
        a = int(line[:8], 16)
        ins[a] = line[10:].rstrip("\n")
        order.append(a)
    nxt = {a: b for a, b in zip(order, order[1:])}
    return ins, nxt


def protos(build):
    p = os.path.join(ROOT, build, "guest-main", "gen", "of_decls.h")
    out = {}
    if not os.path.exists(p):
        return out
    lines = open(p).read().splitlines()
    for i, line in enumerate(lines):
        m = re.match(r"/\* 0x([0-9a-f]{8}) ", line)
        if m and i + 1 < len(lines):
            out[int(m.group(1), 16)] = lines[i + 1]
    return out


def main():
    start = int(sys.argv[1], 16)
    build = "build-drillers"
    if "--build" in sys.argv:
        build = sys.argv[sys.argv.index("--build") + 1]
    entries = set(int(x, 16) for x in open(os.path.join(HERE, "entries.txt")).read().split())
    ins, nxt = listing()
    d, secs = xbe_data.load()
    at = xbe_data.reader(d, secs)
    pr = protos(build)
    seen = set()
    work = [start]
    tails = set()
    tables = []
    while work:
        a = work.pop()
        while a not in seen:
            if a not in ins:
                print("; !! no instruction at 0x%x (listing out of sync)" % a)
                break
            seen.add(a)
            t = ins[a]
            m = re.match(r"(j\w+)\s+0x([0-9a-f]+)", t)
            if m:
                tgt = int(m.group(2), 16)
                if tgt in entries and tgt != start:
                    tails.add(a)
                elif LO <= tgt < HI:
                    work.append(tgt)
                if m.group(1) == "jmp":
                    break
            mt = re.match(r"jmp\s+dword ptr \[4\*(\w+) \+ 0x([0-9a-f]+)\]", t)
            if mt:
                tab = int(mt.group(2), 16)
                k = 0
                while True:
                    v = struct.unpack("<I", at(tab + 4 * k, 4))[0]
                    if not (LO <= v < HI) or v not in ins or k >= 256:
                        break
                    work.append(v)
                    k += 1
                tables.append((tab, k))
                break
            if t.startswith("ret") or t.startswith("jmp"):
                break
            a = nxt.get(a)
            if a is None:
                break
    tabaddrs = set()
    for tab, k in tables:
        vals = [struct.unpack("<I", at(tab + 4 * j, 4))[0] for j in range(k)]
        print("; jump table 0x%x: %s" % (tab, " ".join("%x" % v for v in vals)))
        tabaddrs.update(range(tab, tab + 4 * k))
    seen = set(a for a in seen if a not in tabaddrs)
    last = "?"
    prev = None
    for a in sorted(seen):
        if prev is not None and nxt.get(prev) != a:
            print("        ; ----")
            last = "?"
        prev = a
        t = ins[a]
        if re.match(r"(mov|lea|pop|xor)\s+ecx\b", t):
            last = "%x %s" % (a, t)
        note = ""
        m = re.match(r"(call|jmp)\s+0x([0-9a-f]+)", t)
        if m and (t.startswith("call") or a in tails):
            tgt = int(m.group(2), 16)
            note = "   ; ecx<- %s | %s" % (last, pr.get(tgt, "(not in table)"))
            if a in tails:
                note = "   ; TAIL CALL" + note
        print("%08x  %s%s" % (a, t, note))
        if t.startswith("call"):
            last = "(clobbered by call)"
        elif re.match(r"\w+\s+[^,]*\becx\b,", t) and not re.match(r"(mov|lea|pop|xor|cmp|test)\s", t):
            last = "(modified: %s)" % t


if __name__ == "__main__":
    main()
