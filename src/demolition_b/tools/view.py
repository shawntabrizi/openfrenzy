#!/usr/bin/env python3
"""Show one original function for decompilation: Ghidra's C (export/funcs), then the
disassembly with each call's current prototype from the generated of_decls.h
(analysis helper of the demolition_b module).

Usage: python3 -I src/demolition_b/tools/view.py [--noc] [--nodis] ADDR [ADDR ...]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
DECLS = os.path.join(ROOT, "build-demolition_b", "guest-main", "gen", "of_decls.h")


def protos():
    out = {}
    cur = None
    for line in open(DECLS):
        m = re.match(r"/\* 0x([0-9a-f]{8}) ", line)
        if m:
            cur = m.group(1)
            continue
        if cur and line.strip() and not line.startswith("#define"):
            out[cur] = line.strip()
            cur = None
    return out


def main():
    args = sys.argv[1:]
    show_c = "--noc" not in args
    show_dis = "--nodis" not in args
    args = [a for a in args if not a.startswith("--")]
    p = protos()
    for a in args:
        a = a.lower().replace("0x", "").rjust(8, "0")
        print("=" * 20, a, p.get(a, "?"))
        if show_c:
            path = os.path.join(ROOT, "export", "funcs", a + ".c")
            if os.path.exists(path):
                body = open(path).read()
                body = re.sub(r"\n\s*\n", "\n", body)
                print(body)
        if show_dis:
            dis = subprocess.check_output([sys.executable, "-I", os.path.join(HERE, "dis.py"), a]).decode()
            for line in dis.splitlines():
                m = re.search(r"call\s+0x([0-9a-f]+)", line)
                if m:
                    t = m.group(1).rjust(8, "0")
                    line = re.sub(r"call\s+0x[0-9a-f]+ \S+", "call " + p.get(t, t), line)
                print(line)


main()
