# Status

Updated 2026-10-11, at commit f0ecccb (after integration round 6 and the five commits that
follow it: app bundle, warning cleanup, gameplay ctest, virtual gamepads, draw overhead).
Each claim names its source: a run made for this update (build directory `build-status`), or a
section of `docs/INTEGRATION_NOTES.md`. All runs were on one Mac: Apple Silicon (M3 Max),
macOS 26, Homebrew LLVM 22, SDL 3.4, ANGLE on Metal.

Later change (build directory `build-fix-app`): the self-contained app (SDL3, FFmpeg and their
dependencies in the bundle, hardened runtime) and the virtual gamepad names. The rows "App
bundle" and "Virtual pad names" below come from that run; the other rows are from f0ecccb.

Later change (build directory `build-fix-lgpl-ffmpeg`): the project builds its own LGPL FFmpeg
9.0.2 for Bink only, instead of Homebrew's GPL FFmpeg. The rows "App bundle" and "Bink videos
(LGPL FFmpeg)" below come from that run. In that build: clean build, ctest 6/6 (`-LE gameplay`)
and 13/13 (`-L gameplay -j8`), and the `dsound` and `bdisplay` module tests passed.

## Summary

Fuzion Frenzy plays natively on Apple Silicon. Every function of the game and engine has
decompiled C. The XDK libraries (D3D8, DirectSound, XGraphics, XPP) are replaced by an HLE
platform layer adapted from OpenCE. Boot videos, attract mode, full 6-stage tournaments,
Minigame Frenzy, Options and all 45 retail minigame variations work. Play runs at about 60 fps.
No port fault is open. The main gaps are checks on real hardware (sound heard by a person, a
physical gamepad).

## Build and run on this Mac

One-time setup (details: `docs/BUILD.md`):

```sh
brew install python cmake ninja llvm@22 lld@22 sdl3
```

Build and test (the first configure downloads ANGLE, EGL/GLES headers, musl and the FFmpeg
source into `deps/`, and builds FFmpeg once, about 40 s):

```sh
cd openfrenzy   # the root of your clone
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build -LE gameplay --output-on-failure        # 6 toolchain/boot tests, < 1 s
ctest --test-dir build -L gameplay -j8 --output-on-failure     # 16 game tests, about 5 min
for s in src/*/tests/run_tests.sh; do sh "$s" "$PWD/build" || echo "FAIL $s"; done   # module tests
```

Run from a shell (log on stderr). `orig/game.xiso.iso` must be your own disc image:

```sh
mkdir -p build/run/saves
OPENFRENZY_DATA_ROOT=$PWD/orig OPENFRENZY_SAVE_ROOT=$PWD/build/run/saves \
  OPENFRENZY_WINDOWED=1 build/openfrenzy build/openfrenzy_guest.elf
```

Run the app: `open build/OpenFrenzy.app`. With no argument the host uses
`~/Library/Application Support/OpenFrenzy` for saves, `settings.ini` and `openfrenzy.log`, and
asks for the disc image once. Direct launch of one minigame:
`OPENFRENZY_GAME=Tag OPENFRENZY_VARIATION=2` (1-based). All switches: `docs/BUILD.md`.

## Verified for this update

