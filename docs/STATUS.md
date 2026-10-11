# Status

Updated 2026-10-11 after integration round 6. The source of each claim is
`docs/INTEGRATION_NOTES.md` (rounds 1-6 and the regression sweeps) or a run named below. All runs
were on one Mac: Apple Silicon (M3 Max), macOS 26, Homebrew LLVM 22, SDL 3.4, ANGLE on Metal.

## Summary

The game plays natively on Apple Silicon. Boot videos, attract mode, full 6-stage tournaments,
Minigame Frenzy and Options work. Play runs at about 60 fps. The regression sweep after round 5
found two faults: Jetboats 4 (fixed in 9767d62) and ScratchMatch 4 (a cut variation). Round 6
found no fault, no `UNIMPLEMENTED` call and no assertion. No port fault is open.

## Run

```sh
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build
mkdir -p build/run/saves
OPENFRENZY_DATA_ROOT=$PWD/orig OPENFRENZY_SAVE_ROOT=$PWD/build/run/saves \
  OPENFRENZY_WINDOWED=1 build/openfrenzy build/openfrenzy_guest.elf
```

`orig/game.xiso.iso` must be your own disc image. `docs/BUILD.md` has the settings and the
scripted-input switches.

## What works

| Item | Result | How it was verified |
|---|---|---|
| Build | Clean configure and build in 25-45 s; no link errors; ctest 6/6 | Round 6, and this update (`build-fix-docs-status-refresh`, 43 s, 6/6) |
| Decompiled code | All 5623 game and engine functions in `src/include/of_functions.tsv` have a body. 9 of them are "not a function" entries (Ghidra splits) that only log | `python3 -I scripts/coverage.py` |
| Weak stubs | 5 hand-written weak `FUN_` symbols are left: FUN_00026d40, FUN_000ad830, FUN_001c1a90, FUN_001c1c10, FUN_001c2080. Only FUN_00026d40 (debug menu callback) logs as unimplemented; only the debug menu reaches it | `llvm-nm` of `openfrenzy_guest.elf` (this update); rounds 5-6 |
| XDK entry points | 0 `MISSING`, 0 `UNKNOWN` (D3D 62 HLE, DSOUND 27, XGRPH 4, XPP 8) | `src/runtime/entry/xdk_entry_points.inc` header |
| Boot and attract mode | MICROSOFT.bik, BLITZ60.bik, ATTRACT.bik (Bink on FFmpeg), title, attract video again, RollerBall attract demo | Round 6, window, 200 s |
| 6-stage tournament | Complete: 6 Meta stages, 18 minigames (plus a tie break), "Final Score", winner, title | Sweeps after rounds 4 and 5, window, 2700 s, two runs each |
| Minigame Frenzy | Character select, minigame select, Air Raid played 4 times through "play again"; Babel memory stays level | Sweep after round 5, 330 s |
| Options | Volumes, vibration, song sample, Credits, back to the main menu | Round 3 and sweep after round 5 |
| Minigames | All 45 retail variations (12 games x 4, less the 3 cut ones) reach `countdown PLAYING`. Jetboats 4 ran out of memory in round 3 of a direct launch; after the fix, Jetboats 1-4 ran 300 s each with no fault | Sweep after round 5, direct launch, 90 s each, headless |
| 4 players | 4 scripted pads join, pick characters and play to the first Meta vote | Sweep after round 4, 420 s |
| Keyboard | SDL key events drive the menus and a minigame; synthetic focus loss and gain work | Round 5 (`OPENFRENZY_DEBUG_TEST_KEYS`) |
| Frame rate | 59.5-60 fps in play on ANGLE/Metal; about 1 frame over 20 ms per 10 s | Frame pacing notes (round 5) and round 6 |
| Audio device | Opens the real device (48 kHz stereo); WMA music, ADPCM effects and voices mix; no underrun in 610 s | Round 5 (MacBook Pro speakers, volume 0.1) and round 6 |
| Fullscreen | Default mode is borderless fullscreen, 4:3 pillarboxed, 60 fps | Round 6, 30 s |
| Headless boot | Title, menus, character select, Map, MouseWhack to `countdown PLAYING`; no fault, no `UNIMPLEMENTED` | This update: `SDL_VIDEO_DRIVER=dummy`, videos skipped, round-2 input, 90 s, `Game exited (0)` |

## By design

- Main menu -> Game Demos stays on the menu and logs that it is not available. `XDemos.xbe` is a
  separate Xbox program.
- ScratchMatch 4, Clusteroids 3 and RollerBall 3 are cut in the retail game (the playable table
  is 0). The direct launcher refuses ScratchMatch 4.
- ScratchMatch 1-3 print the original assertion `CScratchmatch_Game.cpp(815)` at the outro of a
  direct launch.

## Not verified

- Audible sound. Nobody listened. Only measurements (levels, device pace) were made.
- A real gamepad. Only scripted pads and SDL key events were used.
- Rumble (vibration) on a real gamepad.
- A real OS focus change. Only synthetic focus events were sent.
- A `.app` bundle. None is built; run the host binary with the image.
- Custom soundtracks. `XFindFirstSoundtrack` always returns none.
- Performance on slower Macs. Only an M3 Max was measured.

## Known limits

- The debug menu callback FUN_00026d40 is not decompiled.
- `docs/LIBRARY_CALLS.md` lists 53 library functions as unidentified. The game runs without a
  name for them.
- Hopper, direct launch only: a token pickup after the round restart can fault (seen once).
  The original code does the same; the board never restarts the round.
- SMAA is not in the build ("smaa" gives FXAA).
- Settings come from defaults and `OPENFRENZY_*` variables only (no config file).
