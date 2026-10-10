# Architecture

This document is the contract for the native port. Every agent and contributor follows it.
If you must deviate, write the reason here first.

The design is adopted from the OpenCE Halo: Combat Evolved Mac ports (CC0):

- `ref/OpenCE-mac-zimm3rmann/port/macos/README.md` — guest/host design, memory model.
- `ref/OpenCE-mac-innerhat/docs/apple-build.md` — toolchain and build setup.
- `ref/OpenCE/port/linux/src/` — the XDK replacement layer (D3D8, DirectSound, kernel, XAPI, files).

Both games use the same XDK era (3911/3925), so the OpenCE platform layer applies with small changes.

## 1. Goal and non-goals

- Goal: run Fuzion Frenzy natively on Apple Silicon macOS (arm64), from decompiled C/C++.
- Functional decompilation. We do not try to match the original bytes.
- No emulator, no x86 translation at runtime. The XBE is an input for analysis only.
- No game data in git. The player supplies `orig/game.xiso.iso` (or an extracted copy).

## 2. Overview

```text
+--------------------------------------------------------------+
|  guest image (arm64_32, ILP32: 32-bit pointers and long)      |
|   src/game, src/engine     decompiled code, original layouts  |
|   src/platform             XDK replacement (D3D8, DSOUND,     |
|                            XAPI, kernel, LIBC, XInput, files) |
|   musl libc (guest)                                           |
|   compiled: clang -emit-llvm -> opt guest_rebase -> llc       |
+---------------------------- host_* calls --------------------+
|  Darwin host (native arm64, Apple clang)                      |
|   loader, 4 GB arena, threads, files, time, errno             |
|   SDL3 (window, input, audio)                                 |
|   GLES 3 via ANGLE -> Metal                                   |
+--------------------------------------------------------------+
```

The game code is compiled as a **guest** with 32-bit data layouts. A small native **host** provides
the operating system services. This is the OpenCE design. We reuse it instead of inventing our own.

## 3. Why a 32-bit-layout guest

The Xbox game stores pointers in 32-bit struct fields, file formats and packed arrays. Ghidra output
assumes `sizeof(void *) == 4` and `sizeof(long) == 4`. A normal arm64 Mac process (LP64) cannot keep
these layouts, and macOS does not map the low 4 GB for user code.

Extra RAM does not fix this. The fix is to keep ILP32 layouts and move every memory access into a
reserved virtual region:

- The guest is compiled for the LLVM target `arm64_32-apple-watchos` (ILP32 on AArch64).
- The guest uses 32-bit pointers, 32-bit `long`, and the original struct offsets.
- So decompiled structs can copy Ghidra's offsets directly. Do not "fix" them to 64-bit.

## 4. Guest compiler: LLVM 22 plus `guest_rebase`

Toolchain (Homebrew): `llvm@22`, `lld@22`, `cmake`, `ninja`, `sdl3`. The pass needs **LLVM 22**.
Apple clang and newer LLVM majors are not supported. Apple clang compiles only the host.

Pipeline for each guest translation unit (see `ref/OpenCE-mac-zimm3rmann/tools/macos_guest_cc.py`):

1. `clang -target arm64_32-... -emit-llvm` produces LLVM IR with 4-byte pointers.
2. `opt -load-pass-plugin=guest_rebase.dylib -passes=<rebase>,verify` runs the rebase pass.
3. `llc -O2 -mtriple=arm64_32-apple-watchos -emulated-tls` produces assembly.
4. `ld.lld` links all guest objects and guest musl into one ELF image (`*_guest.elf`).

The pass (copy from `ref/OpenCE-mac-zimm3rmann/port/macos/compiler/guest_rebase.cpp`) does this:

- Every load, store, and indirect call goes through a 64-bit pointer in LLVM address space 272,
  computed as `BIAS | (uint32_t)ptr`.
