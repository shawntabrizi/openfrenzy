#!/usr/bin/env python3
"""Decompilation coverage per work module (src/include/of_functions.tsv).

A function counts as decompiled when a source file under src/ has a comment
`// 0x<addr>` or `/* 0x<addr>` (docs/ARCHITECTURE.md) followed by a function body.
Functions not decompiled run as generated weak stubs (they are not counted here).
It counts as a stub when that body calls platform_unimplemented /
PLATFORM_UNIMPLEMENTED or contains the word STUB or UNIMPLEMENTED.

Usage: python3 -I scripts/coverage.py [--markdown]
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
ADDR = re.compile(r"(?://|/\*)\s*0x([0-9a-fA-F]{8})\b")
STUB = re.compile(r"platform_unimplemented|PLATFORM_UNIMPLEMENTED|\bSTUB\b|\bUNIMPLEMENTED\b")
EXTS = {".c", ".cc", ".cpp", ".h", ".hpp", ".inc"}


def body_after(text, pos):
    """Text of the first {...} block after pos, or '' if none."""
    start = text.find("{", pos)
    if start < 0:
        return ""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    return text[start:]


def scan(src):
    found = {}  # addr -> "real" | "stub"
    if not src.is_dir():
        return found
    for path in sorted(src.rglob("*")):
        if path.suffix not in EXTS or not path.is_file():
            continue
        text = path.read_text(errors="replace")
        for m in ADDR.finditer(text):
            addr = m.group(1).lower()
            body = body_after(text, m.end())
            if not body:
                continue
            kind = "stub" if STUB.search(body) else "real"
            if found.get(addr) != "real":
                found[addr] = kind
    return found


def main():
    modules = {}
    for line in (ROOT / "src/include/of_functions.tsv").read_text().splitlines():
        if line.startswith("#"):
            continue
        cols = line.split("\t")
        modules.setdefault(cols[1], []).append(cols[0])
    kinds = {}
    for line in (ROOT / "docs/modules.tsv").read_text().splitlines()[1:]:
        cols = line.split("\t")
        kinds[cols[1]] = cols[2]
    found = scan(ROOT / "src")
    rows, tot = [], [0, 0, 0]
    for mid, funcs in modules.items():
        real = sum(found.get(f) == "real" for f in funcs)
        stub = sum(found.get(f) == "stub" for f in funcs)
        rows.append((mid, kinds.get(mid, "?"), len(funcs), real, stub))
        tot[0] += len(funcs)
        tot[1] += real
        tot[2] += stub
    if "--markdown" in sys.argv:
        print("| Module | Kind | Functions | Real code | Stubs | Not started |")
        print("|---|---|---:|---:|---:|---:|")
        for mid, kind, n, real, stub in rows:
            print(f"| `{mid}` | {kind} | {n} | {real} | {stub} | {n - real - stub} |")
        print(f"| **total** | | **{tot[0]}** | **{tot[1]}** | **{tot[2]}** | **{tot[0] - tot[1] - tot[2]}** |")
    else:
        for row in rows:
            print("%-24s %-8s %5d real %5d stub %5d" % row)
        print("total %d real %d stub %d" % tuple(tot))
    print(f"\naddress comments with a body found under src/: {len(found)}", file=sys.stderr)


if __name__ == "__main__":
    main()
