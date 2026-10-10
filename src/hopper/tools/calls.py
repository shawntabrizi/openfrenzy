#!/usr/bin/env python3
"""For each call in an original function, print the callee, its prototype in the generated
of_decls.h, the last write to ECX before it (the x86 `this` of __thiscall callees), the
pushes since the previous call and the stack cleanup after it (copied from
src/foxhunt/tools/calls.py; uses src/hopper/tools/dis.py for the disassembly).

Usage: python3 -I src/hopper/tools/calls.py ADDR[:END] [ADDR ...] [--build DIR]
"""
import os
import re
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
DIS = os.path.join(ROOT, "src", "hopper", "tools", "dis.py")


def load_decls(build):
    decls = {}
    lines = open(os.path.join(build, "guest-main", "gen", "of_decls.h")).read().split("\n")
    for i, line in enumerate(lines):
        m = re.match(r"/\* 0x([0-9a-f]{8}) ", line)
        if m:
            decls[int(m.group(1), 16)] = lines[i + 1].strip()
    return decls


def main():
    args = sys.argv[1:]
    build = os.path.join(ROOT, "build-hopper")
    if "--build" in args:
        i = args.index("--build")
        build = args[i + 1]
        del args[i:i + 2]
    decls = load_decls(build)
    for a in args:
        out = subprocess.check_output([sys.executable, "-I", DIS] + a.split(":")).decode().splitlines()
        print("==", a)
        ecx = "?"
        pushes = []
        for n, line in enumerate(out):
            addr, _, ins = line.partition("  ")
            if re.match(r"(mov|lea|pop|xor)\s+ecx\b", ins):
                ecx = ins
            if ins.startswith("push"):
                pushes.append(ins[4:].strip())
            if ins.startswith("call") or (ins.startswith("jmp") and "FUN_" in ins):
                m = re.search(r"0x([0-9a-f]+)", ins)
                proto = decls.get(int(m.group(1), 16), "") if m else ""
                clean = ""
                if n + 1 < len(out):
                    nxt = out[n + 1].partition("  ")[2]
                    if nxt.startswith("add\tesp"):
                        clean = nxt
                print("  %s %s\n      proto: %s\n      ecx: %s | pushes(last first): %s | %s"
                      % (addr, ins, proto, ecx, list(reversed(pushes[-8:])), clean))
                pushes = []
                ecx = "(clobbered)"
            elif re.match(r"\w+\s+ecx\b", ins) and not re.match(r"(mov|lea|pop|xor|cmp|test|push)\s", ins):
                ecx = "(modified: %s)" % ins


if __name__ == "__main__":
    main()
