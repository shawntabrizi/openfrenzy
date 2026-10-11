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
- Fixed (real menu path): main menu -> Options faulted at once, `pc = "\models\"`. Cause:
  CLPause_EscapeText (classlib_ui 0x00074f60, inlined into CLPause_SongName) converts "@@"
  (3 UTF-16 units with the terminator) into a 2-unit local. On the Xbox the extra unit fell
  into the next local, which is written right after; here it zeroed the low 16 bits of the
  caller's saved x22, which in CGameMode_MainOptions_RefreshTexts held the `L"%S: %S"`
  pointer. The second bkWideSprintf then read rodata as its format and overflowed `text[64]`
  (layout dependent, hence the heisenbug). Fix: `at[3]`. Found with an in-process ARM
  hardware watchpoint on the smashed slot (debug-only host patch, not committed: thread_set_state
  ARM_DEBUG_STATE64 + SIGTRAP handler that steps over the store with a breakpoint at pc+4).
  Verified (windowed frame dump, 62 s): Options opens with correct texts, sound volume 8->9,
  music 10->9, vibration on->off, song sample -> Radio Waves, soundtrack stays (no custom
  soundtracks exist), Credits opens, B -> Options, B -> main menu, no fault.
- Main menu -> Game Demos: XLaunchNewImage("D:\XDemos\XDemos.xbe") exits the game (host).
  Fixed (fix-game-demos-menu, port-only): CGameMode_MainMenu_OnButton case 3 now logs
  "main menu: Game Demos ... is not available in the port" and stays on the main menu. It skips
  the XDemo package load, wait.bmp, D3DDevice_PersistDisplay, PrepareLaunch (stops sound and
  streaming) and the launch, so nothing needs undoing. Why not more: XDemos.xbe is a separate
  Xbox program (own menus, fonts, png and Bink trailers in D:\XDemos\); a port of it is out of
  scope. Build with -DOPENFRENZY_ORIGINAL_XDEMOS_LAUNCH for the original path. Verified (window,
  skip videos, BMP dumps every 60 frames): A on "game demos" logs the line, the menu stays and
  takes input (DOWN wraps to tournament, A opens "game length"), no fault, no exit.
- Audio (`OPENFRENZY_DSOUND_LOG=1`, 6-stage run): device opened (48 kHz stereo), WMA music
  decodes, mixer peak before the limiter over 2692 one-second reports: median 1.19, max
  2.75 (so the limiter acts most of the time). Nobody listened.
- New debug aids: each scripted press logs `test_input: <BUTTON> at <ms>` once (time base to
  match frame dumps); on a fault the host also lists guest code addresses found in the
  4 KB above sp (`sp+0x.. guest <addr>`; for smashed stacks; some entries are stale).
- Script timing: the "game length" menu shows about 18 s after the first XInputGetState with
  the inputs above; a second window run at the same time slows the menus (presses land
  early).
