# port/macos

The Darwin host and the guest compiler pass. Design: `docs/ARCHITECTURE.md`. Build: section 10 there.

## Attribution

Copied from the OpenCE Mac fork `zimm3rmann/halo-ce-ios-macos` at commit `e55684a`
(local checkout `ref/OpenCE-mac-zimm3rmann`), released under CC0 1.0. Each copied file starts with a
comment that names its source path.

| Here | Source |
|---|---|
| `compiler/guest_rebase.cpp` | `port/macos/compiler/guest_rebase.cpp` (pass renamed `halo-rebase` -> `of-rebase`) |
| `host/` | `port/macos/host/` (`HALO_*` -> `OPENFRENZY_*`; `posix_net.c` left out, no System Link) |
| `include/openfrenzy_guest_abi.h` | `port/android/include/halo_android_abi.h` (magic is now `'OFRZ'`) |
| `host_imports.list`, `guest.ld` | `port/android/host_imports.list`, `port/android/guest/guest.ld` |
| `tools/asm_convert.py`, `tools/host_imports.py` | `tools/android_asm_convert.py`, `tools/android_imports.py` |
| `tools/guest_cc.py` | `tools/macos_guest_cc.py` (extended to the full compile pipeline) |
| `dependencies.json`, `licenses/ANGLE.txt` | `port/macos/dependencies.json`, `port/macos/licenses/ANGLE.txt` |

`host/host_syscall.c` includes the guest musl syscall numbers from
`port/guest/libc/arch/arm64_32/bits/syscall.h.in`, and the host includes `port/xdk/src/posix.h`.
