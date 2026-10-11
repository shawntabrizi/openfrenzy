# openfrenzy

A decompilation of an original-Xbox party game (Blitz Games, 2001; internal codename
`BlitzParty`), and a native port to Apple Silicon macOS. The port plays the whole game at about
60 fps: boot videos, attract mode, 6-stage tournaments, Minigame Frenzy and Options.
`docs/STATUS.md` tells what works and what is not verified.

This repository contains decompiled C, but no game data. You must supply your own copy of the game.

## Target

| | |
|---|---|
| Executable | `default.xbe`, USA retail, version 0 |
| SHA-1 | `c6fb6082900532cde5fa8dbb7de8dfa202f0fdad` |
| Build time | 2001-09-25 10:19:05 UTC |
| XDK | 3911 (XAPI, LIBC, kernel), 3925 (D3D8, DSOUND) |

Place your `default.xbe` at `orig/default.xbe` (git-ignored).

## Tooling setup (macOS, Apple Silicon)

Everything under `tools/` and `ghidra/` is git-ignored.

1. JDK 21: `brew install openjdk@21`, then `export JAVA_HOME=/opt/homebrew/opt/openjdk@21`
2. Ghidra 12.1.4: unzip `ghidra_12.1.4_PUBLIC_20260921.zip` from the
   [Ghidra releases](https://github.com/NationalSecurityAgency/ghidra/releases) into `tools/`.
3. XBE loader: unzip `ghidra_12.1.4_PUBLIC_20261009_ghidra-xbe.zip` from the
   [ghidra-xbe releases](https://github.com/XboxDev/ghidra-xbe/releases) into
   `tools/ghidra_12.1.4_PUBLIC/Ghidra/Extensions/`, then
   `chmod +x tools/ghidra_12.1.4_PUBLIC/Ghidra/Extensions/ghidra-xbe/os/*/XbSymbolDatabaseCLI`.
   The macOS symbol scanner is x86_64 and runs under Rosetta.
4. Ghidra ships no Apple Silicon natives. Build them once:
   `cd tools/ghidra_12.1.4_PUBLIC/support/gradle && ./gradlew buildNatives`

## Headless analysis

```sh
tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless ghidra openfrenzy \
  -import orig/default.xbe -overwrite -scriptPath scripts \
  -postScript FunctionReport.java
```

- `scripts/FunctionReport.java` — function count per XBE section.
- `scripts/DecompileSample.java [needle]` — decompiles the first function that references a string
  containing `needle`.

## Native port

See `docs/ARCHITECTURE.md` (design and rules), `docs/BUILD.md` (setup, build, run),
`docs/MODULES.md` (work modules), `docs/LIBRARY_CALLS.md` and `docs/STATUS.md`.

```sh
brew install python cmake ninja llvm@22 lld@22 sdl3 ffmpeg
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
```

## Status

Current state of the port: `docs/STATUS.md`.

Initial analysis (Ghidra auto-analysis + XbSymbolDatabase):

| Section | Functions | Named | Bytes |
|---|---:|---:|---:|
| `.text` (game, engine, statically linked libs) | 5358 | 195 | 1,563,302 |
| `D3D` | 232 | 87 | 50,041 |
| `DSOUND` | 333 | 81 | 32,595 |
| `XGRPH` | 278 | 2 | 56,038 |
| `WMADEC` + `WMADECXM` | 131 | 0 | 47,805 |
| `XPP` | 191 | 12 | 24,693 |

A second pass (`scripts/ExportAt.java`) found about 1280 more `.text` functions that are reached
only through tables (virtual methods, static constructors): 6636 in all, of which 5623 are game and
engine code (`src/include/of_functions.tsv`).

The XBE keeps 241 original source paths (Blitz "Babel" engine under `C:\Babel\Xbox\Src\`, game under
`C:\BlitzParty\Game\Src\`), which are used to name and group functions.
