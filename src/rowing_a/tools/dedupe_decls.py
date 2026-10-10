#!/usr/bin/env python3
"""Keeps the last line for each address in src/rowing_a/rowing_a.decls (later lines are the
checked shapes; earlier ones were provisional). Analysis/build helper of rowing_a."""
import os, re
p = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "rowing_a.decls")
lines = open(p).read().split("\n")
last = {}
for i, l in enumerate(lines):
    m = re.match(r"([0-9a-f]{8}) ", l)
    if m:
        last[m.group(1)] = i
out = [l for i, l in enumerate(lines) if not (re.match(r"[0-9a-f]{8} ", l) and last[l[:8]] != i)]
open(p, "w").write("\n".join(out))