- It lowers varargs and memory intrinsics (`memcpy`, `memset`) explicitly.
- Declarations named `host*` get 64-bit pointer parameters, so the host receives real addresses.
  Opaque GL offsets and handles keep their 32-bit value (the `isHandle` list).
- It checks that pointer size is 4 and address space 272 is 8, and fails the build otherwise.

The pass is an ABI adapter. It is not a sandbox. Only our own compiled code runs as the guest.

Known trap (from OpenCE): ILP32 instruction selection can add the bias twice when it splits an
unaligned 64-bit load from a stack pointer. Keep their machine-level truncation fix and regression test.

Rename the Halo-specific names (`HALO_*`, `halo-rebase`) to an `OPENFRENZY_*` / `of-rebase` prefix
when copying. Keep the logic unchanged.

## 5. Memory: the 4 GB arena

- The host reserves a 4 GB **virtual** arena at `0x10000000000` (1 TB). `guest_pointer(p)` is
  `BIAS | (uint32_t)p`. Guest address 0 stays NULL.
- Reservation is not RAM use. Pages are committed on demand.
- macOS pages are 16 KB. Xbox allocations are 4 KB. Several Xbox allocations share one host page, and
  page protection combines their needs (see `host/host_memory.c`). Texture write tracking therefore
  has 16 KB granularity.
- After `MADV_DONTNEED`, recreate fresh mappings. Darwin does not guarantee zero-filled pages otherwise.
- The low 16 MB of the arena stays unmapped to catch NULL-relative access.
- The guest image, guest heap and the Xbox physical-memory window (used by D3D8 resources and
  `MmAllocateContiguousMemory`) live inside the arena. Copy the layout from OpenCE `host_memory.c`
  and `xbox_memory.c`; record any change in this section.

Original XBE addresses (`0x00011000`...) are **not** reused as guest addresses. Code is recompiled;
the original address survives only in comments and in tables that map data (for example, vtables
and jump tables) back to recompiled functions.

## 6. Darwin host

Copy from `ref/OpenCE-mac-zimm3rmann/port/macos/host/` into `port/macos/host/`:

| File | Job |
|---|---|
| `entry.s`, `host_loader.c` | Load the guest ELF into the arena, relocate it, enter it on a guest stack. |
| `host_main.c` | Process start, data paths, signal handlers. |
| `host_memory.c` | Arena, 16 KB / 4 KB page sharing, protection. |
| `host_thread.c` | Threads, futex, TLS for guest threads. |
| `host_syscall.c`, `host_errno.c` | musl/Linux syscall ABI translated to Darwin. |
| `posix_files.c` | Files and directories. |
| `host_sdl.c` | SDL3 window, events, gamepad, audio device. |
| `host_gl.c`, `host_gl_bridge.c` | GLES 3 entry points, implemented by ANGLE. |
| `host_perf.c` | Frame timing log. |

- Window, input and audio use **SDL3** (3.4.x).
- Rendering uses **OpenGL ES 3** from **ANGLE with the Metal backend**. ANGLE comes as pinned
  prebuilt libraries with SHA-256 checks (`port/macos/dependencies.json`). Keep `licenses/ANGLE.txt`.
- SDL video stays on the main thread. Audio mixing runs on a guest-stack worker; results go to the
  SDL audio callback thread. This avoids the stream-lock deadlock OpenCE found.
- `posix_net.c` is not needed (no System Link in round 1).
- The app is ad-hoc signed. No Apple developer account is required.

## 7. Guest platform layer (XDK replacement)

Our XBE links these XDK libraries statically. We do **not** decompile them. Each one is replaced by
the OpenCE equivalent from `ref/OpenCE/port/linux/src/`, adapted under `src/platform/`:

