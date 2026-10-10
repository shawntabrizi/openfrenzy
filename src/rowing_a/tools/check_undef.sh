#!/bin/sh
# Lists the undefined symbols of the rowing_a objects that no other object of the guest
# build defines (the whole-image link may fail for other modules' reasons; this checks
# only rowing_a's references). Analysis/build helper of rowing_a.
# Usage: sh src/rowing_a/tools/check_undef.sh   (after sh src/rowing_a/tools/build.sh)
ROOT=$(cd "$(dirname "$0")/../../.." && pwd)
G="$ROOT/build-rowing_a/guest-main"
NM=/opt/homebrew/opt/llvm@22/bin/llvm-nm
T=${TMPDIR:-/tmp}/rowing_a_undef.$$
find "$G" -name '*.obj' -path '*/src/rowing_a/*' | xargs $NM -u 2>/dev/null | awk 'NF==2{print $2} NF==1&&$1!~/:$/{print $1}' | sort -u > "$T.u"
{ find "$G" \( -name '*.obj' -o -name '*.o' \) | xargs $NM --defined-only 2>/dev/null;
  find "$G" -name '*.a' | xargs $NM --defined-only 2>/dev/null; } | awk 'NF>=3{print $3}' | sort -u > "$T.d"
comm -23 "$T.u" "$T.d"
rm -f "$T.u" "$T.d"
