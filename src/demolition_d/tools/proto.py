#!/usr/bin/env python3
"""Print the current prototype of functions (by address) from a build's generated of_decls.h
(analysis helper of the demolition_d module).

Usage: python3 -I src/demolition_d/tools/proto.py BUILD_DIR ADDR [ADDR ...]
"""
import re
import sys


def main():
    text = open(sys.argv[1] + "/guest-main/gen/of_decls.h").read()
    for a in sys.argv[2:]:
        a = a.lower().replace("0x", "").zfill(8)
        name = "FUN_" + a
        m = re.search(r"#define %s (\w+)" % name, text)
        if m:
            name = m.group(1)
        protos = [l for l in text.splitlines() if re.search(r"\b%s\(" % name, l) and not l.startswith("#")]
        print("%s %s" % (a, protos[0] if protos else "(no prototype) " + name))


if __name__ == "__main__":
    main()