| XBE section / region | Replaced by (OpenCE source) |
|---|---|
| `D3D` (D3D8 1.0.3925), `XGRPH` | `d3d8_gl.c`, `d3d8_resources.c`, `nv2a_vsh.c`, `nv2a_psh.c`, `xbox_textures.c` |
| `DSOUND` (3925) | `dsound_sdl.c` |
| `WMADEC`, `WMADECXM` | stub first (check which game paths decode WMA) |
| `XPP` | `xinput_sdl.c` |
| `DOLBY` | none (data for the DSP; not needed with SDL audio) |
| `.text` region `xapi` (XAPILIB, CRT start) | `xbox_xapi.c`, `xbox_kernel.c`, `xbox_memory.c` |
| `.text` region `libc` | guest musl + `msvc_crt.c` |
| `.text` region `bink` (RAD Bink) | `bink_null.c` (skip videos) |
| disc access | `xbox_files.c`, `xiso.c` (reads `orig/game.xiso.iso` directly) |

Method: list each XDK entry point the game calls (callees of `.text` game functions that land in a
library section; names come from XbSymbolDatabase in `export/functions.jsonl`). Map each one to the
OpenCE function with the same XDK name. Write a stub that logs loudly for any entry point without an
equivalent. Use `ref/OpenCE/tools/linux_msvc_semantics.py` for MSVC-isms in decompiled code.

The GL renderer is GLES 3 (the OpenCE Android/ANGLE path), not desktop OpenGL 4.1.
`scripts/modules.py` still says "OpenGL 4.1" in the `d3d8` module title. That text is out of date;
this document wins. Fix the title when that script is next changed.

## 8. Source layout

```text
src/
  game/     <- C:\BlitzParty\Game\Src\   (Core, ClassLib, CharLib, Meta, one dir per minigame)
  engine/   <- C:\Babel\Xbox\Src\        (bKernel, BDISPLAY, BACTOR, BINPUT, BGUI, BMATHS, BSOUND)
  platform/ <- XDK replacement (d3d8, dsound, xapi, xinput, vfs, runtime), adapted from OpenCE
  include/  <- shared headers: decompiled structs, Xbox/XDK types
port/macos/
  compiler/ <- guest_rebase.cpp
  host/     <- Darwin host
tools/      <- build scripts (macos_build.py, guest_cc wrapper), from OpenCE, renamed
scripts/    <- analysis scripts (Ghidra, map_sources.py, modules.py)
```

Rules:

- A decompiled function goes in the file that matches its **original source path**, as found by
  `scripts/map_sources.py` (`export/source_map.json`). Example: `0x00027db0`, mapped to
  `Core/CGameMode_Root.cpp` under the game root, goes in `src/game/Core/CGameMode_Root.cpp`.
- Keep the original file name and case. Use `.cpp` for files that were `.cpp` (the game is C++ with
  classes and vtables).
- Confidence levels `tagged`, `bracketed`, `vtable` and `leading`/`trailing` give a file. Use it.
- A `gap` function has only candidate files. Put it in the directory that `scripts/modules.py`
  resolves, in `src/<root>/<Dir>/unplaced.cpp`. Move it to a real file when new evidence appears.
- Region `engine_tail` goes in `src/engine/unplaced_tail.cpp`. Region `crt_init` (static
  constructors) goes next to the global it constructs.
- `scripts/modules.py` (`export/modules.json`) groups the functions into **work modules** for agents
  (one module per agent task, in boot order). Modules decide who does what. They do not decide file
  paths. The `src/<module>` value in `modules.json` is a work label only.
- Regenerate the maps with `python3 -I scripts/map_sources.py` and then `python3 -I scripts/modules.py`.

## 9. Rules for decompiled code

- **Each function keeps a comment with its original address**, and the original source file when known:

  ```c
  // 0x00027db0  Core/CGameMode_Root.cpp
  void CGameMode_Root::Update() { ... }
  ```

  Data with a known original address (globals, vtables, string tables) gets the same comment.
- Keep original layouts. Struct offsets come from Ghidra and must not change. Add
  `static_assert(offsetof(...) == 0x..)` and `static_assert(sizeof(...) == 0x..)` for structs that are
  read from game files or shared with the platform layer.
