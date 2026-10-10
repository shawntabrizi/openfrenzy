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

## Boot path

- `bkRun` 0x4e690 (bkernel) is a stub, so nothing runs past the start of core_init.
- With a throwaway bkRun, boot faults in `CGameMode_Root_CreateDebugMenu`, because `g_Games[k]`
  is NULL while the 13 game constructors are stubs.

## Library identifications

- 0x1e0323 `_purecall`; 0xa6fa0 returns 1; 0xf8ab0 returns 0 (stdcall, 1 argument).
- 0x0005a090 `RADSetMemory` (XbSymbolDatabase names it D3D_SetPushBufferSize); 0x000591c0
  `BinkOpen`; 0x00059f80 `BinkClose`. Bink is the null stub; videos are skipped.
- Register `src/dsound/tests/run_tests.sh` in ctest. Update docs/LIBRARY_CALLS.md for the 10 dsound
  entry points that are now implemented.
