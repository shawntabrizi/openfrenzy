/*
TANKEDUP_B.H

Module tankedup_b: the TankedUp minigame, part 2 (0x001cee60-0x001de950), from
C:\BlitzParty\Game\Src\TankedUp\ (CTank_Entities.cpp, CTank_Char.cpp, CTank_Debug.cpp,
CTank_AI.cpp, Decal.cpp, CTank_MoveQue.cpp and gap functions between them).
Part 1 (0x001c1760-0x001cebd0: CModule_TankedUp, CTank_Game, CTank_Boom, CTank_Spawn,
CTank_Camera, CTank_Route, CTank_Players, CTank_World) is the module tankedup_a, written
at the same time.

Owner: tankedup_b. Other modules may include this header; only tankedup_b edits it.

Conventions (docs/ARCHITECTURE.md):

- Object layouts keep the original (Xbox, ILP32) offsets. Fields are reached with the
  TU_I32/TU_F32/... accessors and the original byte offset; the main offsets have names
  below (ENT_*, TPL_*, ENTS_*).
- x86 `this` (ECX) is the first parameter. Ghidra's first export dropped many ECX and
  stack arguments; tools/RedecompileWithParams.java and tools/calls.py show them.
- Calls into tankedup_a (whose prototypes are still Ghidra's guesses while it is being
  written) go through TU_CALL with the shape read from the x86 code, so they stay right
  whatever prototype tankedup_a gives. The same for engine functions whose table
  prototype has the wrong shape (noted at the call).
- 0x001d5950 is an empty function (`ret`): the game's debug print, compiled out. Callers
  pass a format and arguments; here TU_DEBUG keeps the text and calls it without them.
- x87 compares: Ghidra's "(a < b) == (a == b)" means a > b, "(a < b) != (a == b)" means
  a <= b. They are written as plain compares.
- Read-only .rdata constants are written as literals with their address in a comment.
*/

#ifndef TANKEDUP_B_H
#define TANKEDUP_B_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xtl.h"

/* ---------- raw field access at original offsets */

#define TU_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define TU_I32(p, off) TU_AT(p, off, int)
#define TU_U32(p, off) TU_AT(p, off, unsigned int)
#define TU_F32(p, off) TU_AT(p, off, float)
#define TU_I16(p, off) TU_AT(p, off, short)
#define TU_U8(p, off) TU_AT(p, off, unsigned char)
#define TU_PTR(p, off) TU_AT(p, off, void *)
#define TU_FP(p, off) ((float *)((char *)(p) + (off)))
#define TU_ADDR(p, off) ((void *)((char *)(p) + (off)))
#define TU_STR(p, off) ((char *)(p) + (off))

/* virtual function at byte offset `off` of the object's vtable */
#define TU_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* call a function with the shape read from the x86 code */
#define TU_CALL(fn, real_type) ((real_type)(fn))

/* the empty debug print 0x001d5950 (see the header comment) */
#define TU_DEBUG(...) FUN_001d5950()

#define TU_FILE_ENTITIES "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Entities.cpp"
#define TU_FILE_CHAR "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Char.cpp"
#define TU_FILE_DEBUG "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Debug.cpp"
#define TU_FILE_AI "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_AI.cpp"
#define TU_FILE_DECAL "C:\\BlitzParty\\Game\\Src\\TankedUp\\Decal.cpp"
#define TU_FILE_MOVEQUE "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_MoveQue.cpp"

static inline unsigned int tu_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

static inline float tu_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* x87 _ftol (0x001de998): truncating conversion */
#define TU_FTOL(x) ((int)(long long)(x))
#define TU_FTOLL(x) ((long long)(x))

/* the C library rand() of the XBE (MSVC's; seed DAT_00285630, bkernel) */
extern unsigned int DAT_00285630;
static inline int tu_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- the text reader of the TankedUp data files (tankedup_a opens it)
   +0x08 cursor, +0x0c end of the text, +0x10 file name, +0x210 line number */
#define RD_CUR 0x08
#define RD_END 0x0c
#define RD_NAME 0x10
#define RD_LINE 0x210
#define TU_READER_MORE(rd) (TU_U32(rd, RD_CUR) < TU_U32(rd, RD_END))

