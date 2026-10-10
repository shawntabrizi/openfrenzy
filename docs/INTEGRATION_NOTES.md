# Integration notes

Cross-module problems that module agents reported to the coordinator. Integration agents: read this
before you fix the build, and remove an item when it is fixed.

## Link and toolchain

- `openfrenzy_guest` now links `ofplatform` with `-whole-archive` (0aeb546). Before, a weak stand-in
  in game code kept the real archive member out of the link (proved for `WmaCreateDecoder`). If the
  link now reports duplicate symbols, delete the game-side stand-in, not the platform definition.
  Most are in `src/engine_misc/BSOUND/platform_fallback.c`. dsound now defines: SetAllParameters,
  SetFrequency, SetPosition, SetVelocity, Min/MaxDistance, GetStatus, GetCurrentPosition,
  SetNotificationPositions, WmaCreateDecoder. Also check `XFindFirstSoundtrack` and xkernel stand-ins.
- An undefined weak symbol referenced PC-relatively resolves to the current page, not 0. Do not use
  `if (weak_fn)` checks in guest code (core agent finding).
- `src/runtime/of_math.c` called itself forever through the `halo_math.h` macros; fixed in 57a6602.
- Codegen trap (bkernel review, 2026-10-10): `adrp`/`adr` give the biased address
  (`0x100_xxxxxxxx`), because guest code runs at BIAS + address. When llc packs two pointers into
  one register with `orr xD, xLo, xHi, lsl #32` (a 16-byte struct of pointers passed by value), the
  bias of the low half lands in bits 32-40, so the high pointer gets `| 0x100`. Proved in
  `vfs_fopen` (src/vfs/xbox_files.c): `fopencookie(stream, "r", functions)` stores `close` as
  0x88057540 instead of 0x88057440 (`disc_stream_close`). `fclose` then jumps into the middle of
  `disc_find`, which overwrites the caller's stack and returns to 0 ("fault 11 at 0x0; pc=0").
  The crash comes and goes with code layout: the pointer is only wrong when its bit 8 is 0. This
  is the "unexplained crash" of `src/bkernel/tests/run_tests.sh` (it fails at HEAD 4558e3c in the
  probe's `fclose`). Masking `adrp`/`adr` results is not a general fix (jump tables and constant
  pools load through them without the pass). Needs an architect fix in the toolchain; a local
  workaround (not tested) is to not build pointer structs from `adrp` values (for vfs: a
  non-const `functions`, so it is loaded from memory).
  Fixed (integration round 1, toolchain): the of-rebase pass sends every value use of a
  global address through `mov wD, wS` (inline asm, opaque to known-bits), so the upper half
  is 0 for zext, 64-bit stores, compares, `lsr #32`, pointer arguments and register pairs.
  Load/store address operands, direct callees and llc's own adrp uses (jump tables, constant
  pools) keep the biased address. The earlier asm_convert.py `orr ... lsl #32` patch is
  removed. Test: `guest_adrp_runs` (port/macos/tests/skeleton/adrp.c).

## Shared storage

- `DAT_002ed458` is the CBCar_Game object (vtable at +0, size 0x4bf0). bumpercars_b must start
  `of_bcb_bss` at 0x002ed458 (it used 0x002ed460) and alias `DAT_002ed458` to the start. core now
  declares it `extern` (01af085). bumpercars_a only uses `extern struct CBCar_Game DAT_002ed458`.
- bumpercars_a owns .data 0x00290ee4-0x00292940 and .bss 0x002ed33c-0x002ed3a0.

## Prototype mismatches

- classlib_ui declares 0x71f90 `CLLoading_AddPackage` as `void`; it returns the package.
- bkernel declares 0x4b160 as `(log, name, flags)`; core_init calls `(0, name or 0, 0xe)` and uses
  the result.
- bumpercars_a `CBCar_Game.c` has argument-count errors on its own calls.
- 0x000a5be0, 0x000a5c00 (text reader) and 0x0009194d take ECX `this` as the first argument.
- `CMenuMode_Process` 0x30000 takes only `(int top)`; its vtable slot uses an adapter.

## Known compile failures (shared tree, 2026-10-10 07:30)

- core_modes_a: `CGameMode_GameWinner_ctor`, `CGameMode_GameStats_*` undeclared.
- classlib_ui `CIntroCam.c`: `ParseMap_*` undeclared.

## Boot path (integration round 1, 2026-10-10)

- bkRun is on. A clean build boots, loads packages from the disc image, shows the title
  screen, main menu, character select and the board map, and starts Hopper (verified with
  frame dumps, see below). ctest guest_guest_runs sets OPENFRENZY_BOOT_ONLY=1.
