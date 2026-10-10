#!/usr/bin/env python3
# Adapted from ref/OpenCE-mac-zimm3rmann (github.com/zimm3rmann/halo-ce-ios-macos @ e55684a) tools/macos_guest_cc.py (CC0).
# Changed: one call does the whole OpenCE guest pipeline (their android_guest_cc ninja rule plus
# macos_guest_cc.py), so that CMake can use this script as the guest C/C++/ASM compiler.
"""Guest compiler driver for the rebased ILP32 macOS guest (LLVM 22).

CMake calls it like a normal compiler: ``guest_cc.py [flags] -c source -o object``.

- C/C++ source: clang -emit-llvm (arm64_32-apple-watchos) -> opt with the of-rebase pass
  -> llc to Darwin assembly -> asm_convert.py to ELF assembly -> clang assembles an
  AArch64 ELF object for ld.lld.
- Assembly source (.s/.S): already ELF AArch64 syntax; clang assembles it directly.

Driver options (removed before clang sees the arguments):
  --of-llvm-bin=DIR     LLVM 22 bin directory (default: $OPENFRENZY_LLVM_BIN or llvm@22)
  --of-rebase-plugin=F  the built guest_rebase.dylib (required for C/C++)
"""
import os
from pathlib import Path
import subprocess
import sys

HERE = Path(__file__).resolve().parent
ELF_TARGET = "--target=aarch64-none-elf"


def run(command):
    subprocess.run([str(part) for part in command], check=True)


def main():
    llvm = Path(os.environ.get("OPENFRENZY_LLVM_BIN", "/opt/homebrew/opt/llvm@22/bin"))
    plugin = None
    args = []
    for arg in sys.argv[1:]:
        if arg.startswith("--of-llvm-bin="):
            llvm = Path(arg.split("=", 1)[1])
        elif arg.startswith("--of-rebase-plugin="):
            plugin = arg.split("=", 1)[1]
        else:
            args.append(arg)
    clang = llvm / "clang"

    if "-c" not in args or "-o" not in args:
        # not a compile (for example a version query): plain clang
        return subprocess.call([str(clang), *args])

    output = Path(args[args.index("-o") + 1])
    sources = [a for a in args if Path(a).suffix in (".c", ".cc", ".cpp", ".cxx", ".s", ".S")
               and not a.startswith("-")]
    if len(sources) != 1:
        sys.exit(f"guest_cc.py: expected one source file, got {sources}")
    if Path(sources[0]).suffix in (".s", ".S"):
        # ELF assembly: drop the guest target and dependency flags, keep -I/-D/-o
        kept, skip = [], False
        for a in args:
            if skip:
                skip = False
            elif a in ("-MT", "-MF", "-MQ"):
                skip = True
            elif not a.startswith("--target=") and a not in ("-MD", "-MMD"):
                kept.append(a)
        return subprocess.call([str(clang), ELF_TARGET, *kept])

    if not plugin:
        sys.exit("guest_cc.py: --of-rebase-plugin=<guest_rebase.dylib> is required")
    ir = output.with_suffix(".ll")
    rebased = output.with_suffix(".rebased.ll")
    darwin_s = output.with_suffix(".darwin.s")
    elf_s = output.with_suffix(".elf.s")
    front = list(args)
    front[front.index("-o") + 1] = str(ir)
    front[front.index("-c")] = "-S"
    run([clang, *front, "-emit-llvm", "-DOPENFRENZY_MACOS=1"])
    run([llvm / "opt", f"-load-pass-plugin={plugin}", "-passes=of-rebase,verify",
         "-S", ir, "-o", rebased])
    run([llvm / "llc", "-O2", "-mtriple=arm64_32-apple-watchos", "-aarch64-neon-syntax=generic",
         "-emulated-tls", rebased, "-o", darwin_s])
    run([sys.executable, "-I", HERE / "asm_convert.py", darwin_s, elf_s])
    run([clang, ELF_TARGET, "-c", elf_s, "-o", output])
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except subprocess.CalledProcessError as error:
        sys.exit(error.returncode or 1)
