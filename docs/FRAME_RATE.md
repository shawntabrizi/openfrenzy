# Frame rate above 60 fps: design note

Status: not implemented. The game stays at 60 fps (the default, and the only mode). This note
gives the timing model of the game, an experiment, the blockers and the work that a safe
implementation needs.

## How the game times a frame

- The main loop (`Core_Init`, 0x0002cd80, `src/core/Core/core.c`) does one update and one draw
  in each pass: `CGameModeStack_Update`, `CGameModeStack_Process`, then `Core_UpdateFrameTime`
  and `CGameModeStack_Draw` inside `BeginScene`/`EndScene`, then `CGameModeStack_PostDraw`.
- The step is variable. `Core_UpdateFrameTime` (0x0002cc90) counts the vertical blanks since the
  last frame (`D3DDevice_GetDisplayFieldStatus`), clamps the count to 1-5, and sets the frame
  factor `DAT_0028fda8 = blanks / g_RefreshRate * 60`. 1.0 is one 60 Hz frame.
- `g_RefreshRate` is 60, or 50 for PAL (`Core_ReadVideoSettings`, 0x0002c400). The PAL case
  gives a factor of 1.2: the engine was made to scale its motion by the factor.
- The port emulates the vertical blank with a 60 Hz thread (`vertical_blank_thread`,
  `src/d3d8/d3d8_gl.c`). `D3DDevice_Present` keeps at most two frames queued behind it. Thus
  the loop runs at 60 passes each second.
- `bkTimerRead` (bkernel) is a real clock. Some game clocks use it (the input repeat, the
  RollerBall and Hopper clocks); most use the frame factor.
- There is no fixed tick and no state kept from the previous update. Halo's method (OpenCE
  `display.interpolation`, `port/linux/game/render_interpolation.c`) blends two 30 Hz ticks.
  Fuzion Frenzy has no such ticks: each draw shows the state of the update just before it.

## Experiment (not committed)

A build with a 120 Hz vertical blank thread and `g_RefreshRate = 120` (factor 0.5), TankedUp,
vsync off, window: 117.8-118.4 fps (frame pacing log). The round clock went down 5 s each
600 frames (5 s of wall time). The minigame intro took about the same wall time as at 60 Hz.
The code change was reverted.

## Why it is not on

1. **Fractional steps are not proven safe (option A, "run the update faster").** 94 source
   files read the frame factor (506 uses). Code that counts frames and does not read it runs
   faster at a higher rate. Possible kinds: per-frame AI decisions, `rand()` once a frame, counters that add 1,
   animation frames that step by 1, and physics tuned to a step of 1. The PAL path tests a
   factor of 1.2 (larger steps), not factors below 1. Nobody has checked the 5623 functions for
   this. One clock that looks correct does not prove the gameplay is the same.
2. **True interpolation (option B, "draw between two updates") needs state the engine does not
   keep.** A draw would need the previous and the current transform of each thing drawn: the
   camera (`DAT_002be270`, `bdSetCamera*`), every actor instance matrix (bactor `inst->matrix`),
   particles (classlib_fx), overlays (`COverlay` positions and scales) and per-game objects
   drawn with their own matrices. There is no stable identity that matches a thing in two
   updates at the draw level. Also `CGameModeStack_Draw` is not known to be free of side
   effects (nobody has checked it), so it cannot simply run twice.
3. The default must stay exact. Each option changes the update or the draw for all 45
   variations, so it needs a gameplay test for each before it can be offered.

## Work needed for a safe implementation

Option A (higher update rate; smaller work, the better first step):

1. Make the vertical blank rate and `g_RefreshRate` one setting (for example
   `display.refresh_rate`, 60 by default), so that the factor is 60 / rate.
2. Find each update path that does not scale by the factor: search the decompiled C for
   counters and timers that change by a constant each frame, and for `rand()` calls in
   per-frame code. Scale or accumulate each one (step when the summed factor reaches 1).
3. Prove it with the differential-test harness (`tests/`): the same seed and pad script at
   60 Hz and at 120 Hz must give the same results in game time (scores, positions at fixed game
   times, round end). Run it for the 45 variations and the board.
4. Only then add the opt-in setting. Keep 60 as the default.

Option B (interpolation at the display rate, update at 60 Hz):

1. Keep two snapshots of the camera and of every actor instance matrix, keyed by instance
   pointer, at the end of each update.
2. Split `CGameModeStack_Draw` into a draw without side effects (move effect updates to
   `Process`), so that it can run more than once for one update.
3. Blend the snapshots by the time since the last update (quaternions for rotations, as Halo
   does), and snap on cuts and teleports.
4. Particles, overlays and per-game objects need the same treatment, or they stutter at 60 Hz
   against the smooth camera.