| Item | Result |
|---|---|
| Clean build | `rm -rf build-status`, configure and build: 24 s wall time, exit 0, no link error. 230 `warning:` lines (was 140,391 before 4498a8c) |
| ctest, fast | 6/6 passed (`-LE gameplay`) |
| ctest, gameplay | 13/13 passed in 240 s at `-j8`: `gameplay_board` (title, character select, Map, first minigame to `countdown PLAYING`) and a 60 s direct launch of each of the 12 games. Each test fails on a fault, `UNIMPLEMENTED`, `Out of memory` or an assertion |
| Module tests | All 31 `src/*/tests/run_tests.sh` exit 0, headless. `d3d8` skips its drawing tests without a window. `runtime` reports 1 expected failure (`test_toolchain_strings`, see "Known bugs") |
| App bundle | Self-contained, 26 MB (was 54 MB with Homebrew's FFmpeg): 7 dylibs in `Frameworks` (20 MB): ANGLE (2), SDL3, and the 4 LGPL FFmpeg 9.0.2 libraries (libavformat, libavcodec, libswscale, libavutil; 2.6 MB). The codec libraries of Homebrew's FFmpeg (x264, x265, SVT-AV1, libvpx, dav1d, LAME, Opus, OpenSSL) and libswresample are gone. `otool -L` and `otool -l` find no `/opt/homebrew` path in any Mach-O file of the bundle. Ad-hoc signature with the hardened runtime and 2 entitlements (`allow-unsigned-executable-memory`, `disable-library-validation`; each tested, `docs/BUILD.md`). `codesign --verify --strict --deep` passes. `lsof` and `vmmap` of the running bundled host show every dylib from `Contents/Frameworks` and none from `/opt/homebrew`. No GPL code: the FFmpeg libraries report `LGPL version 2.1 or later` (`avutil_license`, `avcodec_license`, `avformat_license`, `swscale_license`) and `avcodec_configuration` has `--disable-gpl`, no `--enable-gpl`. Not started from the Finder (the sandbox blocks `open`); not tested on a Mac without Homebrew |
| Bink videos (LGPL FFmpeg) | Bundled host, videos on, window, stopped after 100 s: MICROSOFT.bik (105 of 105 frames), BLITZ60.bik (172 of 172, audio 2 ch 44100 Hz) and ATTRACT.bik (1599 of 1599, audio 2 ch 44100 Hz) decode and close; the title and the second ATTRACT.bik follow. Debug screenshots during BLITZ60.bik and ATTRACT.bik show the video frames. The audio capture is silent until BLITZ60.bik opens (3.27 s); from there until the menu music starts (67.6 s) the only voice is the Bink audio stream, with sound in both videos. The audio device was SDL's `dummy` driver: nobody listened |
| Virtual pad names | `OPENFRENZY_DEBUG_VIRTUAL_PADS=4`: the host logs `gamepad 2: Pad 1 OpenFrenzy virtual` to `gamepad 5: Pad 4 OpenFrenzy virtual` (was "OpenFrenzy virtual pad 1" for all 4) |
| Weak stubs | `llvm-nm` of `openfrenzy_guest.elf`: 0 weak `FUN_` symbols (build `build-open-decomp`; was 5 until the five functions were decompiled, `docs/INTEGRATION_NOTES.md` "Weak FUN_ symbols decompiled") |
| XDK entry points | `src/runtime/entry/xdk_entry_points.inc`: 0 `MISSING`, 0 `UNKNOWN`. D3D 62 HLE + 27 internal, DSOUND 27 HLE + 54 internal, XGRPH 4 + 1, XPP 8 + 3 |

## Decompilation coverage

Source: `python3 -I scripts/coverage.py --markdown`, run for this update. The function list is
`src/include/of_functions.tsv` (5623 functions: the game and Blitz engine code of
`default.xbe`, without the statically linked XDK libraries). A function counts as "real" when a
source file has its `// 0x<addr>` comment followed by a body that does not call
`PLATFORM_UNIMPLEMENTED` and does not contain `STUB` or `UNIMPLEMENTED`. The script finds 10830
address comments with a body under `src/`. It does not check that a body is correct; the
gameplay tests and the integration rounds do that.

| Module | Kind | Functions | Real code | Stubs | Not started |
|---|---|---:|---:|---:|---:|
| `core_modes_a` | game | 175 | 175 | 0 | 0 |
| `core_modes_b` | game | 125 | 124 | 1 | 0 |
| `core` | game | 278 | 277 | 1 | 0 |
| `bactor` | engine | 155 | 155 | 0 | 0 |
| `bdisplay` | engine | 113 | 112 | 1 | 0 |
| `engine_misc` | engine | 151 | 151 | 0 | 0 |
| `bkernel` | engine | 221 | 221 | 0 | 0 |
| `classlib_ui` | game | 222 | 222 | 0 | 0 |
| `bumpercars_a` | game | 185 | 185 | 0 | 0 |
| `classlib_fx` | game | 196 | 196 | 0 | 0 |
| `bumpercars_b` | game | 168 | 168 | 0 | 0 |
| `charlib` | game | 213 | 213 | 0 | 0 |
| `carryon_a` | game | 152 | 152 | 0 | 0 |
| `carryon_b` | game | 171 | 171 | 0 | 0 |
| `clusteroids` | game | 142 | 142 | 0 | 0 |
| `demolition_a` | game | 238 | 238 | 0 | 0 |
| `demolition_b` | game | 242 | 242 | 0 | 0 |
| `demolition_c` | game | 183 | 183 | 0 | 0 |
| `demolition_d` | game | 174 | 174 | 0 | 0 |
| `drillers` | game | 114 | 108 | 6 | 0 |
| `foxhunt` | game | 134 | 134 | 0 | 0 |
| `hopper` | game | 137 | 137 | 0 | 0 |
| `meta_a` | game | 186 | 186 | 0 | 0 |
| `meta_b` | game | 179 | 179 | 0 | 0 |
| `mousewhack` | game | 158 | 158 | 0 | 0 |
| `rollerball_a` | game | 247 | 247 | 0 | 0 |
| `rollerball_b` | game | 204 | 204 | 0 | 0 |
| `rowing_a` | game | 162 | 162 | 0 | 0 |
| `rowing_b` | game | 158 | 158 | 0 | 0 |
| `scratchmatch` | game | 137 | 137 | 0 | 0 |
| `tankedup_a` | game | 189 | 189 | 0 | 0 |
| `tankedup_b` | game | 114 | 114 | 0 | 0 |
| **total** | | **5623** | **5614** | **9** | **0** |

The 9 stubs are "not a function" entries (Ghidra splits at data or mid-function addresses) that
only log: 0x0001d460 (core_modes_b), 0x0002fe6c (core), 0x00046b20 (bdisplay), 0x00110000,
0x00110840, 0x00115443, 0x00120000, 0x001214f0, 0x001246e0 (drillers).

Platform layer (not in the function table; it replaces the XDK and the Xbox kernel). Lines of
C/C++/Python without tests:

| Directory | Lines | What |
|---|---:|---|
| `src/d3d8` | 12352 | Direct3D 8 on GLES (ANGLE/Metal), NV2A shaders, texture swizzle; from OpenCE |
| `src/dsound` | 7355 | DirectSound, WMA and ADPCM on SDL audio |
| `src/runtime` | 9765 | XDK entry table, CRT and MSVC semantics, SDL platform, launch, config |
| `src/xkernel` | 2466 | Xbox kernel and XAPI |
| `src/vfs` | 2029 | XDVDFS image reader and file APIs |
| `src/input` | 882 | XInput on SDL gamepads and keyboard |
| `port/macos/host` | 3510 | Darwin host: guest loader, memory arena, threads, files, SDL3, GL, Bink on FFmpeg |
| `port/macos/compiler` | 397 | `guest_rebase.cpp` LLVM pass (32-bit pointers in a 4 GB arena) |
| `port/macos/tools`, `port/guest` | 2333 | asm conversion, guest runtime glue |

## How far the game gets

Sources: `docs/INTEGRATION_NOTES.md` rounds 3-6 and the regression sweeps, plus the gameplay
tests above.

| Item | Result | Source |
|---|---|---|
| Boot and attract mode | MICROSOFT.bik, BLITZ60.bik, ATTRACT.bik (Bink on FFmpeg), title, attract video again, RollerBall attract demo | Round 6, window, 200 s |
| 6-stage tournament | Complete: 6 Meta stages, 18 minigames (plus a tie break), "Final Score", winner, title | Sweeps after rounds 4 and 5, window, 2700 s, two runs each |
| Minigame Frenzy | Character select, minigame select, Air Raid played 4 times through "play again"; memory stays level | Sweep after round 5, 330 s |
| Options | Volumes, vibration, song sample, Credits, back to the main menu | Round 3, sweep after round 5 |
| Minigames | All 45 retail variations (12 games x 4, less 3 cut ones) reach `countdown PLAYING`; Jetboats 1-4 ran 300 s each | Sweep after round 5 |
| 4 players | 4 scripted pads, and separately 4 SDL virtual gamepads, join and play to a minigame; unplug and re-plug pause and resume | Sweep after round 4; round 6 virtual pads (a)-(c) |
| Rumble | The game's rumble requests reach the SDL virtual gamepads (TankedUp, BumperCars, Options test) | Round 6 virtual pads (d) |
| Keyboard | SDL key events drive the menus and a minigame; synthetic focus loss and gain work | Round 5 |
| Frame rate | 59.5-60 fps in play; about 1 frame over 20 ms per 10 s. After f0ecccb the board path's lowest 10 s window is 59.6 fps (was 56.5) | Frame pacing (round 5), draw overhead (round 6) |
| Audio device | Real device at 48 kHz stereo; WMA music, ADPCM effects and voices mix; no underrun in 610 s | Round 5, round 6 |
| Fullscreen | Default is borderless fullscreen, 4:3 pillarboxed, 60 fps | Round 6 |

## By design

- Main menu -> Game Demos stays on the menu and logs that it is not available. `XDemos.xbe` is
  a separate Xbox program.
- ScratchMatch 4, Clusteroids 3 and RollerBall 3 are cut in the retail game. The direct
  launcher refuses ScratchMatch 4.
- ScratchMatch 1-3 print the original assertion `CScratchmatch_Game.cpp(815)` at the outro of
  a direct launch. The gameplay tests allow it.
- Player 1 (port 0) cannot be unplugged; it stays connected for the keyboard.

## Known bugs

1. Fixed: **Toolchain dropped a leading `_` from string literals.**
   `port/macos/tools/asm_convert.py` renamed quoted `.ascii`/`.asciz` operands as if they
   were symbols (`"_stricmp"` became `"stricmp"`), and it split a string at an escaped quote
   (`\"`). Now the string directives keep their quoted operands as written. 23 literals in
   14 objects change back to the source text: the game's event names (`_FileCacheRead` and
   so on), the Rowing suffixes `_a`-`_d`, the d3d8 shader dump name `_lit`, two musl strings,
   and three musl tables (`iswalpha`, `iswpunct`, `iconv`) that had lost bytes.
   `test_toolchain_strings` passes. Details: `docs/INTEGRATION_NOTES.md`, "String literals
   in asm_convert.py". Rowing play test: Jetboats 2 now loads 4 different lane paths (before,
   lanes a and c were a mix); the original game never reads the lanes in play, so play does
   not change ("Rowing lane paths in play").
