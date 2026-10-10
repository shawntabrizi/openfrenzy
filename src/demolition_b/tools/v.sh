#!/bin/sh
# Show functions: capstone disassembly with callee prototypes (demolition_b helper).
# Usage: src/demolition_b/tools/v.sh ADDR[:LENGTH] ...    (needs build-bumpercars_b/venv for capstone)
here=$(cd "$(dirname "$0")" && pwd)
py="$here/../../../build-bumpercars_b/venv/bin/python"
for a in "$@"; do
	addr=${a%%:*}; len=${a#*:}; [ "$len" = "$a" ] && len=0
	echo "==================== $addr $(grep -A1 "^/\* 0x$addr " "$here/../../../build-demolition_b/guest-main/gen/of_decls.h" | tail -1)"
	"$py" -I "$here/cdis.py" "$addr" "$len" | python3 -I "$here/annotate.py"
done
