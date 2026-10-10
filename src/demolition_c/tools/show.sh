#!/bin/sh
# Ghidra C + resolved globals + ECX before each call, for each address (demolition_c helper).
# Usage: sh src/demolition_c/tools/show.sh ADDR [ADDR ...]
D=$(dirname "$0")
for a in "$@"; do
	python3 -I "$D/consts.py" "$a"
	python3 -I "$D/ecx.py" "$a" | grep -v "^=="
done