2. Hopper, direct launch only: a token pickup after the debug round restart faulted once. The
   original code does the same; the board never restarts the round.

## Known limits

- `docs/LIBRARY_CALLS.md` lists 53 library functions as unidentified. The game runs without a
  name for them.
- `-Wint-conversion`, `-Wincompatible-pointer-types`, `-Wincompatible-function-pointer-types`
  and `-Wparentheses` are errors in the game build (audits: `docs/INTEGRATION_NOTES.md`,
  "int/pointer conversion audit", "Object pointer type audit" and "Operator grouping and pointer
  sign"; no real bug in 1054 hits). The game flags still hide `-Wpointer-sign` (126 hits, checked:
  none changes behaviour) and `-Wnonportable-include-path` (853). The platform layer, runtime
  and musl compile with `-w` (516 hits of the default set, none of the audited classes).
- The `.app` has an ad-hoc signature and no notarization (that needs the owner's Apple
  Developer account; steps in `docs/BUILD.md`). It bundles an LGPL FFmpeg built for Bink only
  (`docs/BUILD.md` "Licences").
- Settings come from defaults, `settings.ini` (app only) and
  `OPENFRENZY_*` variables.
- Custom soundtracks: `XFindFirstSoundtrack` always returns none.

## Not verified

- Audible sound. Nobody listened; only levels and device pace were measured.
- A physical gamepad: button layout, SDL mapping, rumble motors. Only scripted pads, SDL key
  events and SDL virtual gamepads were used.
