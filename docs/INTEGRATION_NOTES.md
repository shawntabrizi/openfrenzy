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

## Minigame launcher triage (integration round 1, 2026-10-10)

- `OPENFRENZY_GAME=<name> OPENFRENZY_VARIATION=<n>` starts one minigame directly
  (src/runtime/of_launch.c fills the retail settings table 0x00082860, which nothing fills on
  the Xbox; Core_Init then calls the game's start). Names: BumperCars, Fireworks, Clusteroids,
  Demolition, Drillers, Tag, Hopper, MouseWhack, RollerBall, Jetboats, ScratchMatch, TankedUp.
- Fixed here: classlib_ui `of_clu_data2` (0x00290d14) was a copy of CProjector ray type 1;
  0x000846c0 read its texture at +0x44 past the copy (Hopper countdown fault, pc=0x401921fb).
  It is now classlib_fx `of_clf_data + 0x54`. The score table field names DAT_002b9c48/58/64/68
  were separate commons; they are now aliases into DAT_002b9968 (core_modes_b cmb_globals.c).
- Variation 1, 60 s each, `SDL_VIDEO_DRIVER=dummy`, A pressed every 3 s. "60 s" = no fault
  until the kill; nobody looked at frames.

| Game | Result |
|---|---|
| BumperCars, Clusteroids, Hopper, MouseWhack, RollerBall, Jetboats | countdown PLAYING, no fault in 60 s |
| Demolition | PLAYING, no fault; UNIMPLEMENTED 0x000fd540, 0x000fcb90, 0x000fd180 (demolition_c, not in the table) |
| TankedUp | PLAYING, then fault (below) |
| Fireworks, Drillers, Tag, ScratchMatch | fault before the countdown (below) |

The five faults found here (Fireworks, Drillers, ScratchMatch, Tag, TankedUp) are fixed in
integration round 2 (921ec75); see below.

- Class of bug to check in every module: objects that core's crt/static_init.c constructs on
  DAT_<addr> commons but that a module maps into its own block without DAT_ aliases
  (ScratchMatch above). check_split_globals.py does not catch it.
- Fixed (bkernel timer): Hopper ended at once as "Players 1, 2, 3 and 4 draw!" (clock 1:00)
  when START paused it before the countdown. bkTimerRead now runs at the Xbox's 733 MHz (it
  was the 1 MHz performance counter); bkTimerDelta's 0xffffffff wrap was then 71 minutes.
  Any bkTimer user that compares tick counts with numbers now sees Xbox-sized ticks.
- Open (Hopper, direct launch only): after round 1, HopGame_Restart (0x0013f2c0) deletes every
  extra system, also the floaty score system at 0x002aeac0 (resources + 0x178), and does not
  make it again. A token pickup in round 2 can then fault in baAddExtraItem (seen once:
  read of 0 <- HopFx_TokenUpdate). The original does the same; on the board (Meta on the
  stack) the round does not restart.

## Integration round 2 (2026-10-10)

- Clean build in `build/` links with no errors; ctest 6/6. No UNIMPLEMENTED stub is reached on
  the board path; Demolition (direct launch) reaches 0x000fcb90, 0x000fd180, 0x000fd540 and
  0x000fe610 (demolition_c, not in the function table).
- Board run (window, `OPENFRENZY_DEBUG_TEST_INPUT="START@6000,A@9000,A@11000,A@13000,START@16000,
  START@20000,START@24000,START@28000,A@32000,START@36000,A@40000,A@43000,..."` = A every 3 s):
  title, menus, character select, Stage 1 (Tube Tremors/MouseWhack, Hopper, MouseWhack), the
  first Meta stage and its vote, Stage 2 (TankedUp, Team Clusteroids, TankedUp), into the second
  Meta stage (about 640 s, 60 fps). There it faults: read of 0xd0 in CLRoute_Start <-
  FUN_001586d0 (meta_a AI plan) <- AI_ThinkTail. Fixed: the AI's position was NaN, so
  Pickup_Nearest returned 0 (case 5). Root cause: charlib CRColl_Step 0x000b4dd0 reflected
  the velocity on an uninitialised normal when the total push was zero (the original keeps
  n in sum's stack slot, so n = 0 there). Velocities of 1e8 happened in every board run.
- The character select needs START about 7 s after the character is picked; earlier presses
  are lost (not a bug: the selection animation).
- Fixed: Drillers ctor list (+0x90), ScratchMatch root list alias, foxhunt 0x003022cc block and
  gate list 0x00302330 (both core static-ctor objects), Fireworks AI ring arguments, TankedUp
  revive arguments, meta_a NearestGoal arguments, meta_a transition-table call (one deref too
  many), and the Meta hang: CMetaEmitter_SetRate got MA_U32 bits converted to 1.06e9, so the
  emit loop never ended.
- New bug class: float parameters given raw dword bits (MA_U32, OV_U, desc[i], dr_fbits) are
  value-converted by C. `clang -Wimplicit-int-float-conversion` over all game sources found 16;
  all fixed. It does not see constants or 16-bit values.
- New check: `python3 -I scripts/check_call_arity.py` lists FUN_<addr> casts whose parameter
  count differs from the owner's .decls (49 hits; "FEWER" ones and a leading `this` the owner
  lacks are bugs). Two of the round-2 faults were of this kind.
