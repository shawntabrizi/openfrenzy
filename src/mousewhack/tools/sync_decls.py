#!/usr/bin/env python3
"""Rewrite the section of src/mousewhack/mousewhack.decls for one C file from its definitions
(analysis helper of the mousewhack module).

Usage: python3 -I src/mousewhack/tools/sync_decls.py src/mousewhack/MouseWhack/<File>.c
The section is "# ---- MouseWhack/<File>.c"; it is created before the provisional section when
missing. Lines for the same addresses elsewhere (provisional shapes) are removed.
"""
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DECLS = os.path.abspath(os.path.join(HERE, "..", "mousewhack.decls"))


def main():
    path = sys.argv[1]
    rel = "MouseWhack/" + os.path.basename(path)
    out = subprocess.run([sys.executable, "-I", os.path.join(HERE, "decls_from_src.py"), path],
                         capture_output=True, text=True, check=True).stdout
    new = [l for l in out.splitlines() if l.strip()]
    bad = [l for l in new if l.startswith("#?")]
    if bad:
        print("not mousewhack functions (left out):\n" + "\n".join(bad))
    new = [l for l in new if not l.startswith("#?")]
    addrs = {l.split()[0] for l in new}
    lines = open(DECLS).read().split("\n")
    header = "# ---- " + rel
    # drop the old section and every line of these addresses
    res, skip = [], False
    for l in lines:
        if l.startswith("# ---- "):
            skip = l == header
            if skip:
                continue
        if skip:
            continue
        if l[:8] in addrs and len(l) > 8 and l[8] == " ":
            continue
        res.append(l)
    # insert before the provisional section
    at = next((i for i, l in enumerate(res) if l.startswith("# ---- provisional")), len(res))
    block = [header] + new + [""]
    res[at:at] = block
    text = "\n".join(res)
    while "\n\n\n" in text:
        text = text.replace("\n\n\n", "\n\n")
    open(DECLS, "w").write(text)
    print("%d lines in %s" % (len(new), header))


if __name__ == "__main__":
    main()
