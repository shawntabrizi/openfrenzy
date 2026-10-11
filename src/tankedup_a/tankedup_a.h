/*
TANKEDUP_A.H

Module tankedup_a: the TankedUp minigame, part 1 (0x001c1760-0x001cebd0), from
C:\BlitzParty\Game\Src\TankedUp\ (CModule_TankedUp.cpp, CTank_Game.cpp, CTank_Boom.cpp,
CTank_Spawn.cpp, CTank_Camera.cpp, CTank_Route.cpp, CTank_Players.cpp, CTank_World.cpp and
gap functions between them). Part 2 (tankedup_b) holds CTank_Entities, CTank_Char,
CTank_Debug, CTank_AI, Decal and CTank_MoveQue.

Four variations (CModule_TankedUp, variation 1-4): "Command", "Drop Zone", "Trench Warfare",
"Race".

The game is one static object, CTank_Game at 0x00312f50 (0x2160 bytes, constructed by core's
static constructor through 0x001c1880). It holds the subsystems below at fixed offsets;
each subsystem method is __thiscall on its sub-object:

  +0x0a34  soundSystem   (tankedup_b: ctor 0x001d5880, Initialise 0x001d5960)
  +0x0b20  entitySystem  (CTank_Entities; ctor 0x001cdb20 here, Initialise 0x001d41d0 in b)
  +0x0c18  worldSystem   (CTank_World;  Initialise 0x001cd920)
  +0x0c38  playerSystem  (CTank_Players; vtable 0x00279fd8; Initialise 0x001c7f40)
  +0x0cdc  routeSystem   (CTank_Route;  Initialise 0x001c76f0)
  +0x0d40  cameraSystem  (CTank_Camera; Initialise 0x001c6a70)
  +0x0e30  spawnSystem   (CTank_Spawn;  Initialise 0x001c4770)
  +0x0e34  lightSystem   (4 lights of 0x50 bytes; Initialise 0x001c3820)
  +0x0f78  timerSystem   (struct TuTimer; Initialise 0x001c37b0)
  +0x0fc0  debrisSystem  (Initialise 0x001c34b0)
  +0x0fc4  boomSystem    (CTank_Boom; Initialise 0x001c2e00)
  +0x0fd0  charSystem    (tankedup_b: ctor 0x001d5af0, Initialise 0x001d5b80)

The object at 0x003150b0 (0x158 bytes) is tankedup_b's debug object (CTank_Debug,
Initialise 0x001d6d90). Its fields are the tweakable settings that this module reads
(ambient light, draw switches); see TU_DBG_* below.

Functions missing from src/include/of_functions.tsv, defined here with their FUN_ names:
0x001c3490 (the debris ground test, TankedUp/unplaced_debris.c), 0x001c9f50 (the world
objects' animation callback, TankedUp/CTank_World.c), and 0x001c1a90, 0x001c1c10,
0x001c2080 (CTank_Game's Pushed, Resume and Start, TankedUp/CTank_Game.c; tankedup_b's game
mode jumps to them).

__thiscall methods that do not use `this` and that tankedup_b calls without it are written
without it: 0x001c38a0, 0x001cdc30, 0x001cebd0 (see tankedup_a.decls).

Owner: tankedup_a. Other modules may include this header; only tankedup_a edits it.

Conventions (docs/ARCHITECTURE.md):
- x86 `this` (ECX) is the first parameter. Calls into tankedup_b go through TUB_* casts to
  the real shape read from the binary (tankedup_b_calls.h), so they stay right whatever
  prototype tankedup_b gives the function.
- Fields that are not understood yet are reached with TU_AT(p, off, type) at the original
  byte offset.
- x87 compares: Ghidra's "(a < b) == (a == b)" means a > b, "(a < b) != (a == b)" means
  a <= b. They are written as plain compares.
- 0x001d5950 is an empty function that the compiler folded several empty methods and the
  debug printf into. Calls to it are written as TU_DPRINTF(...) (no code) or as comments.
*/

#ifndef OPENFRENZY_TANKEDUP_A_H
#define OPENFRENZY_TANKEDUP_A_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xtl.h"
#include "tankedup_a_data.h"

/* ---------- raw field access at original offsets */

#define TU_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define TU_I32(p, off) TU_AT(p, off, int)
#define TU_U32(p, off) TU_AT(p, off, unsigned int)
#define TU_F32(p, off) TU_AT(p, off, float)
#define TU_I16(p, off) TU_AT(p, off, short)
#define TU_U16(p, off) TU_AT(p, off, unsigned short)
#define TU_U8(p, off) TU_AT(p, off, unsigned char)
#define TU_I8(p, off) TU_AT(p, off, signed char)
#define TU_PTR(p, off) TU_AT(p, off, void *)
#define TU_FP(p, off) ((float *)((char *)(p) + (off)))
#define TU_P(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define TU_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* x87 _ftol (0x001de998): truncating float to integer conversion */
#define TU_FTOL(x) ((int)(long long)(x))

