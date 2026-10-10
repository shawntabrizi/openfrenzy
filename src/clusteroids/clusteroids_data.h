/*
CLUSTEROIDS_DATA.H

Access to the data blocks of the clusteroids module, with the original (Xbox) layout.
The blocks are defined in Clusteroids/clust_data.c, which tools/gen_clust_data.py
generates from orig/default.xbe (same design as src/core/core_data.h):

  of_cl_data   0x00294bd8-0x00297508  .data of the Clusteroids objects (XBE values)
  of_cl_bss    0x002f7a44-0x002ff858  .bss of the Clusteroids objects (zero)

Clusteroids code names a global by its original address: CL_VAR(int, 0x002f7f6e). Every
access goes through the block object, so the compiler never assumes that two fields of a
block are different objects (the original code indexes across them).

Never put an original address into a pointer: CL_VAR / CL_ADDR turn it into an offset.
*/

#ifndef OPENFRENZY_CLUSTEROIDS_DATA_H
#define OPENFRENZY_CLUSTEROIDS_DATA_H

#include <stdint.h>

extern uint32_t of_cl_data[];
extern unsigned char of_cl_bss[];

#define OF_CL_DATA_START 0x00294bd8u
#define OF_CL_DATA_END 0x00297508u
#define OF_CL_BSS_START 0x002f7a44u
#define OF_CL_BSS_END 0x002ff858u

/* the host address of original address ADDR inside a clusteroids block (ADDR must lie
inside one block; anything else yields a null pointer) */
#define CL_ADDR(addr) \
	((void *)((addr) >= OF_CL_BSS_START && (addr) < OF_CL_BSS_END \
			? of_cl_bss + ((addr) - OF_CL_BSS_START) \
		: (addr) >= OF_CL_DATA_START && (addr) < OF_CL_DATA_END \
			? (unsigned char *)of_cl_data + ((addr) - OF_CL_DATA_START) \
		: (unsigned char *)0))

/* the clusteroids global at original address ADDR, as an lvalue of TYPE */
#define CL_VAR(type, addr) (*(type *)CL_ADDR(addr))

#endif
