# Web build (WebAssembly)

The game runs in a desktop browser as WebAssembly: graphics (WebGL 2), sound (Web Audio),
the Bink videos, keyboard and gamepad input, and the files the game writes. The page loads the
player's own disc image (`game.xiso.iso`) with a file picker and keeps a copy in the browser's
private storage for the site (OPFS), so the player picks it once. No game data is in git or in
the build output.

The rule of the README applies: do not publish built binaries. Build the site yourself and run
it on your own computer (`scripts/web_serve.py` serves it on 127.0.0.1 only).

## Status

| Milestone | State |
|---|---|
| M1 Toolchain: Emscripten, CMake preset, platform layer and game link to `.wasm` | Done |
| M2 Boot in a browser | Done: title screen at 60 fps in Chrome |
| M3 Play: menus, input, audio, saves | Done: board path to a minigame's `countdown PLAYING` with real key events; a scripted Gamepad API pad drives the menus; sound in the Web Audio graph; the game's files persist in OPFS |
| M4 Bink videos | Done: MICROSOFT.bik, BLITZ60.bik, ATTRACT.bik play to the last frame, with sound |
| M5 Quality: settings, 60 fps, browsers | See "Measured" and "Browsers" |
| M6 Tests and docs | `ctest --test-dir build-web -L web` (4 tests); this file; `docs/BUILD.md`; README |

## Build

One-time setup (besides the native build's `docs/BUILD.md` section 1):

```sh
brew install emscripten          # tested: Emscripten 6.0.12 (Homebrew bottle), node, python 3.14
```

Build (the first configure builds FFmpeg for WebAssembly into `deps/`, about 1 minute, once;
the first compile builds SDL 3.4.16 as an Emscripten port into `deps/emscripten-cache/`):

```sh
cmake --preset web               # build-web/, Ninja, cmake/web-toolchain.cmake
cmake --build build-web          # build-web/site/: index.html, app.js, openfrenzy.js, openfrenzy.wasm
```

Options (`cmake -S . -B build-web -D...`): `OPENFRENZY_WEB_ASYNCIFY=ON` (also build
`openfrenzy-asyncify.js` and `.wasm`, 7.4 MB, for browsers without JSPI, such as Safari 26; the
page loads it only there), `OPENFRENZY_WEB_BINK_FFMPEG=OFF` (no FFmpeg; the
videos are skipped), `OPENFRENZY_WEB_WASM_OPT=` (empty: no Binaryen optimisation, every function
apart in stack traces; default `-O2`), `OPENFRENZY_WEB_ASSERTIONS=ON` (Emscripten's run-time
checks).

## Run

```sh
python3 -I scripts/web_serve.py build-web/site          # http://127.0.0.1:8000/
```

Open `http://127.0.0.1:8000/` in Chrome. The page checks the browser, then:

1. **Choose a disc image…**: your `game.xiso.iso` (an XDVDFS image: a plain one, or a whole-disc
   image). The page copies it into the site's storage (about 1-2 s from an SSD; blocks of zeros,
   about half of the image, are not written). It asks the browser to keep the storage.
2. **Settings** (kept in this browser): resolution (native or 640x480), anti-aliasing (off,
   FXAA, SMAA), aspect ratio (4:3, 16:10, 16:9), videos (on or off).
3. **Play**. The canvas fills the page; **Fullscreen** (top right) makes it fullscreen.

The server must send the cross-origin isolation headers (COOP `same-origin`, COEP
`require-corp`): the game's threads need `SharedArrayBuffer`. `web_serve.py` sends them;
`http://127.0.0.1` and `http://localhost` count as secure. Another server must send them too.

Controls (the native keyboard map, `src/input/xinput_map.c`): arrow keys or WASD move, Space or
J is A, Backspace or K is B, F or U is X, R or I is Y, Enter or Escape is START, Tab is BACK, Q
and E are the triggers. A gamepad (standard mapping) works once one of its buttons is pressed
(a browser shows a page a gamepad only then). Sound starts after the first click or key press.

