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

## Compile check

```sh
src/runtime/build_check.sh [build-dir]   # default: build-fix-platform-layer
```

It compiles every file with the guest ABI flags of OpenCE's
`tools/android_build.py` (`--target=arm64_32-apple-watchos`, ILP32, musl
1.2.5 headers, Khronos GLES 3.2 headers, SDL3 headers from Homebrew). musl
and the GLES headers are downloaded into the build directory on first run.

It produces objects only. It does not run the `guest_rebase` LLVM pass and
does not link: the musl runtime, guest runtime, Darwin host, and the rest of
the OpenCE platform layer are not in the tree yet. Undefined symbols left
after this layer: libc/pthread, `halo_gl*` (`gl_functions.c`), `platform_*`
(`sdl_platform.c`), `config_*` (`port_config.c`), `posix_*` and `host_gl_*`
(host), `memory_watch_*`, `xgpu_post_*`/`xgpu_text_*`, `halo_sin`/`halo_cos`
(`port/third_party/musl-math`).
