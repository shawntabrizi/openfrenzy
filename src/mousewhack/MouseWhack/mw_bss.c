/*
MouseWhack/mw_bss.c  (module mousewhack)

The MouseWhack .bss, 0x0030a0a8-0x0030ef04, in its original layout (see mousewhack.h,
MW_ADDR). The six objects that core's static constructors build are tentative definitions
here too (core/crt/static_init.c has the same ones; -fcommon merges them and keeps the
largest size: the arena at 0x0030a1d0 is 0x4850 bytes, core sizes it 0x90).
*/

#include "mousewhack.h"

unsigned char mw_bss0[0x0030a0e4 - 0x0030a0a8] __attribute__((aligned(16)));
unsigned char DAT_0030a0e4[0xc] __attribute__((aligned(16)));
unsigned char DAT_0030a0f0[0x48] __attribute__((aligned(16)));
unsigned char mw_bss1[0x0030a1d0 - 0x0030a138] __attribute__((aligned(16)));
unsigned char DAT_0030a1d0[0x0030ea20 - 0x0030a1d0] __attribute__((aligned(16)));
unsigned char DAT_0030ea20[0x10] __attribute__((aligned(16)));
unsigned char mw_bss3[0x0030ed00 - 0x0030ea30] __attribute__((aligned(16)));
unsigned char DAT_0030ed00[0xc] __attribute__((aligned(16)));
unsigned char mw_bss4[0x0030ee30 - 0x0030ed0c] __attribute__((aligned(16)));
unsigned char DAT_0030ee30[0x10] __attribute__((aligned(16)));
unsigned char mw_bss5[0x0030ef04 - 0x0030ee40] __attribute__((aligned(16)));
