#!/usr/bin/env python3
"""Copy a C file for the web build, with its inline assembly made WebAssembly's.

    python3 -I port/web/tools/wasm_inline_asm.py IN.c OUT.c

The game's data files give Ghidra's global names as symbols inside data blocks with
file-scope assembly (".globl _DAT_x\\n.set _DAT_x, _block + 0x10"). That assembly is written
for the native build, where llc emits Mach-O assembly and port/macos/tools/asm_convert.py turns
it into ELF: one leading underscore of each symbol is dropped (Mach-O's C prefix) and
".weak_definition" becomes ".weak". WebAssembly object files have no C prefix and take ELF's
directives, so this applies the same two rules to the string literals inside each __asm__(...)
or asm(...): every identifier that starts with "_" loses one "_", and the Mach-O directives get
their ELF names. The rest of the file is copied as it is (line numbers stay the same).
"""

import re
import sys

DIRECTIVES = {
    ".weak_definition": ".weak",
    ".weak_reference": ".weak",
    ".weak_def_can_be_hidden": ".weak",
    ".private_extern": ".hidden",
}
ASM_START = re.compile(r"\b(?:__asm__|asm)\s*\(")
IDENTIFIER = re.compile(r"(?<![\w$.\\])_[\w$]*")
STRING = re.compile(r'"(?:[^"\\\n]|\\.)*"')


def convert_literal(text: str) -> str:
    """text: the inside of one string literal"""
    for macho, elf in DIRECTIVES.items():
        text = text.replace(macho + " ", elf + " ")
    return IDENTIFIER.sub(lambda m: m.group(0)[1:], text)


def asm_end(source: str, start: int) -> int:
    """the index just past the parenthesis that closes the one opened before start"""
    depth = 1
    index = start
    while index < len(source):
        char = source[index]
        if char == '"':
            match = STRING.match(source, index)
            if not match:
                raise SystemExit("wasm_inline_asm: unterminated string literal")
            index = match.end()
            continue
        if source.startswith("/*", index):
            index = source.index("*/", index) + 2
            continue
        if source.startswith("//", index):
            index = source.index("\n", index)
            continue
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return index + 1
        index += 1
    raise SystemExit("wasm_inline_asm: unbalanced parentheses in __asm__")


def convert(source: str) -> str:
    out = []
    position = 0
    for match in ASM_START.finditer(source):
        if match.start() < position:
            continue
        end = asm_end(source, match.end())
        out.append(source[position:match.end()])
        body = source[match.end():end]
        out.append(STRING.sub(lambda m: '"' + convert_literal(m.group(0)[1:-1]) + '"', body))
        position = end
    out.append(source[position:])
    return "".join(out)


def main() -> int:
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    with open(sys.argv[1], encoding="utf-8", errors="surrogateescape") as f:
        source = f.read()
    converted = convert(source)
    with open(sys.argv[2], "w", encoding="utf-8", errors="surrogateescape") as f:
        f.write(f'#line 1 "{sys.argv[1]}"\n' + converted)
    return 0


if __name__ == "__main__":
    sys.exit(main())
