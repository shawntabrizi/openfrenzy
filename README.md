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

Fuzion Frenzy plays natively on Apple Silicon macOS: boot videos, attract mode, full tournaments,
Minigame Frenzy, Options and all 45 retail minigame variations, at about 60 fps. Every function of
the game and engine has decompiled C. See `docs/STATUS.md` for what is verified and what is not.

This is a functional decompilation: the C code does what the original code does, but it does not
compile back to the same bytes.

## How it was made

AI agents (Claude) wrote the decompiled code, the platform layer and the docs, from Ghidra output
of the original executable. Expect errors that a human did not catch.

## Legal

- This project is not affiliated with or endorsed by Microsoft or Blitz Games. Fuzion Frenzy is a
  trademark of Microsoft.
- The project's own work is released under CC0 1.0 (`LICENSE.md`). That licence does not cover the
  original game, its data or its trademarks.
- Do not publish built binaries of this project. Build it yourself from source with your own copy
  of the game.

## Credits

The platform layer, renderer, audio and the macOS guest/host design come from
[OpenCE](https://github.com/OpenCommunityEdition/OpenCE) and its macOS forks by
[innerhat-dev](https://github.com/innerhat-dev/OpenCE) and
[zimm3rmann](https://github.com/zimm3rmann/halo-ce-ios-macos), all CC0. Third-party libraries
(ANGLE, musl, SDL3, FFmpeg) are downloaded at build time under their own licences.
