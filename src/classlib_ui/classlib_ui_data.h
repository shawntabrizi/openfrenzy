/*
CLASSLIB_UI_DATA.H

classlib_ui module. Access to the data blocks that classlib_ui owns, with the original
(Xbox) layout (same design as src/core/core_data.h). The blocks are defined in
ClassLib/classlib_ui_data.c, which tools/gen_classlib_ui_data.py generates from
orig/default.xbe:

  of_clu_data   0x0028fda8-0x00290cc0  .data of ClassLib block 1 (XBE values)
  of_clu_data2  0x00290d14-0x00290d54  alias of classlib_fx of_clf_data + 0x54 (ray type 1)
  of_clu_bss    0x002ea490-0x002ed33c  .bss of ClassLib block 1 (zero)

classlib_ui code names a global by its original address through CLU_VAR / CLU_ARR, or
through the readable names below. Other modules keep the Ghidra names (DAT_0028ffb8):
classlib_ui_data.c makes those names aliases into the blocks.

Never put an original address into a pointer: CLU_ADDR turns it into an offset.
*/

#ifndef OPENFRENZY_CLASSLIB_UI_DATA_H
#define OPENFRENZY_CLASSLIB_UI_DATA_H

#include <stdint.h>

extern uint32_t of_clu_data[];
/* 0x00290d14-0x00290d54 is entry 1 of classlib_fx's ray type table (0x00290cc8, 0x4c bytes
each): one storage with classlib_fx, because 0x000846c0 reads the entry's texture at +0x44 */
extern uint32_t of_clf_data[];   /* classlib_fx: 0x00290cc0-0x00290ee4 */
#define of_clu_data2 ((uint32_t *)((unsigned char *)of_clf_data + (0x00290d14u - 0x00290cc0u)))
extern unsigned char of_clu_bss[];

#define OF_CLU_DATA_START 0x0028fda8u
#define OF_CLU_DATA_END 0x00290cc0u
#define OF_CLU_DATA2_START 0x00290d14u
#define OF_CLU_DATA2_END 0x00290d54u
#define OF_CLU_BSS_START 0x002ea490u
#define OF_CLU_BSS_END 0x002ed33cu

/* the host address of original address ADDR inside a classlib_ui block (ADDR must be a
constant inside one block; anything else yields a null pointer) */
#define CLU_ADDR(addr) \
	((void *)((addr) >= OF_CLU_BSS_START && (addr) < OF_CLU_BSS_END \
			? of_clu_bss + ((addr) - OF_CLU_BSS_START) \
		: (addr) >= OF_CLU_DATA_START && (addr) < OF_CLU_DATA_END \
			? (unsigned char *)of_clu_data + ((addr) - OF_CLU_DATA_START) \
		: (addr) >= OF_CLU_DATA2_START && (addr) < OF_CLU_DATA2_END \
			? (unsigned char *)of_clu_data2 + ((addr) - OF_CLU_DATA2_START) \
		: (unsigned char *)0))

/* the global at original address ADDR, as an lvalue of TYPE */
#define CLU_VAR(type, addr) (*(type *)CLU_ADDR(addr))
/* the array of TYPE that starts at original address ADDR */
#define CLU_ARR(type, addr) ((type *)CLU_ADDR(addr))

#endif
