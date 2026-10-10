/*
CORE_DATA.H

openfrenzy core module. Access to the data blocks that the core module owns, with the
original (Xbox) layout. The blocks are defined in crt/core_data.c, which
tools/gen_core_data.py generates from orig/default.xbe:

  of_core_bss     0x002ba4f0-0x002bb138  .bss of the Core\*.obj files (zero)
  of_core_data    0x002839b8-0x0028450c  .data of the Core\*.obj files (XBE values)
  of_core_tweaks  0x002ad120-0x002adf38  tweak variables, set by static constructor 0x001f2da0

Core code names a global by its original address: CORE_VAR(int, 0x002ba7a0). Every access
goes through the one block object, so the compiler never assumes that two fields of a
block are different objects (the original code indexes across them). Other modules keep
their Ghidra names (DAT_002ba7a0): crt/core_data.c makes those names aliases into the
blocks.

Never put an original address into a pointer: CORE_VAR turns it into an offset.
*/

#ifndef OPENFRENZY_CORE_DATA_H
#define OPENFRENZY_CORE_DATA_H

#include <stdint.h>

extern unsigned char of_core_bss[];
extern uint32_t of_core_data[];
extern unsigned char of_core_tweaks[];

#define OF_CORE_BSS_START 0x002ba4f0u
#define OF_CORE_BSS_END 0x002bb138u
#define OF_CORE_DATA_START 0x002839b8u
#define OF_CORE_DATA_END 0x0028450cu
#define OF_CORE_TWEAKS_START 0x002ad120u
#define OF_CORE_TWEAKS_END 0x002adf38u

/* the host address of original address ADDR inside a core block (ADDR must be a constant
expression inside one block; anything else fails to compile or yields a null pointer) */
#define CORE_ADDR(addr) \
	((void *)((addr) >= OF_CORE_BSS_START && (addr) < OF_CORE_BSS_END \
			? of_core_bss + ((addr) - OF_CORE_BSS_START) \
		: (addr) >= OF_CORE_DATA_START && (addr) < OF_CORE_DATA_END \
			? (unsigned char *)of_core_data + ((addr) - OF_CORE_DATA_START) \
		: (addr) >= OF_CORE_TWEAKS_START && (addr) < OF_CORE_TWEAKS_END \
			? of_core_tweaks + ((addr) - OF_CORE_TWEAKS_START) \
		: (unsigned char *)0))

/* the core global at original address ADDR, as an lvalue of TYPE */
#define CORE_VAR(type, addr) (*(type *)CORE_ADDR(addr))

#endif
