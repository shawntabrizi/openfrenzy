/*
META_A.H

Module meta_a: the Meta board game, part 1 (0x0014c9a0-0x0015cc80), from
C:\BlitzParty\Game\Src\Meta\ (CGameMode_Meta.cpp, CModule_Meta.cpp, puzzles.cpp,
CMetaPlayer.cpp, Tiles_Meta.cpp, Effects.cpp and gap functions).

Owner: meta_a. Other modules may include this header; only meta_a edits it.

Conventions (see docs/ARCHITECTURE.md):

- Object layouts keep the original (Xbox, ILP32) offsets. Fields that are not understood
  yet are reached with the MA_I32/MA_F32/... accessors and the original byte offset.
- Virtual calls go through MA_VFN(obj, byte offset of the slot, function type) and pass
  the object as the first argument (the x86 code passed it in ECX).
- Calls whose table prototype (of_decls.h) has the wrong shape go through a cast to the
  real shape. A float parameter that the table still declares as undefined4 goes
  through MA_DUAL (float bit pattern now, a real float once the owner fixes the type).
- Globals keep their Ghidra names (DAT_<address>). Meta/meta_a_globals.c holds weak or
  common fallback definitions with the original size and XBE value; an owner that
  defines the same name strongly replaces them at link time. Read-only .rdata
  constants are written as literals with their address in a comment.
- x87 compares: Ghidra's "(a < b) == (a == b)" means a > b, and
  "(a < b) != (a == b)" means a <= b. They are written as plain compares.
*/

#ifndef META_A_H
#define META_A_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xtl.h"

/* ---------- raw field access at original offsets */

#define MA_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define MA_I32(p, off) MA_AT(p, off, int)
#define MA_U32(p, off) MA_AT(p, off, unsigned int)
#define MA_F32(p, off) MA_AT(p, off, float)
#define MA_I16(p, off) MA_AT(p, off, short)
#define MA_U16(p, off) MA_AT(p, off, unsigned short)
#define MA_U8(p, off) MA_AT(p, off, unsigned char)
#define MA_I8(p, off) MA_AT(p, off, signed char)
#define MA_PTR(p, off) MA_AT(p, off, void *)
#define MA_FP(p, off) ((float *)((char *)(p) + (off)))
#define MA_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define MA_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int ma_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

/* a float stored in an undefined4 */
static inline float ma_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* call `fn` with the table prototype while that is still `table_type`, else with the
   real prototype (see the header comment) */
#define MA_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn)) table_args, ((real_type)(fn)) real_args)

/* the engine's rand() (MSVC's, inlined by the game; seed DAT_00285630, shared) */
extern unsigned int DAT_00285630;
static inline int ma_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- Babel engine helpers (bkernel) */

/* operator new: bkHeapAlloc(size, file, line, flags) */
#define MA_NEW(size, file, line, flags) \
	(((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)( \
		(size), (file), (line), (flags)))
/* operator delete */
#define MA_DELETE(p) (((void (*)(void *))FUN_0004c110)((void *)(p)))
/* debug printf */
#define MA_PRINTF ((void (*)(const char *, ...))FUN_0004b230)

/* ---------- the character controller (charlib). Hopper's players and CMetaPlayer
   derive from it; only offsets this module uses are named here.

   +0x000 vtable            +0x030 character (charlib actor object)
   +0x036 short state       +0x038 short previous state
   +0x03c int frames in state, +0x040 float time in state
   +0x044 float[3] stick direction, +0x054 float stick amount
   +0x068 / +0x06c input objects (button state at +0x14)
   +0x07c float[3] velocity, +0x088 float
   +0x1094 action table, +0x1098 transition table (15 slots each, indexed by state) */

#define MA_CHAR(ctl) MA_I32(ctl, 0x30)

#include "meta_a_globals.h"

/* meta_a's tables (defined next to their code) */
extern const void *const PTR_LAB_002b06d4[15]; /* CMetaPlayer conditions (unplaced_player.c) */
extern const void *const PTR_LAB_002b0710[15]; /* CMetaPlayer states (unplaced_player.c) */
extern const void *const vtable_0026f890[18];  /* CMetaPlayer (unplaced_metaplayer.c) */
extern const void *const vtable_0026f0c8[14];  /* CGameMode_Meta (CGameMode_Meta.c) */
extern const void *const vtable_0026f678[4];   /* CModule_Meta (unplaced_player.c) */

/* functions missing from the function table (Meta/meta_a_foreign.c) */
undefined4 *FUN_000ad830(undefined4 *self);

#endif
