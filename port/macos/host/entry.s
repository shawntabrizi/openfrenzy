// Adapted from ref/OpenCE-mac-zimm3rmann (github.com/zimm3rmann/halo-ce-ios-macos @ e55684a) port/macos/host/entry.s (CC0). Renamed HALO_* to OPENFRENZY_*.
.text
.globl _macos_enter_guest_stack
.p2align 2
_macos_enter_guest_stack:
 mov sp, x0
 mov w0, w1
 bl _host_run_guest_main
 brk #0