/* ---------- CTank_Entities (the entity system; one object, owned by the game)
   An entity list node is 8 bytes: { entity *, next node }. */
#define ENTS_LIST 0x00        /* first node */
#define ENTS_WORLD 0x04       /* the world (tankedup_a) */
#define ENTS_SPAWN 0x08       /* the spawn points (tankedup_a CTank_Spawn) */
#define ENTS_DEBRIS 0x0c      /* the debris system (tankedup_a TankDebris) */
#define ENTS_ROUTE 0x10
#define ENTS_LIGHTS 0x14      /* the tank light/shadow helper (tankedup_a 0x001c3850...) */
#define ENTS_SOUND 0x18       /* the sound bank (Tank sound, 0x001d5960) */
#define ENTS_BOOM 0x1c        /* the explosions (tankedup_a CTank_Boom) */
#define ENTS_CAMERA 0x20      /* the camera (tankedup_a CTank_Camera) */
#define ENTS_PLAYERS 0x24     /* the players (tankedup_a CTank_Players) */
#define ENTS_EFFECTS 0x28     /* particle effects (classlib_fx CParticleEffect) */
#define ENTS_TEX_TRACK_DECAL 0x2c /* the track decal texture */
#define ENTS_TEX_SHADOW 0x30  /* shadow textures per entity type */
#define ENTS_TEX_TRACKS 0x40  /* animated track textures (2) */
#define ENTS_GLOW 0x48        /* "tokenglow.bmp" */
#define ENTS_TICKS 0x50       /* long long: timer ticks per second */
#define ENTS_MESSAGES 0x58    /* the delayed message queue (sorted by time) */
#define ENTS_TEMPLATES 0x5c   /* the entity templates (linked by TPL_NEXT) */
#define ENTS_PT1 0xa4         /* SETTINGS 1Pt_TK */
#define ENTS_PT3 0xa8         /* SETTINGS 3Pt_TK */
#define ENTS_PT5 0xac         /* SETTINGS 5Pt_TK */
#define ENTS_SPEED_TK 0xb0    /* SETTINGS SPEED_TK */
#define ENTS_PRO_TK 0xb8      /* SETTINGS PRO_TK */
#define ENTS_WIN_SWITCH 0xbc  /* SETTINGS WIN_SWITCH_TK */
#define ENTS_FIRE_UP 0xc0     /* SETTINGS FIRE_UP_TK */
#define ENTS_DT 0xc8          /* float: frame time */
#define ENTS_VARIANT 0xcc     /* the game variant (1-...) */
#define ENTS_TWEAKS 0xe8      /* the "tokens" tweak menu */
#define ENTS_TRACK_TIME 0xec  /* float: track decal accumulator */
#define ENTS_SPIN 0xf0        /* float: the glow sprites' angle (+0.05 a frame) */

/* ---------- an entity template (0x6e0 bytes, "TEMPLATE {" in the entity file) */
#define TPL_MENU 0x00
#define TPL_HEALTH 0x08
#define TPL_MINES 0x10
#define TPL_FLAGS 0x14
#define TPL_AI 0x18
#define TPL_TYPE 0x1c
#define TPL_CAMLOOK 0x20
#define TPL_SCOREGIVE 0x24
#define TPL_SCORELOST 0x28    /* float: 1 - SCORELOST */
#define TPL_ROT 0x2c          /* float: 2 pi / ROT */
#define TPL_RADIUS 0x30
#define TPL_LIFE 0x34
#define TPL_MASS 0x38
#define TPL_RESTITUTION 0x3c
#define TPL_FORCE 0x40
#define TPL_HIT_FRZ 0x44
#define TPL_HIT_PRO 0x48
#define TPL_RESTART 0x4c
#define TPL_RELOAD 0x50
#define TPL_GRAVITY 0x54
#define TPL_NAME 0x58         /* char[64] */
#define TPL_ANIM 0x98         /* char[64] */
#define TPL_ACTOR 0xd8        /* char[512] */
#define TPL_TURRET 0x2d8      /* char[512] */
#define TPL_CANNON 0x4d8      /* char[512] */
#define TPL_SCALE 0x6d8
#define TPL_NEXT 0x6dc

