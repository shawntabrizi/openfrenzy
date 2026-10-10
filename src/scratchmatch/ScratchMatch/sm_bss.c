/*
ScratchMatch/sm_bss.c  (module scratchmatch)

The ScratchMatch .bss, 0x0030fe90-0x00312f50, in its original layout (see scratchmatch.h,
SM_ADDR). The block starts 4 bytes before the first address the code uses (0x0030fe94) so that
it keeps the original 16-byte alignment of every address in it. It ends where core's static
object 0x00312f50 starts.
*/

#include "scratchmatch.h"

__attribute__((nocommon, aligned(16))) unsigned char sm_bss[SM_BSS_HI - SM_BSS_LO];

/* Core's static constructor 0x001f4db0 initialises the list at 0x0030fea4 (the root overlay
list of CGameMode_ScratchMatch_Start) by its Ghidra name: alias it into the block, so both
modules use one storage (integration round 2). */
__asm__(".globl DAT_0030fea4\n.set DAT_0030fea4, sm_bss + 0x14\n");