- Automated runs: `OPENFRENZY_DEBUG_TEST_INPUT="START@8000,A@11000,..."` (pad 0 presses),
  `OPENFRENZY_DEBUG_SCREENSHOT_EVERY=300 OPENFRENZY_DEBUG_SCREENSHOT_DIRECTORY=<abs dir>`
  (BMP frame dumps; needs a real window, so run outside the sandbox),
  `OPENFRENZY_BKLOG=1` (Babel log to stderr). The host prints a guest backtrace on a fault;
  symbolize with `llvm-symbolizer --obj=build/openfrenzy_guest.elf 0x<addr>`.
- `scripts/check_split_globals.py build/openfrenzy_guest.elf` lists DAT_ names whose storage
  is split from the owner's data block. It now also checks blocks named `of_*_bss2`, `_bss3`.
  No splits remain: carryon_a's of_co_data2 / of_co_bss2 own carryon_b's tables and globals
  (carryon_b declares them extern; the values matched the XBE), and charlib's of_chl_bss
  starts at 0x002f27d4 (0x002f27c8-0x002f27d4 is the CRay list, classlib_fx of_clf_ray_bss).
- File-scope asm uses Mach-O names: asm_convert.py strips one leading underscore. Write the C
  name `_DAT_x` as `__DAT_x`, and keep a plain `DAT_x` alias too. The generators of core,
  classlib_ui, hopper, charlib, classlib_fx, bumpercars_a, carryon_a and meta_a do this;
  the other modules' generators were not checked.
- The Z: drive file cache (bKernel/xbCache.c) is off: bInitCache reports it DISABLED and
  bkCacheEnable keeps it off. Build with `-DOPENFRENZY_Z_CACHE=1` for the original cache
  (it copies every package to `$OPENFRENZY_SAVE_ROOT/z`, about 300 MB per minute of play).
- Tool changes (port/macos/tools/*.py, the pass) are build dependencies of every guest C
  object (guest_cc.py adds them to the depfile). A build directory made before this change
  needs one clean build.

## Sound (integration round 1, 2026-10-10)

- `bLoadResource: Could not find '<name>.wav' in memory` lines are expected; the Xbox prints
  them too. ClassLib/GenericSamples.cpp (0x00071170, 0x00071540) probes `<kind>00.wav`,
  `<kind>01.wav`, ... until a lookup fails, for every voice kind of every character and every
  commentary kind. Each probe loop ends with one failed lookup. Checked against every SAMP
  file of every .xbp on the disc: no name that fails is in a package that is loaded at that
  time. At boot only CharVoice.xbp (the 16 `selected` samples) is loaded, so the other kinds
  fail until the character packages (Zak.xbp etc.) load. After that, `zak\taunt00` and
  `zak\taunt01` are found and `zak\taunt02` fails. Some kinds are on no package of the disc:
  `unselected`, `zonename`, `levelname`, `stop`, `tiebreak`, `playerwins`, `roundcomplete`,
  `newchampion`, `playerleads`, `charnamejoins`, `charnamemightwin`, `charnamewinsstage`.
  Hopper's `v1\blade_col_coll.obe` / `_shad.obe` probes also fail on the Xbox (the disc
  has only `v1\blade_col.obe`).
- The loaders of samples (0x00055390), audio streams (0x000562d0) and collision meshes
  (0x00052700) return the resource in EAX; Ghidra shows them as void. They now return it.
  Before, bkFindOrLoadResource used whatever x0 held (it was the resource by chance).
- `OPENFRENZY_DSOUND_LOG=1` logs the audio device, each DirectSound buffer (format, data,
  Play), the mixer once a second (voices playing, peak before the limiter) and the WMA
  decoder once a second of PCM (peak). A run outside the sandbox (Hopper and the front end)
  opens the device. The WMA music decodes with a peak of 10000-32768, and the ADPCM effects
  and voices play (up to 19 voices at once in Hopper). Nobody listened to the output.

## Library identifications

- 0x1e0323 `_purecall`; 0xa6fa0 returns 1; 0xf8ab0 returns 0 (stdcall, 1 argument).
- 0x0005a090 `RADSetMemory` (XbSymbolDatabase names it D3D_SetPushBufferSize); 0x000591c0
  `BinkOpen`; 0x00059f80 `BinkClose`. Bink is the null stub; videos are skipped.
- Register `src/dsound/tests/run_tests.sh` in ctest. Update docs/LIBRARY_CALLS.md for the 10 dsound
  entry points that are now implemented.