- Float/dword audit (after 5ed610e). clang's conversion warnings over all game sources
  (-Wfloat-conversion, -Wimplicit-float-conversion, -Wimplicit-int-float-conversion,
  -Wliteral-conversion) and clang-query (implicit float<->int casts, integer literals made
  float) find nothing left; the 664 explicit (int)/FTOL casts and 2095 (float) casts were
  sampled against Ghidra (_ftol 0x001de998 / fild) and are the original conversions.
  The bug class that remained is the arm64 ABI: a call cast to a type whose float slots
  differ from the callee's makes the callee read a register the caller never set (s0 vs w0).
  `python3 -I scripts/check_float_abi.py <build dir>` reads the IR guest_cc.py leaves and
  lists such calls. It found, all fixed: baUpdateExtras given a float frame time (carryon_a,
  demolition_a, meta_a; it takes the bits in `void *arg`), bdDrawPointSprites given the
  point size as an int (classlib_ui CFireworks: garbage point size), CLFireworks_Init and
  _Launch given floats for their bit parameters (core_modes_a GameWinner: the DUAL macro
  chose the float branch), Shadows_Init and CFire_LaunchRack_Init given int 0 for a float,
  and the extras update callback classlib_fx 0x00085a40 declared `float dt` (baUpdateExtras
  passes `void *arg`). Not covered: calls through vtables and other function-pointer tables.
- Direct launch, 75-90 s each, after the fixes: every minigame, variations 1-3, reaches PLAYING
  with no fault, except ScratchMatch 2 and 3: the round ends at once ("Creating WinnerText"),
  then assertions at CScratchmatch_Game.cpp(815) and (384), then a fault in bFixupNodeDefs <-
  baLoadActor <- SMPlayer_InitAll <- SMGame_Restart.
  Correction (fix-scratchmatch-v2v3): the round does not end at once. It plays its full length
  (variations 1 and 2: 68 s after PLAYING, 72 s game length less the 4 s end margin; variation 3:
  63 s or one survivor); the log has no time stamps. All three variations then fault: a direct
  launch keeps start state 2 (g_StartState, Core_InitGameState), and the outro then calls the
  debug restart SMGame_Restart ("Don't do this"), which loads the actors and sets a second time
  from the packages in memory (fixed up twice). The original does the same. The port now ends
  the mode there (SMGame_UpdateOutro); the stack returns to Root.
- Rendering seen in frame dumps: glow sprites draw as solid white/coloured squares in TankedUp
  and Special Delivery (Fireworks). Everything else looked right.
  Fixed: these were ClassLib particles (point sprites, 0x000a9d20), not CTankEntities_DrawGlow.
  The d3d8 fixed-function path built the combiner from stage 0 (no texture) and drew 1-pixel-per-unit
  points without scaling. Now, as the XBE's D3D (0x001f8a20, 0x001f8730): with
  D3DRS_POINTSPRITEENABLE the one combiner stage comes from texture stage 3, read at gl_PointCoord;
  D3DRS_POINTSCALEENABLE sizes points height * size / sqrt(A + B d + C d^2) within min/max (64).

## Integration round 3 (2026-10-10)

- Clean build in `build/` (rm -rf, configure, build: about 25 s with 12 jobs) links with no
  errors; ctest 6/6. The final link keeps 5 weak stubs (FUN_00015270, FUN_000175c0,
  FUN_00018d90, FUN_00055c69, FUN_000bccbf); none was reached in any run below.
