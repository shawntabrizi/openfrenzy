#!/bin/sh
# Compile only the demolition_d objects of a configured build (analysis helper of demolition_d).
# Usage: sh src/demolition_d/tools/build_mine.sh [BUILD_DIR]   (default build-demolition_d)
B=${1:-build-demolition_d}
cd "$B/guest-main" || exit 1
cmake . > /dev/null || exit 1
T=$(ninja -t targets all 2>/dev/null | grep '/src/demolition_d/.*\.obj:' | cut -d: -f1)
ninja gen/of_decls.h > /dev/null 2>&1
ninja -k 0 $T 2>&1 | grep -E "error|warning: (incompatible|implicit)|FAILED" | grep -v "platform_msvc_semantics" 
echo "built: $(echo $T | wc -w) objects"
