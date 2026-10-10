#!/bin/sh
# engine_misc tests: the BMATHS collision tests (BMATHS/unplaced.c) compiled natively for the
# host with a shim for the few bKernel/other-module vector helpers they call, then run.
#
#   src/engine_misc/tests/run_tests.sh
#
# The test sources end in .c.in so the guest build (which globs src/<module>/**/*.c) does not
# compile them into the game image. Exit status 0 when every check passed.
set -eu

ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
OUT="${TMPDIR:-/tmp}/engine_misc_tests"
mkdir -p "$OUT"
cc -std=gnu11 -O1 -Wall -Wno-unused-function -x c \
	-include "$ROOT/src/engine_misc/tests/engine_misc_test_shim.h" \
	-I "$ROOT/src/engine_misc" \
	"$ROOT/src/engine_misc/BMATHS/unplaced.c" \
	"$ROOT/src/engine_misc/tests/maths_test.c.in" \
	-o "$OUT/maths_test" -lm
"$OUT/maths_test"