- Rendering audit (fix-rendering-audit-floaties-and-glyphs): all three items are original
  behaviour; no code changed.
  - Air Raid (Demolition) score floaties are font quads, not point sprites. CDemoInfo_AddText
    feeds the shared "floatyscore" extra system (bumpercars_b bcb_floaty.c, 0x000a6e50,
    0x000a6f20, 0x000a6fb0). CDemoInfo_Init (0x000ebf90) sets rise 300, scale 0.1 and life 325
    (`lea ecx,[esi+0x10]`; stores at +0x90/+0x94/+0x98). The update gets dt = 1/60 at 60 Hz
    (0x000ec0a0; frame dumps confirm 60 frames a second), so a text lives
    age <= 1 * 325 * dt * 64 = 346 frames (5.8 s). It rises 5 units a frame (1733 in all) and
    is drawn at scale 0.1 * (0.5 * age + 2), which reaches 17.5 times the font size. The
    disassembly of 0x000a6fb0 (x87 stack traced) matches the port: grow, scale, the colour
    fade `colour - 3 * age / (size * life)`, the T*R*S matrix and bdPrintFontLite.
  - `particle00.bmp` and `00.bmp`: CGameMode_Meta (0x0014f670) calls Sprite_LoadDefs on
    0x002b0d30 with count 11 (`push 0xb`). Its records are star, paper, aismoke, reddot, aispk,
    streak, bstreak, coronab, ai_bubble, particle and an empty 11th name, so the empty probe is
    original. No package on the disc holds a plain `particle00.bmp` (all 89 .xbp searched; only
    `texturesl\particle00.bmp` in FoxLevel and `textures\particle00.bmp` in Tag1). These
    messages come once per Meta stage load, not once per minigame. FoxLevel's
    `texturesl\shit00.bmp` is also absent from every package.
  - Babel memory at the title: two 2-stage tournaments in one session (window, diagnostic
    build that records file/line in each heap block's reserved words, not committed).
    Title after tournament 1: 40871 Kb free, 282 live blocks, 10242112 bytes; after
    tournament 2: 40909 Kb free, the same 282 blocks and 10242112 bytes. No leak. The 1.1 MB
    difference from boot (41365 Kb) is the 4 character voice packages
    (`packages\english\<name>`, about 1.6 MB) that Core_LoadCharacterVoices (0x00012a60) loads
    for the chosen characters. They stay loaded until the next character selection releases
    them, as in the original. The title music's WMA buffers (393 KB) are not allocated at that
    moment, which offsets part of it.
  - New fault seen (not fixed): with START pressed on MetaVote (START at 704.5 s and 707.5 s,
    each followed by A), the next Meta stage load (`military_part_1.lnd`) faulted with
    `pc=cfb1f0b1d0b1cfb1` (pixel-like data as the return address: a smashed stack). Script:
    the 2-stage recipe, A every 3 s, plus START 1.5 s after each A from 700 s.

## Integration round 4 (2026-10-10)

- Clean build in `build/` (rm -rf, configure, build: 26 s) links with no errors; ctest 6/6.
  No stubs had to be regenerated. The final link still has the 5 weak stubs of round 3
  (FUN_00015270, FUN_000175c0, FUN_00018d90, FUN_00055c69, FUN_000bccbf); no run reached one.
- Attract mode (window, no input, videos on, 200 s): boot logos, ATTRACT.bik twice, then the
  RollerBall demo faulted at NULL + 0x78 (as in round 3). Fixed: CRollerBallGoal_Init
  (0x001912e0) read the goal models from game +0x1440; the XBE computes
  `[goal + 0x38 + 4i + (0x1440 - goal) + game]` = game +0x1478 + 4i (goal system +8,
  goalsign.obe and goalsfx.obe). +0x1440 holds the player 1/2 "opt" character actors, which are
  NULL in the demo (and wrong models in a direct launch). After the fix (420 s run): the RollerBall
  demo plays, then a second demo, videos between; no fault. RollerBall direct launch v1-3
  (75 s each, headless): no fault. The other "(0x.. - param_1)" pointer-delta sites in the
  export (Hud 0x00078e00/0x000796c0, CarryOn 0x000bc3d0, Hopper 0x0013d9f0/0x00143240, Rowing
  0x001b0c40) were checked: the port has the right absolute offsets.
- With `OPENFRENZY_DEBUG_SKIP_VIDEOS=true` the attract cycle never starts a demo (240 s on the
  title); use videos on to test the demos.
- Fixed (latent): meta puzzles FUN_001530c0, conditions CHAR_IN_CYLINDER (8) and
  CAM_IN_CYLINDER (11): the original sscanf has one pointer fewer than conversions. On arm64 the
  missing vararg is a stale stack word, so sscanf could write a float anywhere. No .lnd on the
  disc uses these keywords. These were the only -Wformat-insufficient-args warnings in the build.
- Host fault handler now also prints sp, fp, x0-x28 and the faulting thread.
- Open, intermittent: the Meta stage load fault of round 3 (`pc = cfb1f0b1d0b1cfb1`, right after
  `LoadSingleFile ... military_part_1.lnd`, empty sp scan). Reproduced 1 of 3 runs with the same
  script (`build/run/input_mv.txt`: the 2-stage recipe, A every 3 s, plus START 1.5 s after each
  A from 700 s; videos skipped). The other 2 runs passed the same point with the same stage order
  (indust, then military) and no fault. The pc halves read as smooth signed 16-bit values
  (-12367, -12111, -3919), like PCM samples or a height field. The WMA stream of the previous
  screen is stopped and released just before the load.
  Fixed (fix-meta-load-stack-smash): a short read, not audio. All 6 faulting runs (this one and
  5 debug runs) played Space Tails (Clusteroids) in the Military Base stage; 6 runs without it
  never faulted, and 1 run with it did not fault either. The registers are the same each time: the smashed
  bytes are stale `Cluster.xbp` texture data in the Babel heap (found at odd offsets in the
  package).
  - The .lnd buffer (`bkLoadSingleFile`, Babel heap, so in the contiguous window) reused a 16 KB
    page that the renderer still write-watched from a Clusteroids texture (`memory_watch_protect`;
    a freed Babel block is not unwatched). The D: drive's `fread` goes through the musl cookie
    to `xiso_fs_read` -> `pread`. The kernel does not fault on a protected page; it fails the
    read with EFAULT. So `fread` returned 0x1080 of the 26935 bytes (the text up to the page
    at 0x869fc000), and `bkLoadSingleFile` (which does not check the count) gave the rest as
    stale texture bytes. `LevelLoad_Preprocess` (0x00166320) then read a "word" of more than
    256 bytes into `word[256]` with `sscanf("%s")` and overwrote the frames above it;
    `LevelLoad_Load` (0x00166db0) returned through the smashed x29/x30.
  - Found with: a hex dump of the stack at the fault (the data matched Cluster.xbp), an ARM
    hardware write watchpoint on the smashed slot, armed only while LevelLoad_Load
    runs (pc in `vfscanf` <- `LevelLoad_Preprocess`), and a dump of the text buffer (it differs
    from the disc file from byte 4224 on). These debug patches are not committed.
  - Fix (host, `port/macos/host/host_syscall.c`): `read`, `pread64`, `readv` and `preadv` call
    `host_memory_watch_prepare_write` on the guest destination first, as
    `src/runtime/platform.h` asks of every host write ("call before the host itself (read(),
    the kernel) writes into the range"). `xbox_files.c` ReadFile already bounced through a
    staging buffer; the FILE path (`vfs_fopen`, used by `bkFopen`) and any other musl read did
    not. Test: `src/vfs/tests/run_tests.sh` "fopen watched" (fails without the fix: short fread).
  - Verified (window, same script, 760 s, `audio.enabled` false as in the owner's local config):
    3 runs, no fault; 2 of them played Space Tails in the Military Base stage (one as the last
    game, as in 4 of the 6 faulting runs), and the Military Meta level then loads its actors
    ("mesh name = militarymesh_coll.obe"). ctest 6/6.
  - The hint candidates were checked and are not the cause: `bsStopAudioStreamWMA`, the dsound
    mixer (it only reads guest buffers in place), `wma_xmo.c` (the decoder writes at most the
    packet's frames), guest thread stacks (the main stack is fixed), and
    `LevelLoad_LoadHeightMaps` (no `.hmp` is in the disc listing, and none is loaded).
- Board runs (window, 760-900 s): Power Station (Tube Tremors, Bug Bash v2, Hopper, ...), the first
  Meta stage and vote, Military Base minigames, the second Meta stage. Frame dumps look right.

### Regression sweep after round 4 (fix-regression-sweep)

Builds from `git archive HEAD` (the shared tree had another agent's uncommitted edit that did
not compile), own build dirs, ctest 5/5 (smoke not registered: no `orig/` in the archive).
No new fault was found, so no code changed. `OPENFRENZY_BKLOG=1` is needed for the
`countdown PLAYING` lines; without it the log stops after the disc image line.

- (a) Direct launch, all 12 games x variations 1-3, 90 s each, `SDL_VIDEO_DRIVER=dummy`, A every
  3 s, 6 runs at a time. Done twice: at 9080c5e and at 7ee9e65 (after the Meta load fix and the
  stub drop). Both times all 36 reach `countdown PLAYING` (Drillers, Clusteroids, Hopper,
  Jetboats and Fireworks 1-2 reach it 2-6 times: later rounds), no `OpenFrenzy fault`, no
  UNIMPLEMENTED; the only assertion is ScratchMatch 1-3 `CScratchmatch_Game.cpp(815)` (known,
  original). The `Failed to find anim segment` (Demolition) and `allcrowd.obe` (RollerBall)
  data errors are old; round-2 logs have them too.
- (b) 6-stage tournament, window, round-3 recipe, `OPENFRENZY_DEBUG_SKIP_VIDEOS=true`, 2700 s, at
  9080c5e. Two complete runs: 24 launches each (6 Meta stages, 18 minigames), then GameWinner
  and the title (PressStart at about 2390-2413 s). No fault, no UNIMPLEMENTED, no assertion.
  Frame dumps (every 150 frames, `OPENFRENZY_DISPLAY_RESOLUTION_SCALING=original` so a dump is
  1.2 MB) show the "Final Score" table with 6 stage columns and totals, "Dub: Last",
  "Samson: Third", "Jet Wins!", then "Press START to play".
- (c) 4 humans, the docs/BUILD.md example (pads 2-4 scripted) plus, from 50 s, each of pads 2-4
  pressing A or one full stick direction every 1.5 s; window, 420 s, at 9080c5e. All 4 pads join
  and pick (Dub, Samson, Geena, Jet), START plays. Bombstruck (Demolition v1), Laser Snare
  (Drillers, played twice), Blast Man Standing (Demolition v4), then the first Meta stage and
  its vote. No fault. HUDs: in Blast Man Standing the 4 life counters go 3 -> 2 -> 1/2 per player
  and end "Player 4 wins!"; the Meta deposit screen lists all 4 players with different counts.
  In Bombstruck all 4 scores stay 0 (nobody throws with 150 ms taps; not a port bug as far as
  the frames show). Pads 2-4's characters move in every game.
- Test hazard, not a game bug: two window runs ended early with `halo-linux: quit requested`
  then `Game exited (0)` (SDL_EVENT_QUIT). Both times another agent's window runs logged the
  same quit in the same second (22:28:25), so something outside the game stopped all
  `openfrenzy` processes (SDL turns SIGTERM/SIGINT into a quit event). The run that completed
  used copies of the host and guest under other names in the scratchpad. For long runs while
  other agents work, do the same, or check the log for `quit requested` before you report.

## Integration round 5 (2026-10-10)

- Clean build in `build/` (rm -rf, configure, build: 24 s) links with no errors; ctest 6/6. No
  stubs had to be regenerated. The final link has 5 weak symbols, all hand-written: the
  TankedUp fallbacks FUN_001c1a90/001c1c10/001c2080, meta_a FUN_000ad830, and core
  FUN_00026d40 (the debug menu callback; the only one that logs as unimplemented; only the
  debug menu reaches it).
- Board run (window, round-2 recipe, videos skipped, 150 s): title, menus, character select,
  Power Station, two minigames reach PLAYING (Bug Bash arena and a Clusteroids-style arena in
  the frame dumps); about 52 fps with frame dumps on. Headless 90 s with
  `OPENFRENZY_DSOUND_CAPTURE`: RMS -24.7 dBFS, peak -1.7 dBFS, 0 % clipped, 0 silent seconds.
- Fixed: CDemolition_Player_CreateActor (0x000f9370) looked up 47 anim segment names; the XBE
  loop ends at actor +0x310 (43 names). The extra 4 were empty names past the 0x00297b20
  table, and the port wrote 4 NULL slots at actor +0x310..+0x31c. The 36 misses that remain
  per launch (attack_1, attack_kick, hurt, idle, camera_pose, ...) are names the Demolition
  model does not have (original).
- Checked, original: the `Could not find '' in memory` lines come from COverlay_InitImage
  (0x00073510) for overlay descriptors with no texture and an empty name; the port matches
  the XBE.
- Direct launch Demolition with `OPENFRENZY_VARIATION=4` faults (NULL + 0xc in
  bLoadPackageResources): Demolition reads the setting 0-based, so 4 asks for `demo-v5.xbp`,
  which is not on the disc. Bad input, not a port bug; docs/BUILD.md now says which games read
  it 0-based. Variations 0-3 reach PLAYING.
- Test scripts: a job started with `&` from a non-interactive bash ignores SIGINT, so
  `kill -INT` does not stop the game; use `kill -TERM` (SDL turns it into a quit, then
  `Game exited (0)`).
- With `OPENFRENZY_DEBUG_SKIP_VIDEOS=true` and no input, the attract cycle did start the
  RollerBall demo in this run ("Top of stack now: AttractMode" at about 80 s), unlike the
  round-4 note.
- Not verified: keyboard input with real key events. Key events posted to the process with
  `CGEventPostToPid` (window not focused) did not move the title screen; the computer-use
  daemon was not available. The scripted pad path works.

### Loop bound audit (fix-loop-bound-audit, round 5)

- New check: `python3 -I scripts/check_loop_bounds.py [--all] [--addr <8 hex>] [-v]` computes
  the trip count of each counted loop (constant start, step and limit: `i < 47`,
  `0x264 + i * 4 < 800`, stride, pointer and count-down loops, `do { ... if (v > C) return; }
  while (true)`) in `export/funcs/<addr>.c` and in the src function after `/* 0x<addr>`, and
  lists src loops whose count is not a count of any XBE loop of the function. With the 47-loop
  of CDemolition_Player_CreateActor put back, it reports it (47 vs XBE 43).
- Run after 9f792cc: 888 functions, 1453 src loops, 1385 XBE loops compared; 56 MISMATCH and
  54 XBE-ONLY lines (not counting `for (n = C; n != 0; n--)` rep stos/movs copies). All were
  checked by hand against the Ghidra output: none is a real bound error. The causes are MSVC
  unrolling (fully: 3-6 element loops, `xyz` copies, the 8-step bit counts of COverlay and
  ScratchMatch; partly: 4 x 8 bits, 10 x 2 shuffle swaps), merged nested loops (4 x 9 = 36
  button slots, 6 x 19 = 114 voice sets), loop interchange (DrawMeter), inlined helpers
  (MW_Splat_Reset, rb_UpdateResults, tu_blast, sp_Strip, bg_FindAnims with a `count`
  argument), and counters the script cannot follow (float-typed, derived `iVar6 = iVar5 + 6`).
  One src deviation is intended and documented (CBbash_Game_Outro ends the mode instead of the
  original reset). So no game module changed.
- The script compares counts, not positions: a src loop whose wrong count equals another XBE
  loop's count in the same function is not found.
- Verified on a `git archive HEAD` build (other agents had uncommitted edits): ctest 5/5;
  Demolition variations 1-4, 90 s headless, A every 3 s: each reaches `countdown PLAYING`
  once, no `OpenFrenzy fault`, 36 `Failed to find anim segment` (the known model misses).

### Real audio device (fix-audio-device-real, round 5)

- Where `audio.enabled` comes from: `src/runtime/of_config.c` has the default `true`, and the
  environment (`OPENFRENZY_AUDIO_ENABLED`) overrides it. The port reads no config file; the
  save root holds no settings. The "false" of earlier runs was not a game or save setting: the
  owner's shell profile sets `OPENFRENZY_AUDIO_ENABLED=false` and `SDL_AUDIO_DRIVER=dummy`
  only in agent shells (`$CLAUDECODE`). A fresh install has sound on. `test_config` now checks
  the default with the variable unset (it fails if the default becomes false).
- New logs (`OPENFRENZY_DSOUND_LOG=1`): the host logs the format the device really has (after
  open, `SDL_GetAudioDeviceFormat`) and each 10 s the longest host callback (the handoff to
  the guest audio thread, its mix and the submit). dsound_sdl.c logs each 10 s
  `dsound: device pace` (callbacks, frames, rate, longest interval between callbacks, longest
  mix, how far the frames delivered are behind the wall time, underruns, silent seconds,
  silence gaps), `dsound: underrun` when the frames delivered fall more than 4096 frames
  (85 ms) behind the wall time, and `dsound: silence from / ends at` for each run of 100 ms
  or more of output at or under 1/32768 (output time, so the stack changes logged between
  the two lines can be matched).
- Run: outside the sandbox, real window, `env -u SDL_AUDIO_DRIVER -u OPENFRENZY_AUDIO_ENABLED`,
  fresh save root, round-2 input (A every 3 s to 600 s), `OPENFRENZY_DEBUG_SKIP_VIDEOS=true`,
  `OPENFRENZY_DSOUND_LOG=1 OPENFRENZY_BKLOG=1`, `OPENFRENZY_AUDIO_VOLUME=0.1` (to keep the
  speakers quiet at night; the master volume is a gain in the mix, it does not change the
  device path), host and guest copied under other names; SIGTERM at 615 s, `Game exited (0)`.
  The board went: title, menus, character select, Power Station, Tube Tremors, Hopper,
  MouseWhack, Meta stage and vote, Clusteroids, TankedUp, score screen. No fault.
- Device: `audio device 'MacBook Pro Speakers': 48000 Hz, 2 channels, SDL_AUDIO_F32LE, 1024
  frames a buffer`, the same as the game's stream (48 kHz stereo float), so SDL does not
  convert. (system_profiler showed the speakers at 44.1 kHz before the run; the device ran at
  48 kHz while the game had it open.)
- Pace, 61 windows of 10 s, from the first callback to 610.7 s: 469-470 callbacks a window
  (1024 frames each), 47981-48090 Hz a window (one callback more or less in 10 s is +-0.2 %),
  29,312,000 frames in 610.76 s. Longest interval 23.3 ms in most windows, 29.8 ms at most.
  "Most behind" goes from 9.8 ms to 11.9 ms over the run (device clock against the Mac's
  monotonic clock, about 3.5 ppm; no frames lost), 18.4 ms at most. 0 underruns.
- Mix time: under 1 ms in 56 of 61 windows. In TankedUp (540-590 s) it reached 2.2, 7.2, 3.0,
  15.3 and 1.3 ms; the host callback's longest was the same (15.32 ms), so the wake-up of the
  guest audio thread added almost nothing. The game had frame hitches at the same time (worst
  frame 250 ms at about 555 s) and up to 34 voices. Other agents built and ran games on the
  same Mac during the run, so CPU contention is likely, but the cause is not proved. Decoding
  is outside `mixer_lock`, and the lock sections are short. 15.3 ms is 72 % of the 21.3 ms
  period: no underrun, but not much margin. Nothing was changed (buffer size, thread priority):
  no underrun happened to fix.
- Silence gaps (100 ms or more), 10 in 610 s, all at the start or at a scene change:
  0-0.27 s (boot, before the first voice); 0.33, 0.58, 0.26, 0.67 and 0.14 s when a minigame
  starts (each starts within 0.1 s of `Top of stack now: <game>`, after `bStopAudioStreamWMA`
  stopped the intro music); 0.37 s when the Meta stage starts (level load); 0.12 s at the map
  (`com_mapscreen.xbp` loads, the music restarts); 0.14 s at a score screen (music change);
  1.72 s when TankedUp ends (music stopped, then `com_scorescreen.xbp` loads). No gap in play.
  Silent time outside these: at most 0.16 s a window, in runs under 100 ms.
- Not verified: that the sound is audible or right. Nobody listened. The run used master
  volume 0.1. Only the built-in speakers were tried (no Bluetooth, HDMI or device change during
  the run), and the Bink videos were skipped. The build had other agents' uncommitted edits
  in the shared tree (sdl_platform.c, host_sdl.c frame pacing, test keys).
  Update (fix-keyboard-input-verify): verified with SDL key events; see "Keyboard input" below.

### Keyboard input (fix-keyboard-input-verify)

- New port-only switch `OPENFRENZY_DEBUG_TEST_KEYS` (docs/BUILD.md "Scripted keys"): it pushes
  `SDL_EVENT_KEY_DOWN` / `SDL_EVENT_KEY_UP` (and the window focus events) with `SDL_PushEvent`
  on the event thread. The guest got `SDL_PushEvent` (`port/guest/runtime/guest_sdl.c`) and the
  host `host_sdl_push_event`. `OPENFRENZY_DEBUG_TEST_INPUT` was not set, so the scripted pad
  path did not run.
- Board run with keys only (window, outside the sandbox, renamed copies of the binaries, videos
  skipped, 160 s, stopped with `kill -TERM`, `Game exited (0)`): the round-2 recipe with
  START = `RETURN` and A = `SPACE`. BMP dumps every 300 frames show: title, main menu
  (tournament), character select (Dub picked, CPU difficulty), Stage 1 Power Station, Tube
  Tremors to "Player 3 wins!", the round results, then Twisted System to PLAYING. A second run
  (dumps every 180 frames) reached the same, then a Power Station arena minigame.
- Pad states, from a temporary log in `XInputGetState` (not committed): `RETURN` gives
  START (0x0010), `SPACE` A = 255, `K` B = 255, `LEFT` d-pad left (0x0004) and LX -32767,
  `UP` d-pad up (0x0001) and LY +32767. Each key comes back to 0 when it goes up. The mapping
  in `input_keyboard_to_pad` is correct for these keys. No mapping bug found.
- P1 scored 0 in Tube Tremors with LEFT/UP holds and K presses. The pad log shows the inputs
  reached the game, so this is the blind script (it does not aim at bugs), not an input fault.
- Focus, synthetic (`FOCUS_LOST@70000,FOCUS_GAINED@76000`, during Tube Tremors): the keys
  pushed between the two events gave no pad change; the first key after `FOCUS_GAINED` gave
  its pad state again (LEFT + SPACE at 76020 ms). No focus bug found. The new log lines name
  each focus change.
- Not verified: a real OS focus change. A window started from a background shell does not
  become the front app, so SDL sends no focus events. `osascript` (System Events frontmost,
  `Finder` activate) did not change the front app, and the cmux computer-use daemon was not
  available. The real path differs only in where the focus event comes from:
  `platform_handle_event` handles both the same way.

### Regression sweep after round 5 (fix-full-regression-after-r5)

Build from `git archive HEAD` at e496cb3 (includes 9f792cc, the Demolition anim segment fix)
in `build-fix-full-regression-after-r5/`: clean configure and build (25 s), ctest 5/5 (smoke
not registered: no `orig/` in the archive). All runs used renamed copies of the host and the
guest (`ofr5*`), `OPENFRENZY_BKLOG=1`. No run logged `quit requested`.

- (a) Direct launch, all 12 games x all 4 variation names (BumperCars ... TankedUp 1-4,
  Demolition and RollerBall 0-3), 90 s each, `SDL_VIDEO_DRIVER=dummy`, A every 3 s, 6 at a
  time. 46 of 48 reach `countdown PLAYING` with no fault and no UNIMPLEMENTED; the only
  assertion is the known ScratchMatch 1-3 `CScratchmatch_Game.cpp(815)`. The 4th variations
  are new in this sweep (round 4 ran 1-3 only). Two faults:
  - Jetboats 4 (Current Affair): fault 10 at 0x10000000000 in `memset` <-
    `bsPlayAudioStreamWMA` <- `bsPlayAudioStream` <- `CLMusic_Play` <-
    `CGameMode_MiniGameIntro_Enter` <- `CGameModeStack_Push` <- `FUN_0019e970` (Rowing start)
    <- `FUN_001a2890` <- `CGameModeStack_Process` <- `Core_Init` <- `main`, after
    `EnsureAllocBkg: *** Out of memory on Babel heap` (row_v4, 5.6 MB) and
    `xbAudioStreamWMA.cpp(614) : bkHeapAllocDebug: Out of memory`, at the start of round 3.
    **Port bug, fixed** (9767d62): in the replay branch of the Rowing update (state 6,
    `DAT_002bae98 == 2`) the port called `CGameModeStack_PopTo(..., "BGeffects", 1)`; the
    XBE pushes the name buffer 0x00283b18 (`push 0x283b18` at 0x001a38d9), which Core_Init
    sets to "Root" for a direct launch. With the literal, nothing was popped: the old Rowing
    mode never got `Popped` (no `Free3DBuffers` line), so it never closed row_v<n> and
    row_gen, and Babel free memory at each intro fell 28 MB -> 11 MB -> out of memory. Only
    Jetboats 4 has rounds short enough to reach round 3 in 90 s; 1-3 leak the same way. In a
    tournament or Minigame Frenzy the buffer holds "BGeffects", so nothing changes there.
    CarryOn (0x000be2d5) and core are the other users of 0x00283b18 and already use the
    buffer. Verified: Jetboats 1-4, 300 s each, headless: 4-6 rounds each, no fault, no
    out of memory; v4 Babel free memory at each intro 27.8 MB every round, one
    `Free3DBuffers` line per round.
  - ScratchMatch 4 (Hardcore): fault 10 at 0x100ffffffe4 in `bkLoadPackageFile` <-
    `baLoadActor` <- `baLoadActor` <- `bLoadResource` <- `SMScenic_Load` <-
    `CGameMode_ScratchMatch_Init` <- `CGameMode_MiniGameIntro_Render`. Not a valid
    variation: the playable table 0x002839e4 (`[game * 4 + variation]`, read by
    CMapCube_FindValidGames) is 0 for ScratchMatch 4, Clusteroids 3 (Mines) and RollerBall
    3 (Roll of Honour; setting 2): the 3 variations the retail game never offers (48 - 3 =
    45 minigames). Bad input, not fixed. Clusteroids 3 and RollerBall setting 2 happen to
    run without a fault.
- (b) 6-stage tournament, window, round-3 recipe, `OPENFRENZY_DEBUG_SKIP_VIDEOS=true`, 2700 s,
  frame dumps every 300 frames (`original` scaling). Run 1 at e496cb3: 24 launches (6 Meta
  stages, 18 minigames, among them Blast Man Standing, Tailblazer, Rollmentum, Rhythm,
  Armored Antics, Exterminator, Water Gate Scramble), GameWinner after the last A at about
  2389 s, "Samson Wins!", then "Press START to play". Run 2 with the Rowing fix (at the same
  time as run 1, so about 53 fps): 25 launches (24 plus the tie break "Jetboat Sprint"),
  among them Mystery Beat and Scratch Match (no assertion), Water Gate Scramble and Money
  Pool (Jetboats, which pop normally in a tournament), Tank Tangle and Tank Tracks;
  GameWinner at about 2389 s, then the title. Both: no fault, no UNIMPLEMENTED, no assertion,
  no out of memory.
- (c) Window, videos skipped, fixed build. Options (`START@6000,DOWN@20000,DOWN@21500,
  A@23500`, then one change on each line, A on credits, B three times; 80 s): sound volume
  up, music volume down, vibration on -> off, song sample -> Radio Waves, Credits scroll, B
  -> Options, B -> main menu; no fault. Minigame Frenzy (`START@6000,DOWN@20000,A@23000,
  A@26000,START@29000,START@32000,START@35000`, then A every 3 s; 330 s): character select,
  MiniGameSelect, Air Raid played 4 times through AskAgain ("play again"), no fault; Babel
  free memory at MiniGameSelect stays 39.6 MB after each replay. Character select needs
  START: A alone stays on it.
- Timing: the main menu takes input at about 14-20 s with videos skipped in a window (the
  round-3 note said 6 s START then 11 s); presses at 9-12 s land on the title transition.

## Integration round 6 (2026-10-11)

- Clean build in `build/` (rm -rf, configure, build: 25 s) links with no errors; ctest 6/6.
  No stubs had to be regenerated. The final link has the same 5 hand-written weak FUN_
  symbols as round 5 (FUN_00026d40, FUN_000ad830, FUN_001c1a90, FUN_001c1c10, FUN_001c2080).
  The build prints about 140,000 warnings (most are the `GetLastError` / `WSAGetLastError`
  redeclaration notes, 290 objects x 4).
- Headless (sandbox) 90 s, round-2 input, videos skipped: title, menus, character select, Map,
  MouseWhack to `countdown PLAYING`. No fault.
- Window, real audio device (`OPENFRENZY_AUDIO_VOLUME=0.1`), round-2 input, videos skipped,
  frame dumps every 600 frames, 300 s (stopped by SIGALRM): title, menus, character select,
  Map, TankedUp (played to ScoreScreen), Clusteroids (played to ScoreScreen), TankedUp again
  to PLAYING. 60.0 fps in play (1-2 frames over 20 ms per 10 s), 57 fps in the load windows.
  Device pace 47987-48090 Hz, longest callback interval 25.5 ms, no underrun. Capture: RMS
  -44.9 dBFS, peak -23.7 dBFS at volume 0.1 (the round-5 level less 20 dB). Frame dumps look
  correct (TankedUp arenas, Clusteroids dome). No fault, no UNIMPLEMENTED, no assertion.
  `sound_report.py --misses`: 1005 `Could not find` lines (302 names), only 2 after a holder
  package loaded (`dub.obe`); the rest are probes.
- Window, videos on, no input, 200 s: MICROSOFT.bik, BLITZ60.bik, ATTRACT.bik (25 fps), title,
  ATTRACT.bik again after the idle time, title, then the RollerBall attract demo. No fault.
- Default display mode (no `OPENFRENZY_WINDOWED`), 30 s: borderless fullscreen on the built-in
  display (Metal drawable 3456x2234), frame dumps 2979x2234 (4:3, pillarboxed), main menu
  correct, 60 fps.
- Changed: the platform log prefix is now `openfrenzy:` (was `halo-linux:`, from OpenCE).
  The scripts match the text after the prefix, so they are not affected.
- Not verified: audible sound (nobody listened), a real gamepad, keyboard focus from a real OS
  focus change, anything past 300 s in this round (rounds 3-5 cover full tournaments).

### Build warnings (fix-build-warning-noise)

A clean build printed 140,391 `warning:` lines. Correction to the note above: the
`GetLastError` lines (xdk_pdb.h 1678, 1726, 1744, 1750) are not redeclarations. They are the
same warning as all the others:

| Count | Flag | Cause | Fix |
|---|---|---|---|
| 111,741 | `-Wignored-attributes` | `__stdcall` (via `WINAPI`, `NTAPI`) in the XDK headers, about 385 per game object; AArch64 has no x86 calling conventions | `halo_linux_prefix.h` defines `__stdcall`, `__fastcall`, `__thiscall` empty for `HALO_ARM64_GUEST` (same code; the flag stays on) |
| 28,420 | `-Wduplicate-decl-specifier` | `D3DINLINE` was `static __forceinline`, and `__forceinline` is already `static` | `xdk_d3d8.h`: `D3DINLINE` is `__forceinline` (same code) |
| 220 | (no flag) | `weak identifier ... never declared`: the generated `platform_msvc_semantics.h` has `#pragma weak D3DDevice_SetRenderState` / `D3DXMatrixIdentity`; 110 game objects do not include `xdk_d3d8.h` | not fixed, see below |
| 10 | `-Wformat`, `-Wformat-extra-args` | `meta_a/Meta/puzzles.c` (`%d` into `short[2]`, `%i` into `MA_ADDR`), `meta_b/Meta/LevelLoad.c:535`, `tankedup_b/TankedUp/CTank_Char.c:91` | kept: each is the original's call (comments say so); not bugs on this target |

After: 230 lines (clean build, `build-fix-build-warning-noise`). Link OK, ctest 6/6, headless
90 s board run (round-2 input, videos skipped, `OPENFRENZY_BKLOG=1`) reaches `countdown PLAYING`.
The guest image is the same as before: identical symbol table and `.text` bytes (llvm-nm, objcopy).

- The weak pragmas: clang's warning has no `-W` group, so only `-w` hides it. The pragma must
  come before the static inline definition (after it, clang stops with "weak declaration cannot
  have internal linkage"), so it cannot move into `xdk_d3d8.h` after the functions, and the
  linker needs the weak copies (330 objects define them `W`). Left as is: 2 lines per object
  that does not use D3D.
- Warning classes turned off for game modules: none were added. The game flags have no
  `-Wall`/`-Wextra`, so unused variables and similar classes are not on, and after the two fixes
  the default set prints only the 10 format lines. The existing `-Wno-` list in
  `port/guest/CMakeLists.txt` (`GAME_FLAGS`) is unchanged.
- Conflict to know: `GAME_FLAGS` already has `-Wno-int-conversion` and
  `-Wno-incompatible-function-pointer-types`. In clang 22 these are errors by default in C;
  without the two flags the game sources stop with more than 400 `-Wint-conversion` errors
  (clang's error limit cut several units short). The round-2 audits ran the conversion
  warnings as a separate pass; do that again for those two classes, because the build does
  not show them. Of the float-conversion warnings, only `-Wimplicit-const-int-float-conversion`
  is on by default; `-Wimplicit-int-float-conversion` and `-Wfloat-conversion` are not on in
  the build (nothing turns them off; clang does not enable them without a flag).

### Gamepad path with virtual pads (fix-gamepad-virtual-test, round 6)

Until now only scripted pad states (`OPENFRENZY_DEBUG_TEST_INPUT`, which skips SDL) and pushed
key events had run. New port-only switch `OPENFRENZY_DEBUG_VIRTUAL_PADS` (docs/BUILD.md "Virtual
gamepads", `src/runtime/sdl_platform.c`): SDL virtual gamepads with a timed script. New guest SDL
calls (`port/guest/runtime/guest_sdl.c`): `SDL_AttachVirtualJoystick`, `SDL_DetachVirtualJoystick`,
`SDL_OpenJoystick`, `SDL_CloseJoystick`, `SDL_SetJoystickVirtualAxis/Button`, `SDL_CloseGamepad`;
host imports in `port/macos/host/host_sdl.c`. SDL cannot call guest callbacks, so the host sets
the virtual pad's Rumble callback itself and logs each request. New log switch
`OPENFRENZY_DEBUG_INPUT_LOG=true` (`src/input/xinput_sdl.c`). All runs headless
(`SDL_VIDEO_DRIVER=dummy`), videos skipped, build `build-fix-gamepad-virtual-test`.

- (a) 4-human tournament, virtual pads only (the BUILD.md 4-human recipe on pads 1-4, A every
  3 s from 52 s; 120 s): `GetAttachedDevices: device mask 0x0000000f` (4 pads at bInitInput),
  title, main menu, character select (A from each of pads 1-4 reaches ports 0-3), Map,
  Bombstruck (Demolition v1), `countdown PLAYING` at about 61 s. No fault. Proof that pads 2-4
  are humans: run (c) below. The game logs no player list; the per-player unplug check in
  `Core_UpdatePads` (0x0002c020) and the pause on re-plug act only for players whose "present"
  flag (0x002baed8 + 0x28 * i) is set, and both happened for pads 2, 3 and 4.
- (b) Sticks and triggers (`xinput:` lines, the values `XInputGetState` gives the game):
  `LY+` gives LY 32767, `LY-` gives LY -32768, `LX+` / `LX-` give LX 32767 / -32768,
  `LEFT_TRIGGER=20000` gives LT 155 (20000 * 255 / 32767), `RT` gives RT 255,
  `RIGHTX=-12000` gives RX -12000. Each comes back to 0 on release.
  **Mapping bug, fixed** (`src/input/xinput_map.c` `flip`): a centred SDL Y axis gave LY = RY =
  -1 (the flip was `-value - 1`, from OpenCE). Inside the game's dead zone (10000), so nothing
  moved, but a pad at rest did not read 0 and merged -1 into the keyboard's pad. Now 0 stays 0
  and SDL -32768 gives 32767.
- (c) Hot-plug during Bombstruck (same run plus `P2:UNPLUG@70000,P2:PLUG@76000,P2:A@80000`, then
  the same for pads 3 (90/96/100 s) and 4 (110/116/120 s); 140 s). What happens, each time:
  `gamepad 3 removed`, `bInput: There are now 3 input devices connected (add 0x00000000, remove
  0x00000002)`; on re-plug the new SDL id goes back to port 1, `bInput: There are now 4 input
  devices connected (add 0x00000002 ...)`, then `Paused!` (the pause menu opens for that
  player: `g_PadChangedPort`), A on the re-plugged pad gives `End pause`. After all three,
  stick moves on pads 2-4 reach ports 1-3. No fault.
  While a pad is out, `Core_UpdatePads` adds 20 to 0x002ecdec and pauses the sound; with
  0x002ecdec >= 20 the main loop skips `CGameModeStack_Update` (the game stops) and
  `CLPause_DrawMessages` draws the "controller unplugged" message. That part is from the code
  and the `Paused!` that follows (it needs 0x002ecdec >= 20); headless, nothing was seen on screen.
  **Hot-plug bug, fixed** (`src/runtime/sdl_platform.c`, `SDL_EVENT_GAMEPAD_REMOVED`): SDL keeps a
  removed pad's `SDL_Gamepad` open until it is closed, and `xinput_sdl.c` opens every pad it lists
  but never closed one. Each unplug leaked the gamepad and a host handle (256 handles in all; then
  `out of SDL handles` and new pads are lost). The event now closes it (`SDL_CloseGamepad`, host
  `host_sdl_close_gamepad` frees the handle).
  Player 1 cannot be unplugged: port 0 stays connected for the keyboard (by design).
- (d) Rumble. Direct launch TankedUp and BumperCars (vibration on, the default; 95 s; pads fire
  and drive): after `countdown PLAYING`, hits give `virtual gamepad 2: rumble low 0 high 29618`,
  `... low 39854 high 24802`, then `low 0 high 0`; BumperCars bumps give `low 17560 high 0` and
  others. Only port 0: a direct launch has one human, and `CLRumble_Allowed` rumbles humans only.
  Options menu (vibration line, A twice with each pad): the "off" press sends nothing, the "on"
  press sends the game's test rumble to that pad only: ports 0-3 get e.g. `left 27362 right
  27362`, `left 59120 right 59120`, then `left 0 right 0` on the matching virtual gamepad (ids
  2-5). Each `XInputSetState` is one SDL rumble request (the game sends only changes, 15 times a
  second at most).
- Tests (`src/input/tests/run_tests.sh`): new `test_sdl_virtual` attaches two virtual pads
  through the real SDL source: port mask 0x3, button/stick/trigger on port 1, `XInputSetState`,
  unplug is a removal on port 1 and closes the `SDL_Gamepad`, re-plug is an insertion on port 1.
  New `test_gamepad` checks: a centred SDL stick reads 0. With either fix undone, the tests fail
  (4 failures); with both, 0.
- Not verified: a physical gamepad (button layouts of real pads, SDL's mappings for them, real
  rumble motors), the on-screen "controller unplugged" message and the frozen game (headless
  runs draw nothing), a fifth pad, rumble while a pad is unplugged. Cosmetic: the host's
  `gamepad N:` line names every virtual pad "OpenFrenzy virtual pad 1" (the cause is not
  checked; probably the four pads get one SDL GUID, so one mapping name, as identical real pads do).

## Frame pacing (integration round 5, fix-frame-pacing)

- New: the host logs one line every 10 s from the swap path (`host_sdl_gl_swap_window`,
  port/macos/host/host_sdl.c): `frame pacing: N frames in 10.0 s, F fps, worst W ms, K over
  20 ms, swap S ms avg`. Frame time = swap to swap. Always on.
- Test: board path (`input_mv.txt` recipe of round 2, videos skipped, window, outside the
  sandbox, 300 s, no frame dumps, no other game process running), MacBook Pro M3 Max, built-in
  display (1728x1117 points), drawn at 2560x1920. Script: `build-fix-frame-pacing/run/run.sh`.
  The slow windows (about 50-120 s and 210-280 s) are MouseWhack (Bug Bash): the samples at 70,
  95 and 240 s are all in CGameMode_MouseWhack_Draw, with about 8600 GL draws a frame
  (`OPENFRENZY_PERF_LOG`).

  | Run (290 s after the first window) | Frames | Average fps | Lowest 10 s window | Frames over 20 ms | Worst frame (not the first window) |
  |---|---|---|---|---|---|
  | before, vsync on | 15289 | 52.7 | 41.0 fps | 6762 | 303 ms |
  | before, vsync off | 15761 | 54.3 | 41.1 fps | 4391 | 221 ms |
  | memcmp fix, vsync on | 17259 | 59.5 | 57.3 fps | 1588 | 120 ms |
  | memcmp fix, vsync off | 17223 | 59.4 | 56.1 fps | 972 | 109 ms |
  | memcmp + QoS fix, vsync on | 17256 | 59.5 | 55.9 fps | 31 | 66 ms |
  | memcmp + QoS fix, vsync off | 17301 | 59.7 | 56.9 fps | 24 | 60 ms |

  The first window (title load, one 512 ms frame) is the same in every run. The worst frames
  after it (45-120 ms, one or two a minute) are scene loads.
- Vsync makes no difference: `eglSwapBuffers` takes 0.02 ms with vsync on or off (ANGLE on
  Metal does not block in the swap). Present is paced by the 60 Hz vertical blank emulation of
  d3d8_gl.c (at most 2 frames queued), not by the display.
- Cause 1, slow frames (41-49 fps in MouseWhack): `sample <pid> 10` of the main thread (guest
  frames symbolized with llvm-symbolizer, `build-fix-frame-pacing/run/symsample.py`): memcmp was
  35 % of the main thread (2575 of 7357 samples), called from prepare_draw: d3d8_ff_uniforms
  compares 1440 bytes of fixed-function uniforms on each draw, and the program cache compares
  keys. musl's memcmp is a byte loop. Fix: `port/guest/libc/string/memcmp.c`, 8 bytes at a time,
  same sign of result (checked against the host memcmp with 2 million random cases, aligned
  and unaligned). Not the cause: shader compiles (at most 0.3 ms in a frame, perf log), disc
  reads (none on the main thread in the 3 samples), write-watch faults (the signal handler is
  9-15 of about 7400 main-thread samples, 0.2 %), texture uploads (not in the top of the samples).
- Cause 2, jitter at 60 fps (60 fps average but about 100 frames over 20 ms each 10 s, also on
  the title and menus): timing each part of D3DDevice_Present showed constant work (2.4-2.9 ms)
  but a wait for the vertical blank of 8-24 ms. The vertical blank thread (and the DirectSound
  clock thread) sleep to an absolute time (clock_nanosleep TIMER_ABSTIME, host_syscall.c); at the
  default QoS macOS woke them up to about 7 ms late. Fix: the host gives a thread that sleeps
  to an absolute time user-interactive QoS (once per thread). mach_wait_until instead of
  nanosleep did not help (75 vs 2 frames over 20 ms in the first 10 s window).
- Left (not fixed, not port-layer only): in MouseWhack the main thread is still busy all the
  time (lowest 10 s window 56-57 fps): ANGLE draw calls about 32 %, render pass ends at
  glClear after render-target changes (bdSetRenderTarget, the player shadow projectors) about
  12 %, memcmp still 9 % (now 8 bytes a step), xgpu_texture_get 9 %. A next step could be fewer
  GL state changes per draw in d3d8_gl.c or skipping d3d8_ff_uniforms when no transform,
  material or light changed.
- Not verified: the game window was not frontmost in these runs (started by an agent); a
  ProMotion display at 120 Hz was not set to 60 Hz; frame dumps were off, so the 52 fps of the
  earlier run with dumps every 600 frames was not run again; nobody looked at the motion.

## Draw overhead (integration round 6, fix-d3d8-draw-overhead)

Follow-up to "Left (not fixed)" in the frame pacing section above. Changes in `src/d3d8`:

- Fixed-function uniforms (d3d8_ff.c): a dirty flag for each group of `ff[]` (world-view and
  normal matrix, projection, texture matrices, material, lights). `D3DDevice_SetTransform`
  (through `d3d8_ff_transform_changed`), `SetMaterial`, `SetLight` and `LightEnable` set
  them; a view change also sets the lights (they are in camera space). `d3d8_ff_uniforms`
  computes and compares only the dirty groups, plus the ambient row when `D3DRS_AMBIENT`
  changed. Before, each FVF draw computed all 90 rows and compared 1440 bytes.
- Palettized textures (xbox_textures.c, `palette_hash`): each draw that binds one hashed its
  256-entry palette as one chain of 256 dependent multiplies (most of `xgpu_texture_get`).
  Now 8 independent lanes (the compiler makes them NEON). Only the cache key changes.
- Vertex streams (d3d8_gl.c, `setup_streams`): a `DrawVertices` draw (not a quad list) with
  one stream that is streamed puts its vertices at a multiple of the stride in the stream
  buffer, and gives that vertex as `first` to `glDrawArrays`. The attribute pointers stay the
  same from draw to draw. Before, each draw changed the attribute offset; ANGLE then built
  the vertex descriptor again, set the vertex buffers and looked up the Metal pipeline by
  hash. Indexed draws, quad lists and draws with more than one stream (or a mirrored stream)
  are as before.
- `D3DDevice_Clear` sets its color, depth and stencil masks and the scissor through the GL
  state cache, and no longer calls `xgpu_gl_state_invalidate`. In MouseWhack this is 5
  clears a frame, so 5 fewer cache invalidations a frame. The GPU stats line
  (`OPENFRENZY_DEBUG_GPU_STATS=true`) now also gives `state invalidations` a frame: 2-3 after
  (the "before" build has no counter).
- New check mode `OPENFRENZY_DEBUG_DRAW_CACHE_CHECK=true` (docs/BUILD.md). New unit test
  `test_ff_uniforms` in src/d3d8/tests/d3d8_test.c (15 checks; it fails when an entry point
  does not set its dirty flag).

Step 3 of the task (a cheaper path for the glClear after a render-target change): no change
was necessary. The glClear time was not the clear. At the first GL call after
`glBindFramebuffer`, ANGLE ends the previous render pass and only then encodes its recorded
draws into Metal (`RenderCommandEncoder::encodeMetalEncoder`: `drawPrimitives`,
`setVertexBuffer`, `setRenderPipelineState`). The clear only shows where this deferred work
runs. When the draws change less state, this time falls with them (884 to 160 samples below).
The shadow projectors still need their own render targets.

Measurements: MacBook Pro M3 Max, window, outside the sandbox, videos skipped, drawn at
2560x1920, before = HEAD 4498a8c, after = the same tree plus these changes (built in a separate
worktree, so other agents' uncommitted work is in neither). Scripts:
`build-fix-d3d8-draw-overhead/run/perf.sh`, `summary.sh`, `dumps.sh`, `cmpdumps.py`. Other
agents' game processes ran on the same Mac during the runs (one at about 70 % of a core);
before and after ran directly one after the other.

| Board path, 300 s (`input_mv.txt`) | Frames (after the first window) | Average fps | Lowest 10 s window | Frames over 20 ms | Worst frame | GL draw time a frame, frames with > 5000 draws |
|---|---|---|---|---|---|---|
| before | 16714 | 59.7 | 56.5 fps | 48 | 337 ms | 4.69 ms |
| after | 16811 | 60.0 | 59.6 fps | 53 | 114 ms | 2.11 ms |

The 10 s windows before: 56.8 60.0 60.0 60.0 59.8 60.0 59.9 59.8 60.0 60.0 58.1 58.1 60.0 58.2 ...
56.5 59.5 ...; after: every window 60.0 except the first (57.3, title load), 59.6 and 59.9. The
frames over 20 ms and the worst frames are scene loads (as in round 5); they did not change.

Main thread, `sample <pid> 10` at 70 s (MouseWhack on the board path), symbolized with
`symsample.py` (inclusive samples of about 7500):

| Function | before | after |
|---|---|---|
| CGameMode_MouseWhack_Draw (the frame's work) | 6895 | 3709 |
| D3DDevice_Present (mostly the wait for the vertical blank: idle) | 458 | 3738 |
| GL_DrawArrays (ANGLE draw calls) | 2392 | 947 |
| of these: PipelineCache::getRenderPipeline | 578 | 2 |
| of these: VertexArrayMtl::setupDraw | 615 | 311 |
| guest_glVertexAttribPointer | 261 | 0 |
| guest_glClear (ends of render passes) | 884 | 160 |
| prepare_draw | 2081 | 1042 |
| memcmp | 642 | 390 |
| xgpu_texture_get | 610 | 69 |
| d3d8_ff_uniforms | 269 | 9 |

So MouseWhack's frame work went from about 93 % to about 49 % of the main thread. MouseWhack
direct launch (`OPENFRENZY_GAME=MouseWhack`, 70 s): 60 fps both, GL draw time 4.84 to 2.14 ms
a frame (about 9000 draws a frame).

Correctness:
- Check mode, 0 mismatches: board path 150 s with all three changes (menus, character
  select, Map, MouseWhack; about 56 million draws checked), and MouseWhack direct 40 s with the
  final build (the unit test and a small change to how d3d8_ff_uniforms detects a new array
  came after the board check).
- d3d8 tests (`src/d3d8/tests/run_tests.sh`, headless and `--window`): 138 checks, 0 failed.
  ctest 6/6 on 4498a8c; on master 05333b6 with these changes, ctest 19/19 (the 13 gameplay
  tests too: board to the first minigame and the 12 direct launches, headless).
- Frame dumps (title, board path to MouseWhack, MouseWhack and TankedUp direct; 640x480,
  `OPENFRENZY_DISPLAY_RESOLUTION_SCALING=original`): byte-for-byte equality between runs is
  not possible. Two runs of the same "before" binary already differ in 1-96 % of the pixels
  after frame 0, because the game runs on real time (animation phase, the CPU characters'
  random picks, the input times). Frame 0 of every scene is the same before and after. I
  compared the frames side by side by eye: the same scenes, models, shadows, HUD, text and
  menus, with no missing or wrong geometry.
- Not verified: an exact pixel comparison (needs a deterministic time source); slower Macs;
  indexed draws use the old path, so no gain there; a ProMotion display at 120 Hz.
- Left: prepare_draw is still about 14 % of the main thread in MouseWhack (the pixel shader key
  and the draw-uniform inputs are built and compared on each draw), setup_streams about 11 %
  (the vertex upload into the stream buffer), and memcmp 5 %.

## String literals in asm_convert.py (fix-asm-string-underscore)

Fixes STATUS.md Known bug 1.

- Cause: `asm_convert.py` renames Darwin symbol names (it drops one leading `_`). It sent the
  operands of every directive through the same renaming, so a quoted `.asciz "_stricmp"` lost
  its `_`. Its quoted-token pattern also stopped at an escaped quote (`\"`). The text after
  it was then read as symbols: a `_` at the start of a word in the string was dropped too.
- Fix: one quoted-token pattern with backslash escapes, for the label match and the
  operands. `.ascii`, `.asciz`, `.string` and `.byte` keep each quoted operand as written;
  only the text between the strings goes through the renaming. Symbol names (labels,
  `.globl`, `.set`, relocations, quoted symbols) change as before.
- Check: for each of the 1350 guest objects, the `.darwin.s` file was converted again with
  the old script and compared with the new `.elf.s`. 23 lines in 14 objects differ, all
  `.ascii`/`.asciz`. No other line changes. The changed literals (now as in the source):
  - Event names: `_FileCacheRead`, `_FileCacheWrite` (bKernel/xbCache.c), `_PadChange`,
    `_MemCardChange` (xbVideo.c, unplaced_input.c, BINPUT/input.c, CGameMode_Root.c),
    `_AudioBkgLoad1`, `_AudioBkgLoad2` (xbAudioStream.c, xbAudioStreamWMA.c). Each name was
    changed the same way at every use, so the events matched before too.
  - Rowing (CRowing_World.c): the shape name suffixes `_a`, `_b`, `_c`, `_d` of
    `map\multiline.ase`. `RowAseImport_ReadShapes` finds a node when its NODE_NAME line
    contains both texts (`strstr`). Before the fix the texts were `a`-`d`: lanes `a` and `c`
    took every `raceline` node, and lanes `b` and `d` every `endbox` node. Only the variants
    with `(g_RowVariant & 0xc) == 4` read this file. No test runs Rowing, so this change is
    not verified in play.
  - d3d8_gl.c: the `_lit` suffix of a dumped vertex shader file name
    (only with the `debug.gpu_dump_shaders` setting).
  - musl: `iswalpha` lost 6 bytes of its table, `iswpunct` 1 byte, and the `iconv` charmap
    table 1 byte (a `_` after an escaped quote). Lookups after those offsets read shifted
    data (wide-character classes, legacy charmap conversions). Also a `_` in `towctrans.c` and
    `catopen.c` strings.
- `llvm-strings` of the new `openfrenzy_guest.elf` holds all 15 different literals in `src/`
  that start with `_` (tests not counted). No code in `src/` or `port/` uses the stripped
  names (`"FileCacheRead"` and so on).
- `test_toolchain_strings` is no longer in `src/runtime/tests/known_failures.txt`.
- Tests (clean build `build-fix-underscore` of 5238003 plus this change): ctest `-LE
  gameplay` 6/6, ctest `-L gameplay -j8` 13/13 (240 s), all 31 `src/*/tests/run_tests.sh`
  exit 0 (runtime 8 passed, 0 failed, 0 known failures; d3d8 skips its drawing tests
  without a window).

## Audio levels (integration round 3, 2026-10-10)

- New: `OPENFRENZY_DSOUND_CAPTURE=<path>` (WAV of the final output, docs/BUILD.md) and
  `scripts/wav_levels.py` (RMS, peak, clipped and non-silent share).
- Cause of the hot mix: the port did not apply the Xbox DirectSound headroom. The game never
  changes it (no game callers of SetMixBinHeadroom or SetHeadroom), so the defaults apply:
  - Voice headroom 600 mB (6 dB) for 2D buffers, 0 for 3D (DSBCAPS_CTRL3D) and for flags
    0x82000 (FUN_0020753a, the voice settings init). CDirectSoundVoice_SetVolume (0x00206d45)
    stores volume - headroom. The game's buffers: samples 0xa0 and music 0x1a0 (2D, 6 dB);
    3D samples 0xb0 (0 dB).
  - Mix bin headroom 1 on bins 0-30 and 0 on bin 31 (FUN_00208f1b, the APU start:
    CMcpxAPU_SetMixBinHeadroom(0x7fffffff, 1), (0x80000000, 0)). The voice processor divides a
    voice's gain into a bin by 2^headroom (xemu hw/xbox/mcpx/apu/vp/vp.c, voice_process): -6 dB.
    A 3D voice's direct path goes to the HRTF submixes (bins 6-9, register 0x2c0 = 0x09070806,
    FUN_0020cbcc) under the HRTF headroom, set to 0 (FUN_00208866(0)). So 3D direct = 0 dB,
    3D room send = -6 dB.
  - Result: a 2D voice at volume 0 is now -12 dB, a 3D voice's direct path 0 dB, as on the
    hardware model above. dsound_sdl.c: `voice_headroom_gain`, `MIXBIN_HEADROOM_GAIN`.
- Checked and correct (no change): millibels to linear is 10^(mB/2000)
  (`gain_from_millibels`). The game's volume table 0x00285658 (0-255 to mB, 1000*log2(v/255),
  index 255 = 0) matches the XBE. Music: `ext_MusicMaster(DAT_002bae68)` (0-255, default 0xff);
  each stream gets table[stream volume * master / 255]; the Options screen only shows
  DAT_002bae68 * 11 / 256 (0-10) and writes level * 255 / 10. No extra gain in the WMA path.
  Halo (ref/OpenCE dsound_sdl.c) sets its mix bin headroom to 0 itself; its port has no
  headroom code. This game does not, so the Halo assumption did not hold.
- Not known: the gain of the Xbox DSP program (effects image) from the mix bins to the
  output. The port does not run it. If it amplified the front bins, the port is now too quiet.
- Measured (headless in the sandbox: no audio device, the clock thread mixes; 69 s each;
  menus = `OPENFRENZY_DEBUG_TEST_INPUT="START@6000,A@9000,A@11000,A@13000,START@16000,
  START@20000,START@24000,START@28000,A@32000,START@36000,A@40000,A@43000,A@46000,..."`,
  Hopper = `OPENFRENZY_GAME=Hopper` with A every 3-5 s). Peak before the limiter is from
  the `dsound: mixed` lines (one a second). Limiter ceiling 0.891 (-1 dBFS).

  | Run | RMS | Peak (WAV) | Clipped | Non-silent | Pre-limiter peak median / max | Seconds over ceiling |
  |---|---|---|---|---|---|---|
  | menus, before | -14.2 dBFS | -1.00 dBFS | 0 % | 96.8 % | 0.957 / 2.096 | 42 of 69 |
  | menus, after | -25.3 dBFS | -4.75 dBFS | 0 % | 93.4 % | 0.248 / 0.579 | 0 of 69 |
  | Hopper, before | -12.0 dBFS | -1.00 dBFS | 0 % | 99.5 % | 1.251 / 1.975 | 66 of 69 |
  | Hopper, after | -21.5 dBFS | -3.65 dBFS | 0 % | 98.7 % | 0.440 / 0.657 | 0 of 69 |

  "Before" is the same build with the two headroom constants set to unity. The WAV peak
  "before" is the limiter ceiling (-1 dBFS); no sample is at full scale in either case.
- Not verified: nobody listened; frames of the menus run were not dumped (the script and
  buffer counts show menu activity, not which screens); the 6-stage tournament was not run
  again; a run with a real audio device (outside the sandbox) was not made with the capture.

## Sound content check (integration round 4)

Question: do the effects, the announcer and the character voices play, and are the ~1000
`bLoadResource: Could not find '<name>NN.wav' in memory` lines a run original probing or a port bug?
Checked by measurement only (nobody listened). Tools: `scripts/sound_report.py` and the new
`OPENFRENZY_DSOUND_LOG` lines (docs/BUILD.md, "Audio capture").

- The probes are original behaviour. Checked against the XBE (bLoadResource 0x0004bb30,
  bkFindOrLoadResource 0x0004b720, the set loaders 0x00071170 / 0x00071540, the commentator
  package loader 0x00071880 and the callers of 0x00071170) and against the filename tables of
  every .xbp on the disc:
  - A set loader probes `<kind>00.wav`, `01`, ... in memory until one fails, for all 58
    commentary kinds (0x0028fec0) or all 19 voice kinds (0x0028fe70) of a character. So every
    load ends each kind with one miss: `movestage06.wav` is the end of `movestage00-05`.
  - One commentator package is in memory at a time (com_ingame, com_mapscreen, ...; 0x00071880
    closes the last one first), and each holds only its screen's kinds. So `zonewrapup00`
    (only in com_zonescorescreen) misses on every other screen. `roundcomplete`, `stop`,
    `playerwins`, `levelname`, `zonename`, `tiebreak`, `playerleads`, `charnamemightwin`,
    `charnamejoins`, `newchampion`, `charnamewinsstage` and `unselected` are in no package on the
    disc: the game wins a round with `char<N>wins` instead.
  - `dub\bad00.wav` before Dub.xbp: CLSamples_LoadGeneric (0x000712b0) probes all 6 characters
    right after CharVoice.xbp (only the `selected` samples). The character packages are
    background loaded later; CGameMode_MiniGameIntro_Update (0x000237b0) and the character select
    (0x00017070) call 0x00071170 again once a package is in. The port matches the XBE in all of
    these. In the runs below the voices of every loaded character were found after that.
  - Data quirks of the disc (same on the Xbox): `ffintro04.wav` is missing, so `ffintro05-08` are
    never used; Fly has `fly\hurt01-04` but no `hurt00`, so Fly has no hurt voice; Jet has no
    `revenge`.
  - `scripts/sound_report.py --misses` lists probes of a file that is on the disc and that fail
    after a package holding it was loaded. Hopper run: 261 misses, 0 such. Board run: 2259
    misses, 0 such sound files (6 are `dub.obe`, a model; see below).
- Background-loaded packages have no filename table (the XBE's background loader does not call
  bkLoadFilenameTable 0x0004dec0 either), so the start log shows `'?'` for their files; the
  script names them from the disc.
- Hopper direct launch (`OPENFRENZY_GAME=Hopper`, headless, A every 3 s, 75 s): announcer
  getready, countdown 02/01/00, go, almostover, char2wins (-15 dBFS each, own mix peak); the
  voices of Dub, Fly, Geena and Zak (jump 156 starts, land 3D 153, hurt 34: all heard, -19, -9.5
  and -20 dBFS); refbot, tokens, horn; music boom.wma and blitz_cowboy.wma (-11 to -12 dBFS). Whole WAV: RMS
  -21.5 dBFS, 0 silent seconds of 74; the RMS goes from -26 to -19.5 dBFS during the countdown.
  The direct launch loads the character packages too, so the voices play there.
- Board path (`build/run/input_mv.txt`, videos skipped, headless, 900 s, no fault): every
  started buffer added sound to the mix, except a few looping buffers that the game stops in
  the frame it starts them (refbotlaser). Announcer: fuzionfrenzy, charname, selectroute,
  choosegame, getready 8, countdown 24, go 8, almostover 7, char1/3/5/6wins, teamwins, teamup,
  char5good, char1rubbish, powerstwrapup, endstage. Voices: jump 352, land 363, hurt 213,
  knockovr 43, score 19, respawn 14, taunt 8, first 6, revenge 4, pickedup 4, steal 4, stolen 1,
  selected 1; all heard but 2 land and 1 hurt (3D, out of earshot or cut off). Music: 10 WMA tracks (fuzionfrenzy, boom,
  chemical_brain, hey_fatty, casino_royale, geena, lullaby_for_broadmore, sidewinder,
  the_ease_of_tomorrow, rok_yer_world), -11 to -13 dBFS. WAV: RMS -22.6 dBFS, peak -1.0 dBFS, 0 %
  clipped. 66 silent seconds of 899: 2 are scene changes (505 s, 684 s); 64 are from 707 s, where the
  script presses START every 3 s and the pause menu stops the music each time.
- Effects that start while the effects volume (0x00285650) is 0 are silent: the game fades the
  effects volume (0x00071c20) during loads. This is original behaviour.
- No port bug was found in sample loading or playing, so no game code was changed.
- Not sound, not investigated: 235 `Could not find '' in memory` (empty name) and 6 `dub.obe`
  misses on the score screen after CharacterSel/Meta are closed.
- Not verified: nobody listened; no run with a real audio device (sandbox); the Welsh
  commentary path (0x000716a0) was not run.

## Bink video (integration round 3)

- `src/dsound/bink_host.c` implements the Bink entry points; the host decodes with FFmpeg 8
  (`port/macos/host/host_bink.c`, imports `host_bink_*`). The guest passes the disc image's
  file descriptor and the .bik byte range; the host reads it with pread (no copy in guest
  memory). Frames go to the game's LIN_ARGB8888 texture as B, G, R, A (BINKSURFACE32, flag 3).
  The first audio track (44.1 kHz stereo in BLITZ60.bik and ATTRACT.bik; MICROSOFT.bik has
  none) goes to a DirectSound stream (16-bit PCM packets); BinkDoFrame calls
  DirectSoundDoWork, since the game's loop waits in bdVideoPlayStream. Frame timing is the
  wall clock from the first BinkDoFrame.
- Verified (window, frame dumps every 30 frames, timestamped log): boot shows MICROSOFT.bik
  (105 frames, 3500 ms), BLITZ60.bik (172 frames, 5733 ms), ATTRACT.bik (1599 frames at 25
  fps), then "Press START to play". After 30 s idle at the title (DAT_00283760 = 1800
  frames) ATTRACT.bik plays again. START at 25 s stopped the boot ATTRACT.bik after 395
  frames; START at 80 s stopped the idle one after 628 frames; both return to the title.
  The DirectSound log shows one voice with real peaks during BLITZ60 and ATTRACT. Nobody
  listened to the sound; lip sync was not checked.
- Boot now takes about 75 s to the title without input. Scripted runs that assume the title
  at once need `OPENFRENZY_DEBUG_SKIP_VIDEOS=true` (or a START press per video).
- New path, open fault: the attract cycle now goes on to a demo minigame (CGameMode_AttractMode
  alternates video and demo). In 4 of 4 runs that reached a demo it chose RollerBall, which faults at
  NULL + 0x78 during loading: baInitActorInstance <- CRollerBallGoal_Init <- FUN_00195930 <-
  FUN_001965a0 <- CRollerballWorld_LoadLOM <- CRollerBallGame_Initialise <-
  CGameMode_MiniGameIntro_Render (about 30 s after the second title). Direct launch of
  RollerBall does not fault (round 3 sweep), so the demo set-up differs. Not fixed (rollerball).

## Library identifications

- 0x1e0323 `_purecall`; 0xa6fa0 returns 1; 0xf8ab0 returns 0 (stdcall, 1 argument).
- 0x0005a090 `RADSetMemory` (XbSymbolDatabase names it D3D_SetPushBufferSize); 0x000591c0
  `BinkOpen`; 0x00059f80 `BinkClose`; from bdVideoPlayStream's calls: 0x000585c0
  `BinkDoFrame`, 0x00057fb0 `BinkCopyToBuffer`, 0x00059e20 `BinkNextFrame`, 0x00058b60
  `BinkWait`, 0x00058e00 `BinkGetSummary`. All in `src/dsound/bink_host.c` (see "Bink video").
- Register `src/dsound/tests/run_tests.sh` in ctest. Update docs/LIBRARY_CALLS.md for the 10 dsound
  entry points that are now implemented.