Address parameters (for testing): `?set=NAME=value` sets an `OPENFRENZY_*` variable (any
number; `docs/BUILD.md`), for example `?set=OPENFRENZY_GAME=Hopper&set=OPENFRENZY_VARIATION=2`
for a direct launch, or `?set=OPENFRENZY_WEB_FRAME_LOG=1` for a frame time line each second;
`?play` starts the game when the disc image is there; `?asyncify` loads the Asyncify build
where JSPI is too. For a browser that cannot be driven (Safari), `scripts/web_serve.py --disc
orig/game.xiso.iso --report LOG` serves the image at `/__disc` and keeps what the page posts:
`?image-url=/__disc&report&play` imports the image from the server, posts the log lines, and
starts the game; `?wipe` then removes everything the site keeps in that browser.

**Export saves** downloads the save folder as a zip; **Download the log** the game's output.
Fuzion Frenzy writes no save games: its only file is the debug log (`z:\debugLog.txt`, in the
storage as `saves/z/debugLog.txt`). Settings are the page's (localStorage). **Delete the disc
image** removes the copy.

## Browsers

Checked on one Mac (Apple Silicon M3 Max, macOS 26), each with the same build:

| Browser | Result |
|---|---|
| Chrome 154 (installed, headless through Playwright) | Plays: boot videos, title, menus, character select, Map, minigames, score screen; 60 fps; sound in the Web Audio graph; real key events and a scripted Gamepad API pad. The 6-stage tournament: see "Measured" |
| Firefox 157 (installed, headless through Puppeteer and WebDriver BiDi) | Plays: boot videos with sound, title at 60-61 fps, Hopper to `countdown PLAYING` at 60-61 fps; the game's own screenshot of its frame is correct. A browser screenshot of the page shows the canvas black (the worker draws it; OpenCE notes the same); taking one stalls the game for about 10 s |
| WebKit 26.5 (Playwright's build) | Plays: Hopper to `countdown PLAYING`, 50-61 fps, picture correct |
| Safari 26.6.2 (installed) | No JSPI (`WebAssembly.Suspending` is undefined). With the Asyncify build (`OPENFRENZY_WEB_ASYNCIFY=ON`): the checks pass, the image is imported from the test server (1.0 s), the game boots and the scripted pad (`OPENFRENZY_DEBUG_TEST_INPUT`) takes it into Hopper's intro and the minigame. The frame rate was not measured: the display was locked, and Safari slows (about 6 fps) and then stops the animation frames of a window that cannot be seen, so the game waits. Without the Asyncify build the page says that JSPI is missing |
| Firefox 146 (Playwright's build) | No JSPI. The Asyncify build plays Hopper to `countdown PLAYING` (see "Measured" for its frame rate) |

What the page needs: cross-origin isolation, JSPI (`WebAssembly.Suspending`), WebGL 2 in a
worker (OffscreenCanvas), OPFS, and 2.1 GB of shared WebAssembly memory (a 64-bit desktop
browser). About 2.1 GB of storage for the disc image.

## Measured

In Chrome 154 on the M3 Max (headless, ANGLE on Metal), release build:

- Frame rate: 60-61 fps at the title, in the menus and in Hopper with every display setting
  (native resolution, 640x480, FXAA, SMAA, 16:9). On the board path most seconds are at 60;
  seconds with a package loading drop lower (one at 19 fps at a load).
- Game work per frame (the game's worker between two yields): about 1 ms at the title, about
  10 ms in Hopper (4 players).
- Start: from **Play** to the title's first frame about 1 s with the videos off; the
  WebAssembly module is 5.6 MB (`openfrenzy.wasm`), the loader 1.4 MB (`openfrenzy.js`).
- Memory: the WebAssembly memory is 2.125 GB (reserved; pages are used as the game touches
  them). Resident in a run of the tournament: the game's renderer process 641 MB, all Chrome
  processes of the profile 1.65 GB.
- Disc image copy: 1.1 s (Chrome), 1.6 s (Firefox), 0.8 s (WebKit), 2.08 GB from the local SSD.

## Tests

```sh
mkdir -p tools/web-test && cd tools/web-test && npm init -y && npm install playwright@1.58.2
cd ../.. && ctest --test-dir build-web -L web --output-on-failure
```

Four tests (`port/web/CMakeLists.txt`). Three are runs of `port/web/tests/drive.mjs` in the
installed Chrome, headless, with a profile of its own in `build-web/test-profile` (the first
test copies `orig/game.xiso.iso` into it) and the server on 127.0.0.1:8791:

| Test | What it checks |
|---|---|
| `web_boot` (30 s) | the game's root mode, MICROSOFT.bik and BLITZ60.bik decoded to their last frame, sound in the Web Audio output |
| `web_board` (120 s) | the native board recipe as key events: character select, Map, a minigame's `countdown PLAYING`; 55 fps or more for 90 % of the seconds |
| `web_gamepad` (25 s) | a scripted standard-mapping pad (`navigator.getGamepads`) through SDL: START and A to character select |
| `web_tools` (1 s) | `port/web/tests/test_web_tools.py`: the call adapter on small modules run in Node (each call traps before the adapter and gives the AArch64 result after it: an extra argument dropped, a missing one 0, integer and float classes apart, a double to a float parameter, a void callee's first integer argument), and the inline assembly converter's names |

Each test fails on a wasm trap (`RuntimeError`), a worker error, `OpenFrenzy fault`,
`UNIMPLEMENTED`, `Out of memory` or an assertion. Without Playwright or the disc image they are
skipped (exit 77). They run one at a time (one profile, one port).

`drive.mjs` by hand (logs, screenshots and the game's files in `--out`):

```sh
node port/web/tests/drive.mjs --site build-web/site --iso orig/game.xiso.iso --out /tmp/run \
  --seconds 60 --shots 10,30 --keys "6:Enter,9:Space" --set OPENFRENZY_WEB_FRAME_LOG=1
```

Options: `--browser chrome|firefox|webkit|firefox-system` (`firefox-system` is the installed
Firefox through Puppeteer: `npm install puppeteer-core@24.10.0` in `tools/web-test`; `webkit`
uses Playwright 1.62.0 in `tools/web-test-webkit`, whose WebKit build is installed),
`--pad "S:BUTTON[+MS],..."` (standard button indices: 0 A, 1 B, 9 START), `--expect RE`,
`--forbid RE`, `--min-fps N`, `--pull DIR` (copy an OPFS folder, for example the game's own
BMP screenshots: `--set OPENFRENZY_DEBUG_SCREENSHOT_EVERY=600 --set
OPENFRENZY_DEBUG_SCREENSHOT_DIRECTORY=/data/saves --pull saves`), `--viewport WxH`,
`--profile DIR`, `--port N`, `--headed`. Each run logs the Web Audio level every 5 s and the
storage's files at the end.

Chrome and Firefox do not start inside the Claude Code sandbox (Mach bootstrap and profile
writes are denied): run the browser tests outside it.

## Design

Details and the reasons: `docs/ARCHITECTURE.md` section 18. In short:

- wasm32 is ILP32, as the Xbox is. The game and the platform layer are compiled directly with
  Emscripten (`HALO_WEB=1` marks every web difference). No guest/host split, no `of-rebase`
  pass, no guest musl: those exist because macOS has no ILP32 process. After OpenCE's web port
  (CC0, OpenCE commit e0b581b1).
- Memory: `0x88000000` bytes; the Xbox contiguous window `0x80000000-0x88000000` is its top
  128 MB, the C heap is below it.
- Threads: Emscripten pthreads. `PROXY_TO_PTHREAD`: the game's `main` runs on a worker, which
  owns the canvas (OffscreenCanvas) and may block.
- Frames: at each Present the game yields with JSPI until the worker's next animation frame
  (`port/web/src/web_frame.c`). The game's own loops (mode loop, loading screens, Bink player)
  stay as they are.
- Calls whose type differs from the callee's: legal on AArch64, a trap in WebAssembly. They are
  adapted after the link (`port/web/tools/wasm_icall_adapt.py`) as AArch64 passes arguments.
- Renderer differences for WebGL 2 (`src/d3d8`): each vertex or index upload goes into a
  buffer object of its own (ANGLE on Metal copied the whole 16 MB stream buffer for each
  partial write: 4 fps); immediate vertices are planar (stride limit 255 bytes); textures are
  put in RGBA order on the CPU (no texture swizzle); S3TC only for 2D textures with sides that
  are multiples of 4.
- Files: WasmFS with the OPFS backend at `/data` (`port/web/src/web_main.c`); the disc image
  is read in place with `pread`, as natively.
- Sound: SDL 3.4.16's Emscripten audio (a ScriptProcessorNode on the page's thread); the
  game's mixer runs on its own worker.
- Bink: `port/macos/host/host_bink.c` compiled unchanged (`port/web/src/web_bink.c`) on an
  LGPL, Bink-only FFmpeg 9.0.2 built with Emscripten (`port/web/ffmpeg.cmake`).
- The data files' Mach-O inline assembly is converted for wasm
  (`port/web/tools/wasm_inline_asm.py`).

## Limits and known issues

- Safari 26.6 has no JSPI (see "Browsers"): it needs the Asyncify build
  (`OPENFRENZY_WEB_ASYNCIFY=ON`), which is slower: Asyncify instruments every function that
  can reach the frame yield, and Emscripten's function pointer cast emulation (which Asyncify
  needs instead of the call adapter) widens every indirect call.
- The page must be cross-origin isolated by its server (no service worker fallback, which
  OpenCE has for GitHub Pages).
- WebGL: two `GL_INVALID_OPERATION` errors from `glBlitFramebuffer` at Present in the first
  frames (when the back buffer's size changes); the frames after them have none
  (`OPENFRENZY_DEBUG_GL_DEBUG=true` finds them; that setting reads every GL error back from the
  GPU process and runs at 2 fps).
- No touch controls and no custom soundtracks (as natively).
- Audible sound was not checked by a person: the tests read the Web Audio output's level.
- Only one Mac was used. No measurement on a slower machine.

## Files

| File | What it is |
|---|---|
| `CMakePresets.json`, `cmake/web-toolchain.cmake` | the `web` preset and toolchain (Emscripten's CMake platform file, the cache in `deps/`) |
| `port/web/CMakeLists.txt` | the web build: platform layer, game, link, the call adapter, the site, the tests |
| `port/web/ffmpeg.cmake` | FFmpeg for WebAssembly (LGPL, Bink only) |
| `port/web/openfrenzy_sdl3.py` | SDL 3.4.16 as an Emscripten port (from OpenCE) |
| `port/web/src/web_main.c` | entry point: page arguments, OPFS at `/data`, the heap limit |
| `port/web/src/web_frame.c` | the JSPI yield once a frame, frame statistics |
| `port/web/src/web_gl_host.c`, `web_memory_watch.c`, `host_watch_hash.{c,h}` | the host GL services and the memory watch by page hashes (from OpenCE) |
| `port/web/src/web_bink.c`, `web_icall.c`, `posix_files.c` | Bink (host_bink.c), the call adapter's report, the file helpers of `posix.h` |
| `port/web/src/web_library.js`, `web_pre.js`, `web_shared.h` | the JavaScript halves; the words the page reads |
| `port/web/tools/wasm_icall_adapt.py`, `wasm_inline_asm.py` | the post-link call adapter; the inline assembly converter |
| `port/web/site/` | the page (`index.html`, `app.js`, `style.css`) and the disc image importer (`import-worker.js`) |
| `port/web/tests/drive.mjs` | the browser driver of the tests |
| `scripts/web_serve.py` | the local server (COOP/COEP, 127.0.0.1) |

## Licences

The site's code is the project's (CC0) and OpenCE's (CC0). `openfrenzy.wasm` also contains
SDL 3 (zlib licence), Emscripten's C library (MIT and University of Illinois/NCSA; musl: MIT)
and FFmpeg 9.0.2 (LGPL 2.1 or later; `deps/ffmpeg-web-*/LICENSES/SOURCE.txt` has the source and
the configure line). FFmpeg is linked statically: the source and the build files to build it
again and relink are in this repository.