- Board, 2 stages (window, scripted pad 0, `build/run/board.sh` recipe of round 2, 1500 s):
  the full tournament ends. Power Station (Tube Tremors, Hopper, a third game), Military Base
  (Tank Tracks, Space Tails, Special Delivery), the final Meta stage, "Geena: Third",
  "Naomi Wins!", then back to the title screen (about 780 s). No fault, no UNIMPLEMENTED,
  no assertion. The title screen then tries ATTRACT.bik (Bink is null) and stays on the title.
- Board, 6 stages (`START@6000,A@11000,A@15000,RIGHT@20000,RIGHT@21000,A@23000,START@26000,
  START@30000,START@34000,START@38000,A@42000,START@46000`, then A every 3 s; 2700 s): the
  full tournament ends (Outlands, Coliseum, Downtown, Military Base, ..., Stage 6 Waterfront,
  the "Final Score" table of 6 stages, then the title, at about 2300 s). No fault,
  no UNIMPLEMENTED, no assertion. Babel free memory at the title: 41365 Kb at boot,
  40263-40637 Kb after a tournament.
- Minigame Frenzy (main menu DOWN once): character select, CPU difficulty, Air Raid played,
  "Do you want to play again?", replay. No fault in 270 s.
- Direct launch, all 12 games x variations 1-3, 90 s each, headless: all reach PLAYING with
  no fault, except MouseWhack variation 2: fault at the round restart (debug start state 2
  only): CLList_Remove <- Actor_Delete <- FUN_00172730 (CBbash_Arena reset) <-
  CBbash_Game_Update. ScratchMatch 1-3 print the (original) assertion
  CScratchmatch_Game.cpp(815) at the outro, as in round 2.
  Fixed (fix-mousewhack-v2-restart-fault): the original does the same. Variation 2 is Tube
  Tremors (mw_Variation 1), the only one with the train: a bug of type 8 whose actor (+0) is
  NULL (its platform is at +4). The round reset (FUN_00172730; the XBE inlines it in
  CBbash_Game_Outro 0x00173470) calls Actor_Delete(bug + 0) for every bug with no type check,
  so CLList_Remove reads address 0. The port ends the mode there instead, as ScratchMatch
  does; the stack returns to Root. MouseWhack 1-3, 120 s each: no fault.
- Open fault (real menu path): main menu -> Options faults at once, `pc = "\models\"`
  (0x5c736c65646f6d5c, string bytes as the return address: a stack buffer overflow). The
  stack scan shows CGameMode_MainMenu_OnButton -> CMB_PUSH_MODE -> MainOptions. Trace
  prints in CGameMode_MainOptions_Start show Start runs to its end; with prints in
  RefreshTexts the fault and the garbage go away (heisenbug). Without them, glyphs
  0x0330-0x0368 that are not in the font are drawn each frame. A print after the second
  bkWideSprintf call in RefreshTexts showed an empty result (text[0] == 0) for
  `"%S: %S"`. Repro: `OPENFRENZY_DEBUG_TEST_INPUT="START@6000,A@11000,DOWN@15500,
  DOWN@16200,A@17500"`, headless, 40 s.
- Main menu -> Game Demos: XLaunchNewImage("D:\XDemos\XDemos.xbe") exits the game (host).
- Audio (`OPENFRENZY_DSOUND_LOG=1`, 6-stage run): device opened (48 kHz stereo), WMA music
  decodes, mixer peak before the limiter over 2692 one-second reports: median 1.19, max
  2.75 (so the limiter acts most of the time). Nobody listened.
- New debug aids: each scripted press logs `test_input: <BUTTON> at <ms>` once (time base to
  match frame dumps); on a fault the host also lists guest code addresses found in the
  4 KB above sp (`sp+0x.. guest <addr>`; for smashed stacks; some entries are stale).
- Script timing: the "game length" menu shows about 18 s after the first XInputGetState with
  the inputs above; a second window run at the same time slows the menus (presses land
  early).

## Library identifications

- 0x1e0323 `_purecall`; 0xa6fa0 returns 1; 0xf8ab0 returns 0 (stdcall, 1 argument).
- 0x0005a090 `RADSetMemory` (XbSymbolDatabase names it D3D_SetPushBufferSize); 0x000591c0
  `BinkOpen`; 0x00059f80 `BinkClose`. Bink is the null stub; videos are skipped.
- Register `src/dsound/tests/run_tests.sh` in ctest. Update docs/LIBRARY_CALLS.md for the 10 dsound
  entry points that are now implemented.
