#!/bin/sh
# Build the charlib snapshot tree and show only diagnostics from charlib files, then run
# ctest. build-charlib-tree is a private git worktree (detached at master) with src/charlib
# copied in from the main tree, so other modules' uncommitted work does not break this
# build. When a committed file of another module does not compile, it is set aside in the
# private tree only (renamed *.c.off; its functions fall back to the weak stubs) and named
# in the output.
#
# Usage: src/charlib/tools/build.sh [max lines]   (src/charlib/tools/build.sh update: move
# the private tree to the current master first)
R=$(cd "$(dirname "$0")/../../.." && pwd)
T=$R/build-charlib-tree
if [ "$1" = "update" ]; then
	rm -rf "$T/src/charlib"
	git -C "$T" checkout -q -f --detach master
	git -C "$T" clean -q -f src
	shift
fi
rsync -a --delete --exclude tools/__pycache__ "$R/src/charlib/" "$T/src/charlib/"
for pass in 1 2 3; do
	cmake --build "$T/build-charlib/guest-main" -- -k 0 > "$T/build.log" 2>&1
	broken=$(grep -E "^FAILED: .*\.c\.obj" "$T/build.log" | grep -v "/src/charlib/" |
		sed -E 's#.*/build-charlib-tree/(src/[^ ]*\.c)\.obj.*#\1#' | sort -u)
	[ -z "$broken" ] && break
	for f in $broken; do
		echo "set aside (does not compile at master): $f"
		mv "$T/$f" "$T/$f.off"
	done
	cmake -S "$T" -B "$T/build-charlib" > /dev/null 2>&1
done
cmake --build "$T/build-charlib" > "$T/build-top.log" 2>&1
status=$?
cat "$T/build-top.log" >> "$T/build.log"
grep -E "(charlib/[^ ]*:[0-9]+:[0-9]+: (warning|error))|ld.lld|error:" "$T/build.log" |
	grep -v "Wignored-attributes\|platform_msvc_semantics" | sort -u | head -${1:-40}
echo "build status: $status"
ctest --test-dir "$T/build-charlib" 2>&1 | tail -3
