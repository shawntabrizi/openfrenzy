/*
TANKEDUP_A_DATA.H

Access to the TankedUp data blocks, with the original (Xbox) layout. The blocks are
defined in TankedUp/tu_data.c, which tools/gen_tu_data.py generates from orig/default.xbe
(same design as src/clusteroids/clusteroids_data.h):

  of_tu_data   0x002b6950-0x002b74a4  .data: debris sprite definitions, debris types
  of_tu_bss    0x00312f50-0x003153a0  .bss: CTank_Game (0x00312f50, 0x2160 bytes), the
                                      tankedup_b debug object (0x003150b0, 0x158 bytes),
                                      vertex buffers and option flags

TankedUp code names a global by its original address: TU_VAR(int, 0x003150bc). Every
access goes through the block object, so the compiler never assumes that two fields of a
block are different objects (the original code indexes across them).

Other modules reach these addresses through their Ghidra names (DAT_00312f50, DAT_003150b0,
...): tu_data.c exports them as aliases into the blocks.

Never put an original address into a pointer: TU_VAR / TU_ADDR turn it into an offset.
Owner: tankedup_a.
*/

#ifndef OPENFRENZY_TANKEDUP_A_DATA_H
#define OPENFRENZY_TANKEDUP_A_DATA_H

#include <stdint.h>

extern uint32_t of_tu_data[];
extern unsigned char of_tu_bss[];

#define OF_TU_DATA_START 0x002b6950u
#define OF_TU_DATA_END 0x002b74a4u
#define OF_TU_BSS_START 0x00312f50u
#define OF_TU_BSS_END 0x003153a0u

/* the host address of original address ADDR inside a TankedUp block (ADDR must lie inside
one block; anything else yields a null pointer) */
#define TU_ADDR(addr) \
	((void *)((addr) >= OF_TU_BSS_START && (addr) < OF_TU_BSS_END \
			? of_tu_bss + ((addr) - OF_TU_BSS_START) \
		: (addr) >= OF_TU_DATA_START && (addr) < OF_TU_DATA_END \
			? (unsigned char *)of_tu_data + ((addr) - OF_TU_DATA_START) \
		: (unsigned char *)0))

/* the TankedUp global at original address ADDR, as an lvalue of TYPE */
#define TU_VAR(type, addr) (*(type *)TU_ADDR(addr))

#endif
