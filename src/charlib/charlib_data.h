/*
CHARLIB_DATA.H

charlib module. Access to the data blocks that charlib owns, with the original (Xbox)
layout (same design as src/core/core_data.h and src/classlib_ui/classlib_ui_data.h). The
blocks are defined in CharLib/charlib_data.c, which tools/gen_charlib_data.py generates
from orig/default.xbe:

  of_chl_data   0x00292c48-0x00293a50  .data of CharLib text block 1 (XBE values)
  of_chl_data2  0x00293e80-0x00293f14  .data of CharLibShadow.cpp (XBE values)
  of_chl_data3  0x00294afc-0x00294bd0  .data of the CCamera.cpp block (XBE values)
  of_chl_bss    0x002f27d4-0x002f6b80  .bss of CharLib text block 1 (zero; 0x002f27c8-0x002f27d4
                is the CRay list, classlib_fx of_clf_ray_bss)
  of_chl_bss2   0x002f6c10-0x002f6d20  .bss of CharLibShadow.cpp (zero)
  of_chl_bss3   0x002f7a20-0x002f7a44  .bss of the CCamera.cpp block (zero)

The CharLib object files are linked between CarryOn object files, so their data is
interleaved with CarryOn data; the block limits follow which modules' functions read each
address (CarryOn data at 0x00293a50-0x00293e7f, 0x00293f14-0x00294afb, 0x002f6b80-0x002f6c0f
and 0x002f6d20-0x002f7a1f is not ours).

charlib code names a global by its original address through CHL_VAR / CHL_ARR, or through
the readable names in charlib.h. Other modules keep the Ghidra names (DAT_00292c4c):
charlib_data.c makes those names aliases into the blocks.

Never put an original address into a pointer: CHL_ADDR turns it into an offset.
*/

#ifndef OPENFRENZY_CHARLIB_DATA_H
#define OPENFRENZY_CHARLIB_DATA_H

#include <stdint.h>

extern uint32_t of_chl_data[];
extern uint32_t of_chl_data2[];
extern uint32_t of_chl_data3[];
extern unsigned char of_chl_bss[];
extern unsigned char of_chl_bss2[];
extern unsigned char of_chl_bss3[];

#define OF_CHL_DATA_START 0x00292c48u
#define OF_CHL_DATA_END 0x00293a50u
#define OF_CHL_DATA2_START 0x00293e80u
#define OF_CHL_DATA2_END 0x00293f14u
#define OF_CHL_DATA3_START 0x00294afcu
#define OF_CHL_DATA3_END 0x00294bd0u
#define OF_CHL_BSS_START 0x002f27d4u
#define OF_CHL_BSS_END 0x002f6b80u
#define OF_CHL_BSS2_START 0x002f6c10u
#define OF_CHL_BSS2_END 0x002f6d20u
#define OF_CHL_BSS3_START 0x002f7a20u
#define OF_CHL_BSS3_END 0x002f7a44u

#define CHL_IN(addr, s, e) ((addr) >= (s) && (addr) < (e))

/* the host address of original address ADDR inside a charlib block (ADDR must be a
constant inside one block; anything else yields a null pointer) */
#define CHL_ADDR(addr) \
	((void *)(CHL_IN(addr, OF_CHL_BSS_START, OF_CHL_BSS_END) \
			? of_chl_bss + ((addr) - OF_CHL_BSS_START) \
		: CHL_IN(addr, OF_CHL_BSS2_START, OF_CHL_BSS2_END) \
			? of_chl_bss2 + ((addr) - OF_CHL_BSS2_START) \
		: CHL_IN(addr, OF_CHL_BSS3_START, OF_CHL_BSS3_END) \
			? of_chl_bss3 + ((addr) - OF_CHL_BSS3_START) \
		: CHL_IN(addr, OF_CHL_DATA_START, OF_CHL_DATA_END) \
			? (unsigned char *)of_chl_data + ((addr) - OF_CHL_DATA_START) \
		: CHL_IN(addr, OF_CHL_DATA2_START, OF_CHL_DATA2_END) \
			? (unsigned char *)of_chl_data2 + ((addr) - OF_CHL_DATA2_START) \
		: CHL_IN(addr, OF_CHL_DATA3_START, OF_CHL_DATA3_END) \
			? (unsigned char *)of_chl_data3 + ((addr) - OF_CHL_DATA3_START) \
		: (unsigned char *)0))

/* the global at original address ADDR, as an lvalue of TYPE */
#define CHL_VAR(type, addr) (*(type *)CHL_ADDR(addr))
/* the array of TYPE that starts at original address ADDR */
#define CHL_ARR(type, addr) ((type *)CHL_ADDR(addr))

#endif
