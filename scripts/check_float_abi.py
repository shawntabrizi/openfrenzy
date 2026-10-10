#!/usr/bin/env python3
"""Find calls whose float arguments do not match the callee's float parameters (arm64 ABI).

On the Xbox every argument went on the stack, so a float passed where the callee reads a
dword (or the reverse) still carried the right bits. On arm64 a float goes in a float
register (s0..s7) and a dword in a general register (w0..w7): when the two sides differ,
the callee reads a register that the caller never set. Integration round 2 found eight
such calls (baUpdateExtras frame time, bdDrawPointSprites point size, CLFireworks sizes,
Shadows_Init, CFire_LaunchRack_Init) and one extras update callback (classlib_fx
0x00085a40) that took `float dt` while baUpdateExtras passes `void *arg`.

The check reads the LLVM IR (before the rebase pass) that guest_cc.py leaves next to every
object (<build>/guest-main/CMakeFiles/openfrenzy_guest.dir/**/<file>.c.ll), so it sees
exactly what clang emitted after macros, casts and __builtin_choose_expr:

1. every direct call `call ... @f(args)` whose argument classes (float / not float)
   differ from f's declaration or definition in the same module (variadic calls skipped);
2. every update/draw callback given to baCreateExtraSystem that has a float parameter.

Not covered: calls through vtables or other function-pointer tables (the callee is not
known in the IR), and the platform modules (runtime, d3d8, dsound, xkernel, vfs, input).

Usage: python3 -I scripts/check_float_abi.py <build dir>     (after a build)
Exit status 1 when anything is found.
"""
import glob
import os
import re
import sys

SKIP = ("/src/runtime/", "/src/d3d8/", "/src/dsound/", "/src/xkernel/", "/src/vfs/", "/src/input/")
DEF = re.compile(r"^(?:define|declare)\b[^@]*?\s(\S+)\s+@\"?([\w.$]+)\"?\((.*)\)[^)]*$")
CALL = re.compile(r"\bcall\s+(?:[\w]+\s+)*?(\S+?)\s+(?:\(([^)]*)\)\s+)?@\"?([\w.$]+)\"?\((.*)\)")
EXTRAS = re.compile(r"call [^@]*@baCreateExtraSystem\((.*)\)")
FLOAT_TYPES = ("float", "double", "x86_fp80", "fp128")


def split(args):
    out, depth, cur = [], 0, ""
    for c in args:
        if c in "([{<":
            depth += 1
        elif c in ")]}>":
            depth -= 1
        if c == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += c
    if cur.strip():
        out.append(cur.strip())
    return out


def kind(t):
    word = t.split()[0] if t.strip() else ""
    if word == "...":
        return "..."
    return "float" if word in FLOAT_TYPES else "int"


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    root = os.path.join(sys.argv[1], "guest-main", "CMakeFiles", "openfrenzy_guest.dir")
    files = [f for f in glob.glob(os.path.join(root, "**", "*.c.ll"), recursive=True)
             if "/src/" in f and not any(s in f for s in SKIP)]
    if not files:
        sys.exit(f"no IR under {root}: build first")
    hits, defined = set(), {}
    for path in files:
        src = path.split("/src/", 1)[1][:-3]
        lines = open(path, errors="replace").read().splitlines()
        sig = {}
        for line in lines:
            m = DEF.match(line)
            if m:
                ret, name, args = m.groups()
                sig[name] = ("void" if ret == "void" else kind(ret), [kind(a) for a in split(args)])
                if line.startswith("define"):
                    defined[name] = (src, args)
        fn = "?"
        for line in lines:
            if line.startswith("define"):
                m = DEF.match(line)
                fn = m.group(2) if m else "?"
            m = CALL.search(line)
            if not m or m.group(3).startswith("llvm.") or m.group(3) not in sig:
                continue
            ret, fty, name, args = m.groups()
            if fty and "..." in fty:
                continue
            dret, dparams = sig[name]
            problems = []
            cret = "void" if ret == "void" else kind(ret)
            if "void" not in (cret, dret) and cret != dret:
                problems.append(f"return {cret} -> {dret}")
            for i, (a, p) in enumerate(zip([kind(x) for x in split(args)], dparams)):
                if p == "...":
                    break
                if a != p:
                    problems.append(f"arg{i} {a} -> {p}")
            if problems:
                hits.add(f"{src}: in {fn}: call {name}: {'; '.join(problems)}")
    for path in files:
        src = path.split("/src/", 1)[1][:-3]
        for m in EXTRAS.finditer(open(path, errors="replace").read()):
            for role, arg in zip(("update", "draw"), (split(m.group(1))[2:3] + split(m.group(1))[4:5])):
                name = re.search(r"@\"?([\w.$]+)", arg)
                if name and name.group(1) in defined and re.search(r"\b(float|double)\b",
                                                                    defined[name.group(1)][1]):
                    hits.add(f"{src}: extras {role} callback {name.group(1)} has a float parameter "
                             f"(baUpdateExtras/baDrawExtras pass void *arg)")
    for h in sorted(hits):
        print(h)
    print(f"{len(hits)} hit(s) in {len(files)} game source files", file=sys.stderr)
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
