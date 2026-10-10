#!/bin/sh
# Print the Ghidra C of each function, then the calls that set ECX first (x86 `this`)
# and the function's ret instructions (ret N = N bytes of stack arguments).
# Usage: src/tankedup_b/tools/show.sh ADDR [ADDR ...]   (analysis helper of tankedup_b)
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)
for a in "$@"; do
	echo "=================== $a"
	grep -v "^$" "$root/export/funcs/$a.c" | grep -v WARNING
	echo "--- ecx / ret"
	python3 -I "$here/ecx.py" "$a" | grep -v "clobbered by call\|ecx<- ?$" | grep -v "^==" | sed 's/  */ /g'
	python3 -I "$here/dis.py" "$a" | grep -E "\sret" | sort -u -k2
done
