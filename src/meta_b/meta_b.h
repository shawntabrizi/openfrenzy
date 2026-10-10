/*
META_B.H

Module meta_b: the Meta board game, part 2 (0x0015cef0-0x0016be60), from
C:\BlitzParty\Game\Src\Meta\ (CLightning.cpp, CMeta_ExplSmoke.cpp, CMeta_Explosion.cpp,
CMLaser.cpp, CScorchMarks.cpp, CBomb.cpp, BigRedButton.cpp, Searchlights.cpp, CWater.cpp,
CMeta_Rain.cpp, CMeta_Seagulls.cpp, BigGun.cpp, LevelLoad.cpp and gap functions).

Owner: meta_b. Other modules may include this header; only meta_b edits it.

Conventions (docs/ARCHITECTURE.md):

- Object layouts keep the original (Xbox, ILP32) offsets. Fields that are not understood
  yet are reached with the MB_I32/MB_F32/... accessors and the original byte offset.
- x86 `this` (ECX) is the first parameter.
- Calls to functions of other modules whose table prototype (of_decls.h) has the wrong
  shape go through a cast to the real shape. A float parameter that the table still
  declares as undefined4 goes through MB_DUAL (float bit pattern now, a real float once
  the owner fixes the type).
- Globals keep their Ghidra names (DAT_<address>). Meta/meta_b_globals.c defines the
  globals of this module and weak fallbacks (original size and XBE value) for globals of
  other modules; an owner's strong definition replaces a fallback at link time. Read-only
  .rdata constants are written as literals with their address in a comment.
- x87 compares: Ghidra's "(a < b) == (a == b)" means a > b, and "(a < b) != (a == b)"
  means a <= b. They are written as plain compares.
*/

#ifndef META_B_H
#define META_B_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xtl.h"

/* ---------- raw field access at original offsets */

#define MB_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define MB_I32(p, off) MB_AT(p, off, int)
#define MB_U32(p, off) MB_AT(p, off, unsigned int)
#define MB_F32(p, off) MB_AT(p, off, float)
#define MB_I16(p, off) MB_AT(p, off, short)
#define MB_U16(p, off) MB_AT(p, off, unsigned short)
#define MB_U8(p, off) MB_AT(p, off, unsigned char)
#define MB_I8(p, off) MB_AT(p, off, signed char)
#define MB_PTR(p, off) MB_AT(p, off, void *)
#define MB_FP(p, off) ((float *)((char *)(p) + (off)))
#define MB_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define MB_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int mb_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

/* a float stored in an undefined4 */
static inline float mb_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* call `fn` with the table prototype while that is still `table_type`, else with the
   real prototype (see the header comment) */
#define MB_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn)) table_args, ((real_type)(fn)) real_args)

/* the engine's rand() (MSVC's, inlined by the game; seed DAT_00285630, shared) */
extern unsigned int DAT_00285630;
static inline int mb_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* rand() / 32767 as the game computes it: rand() * 3.0518509e-05 (0x00253740) */
#define MB_RAND_SCALE 3.0518509e-05f
/* rand() * 2pi / 32767 (0x00263520) */
#define MB_RAND_ANGLE 1.9175344e-04f

/* ---------- Babel engine helpers (bkernel) */

/* operator new: bkHeapAlloc(size, file, line, flags) */
#define MB_NEW(size, file, line) bkHeapAlloc((size), (file), (line), 0x1001)
/* operator delete */
#define MB_DELETE(p) bkHeapFree((void *)(p))

/* intrusive lists of ClassLib (CLList_*): { next, prev, count }; nodes start with
   next/prev. CLList_Next(list, NULL) is the first node, NULL after the last. */
#define MB_LIST_NEXT(list, node) \
	((void *)CLList_Next((unsigned int *)(list), (unsigned int *)(node)))
#define MB_LIST_INSERT(list, node, at_tail) \
	CLList_Insert((int *)(list), (int *)(node), (at_tail))
#define MB_LIST_REMOVE(list, node) \
	((void *)CLList_Remove((int *)(list), (int *)(node)))

/* ---------- render states (bdisplay FUN_00044b00(state, value, 0)) */
#define MB_SET_RS(state, value) FUN_00044b00((state), (uint)(value), 0)

/* ---------- globals of other modules used here (fallbacks in Meta/meta_b_globals.c) */

extern int DAT_002bae94;            /* number of players (core) */
extern float DAT_0028fda8;          /* frame time factor, 1.0 = 1/60 s */
extern char DAT_002855f0[];         /* the identity matrix (bkernel) */
extern int DAT_003055d0;            /* display settings object (bdisplay); +0x48 cull mode */
extern int DAT_0030564c;            /* current game state object (meta_a); +0x68 phase */
extern int DAT_003075bc[];          /* the meta players (CMetaPlayer *), by player (meta_a) */
extern unsigned int DAT_002bc490[]; /* colour channel tables: red, green, blue, alpha */
extern unsigned int DAT_002bd640[];
extern unsigned int DAT_002bc898[];
extern unsigned int DAT_002bce18[];
extern char DAT_00290dc8[];         /* CProjSpiralType table, whitebeam.bmp (classlib_fx) */
extern long long DAT_002cd7a0;      /* timer ticks per second (bkernel) */
extern int DAT_002ecdec;            /* the game is paused (classlib_ui) */
extern float DAT_0028fdb8[4];       /* (1, 0, 0, 1) (classlib_ui) */
extern float DAT_0028fdc8[3];       /* (0, 1, 0) (classlib_ui) */
extern float DAT_0028fdd8[3];       /* (0, 0, 1) (classlib_ui) */
extern float DAT_00292c84;          /* gravity (charlib) */
extern float DAT_002ea478[4];       /* a zero vector (classlib_fx) */
extern unsigned char DAT_002ed324[]; /* the rumble manager (core, CRumble) */
extern int DAT_00290da4;            /* CProjTrail texture (classlib_fx) */
extern int *DAT_002ea488;           /* charlib's actor list object (lists at +0, +0x18) */
extern int DAT_002b0580, DAT_002b0584, DAT_002b0588, DAT_002b058c; /* water fog colour (meta_a) */
extern int DAT_00290d80[4];         /* CProjTrail head colour (classlib_fx) */
extern int DAT_00290d90[4];         /* CProjTrail tail colour (classlib_fx) */

/* ---------- the Meta .data block 0x002b04e4-0x002b2670 (tweakables, tables): meta_a keeps
   one copy with the original layout (src/meta_a/Meta/meta_a_data.c, `of_meta_data`) and
   aliases for the addresses Ghidra names. Addresses it has no alias for are reached with
   MB_META / MB_META_PTR. */
extern unsigned int of_meta_data[];
#define MB_META_PTR(addr, type) ((type *)((char *)of_meta_data + ((addr) - 0x002b04e4)))
#define MB_META(addr, type) (*MB_META_PTR(addr, type))

/* aliased by meta_a */
extern float DAT_002b0a4c;          /* lightning spiral radius 24.0 */
extern int DAT_002b0a50;            /* lightning spiral turns 10 */
extern float DAT_002b0a54;          /* lightning spiral rise 120.0 */
extern float DAT_002b0a58;          /* lightning spiral sparkle distance 1.0 */
extern float DAT_002b0a5c;          /* vent: push above which the player is knocked 1.0 */
extern float DAT_002b0a60;          /* vent: damage 100.0 */

#endif
