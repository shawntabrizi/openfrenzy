# Web build (WebAssembly)

The game runs in a desktop browser as WebAssembly. The page loads the player's own disc image
(`game.xiso.iso`) with a file picker and keeps a copy in the browser's private storage for the
site (OPFS). No game data is in git or in the build output.

## Status

| Milestone | State |
|---|---|
| M1 Toolchain: Emscripten, CMake preset, platform layer and game link to `.wasm` | Done |
| M2 Boot in a browser | Done: title screen at 60 fps in Chrome (headless, Metal GPU) |
| M3 Play: menus, input, audio, saves | Done: board path to a minigame's `countdown PLAYING` with real key events; scripted Gamepad API pad drives the menus; sound in the Web Audio graph; the game's files persist in OPFS |
| M4 Bink videos | Done: MICROSOFT.bik, BLITZ60.bik, ATTRACT.bik play to the last frame, with sound |
| M5 Quality: settings, 60 fps, browsers | Not started |
| M6 Tests and docs | Not started |

M1: Emscripten 6.0.12 (Homebrew `emscripten`), `cmake --preset web` builds
`build-web/site/openfrenzy.js` and `openfrenzy.wasm` (3.7 MB). The link has no warning, no
undefined symbol and no weak stub; `-Wl,--fatal-warnings` makes a direct call with a wrong
type a link error.

M2: `port/web/site/` (page, disc image import into OPFS), `scripts/web_serve.py` (COOP/COEP,
127.0.0.1), `port/web/tests/drive.mjs` (Playwright driver). The disc image (2.08 GB) is copied
once (about 1 s from a local SSD in Chrome). Boot reaches "Press START to play" at 60 fps.
Two web-only renderer changes were needed: each vertex or index upload goes into a buffer
object of its own (ANGLE on Metal copied the whole 16 MB stream buffer for each partial write:
4 fps), and immediate-mode vertices are planar (WebGL's stride limit is 255 bytes).

M3: the native board recipe (`tests/run_game_check.py`), sent as real key events through the
page (Enter is START, Space is A): title, main menu, game size, character select, Map, minigame
intro, `countdown PLAYING` (56 s), score screen (120 s). Frames: 60 fps for most seconds, short
dips to 53-58 fps, and slower seconds while a package loads. Gamepad: `drive.mjs --pad` puts a
scripted standard-mapping pad behind `navigator.getGamepads`; SDL reads it and its START and A
presses drive the menus to character select (no key event). Sound: an analyser on the Web
Audio output reads peak 0.1-0.2 during the menu music. Saves: Fuzion Frenzy writes no save
games; its only file (`z:\debugLog.txt`, `saves/z/debugLog.txt`) is in OPFS after a run and
stays there. Three bugs that the native build hid were fixed in the game code: 9 functions
were decompiled as `void` although their callers use the value the original leaves in eax or
st0 (`CLLoading_AddPackage` returns the package buffer: without it the background loader never
started on the web).

M4: `port/web/ffmpeg.cmake` builds the native build's LGPL, Bink-only FFmpeg 9.0.2 with
Emscripten (static, no assembly, about 1 minute, once, into `deps/`), and
`port/web/src/web_bink.c` compiles `port/macos/host/host_bink.c` unchanged. The module grows
from 4.2 MB to 5.6 MB.

## Design (summary; details in docs/ARCHITECTURE.md section 18)

- wasm32 is ILP32, as the Xbox is. The game and the platform layer are compiled directly with
  Emscripten. There is no guest/host split, no `of-rebase` pass and no guest musl: those exist
  because macOS has no ILP32 process. This follows OpenCE's web port (CC0, commit e0b581b1).
- The WebAssembly memory is `0x88000000` bytes: the Xbox contiguous window
  `0x80000000-0x88000000` is its top 128 MB, and the C heap stays below it.
- Threads: Emscripten pthreads (SharedArrayBuffer, so the page must be cross-origin
  isolated). `PROXY_TO_PTHREAD`: the game's `main` runs on a worker and may block.
- Frames: the canvas is an OffscreenCanvas owned by the game's worker. At each Present the
  game yields to the browser with JSPI until the worker's next animation frame
  (`port/web/src/web_frame.c`). The game's own loops stay as they are.
- Indirect calls whose type differs from the callee's (legal on AArch64, a trap in wasm) are
  adapted after the link (`port/web/tools/wasm_icall_adapt.py`).
- File-scope assembly in the game's data files is Mach-O syntax (the native toolchain converts
  it); the web build compiles converted copies (`port/web/tools/wasm_inline_asm.py`).
