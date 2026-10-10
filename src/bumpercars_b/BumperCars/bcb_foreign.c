/*
BumperCars/bcb_foreign.c  -  weak fallbacks for initialised globals of other modules that no
module defines yet. An owner's strong definition replaces a fallback at link time; the values
are the XBE's.
*/

#include "bumpercars_b.h"

/* 0x00285634  the identity quaternion (0, 0, 0, 1) (bkernel maths data, after the rand seed
0x00285630) */
__attribute__((weak)) float DAT_00285634[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