/* ---------- an entity (0xf08 bytes; tankedup_a 0x001cebd0 initialises it) */
#define ENT_ID 0x000
#define ENT_NAME 0x008        /* char[64] */
#define ENT_SPAWNLIST 0x048   /* char[64] */
#define ENT_DEAD 0x094
#define ENT_ANIM 0x09c        /* char[64] */
#define ENT_ACTOR 0x2dc       /* char[512] model names */
#define ENT_TURRET 0x4dc
#define ENT_CANNON 0x6dc
#define ENT_TEMPLATE_NAME 0x8dc
#define ENT_TEMPLATE 0x91c    /* the template */
#define ENT_AI 0x924
#define ENT_HEALTH 0x930
#define ENT_MINES 0x934
#define ENT_ON_GROUND 0x93c
#define ENT_LAP 0x950
#define ENT_ROUTENUM 0x968
#define ENT_MASS 0x98c
#define ENT_RADIUS 0x9b0
#define ENT_FRICTION 0x9b8    /* float: ground friction of this frame (-1000: none) */
#define ENT_POS 0xa08         /* float[4] */
#define ENT_ROT 0xa18         /* float[4] x, y (heading), z */
#define ENT_ROT_START 0xa28
#define ENT_VEL 0xa38
#define ENT_OLD_VEL 0xa48
#define ENT_FORCE 0xa58
#define ENT_TIMER 0xa88       /* a struct TuTimer of tankedup_a (life time; ctor 0x001c3790) */
#define ENT_MODEL 0xacc       /* bkResource * */
#define ENT_INST 0xad0        /* BActorInstance * */
#define ENT_TURRET_MODEL 0xad4
#define ENT_TURRET_INST 0xad8
#define ENT_LOW_MODEL 0xadc
#define ENT_LOW_INST 0xae0
#define ENT_LOW_TURRET_MODEL 0xae4
#define ENT_LOW_TURRET_INST 0xae8
#define ENT_LOW_ACTOR 0xaec   /* char[512] */
#define ENT_LOW_TURRET 0xcec  /* char[512] */
#define ENT_CANNON_MODEL 0xeec
#define ENT_CANNON_INST 0xef0
#define ENT_ANIM_SEG 0xef4
#define ENT_TRACKS 0xef8      /* the track decal (Decal.cpp; bumpercars_b's BCarTrack methods) */
#define ENT_TOKEN 0xefc       /* struct CToken * (classlib_fx) */
#define ENT_MOVEQUE 0xf00     /* CTank_MoveQue */

#define TU_ENT_TYPE(ent) TU_I32(TU_PTR(ent, ENT_TEMPLATE), TPL_TYPE)

/* ---------- functions missing from src/include/of_functions.tsv (defined here) */

/* 0x001d7d60: the animation callback of entities with an ANIMATION (BAnimCallback shape);
   reason 0 (the animation ended) sets +0x964 of the entity (context) */
int FUN_001d7d60(int reason, void *instance, void *node, void *anim, void *context);

/* ---------- globals of other modules */

extern int DAT_002baed4[];          /* core: players, 10 ints each, [0] character */
extern int DAT_002baefc, DAT_002baf24, DAT_002baf4c; /* characters of players 2-4 */
extern int DAT_002bae94;            /* core: number of players */
extern int DAT_002bae70;            /* core: the difficulty (0 easy, 1 medium, 2 hard) */
extern int DAT_002bae8c;
extern int DAT_002baeb8;
extern int DAT_002baed8[];
extern int DAT_002baee0[];
extern unsigned char DAT_00290718[]; /* character colours, 0x60 bytes per character */
extern unsigned char DAT_00290719[];
extern unsigned char DAT_0029071a[];
extern float DAT_002855f0[16];      /* the identity matrix (bkernel) */
extern unsigned int DAT_002bc490[256]; /* colour channel tables (bdisplay) */
extern unsigned int DAT_002bd640[256];
extern unsigned int DAT_002bc898[256];
extern unsigned int DAT_002bce18[256];
extern unsigned long long DAT_002cd7a0; /* timer ticks per second (bkernel) */
extern float DAT_0028fda8;          /* frame time scale (frames of 1/60 s this frame) */
extern int DAT_002ecfac;            /* the HUD (classlib_ui) */
extern void *DAT_002db808;          /* the DirectSound object (engine_misc) */

#endif
