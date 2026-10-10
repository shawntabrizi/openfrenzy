/*
demolition_b.h - the demolition_b module (Minigame Demolition, part 2 of 4: 0x000f0bf0-0x000fca70)

Original sources: C:\BlitzParty\Game\Src\Demolition\
  CDemolition_Bomb.cpp     bombs (a CharLib actor), the bomb systems of the game variations
  CDemolition_World.cpp    the world: start positions, .lom object files, collision
  CDemolition_CharLib.cpp  CDemolition_CharLib (a CharLib character for this game)
  CDemolition_Player.cpp   CDemolition_Player (the player character, its states)
  (unplaced)               gap functions: explosions, the bomb object base, cameras, ...

Classes (vtables rebuilt in the .c files; sizes from the constructors):
  vtable 0x0026b0d4  CDemolition_Object      base of the bomb object (demolition_d base 0x0010bd30)
  vtable 0x0026ae18  CDemolition_Bomb        the bomb object (0x4670 bytes)
  vtable 0x0026b2c0  CDemolition_CharLib     CharLib character (secondary vtable at +0x1130)
  vtable 0x0026ae30  CDemolition_BombChar    CharLib character carrying a bomb (secondary at +0x1130)
  vtable 0x0026b448  CDemolition_Player      the player (secondary at +0x1130)
  vtable 0x0026af2c  CDemolition_BombSystem  base of the bomb systems
  vtables 0x0026af48, 0x0026af6c, 0x0026af90, 0x0026afb4: the bomb systems of the variations

Layouts are those of the original 32-bit game (docs/ARCHITECTURE.md, section 3). Most objects
are reached through byte offsets (DB_I32(p, 0x1194)), because the classes are shared with the
other Demolition modules (demolition_a/c/d) and their full layouts are not known yet.
*/

#ifndef DEMOLITION_B_H
#define DEMOLITION_B_H

#include <string.h>

/* ---------- raw field access (original offsets) */
#define DB_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define DB_I32(p, off) DB_AT(p, off, int)
#define DB_U32(p, off) DB_AT(p, off, unsigned int)
#define DB_F32(p, off) DB_AT(p, off, float)
#define DB_I16(p, off) DB_AT(p, off, short)
#define DB_U16(p, off) DB_AT(p, off, unsigned short)
#define DB_U8(p, off) DB_AT(p, off, unsigned char)
#define DB_I8(p, off) DB_AT(p, off, signed char)
#define DB_PTR(p, off) DB_AT(p, off, void *)
#define DB_FP(p, off) ((float *)((char *)(p) + (off)))
#define DB_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define DB_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int db_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

/* a float stored in an undefined4 */
static inline float db_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* Call `fn` with the table prototype while that is still `table_type`, else with the real
   prototype. Callers in other modules compile against of_decls.h, whose prototype changes
   when the owner fixes it (a float parameter or result); this keeps both cases correct. */
#define DB_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn)) table_args, ((real_type)(fn)) real_args)

/* the engine's rand() (MSVC's, inlined by the game; seed DAT_00285630, shared) */
extern unsigned int DAT_00285630;
static inline int db_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- globals of other modules (weak fallbacks in Demolition/demolition_b_globals.c) */

/* 0x002ff85c  the Demolition game object (CDemolition_Game; demolition_a) */
extern char *DAT_002ff85c;
/* 0x0028fda8  frame time factor (1.0 at 60 Hz) */
extern float DAT_0028fda8;
/* 0x002bae94  number of players */
extern int DAT_002bae94;
/* 0x002bae70  difficulty of the CPU players (0-2) */
extern int DAT_002bae70;
/* 0x002baed4  the player records (core): 10 ints per player; [0] character, [1] human,
   [3] pad port bits (0x002baee0), [8] team (0x002baef4) */
extern int DAT_002baed4[];
/* 0x00290750  the characters (0x60 bytes each); +0 the AI think interval */
extern int DAT_00290750;

/* ---------- vtables of this module (rebuilt from the XBE in Demolition/demolition_b_vtables.c) */
extern const void *const vtable_0026ae18[];   /* CDemolition_FragBomb */
extern const void *const vtable_0026ae2c[];   /* CDemolition_RemoteBomb, receiver at +0x1130 */
extern const void *const vtable_0026ae30[];   /* CDemolition_RemoteBomb */
extern const void *const vtable_0026af2c[];   /* CDemolition_TimedBomb */
extern const void *const vtable_0026af30[];   /* CDemolition_CrossBomb */
extern const void *const vtable_0026af44[];   /* CDemolition_BombSystem */
extern const void *const vtable_0026af48[];   /* CDemolition_BombSystemVariationFrag */
extern const void *const vtable_0026af6c[];   /* CDemolition_BombSystemVariationRemote */
extern const void *const vtable_0026af90[];   /* CDemolition_BombSystemVariationTimed */
extern const void *const vtable_0026afb4[];   /* CDemolition_BombSystemVariationCross */
extern const void *const vtable_0026b0d4[];   /* CDemolition_Object */
extern const void *const vtable_0026b0e8[];
extern const void *const vtable_0026b0f0[];
extern const void *const vtable_0026b0f8[];
extern const void *const vtable_0026b2c0[];   /* CDemolition_CharLib */
extern const void *const vtable_0026b3b4[];   /* CDemolition_CharLib, receiver at +0x1130 */
extern const void *const vtable_0026b3b8[];
extern const void *const vtable_0026b440[];   /* CDemolition_Player, receiver at +0x1130 */
extern const void *const vtable_0026b448[];   /* CDemolition_Player */

#endif
