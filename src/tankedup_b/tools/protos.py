#!/usr/bin/env python3
"""Print the prototype that the build declares (of_decls.h) for each function address,
following the "#define FUN_x name" renames. Analysis helper of tankedup_b.

Usage: python3 -I src/tankedup_b/tools/protos.py [build dir] ADDR... | --callees ADDR...
  --callees: every function that the given functions call (from export/functions*.jsonl)
"""
import json
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    args = sys.argv[1:]
    build = os.path.join(ROOT, "build-tankedup_b")
    if args and not re.match(r"^(0x)?[0-9a-fA-F]{5,8}$", args[0]) and args[0] != "--callees":
        build = args.pop(0)
    decls = open(os.path.join(build, "guest-main", "gen", "of_decls.h")).read()
    if args and args[0] == "--callees":
        funcs = {}
        for f in ("functions.jsonl", "functions_extra.jsonl"):
            p = os.path.join(ROOT, "export", f)
            if os.path.exists(p):
                for line in open(p):
                    j = json.loads(line)
                    funcs[j["addr"]] = j
        addrs = []
        for a in args[1:]:
            for c in funcs.get(a.lower().zfill(8), {}).get("callees", []):
                if c not in addrs:
                    addrs.append(c)
    else:
        addrs = [a.lower().replace("0x", "").zfill(8) for a in args]
    for a in addrs:
        name = "FUN_" + a
        m = re.search(r"#define %s (\w+)" % name, decls)
        if m:
            name = m.group(1)
        m = re.search(r"^[^#\n]*\b%s\([^;]*\);" % re.escape(name), decls, re.M)
        print("%s  %s" % (a, m.group(0) if m else "(no declaration: %s)" % name))


if __name__ == "__main__":
    main()
