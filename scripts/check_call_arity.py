#!/usr/bin/env python3
"""Find calls through a cast of FUN_<addr> whose parameter count differs from the owner's
prototype in src/<module>/<module>.decls.

Modules call other modules' functions through casts, e.g.
  ((void (*)(void *, int))FUN_0018e540)(sfx, sample)    or    X_CALL(FUN_001cdc30, void (*)(void *, void *))(a, b)
When the owner wrote the function without the thiscall `this` (or with a different
signature), the arguments shift: the callee reads the wrong registers. Integration round 2
found three faults of this kind (Fireworks 0x0007b970, TankedUp 0x001cdc30).

Not every hit is a bug: extra trailing arguments are ignored by the callee, and a cast to an
unprototyped `int (*)()` (callback tables) reports 0 parameters. A hit with FEWER parameters
than the prototype, or one where the caller passes `this` first and the owner has no `this`,
needs a fix. Check the original call site (push order, ECX) before you change either side.

Usage: python3 -I scripts/check_call_arity.py      (run from the repository root)
"""
import glob
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))


def count(args):
    args = args.strip()
    if args in ("", "void"):
        return 0
    depth, n = 0, 1
    for c in args:
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif c == "," and depth == 0:
            n += 1
    return n


def prototypes():
    decl = {}
    for path in glob.glob(os.path.join(ROOT, "src", "*", "*.decls")):
        for line in open(path):
            m = re.match(r"([0-9a-f]{8})\s+(.*?)\s*;?\s*$", line)
            if not m or "(" not in m.group(2):
                continue
            sig = m.group(2)
            decl[int(m.group(1), 16)] = (count(sig[sig.find("(") + 1:sig.rfind(")")]), sig)
    return decl


ARGS = r"\(([^()]*(?:\([^()]*\)[^()]*)*)\)"
MACRO = re.compile(r"FUN_([0-9a-f]{8}),\s*[\w\s\*]+\(\s*\*\s*\)\s*" + ARGS)
CAST = re.compile(r"\(\s*[\w\s\*]+\(\s*\*\s*\)\s*" + ARGS + r"\s*\)\s*&?FUN_([0-9a-f]{8})")


def main():
    decl = prototypes()
    hits = set()
    for path in glob.glob(os.path.join(ROOT, "src", "**", "*.[ch]"), recursive=True):
        if os.sep + "tests" + os.sep in path:
            continue
        text = open(path, errors="replace").read()
        found = [(m.group(1), m.group(2)) for m in MACRO.finditer(text)]
        found += [(m.group(2), m.group(1)) for m in CAST.finditer(text)]
        for addr, args in found:
            a = int(addr, 16)
            if a in decl and "..." not in decl[a][1] and decl[a][0] != count(args):
                kind = "FEWER" if count(args) < decl[a][0] else "more"
                hits.add((os.path.relpath(path, ROOT), a, count(args), decl[a][0], kind, decl[a][1]))
    for path, a, n, want, kind, sig in sorted(hits):
        print(f"{kind:5} {path}: 0x{a:08x} cast with {n} parameters, owner has {want}: {sig}")
    print(f"{len(hits)} mismatches", file=sys.stderr)
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
