#!/bin/sh
# Disassemble an address range with the callee prototypes of of_decls.h (demolition_b helper).
# Usage: src/demolition_b/tools/dis2.sh START END
here=$(cd "$(dirname "$0")" && pwd)
python3 -I "$here/dis.py" "$1" "$2" | grep -v "nop$" | python3 -I "$here/annotate.py"
