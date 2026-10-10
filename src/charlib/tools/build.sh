#!/bin/sh
# Build the charlib snapshot tree (build-charlib-tree: a git worktree of master, with
# src/charlib copied in from the main tree, so other modules' unfinished work does not
# break this build) and show only diagnostics from charlib files, then run ctest.
T=/Users/shawntabrizi/Documents/GitHub/openfrenzy/build-charlib-tree
rsync -a --delete --exclude tools/__pycache__ /Users/shawntabrizi/Documents/GitHub/openfrenzy/src/charlib/ "$T/src/charlib/"
cmake --build "$T/build-charlib" > "$T/build.log" 2>&1
status=$?
grep -E "(charlib/[^ ]*:[0-9]+:[0-9]+: (warning|error))|ld.lld|error:|FAILED" "$T/build.log" | grep -v "Wignored-attributes\|platform_msvc_semantics" | head -${1:-40}
echo "build status: $status"
ctest --test-dir "$T/build-charlib" 2>&1 | tail -3