- Do not use `uintptr_t` casts to 64-bit types in guest code. Pointers are 4 bytes in the guest.
- Do not call Darwin, SDL or GL from game or engine code. Only `src/platform/` talks to the host, and
  only through `host*` functions declared for the rebase pass.
- Name unknown functions `FUN_<addr>` (Ghidra name) until the purpose is clear. Rename then, and keep
  the address comment.
- Input for decompilation: `export/funcs/<addr>.c` (Ghidra C) and `export/functions.jsonl`
  (callers, callees, strings). Use `scripts/DecompileAt.java` with headless Ghidra (`-readOnly`) when a
  function is missing from the export.

## 10. Build

CMake plus Ninja. The build skeleton works (integration round 1): an empty guest `main()` builds
and runs in the host arena.

```sh
brew install python cmake ninja llvm@22 lld@22 sdl3
cmake -S . -B build-<name> -G Ninja        # first run downloads ANGLE + Khronos headers to deps/
cmake --build build-<name>
ctest --test-dir build-<name> --output-on-failure
```

| Path | Job |
|---|---|
| `CMakeLists.txt` | Host project (Apple clang): the `of-rebase` pass, the `openfrenzy` host, guest sub-projects, tests. |
| `cmake/guest-toolchain.cmake` | Toolchain for guest sub-projects: compiler `port/macos/tools/guest_cc.py`, linker `ld.lld` with `port/macos/guest.ld`. |
| `cmake/Dependencies.cmake` | Pinned, SHA-256-checked ANGLE and Khronos headers from `port/macos/dependencies.json`. |
| `port/macos/tools/guest_cc.py` | One guest compile: clang -emit-llvm -> opt `of-rebase` -> llc -> `asm_convert.py` -> ELF object. |
| `port/macos/tools/host_imports.py` | Guest import stubs and the host's import table, from `port/macos/host_imports.list`. |
| `port/macos/tests/skeleton/` | Guest without musl: `skeleton.c` (empty `main`, exit 0) and `probe.c` (rebased memory, exit 42). |

- Outputs: `build-<name>/openfrenzy` (host), `build-<name>/*.elf` (guest images).
  Run one with `build-<name>/openfrenzy build-<name>/openfrenzy_skeleton.elf`.
- `OPENFRENZY_LLVM_BIN` (cache variable) selects LLVM 22. `ld.lld` is searched in `lld@22` first.
- ANGLE has no Homebrew formula. It is the pinned prebuilt download from OpenCE (section 6).
- The tests set `SDL_VIDEO_DRIVER=dummy`, so they also run without a window server (sandbox, CI).
- Build scripts live in `port/macos/tools/`, not `tools/`: `tools/` is git-ignored (Ghidra).
- Each agent uses its own build directory (`build-<task>`), to allow parallel work.
- Build outputs, `deps/`, game data, `orig/`, `export/`, `ref/` and `tools/ghidra_*` stay out of git.

Not done yet: guest musl (`libguestc.a`), the generated GL and posix import stubs (OpenCE
`android_gl_stubs.py`, `android_posix_stubs.py`), the compiler runtime (`libclang_rt.builtins`) in the
guest link, the game guest project, and the `.app` bundle.

## 11. Attribution

Code copied from OpenCE and its Mac forks is CC0. Keep a header comment in each copied file with the
source repository and path, for example `// Adapted from OpenCE port/linux/src/d3d8_gl.c (CC0)`.
Third-party licenses (ANGLE, SDL3, musl) go in `port/macos/licenses/`.

## 12. Status (when this file was written)

- Done: Ghidra export, source map (`export/source_map.json`), work modules (`export/modules.json`).
- Done (round 1): `CMakeLists.txt`, guest toolchain, `port/macos/` (host, compiler pass), skeleton tests.
- Not done: no `src/` yet; see the "Not done yet" list in section 10.
- Open: the exact arena layout for the Xbox physical-memory window; the WMA decoder need; the XDK
  entry-point map (section 7).