/* the debug printf of the original build, compiled to the empty function 0x001d5950 */
#define TU_DPRINTF(...) ((void)0)

/* MSVC rand() (0x001e133e): the C library's per-thread LCG (seed starts at 1) */
int tu_rand(void);

static inline float tu_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

static inline unsigned int tu_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

/* bKernel heap: operator new / delete */
#define TU_NEW(size, file, line) bkHeapAlloc((uint)(size), (file), (line), 0x1001)
#define TU_DELETE(p) bkHeapFree((void *)(p))

/* source file names in allocation calls */
#define TU_FILE_MODULE "C:\\BlitzParty\\Game\\Src\\TankedUp\\CModule_TankedUp.cpp"
#define TU_FILE_GAME "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Game.cpp"
#define TU_FILE_BOOM "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Boom.cpp"
#define TU_FILE_SPAWN "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Spawn.cpp"
#define TU_FILE_CAMERA "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Camera.cpp"
#define TU_FILE_ROUTE "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Route.cpp"
#define TU_FILE_PLAYERS "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_Players.cpp"
#define TU_FILE_WORLD "C:\\BlitzParty\\Game\\Src\\TankedUp\\CTank_World.cpp"

/* ---------- the game object (0x00312f50) */

/* sub-object offsets inside CTank_Game */
#define TG_SOUND 0x0a34
#define TG_ENTITIES 0x0b20
#define TG_WORLD 0x0c18
#define TG_PLAYERS 0x0c38
#define TG_ROUTE 0x0cdc
#define TG_CAMERA 0x0d40
#define TG_SPAWN 0x0e30
#define TG_LIGHTS 0x0e34
#define TG_TIMER 0x0f78
#define TG_DEBRIS 0x0fc0
#define TG_BOOM 0x0fc4
#define TG_CHARS 0x0fd0

struct CTank_Game {
	int paused;                  /* +0x000 the game is paused */
	int started;                 /* +0x004 InitGameOneTime has run */
	int variation;               /* +0x008 1-4, -1 none */
	void *mode;                  /* +0x00c the CGameMode_TankedUp (tankedup_b, 0x2c bytes) */
	void *package;               /* +0x010 the common package */
	void *levelPackage;          /* +0x014 the variation's package */
	int valid;                   /* +0x018 the game details loaded */
	unsigned char list[0xc];     /* +0x01c an empty list head (CLList_Init) */
	char objectFile[0x200];      /* +0x028 OBJECTFILE (world objects) */
	char entityFile[0x200];      /* +0x228 ENTITYFILE */
	char routeFile[0x200];       /* +0x428 ROUTEFILE */
	char cameraFile[0x200];      /* +0x628 CAMERAFILE */
	char spawnFile[0x200];       /* +0x828 SPAWNFILE */
	int timerSeconds;            /* +0xa28 TIMER */
	int state;                   /* +0xa2c the game state (0-7; 7 = end) */
	int hud;                     /* +0xa30 the overlays were created */
	unsigned char systems[0x2160 - 0xa34]; /* +0xa34 the subsystems (TG_*) */
};
OF_ASSERT_OFFSET(struct CTank_Game, objectFile, 0x28);
OF_ASSERT_OFFSET(struct CTank_Game, timerSeconds, 0xa28);
OF_ASSERT_OFFSET(struct CTank_Game, systems, 0xa34);
OF_ASSERT_SIZE(struct CTank_Game, 0x2160);

#define TU_GAME (&TU_VAR(struct CTank_Game, 0x00312f50))
#define TU_SYS(game, off) ((void *)((char *)(game) + (off)))

/* CTank_Game methods missing from the function table (TankedUp/CTank_Game.c) */
void FUN_001c1a90(struct CTank_Game *self);
void FUN_001c1c10(struct CTank_Game *self);
void FUN_001c2080(struct CTank_Game *self);

/* ---------- the timer system (CTank_Game +0xf78; also inside the camera at +0x20) */

struct TuTimer {
	unsigned char running;       /* +0x00 */
	unsigned char paused;        /* +0x01 */
	unsigned char pad02[6];
	unsigned long long start;    /* +0x08 tick count at the start */
	unsigned long long rate;     /* +0x10 ticks per second */
	unsigned long long pausedAt; /* +0x18 tick count when paused */
	unsigned long long last;     /* +0x20 tick count of the last frame */
	float limit;                 /* +0x28 length in seconds */
	float limitMs;               /* +0x2c length in milliseconds */
	float frame;                 /* +0x30 the last frame's length (1.0 = 1/60 s) */
	float remaining;             /* +0x34 seconds left (>= 0) */
	float fraction;              /* +0x38 part of the length that has passed (0-1) */
	float elapsed;               /* +0x3c seconds since the start */
	int seconds;                 /* +0x40 the length given to Initialise (whole seconds) */
};
OF_ASSERT_OFFSET(struct TuTimer, rate, 0x10);
OF_ASSERT_OFFSET(struct TuTimer, limit, 0x28);
OF_ASSERT_OFFSET(struct TuTimer, seconds, 0x40);

