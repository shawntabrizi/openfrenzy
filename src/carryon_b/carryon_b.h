/*
CARRYON_B.H

Module carryon_b: the CarryOn (fireworks) minigame, part 2 (0x000ca230-0x000d9360), from
C:\BlitzParty\Game\Src\CarryOn\ (CFire_Conveyor.cpp, CFire_LaunchRack.cpp,
CFire_V2_Glitter.cpp, CFire_Arena.cpp, CFire_Fireworks.cpp, CFire_Lights.cpp,
CFire_Sprites.cpp, FWorkTypes.cpp, CFire_Glitter.cpp, CFire_Romans.cpp and gap functions).

Owner: carryon_b. Other modules may include this header; only carryon_b edits it.

Conventions (docs/ARCHITECTURE.md):

- Object layouts keep the original (Xbox, ILP32) offsets. Fields that are not understood
  yet are reached with the CB_I32/CB_F32/... accessors and the original byte offset.
- x86 `this` (ECX) is the first parameter. Ghidra often drops a `this` that a caller loads
  into ECX; src/carryon_b/tools/ecx.py shows it for every call.
- Calls to functions of other modules (mainly carryon_a, which is written at the same time)
  go through a cast to the real shape (CB_CALL), read from the x86 code. A float that the
  table still declares as undefined4 goes through CB_DUAL (the float's bit pattern while the
  table type is in force, a real float once the owner fixes the type).
- Globals keep their Ghidra names (DAT_<address>). CarryOn/carryon_b_globals.c defines weak
  fallbacks (original size and XBE value) for globals that other modules may also define.
  Read-only .rdata constants are written as literals with their address in a comment.
- x87 compares: Ghidra's "(a < b) == (a == b)" means a > b, and "(a < b) != (a == b)"
  means a <= b. They are written as plain compares.
*/

#ifndef CARRYON_B_H
#define CARRYON_B_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xtl.h"

/* ---------- raw field access at original offsets */

#define CB_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define CB_I32(p, off) CB_AT(p, off, int)
#define CB_U32(p, off) CB_AT(p, off, unsigned int)
#define CB_F32(p, off) CB_AT(p, off, float)
#define CB_I16(p, off) CB_AT(p, off, short)
#define CB_U16(p, off) CB_AT(p, off, unsigned short)
#define CB_U8(p, off) CB_AT(p, off, unsigned char)
#define CB_PTR(p, off) CB_AT(p, off, void *)
#define CB_FP(p, off) ((float *)((char *)(p) + (off)))
#define CB_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define CB_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* call a function of another module with the shape read from the x86 code */
#define CB_CALL(fn, real_type) ((real_type)(fn))

static inline unsigned int cb_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

static inline float cb_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* call `fn` with the table prototype while that is still `table_type`, else with the
   real prototype (see the header comment) */
#define CB_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn)) table_args, ((real_type)(fn)) real_args)

/* the C library rand() of the XBE (MSVC's, inlined by the game; seed DAT_00285630) */
extern unsigned int DAT_00285630;
static inline int cb_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- Babel engine helpers (bkernel) */

#define CB_NEW(size, line) bkHeapAlloc((size), CB_FILE, (line), 0x1001)
#define CB_DELETE(p) bkHeapFree((void *)(p))

/* ---------- globals of other modules (fallbacks in CarryOn/carryon_b_globals.c) */

extern int DAT_002bae94;            /* number of players (core) */
/* per-player records of the core game state, 10 ints (0x28 bytes) each; [0] is the
   character of the player, DAT_002baed4 is the same table one record later */
extern int DAT_002baeac[];
extern int DAT_002baed4[];
/* character colour table, 0x60 bytes per character: red, green, blue at +0, +1, +2
   (classlib_ui/charlib) */
extern unsigned char DAT_00290718[];
extern unsigned char DAT_00290719[];
extern unsigned char DAT_0029071a[];
extern char DAT_002855f0[];         /* the identity matrix (float[16]) */
extern unsigned int DAT_002bc490[]; /* colour channel tables: red, green, blue, alpha */
extern unsigned int DAT_002bd640[];
extern unsigned int DAT_002bc898[];
extern unsigned int DAT_002bce18[];
extern unsigned int DAT_002bca94;
/* the arena bounds of charlib: x0, z0, width, depth */
extern float DAT_00292c60;
extern float DAT_00292c64;
extern float DAT_00292c68;
extern float DAT_00292c6c;

/* ---------- CarryOn globals shared with carryon_a */

/* 0x002f6d28  the four players (CCarryOn_Player *, carryon_a): +0x30 actor instance
   (position at +0xd0), +0x1130 rockets carried, +0x1134 score */
