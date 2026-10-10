# XDK replacement (HLE) layer

This file describes the OpenCE-derived XDK layer. It first lived in `port/xdk/`. It is now split
over the platform modules `src/runtime` (shared headers `include/`, `compat/`, `entry/`,
`platform.h`, `msvc_crt.c`), `src/vfs` (`xiso.c`, `xbox_files.c`, `posix.h`),
`src/xkernel` (`xbox_kernel.c`, `xbox_xapi.c`, `xbox_memory.c`), `src/d3d8`
(`d3d8_*.c`, `nv2a_*.c`, `xbox_textures.c`, `gl.h`, `xgpu.h`), `src/dsound` (`dsound_sdl.c`)
and `src/input` (`xinput_sdl.c`). The table below gives the original OpenCE paths.

The XBE statically links the Xbox SDK libraries (sections `D3D`, `DSOUND`,
`XGRPH`, `XPP`, plus the kernel/XAPI imports). The port does not decompile
them. Game code is routed to this layer instead.

## Origin and attribution

Everything in `src/` and `include/` is copied from **OpenCE**, the Halo: Combat
Evolved decompilation and port (CC0 1.0, `ref/OpenCE`, commit
`231708d2e0ea651687373925cf434d520f45c877`):

| Here | OpenCE |
|---|---|
| `src/*.c`, `src/*.h` | `port/linux/src/` |
| `include/xdk/` (clean-room XDK headers), `include/halo_math.h` | `port/include/` |
| `include/linux/` (MSVC/XDK prefix and CRT headers) | `port/linux/include/` |
| `compat/{hud_hires,menu_files,text_hires,touch_input,voice_audio}.h` | `port/linux/src/` (declarations only) |
| `../guest/libc/arch/arm64_32/` (musl arch for the ILP32 guest) | `port/android/guest/libc/arch/arm64_32/` |
| `../../scripts/linux_msvc_semantics.py` | `tools/linux_msvc_semantics.py` |

The arm64_32 musl arch headers derive from musl (MIT). OpenCE's own
`LICENSE.md` is CC0 1.0.

Copied files keep their OpenCE names and comments ("Linux", "Halo"). Changes
made here:

- `src/xbox_memory.c`: removed the Halo Custom Edition tag-cache reservation.
- `src/xbox_textures.c`: includes `compat/halo_cache_file_formats.h` in place
  of Halo's `../game/cache_file_formats.h`.
- `compat/halo_hooks.c`: inert versions of the Halo-only hooks the layer calls
  (high-res HUD/text, PC menu art, touch controls, voice chat, debug console,
  render interpolation, split-screen menus).

## Entry-point table

`entry/xdk_entry_points.inc` has one row per named function in `D3D`, `DSOUND`,
`XGRPH` and `XPP` (from `export/functions.jsonl`), plus each unnamed one that
game code calls. `entry/xdk_entry_points.c` turns it into
`xdk_entry_points[]` and `xdk_entry_point_find(address)`.

Row kinds: `HLE` (implemented here), `MISSING` (game calls it, no HLE yet),
`UNKNOWN` (game calls it, not identified), `INTERNAL` (only called inside the
library). `MISSING` and `UNKNOWN` rows point to a stub that calls
`platform_unimplemented` and returns 0. `scripts/gen_xdk_entry_points.py` regenerates the file. Hand
identifications of unnamed functions go in its `IDENTIFIED` map.

## Regenerating the entry-point table

```sh
cmake -S . -B build-runtime -G Ninja          # once
src/runtime/build_check.sh [build-dir]        # default: build-runtime
```

It builds the platform layer with the CMake build (guest flags, the of-rebase
pass), lists the symbols `libofplatform.a` defines (`llvm-nm`), merges
`export/functions.jsonl` with `export/functions_extra.jsonl` (calls from
functions that only the second Ghidra pass found are added to the callers),
runs `scripts/gen_xdk_entry_points.py`, and rebuilds. No network is needed.
The earlier version compiled the layer without the rebase pass and downloaded
musl and the GLES headers; the CMake build replaces it.

## Runtime module tests

```sh
src/runtime/tests/run_tests.sh [build-dir] [test_name ...]   # headless
SDL_VIDEO_DRIVER= OPENFRENZY_WINDOWED=1 src/runtime/tests/run_tests.sh build-runtime test_d3d_window
```

Each `tests/test_*.c` is a guest image compiled with the platform layer's
flags and run in the host; it passes when the guest exits 0. Tests that fail
because of another module are listed in `tests/known_failures.txt` (reported
as XFAIL, and as XPASS once fixed). A real window needs the window server, so
the second command runs outside the Claude Code sandbox.

| Test | What it proves |
|---|---|
| `test_config` | settings: defaults, `OPENFRENZY_*` overrides, writes; data/save roots are the host's |
| `test_d3d_window` | the game's `bStartDisplay` device creation and 120 Clear/Present frames; with a window: GLES context, drawable, no GL error (debug screenshots match the clear colours) |
| `test_d3d_present_order` | the same without starting the vertical blank thread (known d3d8 failure) |
| `test_input_events` | keyboard/mouse state and the debug-keyboard queue (virtual keys, XDK flags) |
| `test_msvc_crt` | MSVC CRT behaviour: wide `%S`, `%I64d`, `_stricmp`, `_itoa`, `_splitpath` |
| `test_unimplemented` | the stub log counts each function once across threads |
| `test_disc_read` | `CreateFileA`/`ReadFile` on `D:\` of the real disc image (needs vfs) |
| `test_toolchain_strings` | string literals keep a leading `_` (known toolchain failure) |
