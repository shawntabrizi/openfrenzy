/*
ROWING_B_DATA.H

Access to the Rowing data blocks with the original (Xbox) layout. The blocks are defined in
Rowing/rowing_data.c, which tools/gen_rowing_data.py generates from orig/default.xbe (same
design as src/clusteroids/clusteroids_data.h):

  of_row_data  0x002b5474-0x002b5b60  .data of the Rowing objects (XBE values)
  of_row_bss   0x0030ef08-0x0030fe94  .bss of the Rowing objects (zero)

The rowing_a and rowing_b modules share these blocks. rowing_b owns the file; rowing_a (and
any other module) names a global by its Ghidra name (extern DAT_0030f0b0): every Ghidra name
inside a block is an assembler alias into it.

rowing_b code reaches a global by its original address: RW_VAR(int, 0x0030f0b0). Every access
goes through the block object, so the compiler never assumes that two fields of a block are
different objects (the original code indexes across them).

Never put an original address into a pointer: RW_VAR / RW_ADDR turn it into an offset.
*/

#ifndef OPENFRENZY_ROWING_B_DATA_H
#define OPENFRENZY_ROWING_B_DATA_H

#include <stdint.h>

extern uint32_t of_row_data[];
extern unsigned char of_row_bss[];

#define OF_ROW_DATA_START 0x002b5474u
#define OF_ROW_DATA_END 0x002b5b60u
#define OF_ROW_BSS_START 0x0030ef08u
#define OF_ROW_BSS_END 0x0030fe94u

/* the host address of original address ADDR inside a Rowing block (ADDR must lie inside one
block; anything else yields a null pointer) */
#define RW_ADDR(addr) \
	((void *)((addr) >= OF_ROW_BSS_START && (addr) < OF_ROW_BSS_END \
			? of_row_bss + ((addr) - OF_ROW_BSS_START) \
		: (addr) >= OF_ROW_DATA_START && (addr) < OF_ROW_DATA_END \
			? (unsigned char *)of_row_data + ((addr) - OF_ROW_DATA_START) \
		: (unsigned char *)0))

/* the Rowing global at original address ADDR, as an lvalue of TYPE */
#define RW_VAR(type, addr) (*(type *)RW_ADDR(addr))

#endif