extern void *DAT_002f6d28[4];
/* 0x002f6d5c  the CarryOn game (carryon_a): +0x00 package, +0x250 variant,
   +0x2b4 rocket animation, +0x374 catch radius */
extern void *DAT_002f6d5c;
extern int DAT_002f6d60;            /* debug drawing of the launch rack */
extern int *DAT_002f7960;           /* set to 1 while the rack has rockets to launch */
extern int DAT_00293b44;            /* rockets in flight (carryon_a) */
/* launch rack tweakables (0x00294390-0x00294398) */
extern int DAT_00294390;            /* slots 20 */
extern float DAT_00294394;          /* fill time 600.0 */
extern float DAT_00294398;          /* time between launches 7.5 */

/* ---------- module objects */

/* a fire light (0x50 bytes) */
struct CFire_Light {
	struct BLight *blight;                /* +0x00 the engine light, NULL when detached */
	int group;                            /* +0x04 */
	int type;                             /* +0x08 0: point, 1: directional, 2: spot */
	unsigned int flags;                   /* +0x0c 1: on, 2: switched off, 4: BLight flag 4 */
	float pos[4];                         /* +0x10 position (direction of a directional light) */
	float target[4];                      /* +0x20 */
	float angle;                          /* +0x30 spot cone */
	int r, g, b;                          /* +0x34 */
	float range;                          /* +0x40 */
	int unk44[2];                         /* +0x44 */
	struct CFire_Light *next;             /* +0x4c */
};

/* the light manager (0x5c bytes, DAT_002f7988) */
struct CFire_Lights {
	struct CFire_Light *slots[8];         /* +0x00 lights that own an engine light */
	struct BLight *blights[8];            /* +0x20 */
	struct CFire_Light *list;             /* +0x40 */
	struct CFire_Light *flash[3];         /* +0x44 */
	float flashTime[3];                   /* +0x50 */
};

extern struct CFire_Lights *DAT_002f7988;

/* a conveyor path: points (float[4] each) and their number (carryon_a) */
struct CFire_Path {
	float (*points)[4];   /* +0x00 */
	int count;            /* +0x04 */
	int unk08;            /* +0x08 */
};

/* a piece on the conveyor (0x48 bytes; updated and drawn by carryon_a 0x000c9ee0,
   0x000ca060) */
struct CFire_ConveyorItem {
	int active;                 /* +0x00 */
	int player;                 /* +0x04 */
	float pos[4];               /* +0x08 */
	float pos2[4];              /* +0x18 */
	float delay;                /* +0x28 */
	int point;                  /* +0x2c */
	int point2;                 /* +0x30 */
	float speed;                /* +0x34 18.0 */
	int flag;                   /* +0x38 */
	struct CFire_Path *path;    /* +0x3c */
	struct bdVertexBuffer *vb;  /* +0x40 */
	struct bkResource *texture; /* +0x44 */
};

/* the conveyors: two rows of pieces on the same paths */
struct CFire_Conveyor {
	float pos[4];                         /* +0x00 */
	int unk10;                            /* +0x10 */
	int numItems;                         /* +0x14 */
	struct CFire_ConveyorItem **items;    /* +0x18 */
	int numItems2;                        /* +0x1c */
	struct CFire_ConveyorItem **items2;   /* +0x20 */
};

/* the launch rack (vtable 0x00268788): the players fill its slots with rockets; when it
   is full or the fill time runs out it launches them one by one */
struct CFire_LaunchRack {
	const void *const *vtbl;              /* +0x00 */
	struct CFire_Fireworks *fireworks;    /* +0x04 */
	float fillTime;                       /* +0x08 */
	int slots[50];                        /* +0x0c the team (1-4) of each rocket, 0: empty */
	float launchTime;                     /* +0xd4 */
	int launchesLeft;                     /* +0xd8 */
	int hasRockets;                       /* +0xdc */
	float pos[4];                         /* +0xe0 */
	float angle;                          /* +0xf0 */
	float halfWidth;                      /* +0xf4 */
	float halfDepth;                      /* +0xf8 */
	struct bkResource *models[6];         /* +0xfc */
	struct BActorInstance *actors[6];     /* +0x114 */
	int bigShow;                          /* +0x12c half the slots or more were full */
};

/* a rocket dropped in the arena (0x3c bytes; ctor 0x000cb5d0) */
struct CFire_Rocket {
	float spin;                           /* +0x00 */
	float phase;                          /* +0x04 */
	float pos[4];                         /* +0x08 */
	float vel[4];                         /* +0x18 */
	float groundY;                        /* +0x28 */
	float floorY;                         /* +0x2c */
	int state;                            /* +0x30 0: none, 2-5: bouncing, 6-7: lying */
	int type;                             /* +0x34 */
	struct BActorInstance *actor;         /* +0x38 */
};

#endif
