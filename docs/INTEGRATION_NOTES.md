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