/* ---------- the light system (CTank_Game +0xe34): 4 lights */

struct TuLight {
	void *light;                 /* +0x00 struct BLight * */
	int state;                   /* +0x04 0 free, 1 no light, 2 sun, 4 spot (a tank's), 5 tracking spot, 6 flash */
	int drawCone;                /* +0x08 the debug cone is drawn */
	float pos[4];                /* +0x0c */
	float target[4];             /* +0x1c */
	float cone;                  /* +0x2c the debug cone's angle */
	int r, g, b;                 /* +0x30 */
	int owner;                   /* +0x3c who placed a tracking spot */
	float time;                  /* +0x40 age of a flash */
	float duration;              /* +0x44 life of a flash (0 = forever) */
	float range;                 /* +0x48 a flash's start range factor */
	int sun;                     /* +0x4c the sun light (dimmed while paused) */
};
OF_ASSERT_SIZE(struct TuLight, 0x50);

/* ---------- the text reader of tankedup_b (vtable 0x0027bca8; 0x000a5c00 of bumpercars_b
   initialises it: the layouts are the same) */

struct TuTextReader {
	const void *vtable;          /* +0x000 */
	int unknown04;               /* +0x004 */
	char *cur;                   /* +0x008 the next character */
	char *end;                   /* +0x00c the end of the text */
	char name[0x200];            /* +0x010 the file name */
	int line;                    /* +0x210 the line number */
};
OF_ASSERT_SIZE(struct TuTextReader, 0x214);

/* ---------- the debug object of tankedup_b (0x003150b0): settings this module reads */

#define TU_DBG_CAMERA TU_VAR(int, 0x003150bc)      /* debug camera, -1 off */
#define TU_DBG_VARIATION TU_VAR(int, 0x003150c4)   /* the variation asked for */
#define TU_DBG_AMBIENT_R TU_VAR(int, 0x003150d0)   /* ambient light colour */
#define TU_DBG_AMBIENT_G TU_VAR(int, 0x003150d4)
#define TU_DBG_AMBIENT_B TU_VAR(int, 0x003150d8)
#define TU_DBG_SPOT_R TU_VAR(int, 0x003150dc)      /* tracking spot colour */
#define TU_DBG_SPOT_G TU_VAR(int, 0x003150e0)
#define TU_DBG_SPOT_B TU_VAR(int, 0x003150e4)
#define TU_DBG_SPOT_RANGE TU_VAR(float, 0x003150e8) /* tracking spot range (1 / factor) */
#define TU_DBG_SUN TU_VAR(int, 0x0031517c)         /* the sun lights are on */
#define TU_DBG_DRAW_WORLD TU_VAR(int, 0x00315180)
#define TU_DBG_DRAW_BOOM TU_VAR(int, 0x00315184)
#define TU_DBG_DRAW_ENTITIES TU_VAR(int, 0x00315188)
#define TU_DBG_DRAW_PLAYERS TU_VAR(int, 0x0031518c)
#define TU_DBG_DRAW_CHARS TU_VAR(int, 0x00315190)
#define TU_DBG_DRAW_DEBRIS TU_VAR(int, 0x00315198)
#define TU_DBG_SPOTS TU_VAR(int, 0x003151a0)       /* the tracking spots are on */

/* ---------- other modules' data */

extern int DAT_002ecdec;            /* classlib_ui: the game is paused */
extern unsigned char DAT_002ecd90[]; /* classlib_ui: the pause menu */
extern unsigned char DAT_002ba750[]; /* core: the game mode stack */
#define TU_MODESTACK ((struct CGameModeStack *)DAT_002ba750)
extern struct Settings *DAT_002ba9a0; /* core: the command-line / ini settings */
extern int DAT_00283e58;            /* core: slow motion (frames count 1.5 times) */
extern unsigned int DAT_002cd7a0;   /* bkernel: timer ticks per second, low and high word */
extern unsigned int DAT_002cd7a4;
extern float DAT_002855f0[16];      /* bkernel: the identity matrix */
/* bdisplay: colour channel tables (byte -> D3DCOLOR bits) */
extern unsigned int DAT_002bc490[];
extern unsigned int DAT_002bc898[];
extern unsigned int DAT_002bce18[];
extern unsigned int DAT_002bd640[];
extern unsigned int DAT_00285630;   /* bkernel: the game's inlined rand() seed */
extern int DAT_002baf08;            /* core: the pad port of player 1 (g_PlayerPort(1)) */
extern int DAT_002baeb8;            /* core: (g_MusicOn) the intro fades in while it is set */
extern int DAT_002ba928;            /* core: screen width, height */
extern int DAT_002ba92c;
extern int DAT_002ecfac;            /* classlib_ui: the HUD (CLHud *) */
extern int DAT_002bae98;            /* core: g_StartState (2: the end of a variation does not end the mode) */

#include "tankedup_a_bcalls.h"

#endif
