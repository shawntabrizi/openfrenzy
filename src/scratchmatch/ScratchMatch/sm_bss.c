/*
ScratchMatch/sm_bss.c  (module scratchmatch)

The ScratchMatch .bss, 0x0030fe90-0x00312f50, in its original layout (see scratchmatch.h,
SM_ADDR). The block starts 4 bytes before the first address the code uses (0x0030fe94) so that
it keeps the original 16-byte alignment of every address in it. It ends where core's static
object 0x00312f50 starts.
*/

#include "scratchmatch.h"

unsigned char sm_bss[SM_BSS_HI - SM_BSS_LO] __attribute__((aligned(16)));
