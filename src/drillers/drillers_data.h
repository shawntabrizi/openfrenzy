/*
DRILLERS_DATA.H

Access to the data blocks of the drillers module, with the original (Xbox) layout.
The blocks are defined in Drillers/drillers_data.c, which tools/gen_drillers_data.py
generates from orig/default.xbe (same design as src/clusteroids/clusteroids_data.h):

  of_dr_data   0x00298d50-0x002ab26c  .data of the Drillers objects (XBE values)
  of_dr_bss    0x002ff9e0-0x00302138  .bss of the Drillers objects (zero)

Drillers code names a global by its original address: DR_VAR(int, 0x002ff9fc). Every
access goes through the block object, so the compiler never assumes that two fields of a
block are different objects (the original code indexes across them).

Never put an original address into a pointer: DR_VAR / DR_ADDR turn it into an offset.
*/

#ifndef OPENFRENZY_DRILLERS_DATA_H
#define OPENFRENZY_DRILLERS_DATA_H

#include <stdint.h>

extern uint32_t of_dr_data[];
extern unsigned char of_dr_bss[];

#define OF_DR_DATA_START 0x00298d50u
#define OF_DR_DATA_END 0x002ab26cu
#define OF_DR_BSS_START 0x002ff9e0u
#define OF_DR_BSS_END 0x00302138u

/* the host address of original address ADDR inside a drillers block (ADDR must lie inside
one block; anything else yields a null pointer) */
#define DR_ADDR(addr) \
	((void *)((addr) >= OF_DR_BSS_START && (addr) < OF_DR_BSS_END \
			? of_dr_bss + ((addr) - OF_DR_BSS_START) \
		: (addr) >= OF_DR_DATA_START && (addr) < OF_DR_DATA_END \
			? (unsigned char *)of_dr_data + ((addr) - OF_DR_DATA_START) \
		: (unsigned char *)0))

/* the drillers global at original address ADDR, as an lvalue of TYPE */
#define DR_VAR(type, addr) (*(type *)DR_ADDR(addr))

/* a pointer of TYPE to original address ADDR */
#define DR_PTR(type, addr) ((type *)DR_ADDR(addr))

#endif
