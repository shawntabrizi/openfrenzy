# Build

Exact setup, build, run and test commands for Apple Silicon macOS. Design: `docs/ARCHITECTURE.md`.

## 1. One-time setup

```sh
brew install python cmake ninja llvm@22 lld@22 sdl3 ffmpeg
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
| `openfrenzy_skeleton.elf`, `openfrenzy_probe.elf`, `openfrenzy_adrp.elf` | toolchain tests without musl |
| `guest-main/gen/` | generated `of_decls.h`, `of_stubs.c`, `of_crt_init.c`, GL/posix wrappers |

Tests (`ctest`): `guest_skeleton_runs` (exit 0), `guest_probe_runs` (exit 42),
`guest_adrp_runs` (global addresses have a zero upper half; exit N = check N failed),
`guest_platform_check_runs`, `guest_guest_runs` (the game image boots and exits 0),
`guest_smoke_runs` (needs `orig/game.xiso.iso`; not registered without it). Tests run headless
(`SDL_VIDEO_DRIVER=dummy`), so they also pass in the sandbox.

Rebuild after editing: `cmake --build build-<id>` (new files under `src/` are found
automatically). The guest is an ExternalProject (`build-<id>/guest-main`); to see its errors in
full, run `cmake --build build-<id>/guest-main`.

Bink videos: the host decodes them with Homebrew FFmpeg (`ffmpeg`: libavformat, libavcodec,
libswscale; `port/macos/host/host_bink.c`). CMake finds it with pkg-config and prints
`Bink video: FFmpeg <version>`. Without FFmpeg, or with `-DOPENFRENZY_BINK_FFMPEG=OFF`, the host
builds without it and the game skips every video (the old `bink_null.c` behaviour).

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
| `OPENFRENZY_GAME=<name>` | start that minigame directly (BumperCars, Fireworks, Clusteroids, Demolition, Drillers, Tag, Hopper, MouseWhack, RollerBall, Jetboats, ScratchMatch, TankedUp); `src/runtime/of_launch.c` |
| `OPENFRENZY_VARIATION=<n>` | its variation, 1-4 for every minigame (1 when not set); needs `OPENFRENZY_GAME`. A bad value stops the run before the game starts (exit 2). See "Direct launch variations" below |
| `OPENFRENZY_DSOUND_LOG=1` | log the audio device (its real format, from the host), DirectSound buffers, the mix level and the WMA decode level, and each 10 s the device pace (callbacks, frames and their rate, longest interval and mix, underruns, silent time) and every silence gap of 100 ms or more (`src/dsound/dsound_sdl.c`). Agent shells may set `OPENFRENZY_AUDIO_ENABLED=false` and `SDL_AUDIO_DRIVER=dummy`; unset both to test a real device (`env -u ...`) |
| `OPENFRENZY_DEBUG_TEST_KEYS=<script>` | real SDL key and focus events for automated runs of the keyboard path; see "Scripted keys" below |
| `OPENFRENZY_DEBUG_SKIP_VIDEOS=true` | skip the Bink videos (boot logos, attract video), as builds without FFmpeg do. Boot reaches the title in a few seconds instead of about 75 s; use it for scripted runs whose presses assume the title at once (`START@6000`) |
| `OPENFRENZY_DSOUND_CAPTURE=<path>` | write the final audio output (after the limiter) to a WAV file: 48 kHz, stereo, 16-bit. It works without an audio device too (sandbox). See "Audio capture" below |
| `SDL_VIDEO_DRIVER=dummy` | no window (tests, sandbox) |

### Direct launch variations

`src/runtime/of_launch.c` takes `OPENFRENZY_VARIATION` 1-based for every minigame, the same as the
board (`CGameMode_AskAgain` calls the game's start with variation + 1). It refuses a value that is
not 1-4, a variation without `OPENFRENZY_GAME`, and an unknown game name (exit 2, with the valid
names). For Meta the value goes in unchanged. The original code reads it like this (the port
converts, the decompiled game code is not changed):

| Game | Reads the "variation" setting | Variation packages on the disc | Cut (never played by the board) |
|---|---|---|---|
| BumperCars | again, 1-based (0x00087400) | `bump-v01`..`v04` | - |
| Fireworks | start argument only (0x000b1aa0) | `carryon_v1`..`v4` | - |
| Clusteroids | start argument only (0x000d9410) | `clust-v4` (1-3 use `Cluster`) | 3: warning |
| Demolition | again, **0-based** (0x000e9530); the port writes n - 1 | `Demo-v1`..`v4` | - |
| Drillers | again, 1-based (0x0010fe50) | `drill-v1`..`v4` | - |
| Tag | start argument only (0x001253c0) | `Tag1`, `Tag3` | - |
| Hopper | start argument only (0x0013b9f0) | `hop-v1`..`v4` | - |
| MouseWhack | start argument only (0x0016cd40) | `bug-v1`..`v3` (variations 1 and 4 both opened `Bug-V1` in the test runs) | - |
| RollerBall | again, **0-based** (0x00188cb0); the port writes n - 1 | `Roll-v1`..`v5` | 3: warning |
| Jetboats | start argument only (0x0019d590) | `row_v1`..`v4` | - |
| ScratchMatch | start argument only (0x001ba5c0) | `Scratch1`..`Scratch4` (`Scratch4` is an empty 4 KB package) | 4: **refused** (faults in `SMScenic_Load`) |
| TankedUp | again, 1-based (0x001c1fb0) | `tank-v01`..`v04` | - |

"Cut": the entry in the playable table at 0x002839e4 is 0, so the retail board never picks it.
Checked headless, 60 s each, A every 3 s: variations 1 and 4 of every game, and Clusteroids 3,
RollerBall 3, ScratchMatch 3, reach `countdown PLAYING` with no fault (ScratchMatch 4 is refused).

### Scripted input (automated runs)

`OPENFRENZY_DEBUG_TEST_INPUT="ITEM[,ITEM...]"`, with `ITEM` = `[P<n>:]CONTROL@MS`. Pad n (1-4,
default 1) holds CONTROL for 150 ms, starting MS milliseconds after the first `XInputGetState`
call. The old pad-1 syntax (`START@6000,A@11000`) still works. `src/input/xinput_sdl.c`.

| Controls | Effect |
|---|---|
| `START BACK UP DOWN LEFT RIGHT LTHUMB RTHUMB` | digital buttons (`UP`..`RIGHT`: the d-pad) |
| `A B X Y BLACK WHITE LT RT` | analog buttons and triggers, full pressure |
| `LX+ LX- LY+ LY- RX+ RX- RY+ RY-` | full stick deflection (Xbox axes: `LY+` is up) |

- A pad 2-4 that the script names is connected from the start, with no gamepad attached. A
  real SDL gamepad on that port replaces it.
- Each press is logged once: `test_input: P2:A at 27510 ms` (pad 1 has no `P1:` prefix). An
  unknown control is logged with `(unknown control)`. A syntax error stops the script there.
- Example, a 4-human tournament to the first minigame (window, about 60 fps):
  `START@6000,A@11000,A@15500,A@23000,A@27000,P2:A@27500,P3:A@28000,P4:A@28500,
  P2:RIGHT@30000,P3:LX+@30000,P3:LX+@31000,P4:LX-@30000,A@33000,P2:A@33500,P3:A@34000,
  P4:A@34500,START@38000,START@42000,START@46000`, then `A` every 3 s.

### Scripted keys (keyboard path)

`OPENFRENZY_DEBUG_TEST_KEYS="ITEM[,ITEM...]"`, with `ITEM` = `NAME@MS[+HOLD]`. Port-only
(`src/runtime/sdl_platform.c`). Key `NAME` goes down MS milliseconds after the first keyboard read
(the first `XInputGetState` of pad 1, the same time base as `OPENFRENZY_DEBUG_TEST_INPUT`) and
up HOLD ms later (default 150). `NAME` is an SDL scancode name in any case: `RETURN`, `ESCAPE`,
`SPACE`, `J`, `K`, `UP`, `LEFT`, `TAB`, `KEYPAD 8`. `FOCUS_LOST@MS` and `FOCUS_GAINED@MS` post
the window focus events.

- The events go into SDL's own queue (`SDL_PushEvent`, host `host_sdl_push_event`) on the event
  thread. Then `SDL_PollEvent`, `platform_handle_event`, `platform_input_read` and
  `input_keyboard_to_pad` (`src/input/xinput_map.c`) handle them, as they do a real key. Use it
  to test the keyboard layout. Use `OPENFRENZY_DEBUG_TEST_INPUT` for pads 2-4.
- Each press is logged: `test_keys: SPACE at 9004 ms`. Focus changes are logged:
  `window focus lost: keyboard off, keys released`, `window focus gained: keyboard on`.
- While the window has no focus, the keyboard gives an empty pad (keys pushed then are ignored).
- Example: the round-2 board recipe with keys only (START = `RETURN`, A = `SPACE`):
  `RETURN@6000,SPACE@9000,SPACE@11000,SPACE@13000,RETURN@16000,RETURN@20000,RETURN@24000,
  RETURN@28000,SPACE@32000,RETURN@36000`, then `SPACE` every 3 s. Add `LEFT@52000+700` (d-pad
  and left stick) or `K@52500` (B) for minigame input.

### Audio capture

`OPENFRENZY_DSOUND_CAPTURE=<abs path>.wav` writes what the mixer sends to the device. The mixer
writes it after the limiter, from the audio callback (or from the clock thread when no device
opens). The WAV header is updated each second, so a run that a timeout kills still gives a
good file. Measure it with:

```sh
python3 -I scripts/wav_levels.py <file.wav>            # RMS, peak, clipped %, non-silent %
python3 -I scripts/wav_levels.py <file.wav> --window 5  # also RMS and peak each 5 s
```

Which sounds started, and whether each was heard: run with `OPENFRENZY_DSOUND_LOG=1
OPENFRENZY_BKLOG=1` and the capture, then

```sh
python3 -I scripts/sound_report.py <run.log>            # per file: starts, heard, mix peak, first time
python3 -I scripts/sound_report.py <run.log> --misses   # also: classify "Could not find" probes
```

With `OPENFRENZY_DSOUND_LOG=1`, BSOUND logs each start (`sound: sample|sample 3D|stream WMA|stream
ADPCM '<file>' ... at <s> s: buffer <p>, channel, volume ...`; `'?'` for files of background-loaded
packages, which have no filename table: the script names them from the disc), and the mixer logs
each buffer's own peak in the mix when it ends, stops or is released (every 5 s while it plays:
music): `dsound: buffer <p> ended at <s> s after <d> s: its mix peak <x>`. The times are output
seconds, the same as the position in the capture WAV.

macOS has no `timeout`; use `perl -e 'alarm shift; exec @ARGV' 70 build-<id>/openfrenzy ...`.

### Display settings

The game is 4:3 only: it always has 640 columns and 480 lines in its own units. A wider or
taller window gets black bars.

| Variable | Values | Effect |
|---|---|---|
| `OPENFRENZY_DISPLAY_RESOLUTION_SCALING` | `native` (default), `original` | `native` draws at the window's real pixels (Retina: 2 for each point), with the same scale both ways. `original` draws 640x480 and scales it up. |
| `OPENFRENZY_DISPLAY_ANTI_ALIASING` | `off` (default), `fxaa`, `smaa`, `msaa2x`, `msaa4x`, `msaa8x`, `ssaa2x` | `fxaa`: one pass over the 3D scene before the HUD (`COverlay_DrawAll`), or over the full frame at present when the frame has no HUD (menus). `smaa` gives `fxaa` (no SMAA in this build). `msaaNx`: N samples a pixel. `ssaa2x`: draws at 2 times the size each way, then scales down (with `native` only). |
| `OPENFRENZY_DISPLAY_VSYNC` | `true` (default), `false` | wait for the display's refresh |

The log line `screen: 640x480 drawn at WxH` gives the size the game draws at. It changes when
the window size changes. `OPENFRENZY_DEBUG_SCREENSHOT_EVERY=<frames>` with
`OPENFRENZY_DEBUG_SCREENSHOT_DIRECTORY=<abs dir>` writes BMP frames at that size.

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
