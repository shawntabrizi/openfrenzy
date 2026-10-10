# Build

Exact setup, build, run and test commands for Apple Silicon macOS. Design: `docs/ARCHITECTURE.md`.

## 1. One-time setup

```sh
brew install python cmake ninja llvm@22 lld@22 sdl3
```

- `llvm@22` is required for the guest (the `of-rebase` pass is built against it). Homebrew's
  default `llvm` (23 on this Mac) is not used. Override with `-DOPENFRENZY_LLVM_BIN=<dir>`.
- `ld.lld` is taken from `lld@22` first.
- The first CMake configure downloads, with SHA-256 checks, into `deps/` (git-ignored): ANGLE
  (prebuilt xcframeworks, GLES on Metal), the Khronos EGL/GLES headers and musl 1.2.5
  (`port/macos/dependencies.json`). After that no network is needed.

Game files (git-ignored, never commit them):

| Path | What |
|---|---|
| `orig/default.xbe` | the game executable (analysis only) |
| `orig/game.xiso.iso` | your disc image (XDVDFS); the game reads it at run time |

## 2. Build

Each agent uses its own build directory (`build-<id>`).

```sh
cd /path/to/openfrenzy
cmake -S . -B build-<id> -G Ninja
cmake --build build-<id>
ctest --test-dir build-<id> --output-on-failure
```

A clean build took 17 seconds on this Mac (930 files; musl and the platform layer go through
the rebase pass). Outputs in `build-<id>/`:

| File | What |
|---|---|
| `openfrenzy` | the Darwin host (native arm64) |
| `openfrenzy_guest.elf` | the game image: decompiled code + weak stubs + platform layer |
| `openfrenzy_smoke.elf` | smoke test: SDL3 window + GLES clear + disc reads |
| `openfrenzy_platform_check.elf` | links every platform object (catches undefined symbols) |
| `openfrenzy_skeleton.elf`, `openfrenzy_probe.elf` | toolchain tests without musl |
| `guest-main/gen/` | generated `of_decls.h`, `of_stubs.c`, `of_crt_init.c`, GL/posix wrappers |

Tests (`ctest`): `guest_skeleton_runs` (exit 0), `guest_probe_runs` (exit 42),
`guest_platform_check_runs`, `guest_guest_runs` (the game image boots and exits 0),
`guest_smoke_runs` (needs `orig/game.xiso.iso`; not registered without it). Tests run headless
(`SDL_VIDEO_DRIVER=dummy`), so they also pass in the sandbox.

Rebuild after editing: `cmake --build build-<id>` (new files under `src/` are found
automatically). The guest is an ExternalProject (`build-<id>/guest-main`); to see its errors in
full, run `cmake --build build-<id>/guest-main`.

## 3. Run

The host needs a data root and a save root. Use `orig/` as the data root, so the disc image is
found at `$OPENFRENZY_DATA_ROOT/game.xiso.iso`.

```sh
mkdir -p build-<id>/run/saves
export OPENFRENZY_DATA_ROOT=$PWD/orig OPENFRENZY_SAVE_ROOT=$PWD/build-<id>/run/saves

# the game (today: static constructors and main run as stubs, then exit 0)
build-<id>/openfrenzy build-<id>/openfrenzy_guest.elf

# the smoke test in a real window for 4 seconds
OPENFRENZY_WINDOWED=1 OPENFRENZY_SMOKE_SECONDS=4 build-<id>/openfrenzy build-<id>/openfrenzy_smoke.elf
```

Environment:

| Variable | Effect |
|---|---|
| `OPENFRENZY_DATA_ROOT` | data folder (required for a command-line run) |
| `OPENFRENZY_SAVE_ROOT` | save folder (created) |
| `OPENFRENZY_ISO` | disc image path (default `$OPENFRENZY_DATA_ROOT/game.xiso.iso`) |
| `OPENFRENZY_WINDOWED=1` | a window instead of borderless fullscreen |
| `OPENFRENZY_STUB_ABORT=1` | stop at the first unimplemented function |
| `OPENFRENZY_STUB_LOG=all` | log every call of a stub, not only the first |
| `OPENFRENZY_<SECTION>_<KEY>` | a setting of `src/runtime/of_config.c`, e.g. `OPENFRENZY_DISPLAY_VSYNC=false` |
| `SDL_VIDEO_DRIVER=dummy` | no window (tests, sandbox) |

Sandbox note (Claude Code): a window needs the window server, which the sandbox blocks
("The video driver did not add any displays"). Run windowed tests outside the sandbox, or use
`SDL_VIDEO_DRIVER=dummy`.

## 4. Analysis (Ghidra, headless, read-only)

```sh
export JAVA_HOME=/opt/homebrew/opt/openjdk@21
G=tools/ghidra_12.1.4_PUBLIC/support/analyzeHeadless
$G ghidra openfrenzy -process default.xbe -noanalysis -readOnly -scriptPath scripts \
   -postScript DecompileAt.java 2cd80            # print C for one or more addresses
$G ghidra openfrenzy -process default.xbe -noanalysis -readOnly -scriptPath scripts \
   -postScript ExportAt.java "$PWD/export" 14b6c0 # export a missed function (+ its call targets)
```

Ghidra writes `~/Library/ghidra/`, so run it outside the sandbox. `-readOnly` keeps the project
unchanged, so several read-only runs may run at once.

Regenerate the maps (architect; needs `export/` and `orig/default.xbe`):

```sh
python3 -I scripts/map_sources.py        # export/source_map.json
python3 -I scripts/modules.py            # export/modules.json, docs/MODULES.md, docs/modules.tsv
python3 -I scripts/gen_function_table.py # src/include/of_functions.tsv
python3 -I scripts/gen_library_calls.py  # docs/LIBRARY_CALLS.md
python3 -I scripts/coverage.py --markdown
```

## 5. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `The of-rebase pass needs LLVM 22` | `brew install llvm@22`, or pass `-DOPENFRENZY_LLVM_BIN` |
| `gen_game_decls: error: ... belongs to module X` | a `.decls` line names a function of another module |
| `ld.lld: error: undefined symbol: platform_...` | the platform layer calls something nobody defines: the owning platform module adds it |
| `ld.lld: error: duplicate symbol: FUN_...` | two modules define one function; the table's `module` column decides who |
| `Game data folder is missing` | set `OPENFRENZY_DATA_ROOT` |
| `ld: error: couldn't create cache file ... xcrun_db` | sandbox noise from Apple's linker; the host still links |
| gpg errors on commit (sandbox) | the sandbox blocks gpg-agent; earlier commits here are unsigned (`git -c commit.gpgsign=false commit ...`) |