- The on-screen "controller unplugged" message (the headless runs draw nothing).
- A real OS focus change. Only synthetic focus events were sent.
- The FFmpeg source comes from the `n9.0.2` tag archive of FFmpeg's GitHub mirror, not from
  the ffmpeg.org release tarball: ffmpeg.org did not answer during the change (`docs/BUILD.md`
  "Bink videos"). A build from the ffmpeg.org tarball was not made.
- Starting `OpenFrenzy.app` from the Finder, its disc image file dialog, and a windowed run of
  the app.
- In this update: no window run and no full tournament. The tournament, Minigame Frenzy and
  Options results above come from the rounds named, not from a run at f0ecccb.
- Performance on Macs other than an M3 Max.

## Next steps (in priority order)

1. Done: the `asm_convert.py` leading-underscore bug (Known bugs 1).
2. Do the checks that need a person: listen to the sound in a tournament, play with a physical
   Xbox-style gamepad (buttons, sticks, rumble, unplug message), start the `.app` from the
   Finder and pick the disc image in its dialog.
3. Done: the `-Wint-conversion` and `-Wincompatible-function-pointer-types` audit (774 hits
   to 0), the `-Wincompatible-pointer-types` audit (264 hits to 0) and the `-Wparentheses`
   audit (16 hits to 0). No real bug found; all four are errors now. `-Wpointer-sign` was
   scanned (126 hits, no real case) and stays off.
4. Run a full 6-stage tournament and Minigame Frenzy at the current commit (the last ones ran
   before the draw-overhead and input changes). Add a long tournament test under a new ctest
   label if its run time is acceptable.
5. Release `.app`: done: an LGPL FFmpeg built for Bink only (26 MB app). Still to do: sign
   with a Developer ID, test without `disable-library-validation`, notarize (`docs/BUILD.md`),
   and start it on a Mac without Homebrew.
6. Measure performance on a slower Mac (M1 base).
7. Name the 53 unidentified library calls. (The 5 weak `FUN_` symbols: done.)
8. Low: custom soundtracks. (SMAA: done, `display.anti_aliasing = smaa`.)
