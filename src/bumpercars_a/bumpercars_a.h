/*
BUMPERCARS_A.H

Module bumpercars_a: the BumperCars minigame, part 1 (0x00080d00-0x000952e0), and the code
the linker placed there from other directories:

  ClassLib/Debris.cpp     0x00080bf0-0x000827f0  the "debris" extra system (shared by every
                                                 minigame), 0x00080d00-0x00081ed0 its API
  ClassLib/DebrisSys.cpp  0x000868f0-0x00086b70  wind and swirls of the debris system (gap
                                                 functions; DebrisSys_InitWind 0x00086880 is
                                                 classlib_fx)
  BumperCars (*.cpp)      CModule_BumperCars, CBCar_Game, CBCar_Debug, CBCar_Camera,
                          CBCar_Goal, CBCar_Sound, CBCar_Light, CBCar_Flame and gap functions

Owner: bumpercars_a. Other modules may include this header; only bumpercars_a edits it.

Conventions (docs/ARCHITECTURE.md):
- Layouts are the original 32-bit ones; offsets are checked with OF_ASSERT_OFFSET.
- Fields not understood yet are reached with BCA_I32/BCA_F32/... and the original offset.
- Virtual calls go through BCA_VFN(obj, byte offset of the slot, function type), with the
  object as the first argument (the x86 code passed it in ECX).
- Calls into other modules whose table prototype has the wrong shape (a lost ECX `this`,
  float parameters typed undefined4) go through a cast to the real shape, named here.
- Globals shared with other modules keep their Ghidra names (DAT_<addr>).
*/

#ifndef OPENFRENZY_BUMPERCARS_A_H
#define OPENFRENZY_BUMPERCARS_A_H

#include "xtl.h"
#include "bactor.h"
#include "engine_misc_maths.h"
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== helpers */

#define BCA_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define BCA_I32(p, off) BCA_AT(p, off, int)
#define BCA_U32(p, off) BCA_AT(p, off, unsigned int)
#define BCA_F32(p, off) BCA_AT(p, off, float)
#define BCA_PTR(p, off) BCA_AT(p, off, void *)
#define BCA_U8(p, off) BCA_AT(p, off, unsigned char)

/* virtual function at byte offset `off` of the object's vtable */
#define BCA_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* bKernel heap (bkernel bkHeapAlloc / bkHeapFree): operator new and delete */
#define BCA_NEW(size, file, line, flags) bkHeapAlloc((uint)(size), (file), (line), (flags))
#define BCA_DELETE(p) bkHeapFree((void *)(p))

/* the game's debug print (bkernel bkPrintf) */
#define BCA_Printf bkPrintf

/* x87 _ftol (0x001de998): truncating float to integer conversion */
#define BCA_FTOL(x) ((int)(longlong)(x))

/* MSVC rand(), inlined into the game: holdrand is the C library's seed at 0x00285630 */
extern unsigned int DAT_00285630;
static inline int bca_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int bca_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

static inline float bca_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* ======================================================================== other modules' globals */

/* 0x0028fda8  frame time scale: 1.0 at the nominal frame rate (core) */
extern float DAT_0028fda8;
/* 0x0028fdc8  ClassLib up vector (0, 1, 0) (classlib_ui data) */
extern float DAT_0028fdc8[3];
/* 0x002ea478  a zero vector (classlib_ui) */
extern float DAT_002ea478[4];
/* 0x002ba990  frames per second (core g_Fps) */
extern float DAT_002ba990;
/* 0x00290ea8  the debris vertex buffer "debrisVB" (classlib_fx CSpriteVB) */
extern unsigned int DAT_00290ea8[];
/* 0x00290cb4  debris sprite size scale (25.0); 0x00290cb8 debris sprites on (classlib_fx data) */
extern float DAT_00290cb4;
extern int DAT_00290cb8;
/* 0x00290ed8  debris drawing on (classlib_fx data) */
extern int DAT_00290ed8;
/* 0x00285644  the ray tests' epsilon (engine_misc data) */
extern float DAT_00285644;
/* 0x002ed44c  the "actordebris" system (classlib_fx 0x00086160) */
extern int DAT_002ed44c;

/* ======================================================================== ClassLib/Debris.cpp */

struct BExtraSystem;

/* the stage table of a debris type (DebrisType.params). Stages 0-2 run in turn; the colour
and size of a piece blend from stage i to stage i+1 over the stage. */
struct DebrisParams {
	int alpha[4];      /* 0x00 */
	int red[4];        /* 0x10 */
	int green[4];      /* 0x20 */
	int blue[4];       /* 0x30 */
	float life[3];     /* 0x40 seconds of stages 0-2 */
	float lifeRandom;  /* 0x4c added to stage 1 */
	float size[4];     /* 0x50 */
	float spin;        /* 0x60 rotation speed */
	float spinRandom;  /* 0x64 */
	float maxSpeed;    /* 0x68 0: no limit */
	float drag;        /* 0x6c velocity factor per frame */
	float bounce;      /* 0x70 restitution on the ground */
	float gravity;     /* 0x74 */
};
OF_ASSERT_OFFSET(struct DebrisParams, gravity, 0x74);

/* a streak drawn behind a piece (DebrisType.streak) */
struct DebrisStreak {
	float length;      /* 0x00 */
	float width0a;     /* 0x04 */
	float width1a;     /* 0x08 */
	float width0b;     /* 0x0c */
	float width1b;     /* 0x10 */
	int alpha;         /* 0x14 0-0x7f scale of the colour */
};

/* a debris type: what Debris_Add makes (passed by every minigame) */
struct DebrisType {
	const char *name;              /* 0x00 "STEAM1", "RESPAWNCHAR", ... */
	struct DebrisParams *params;   /* 0x04 */
	float *spinInit;               /* 0x08 8 floats: axis/angle and rates, or NULL */
	void *spriteAnim;              /* 0x0c CSpriteAnim (classlib_fx) */
	int actorDebris;               /* 0x10 nonzero: an actor debris type (other system) */
	float windResistance;          /* 0x14 */
	struct DebrisType *onDeath;    /* 0x18 spawned when a piece dies */
	struct DebrisType *trail;      /* 0x1c spawned along the path */
	float trailInterval;           /* 0x20 */
	struct DebrisStreak *streak;   /* 0x24 NULL: a sprite */
	unsigned int flags;            /* 0x28 see DEBRIS_* */
	/* 0x2c: colour ranges (bytes) used when flag 0x40000 is set */
};
/* DebrisType.flags */
#define DEBRIS_GRAVITY      0x00000001u /* (the piece's flags) */
#define DEBRIS_WIND         0x00000008u
#define DEBRIS_COLLIDE      0x00000080u
#define DEBRIS_KEEP_ON_STOP 0x00000200u
#define DEBRIS_STREAK_ABS   0x00002000u
#define DEBRIS_NO_FALL      0x00020000u
#define DEBRIS_RANDOM_RGB   0x00040000u
#define DEBRIS_GROUND       0x00080000u

/* the spin state of a piece (0x30 bytes): DebrisType.spinInit gives axisAngle and the
last four fields. The piece is drawn at pos + perp rotated about the axis, times radius. */
struct DebrisSpin {
	float axisAngle[4];  /* 0x00 axis xyz, angle */
	float perp[4];       /* 0x10 a vector perpendicular to the axis */
	float angleRate;     /* 0x20 */
	float radius;        /* 0x24 */
	float radiusRate;    /* 0x28 */
	unsigned int flags;  /* 0x2c bit 0: the axis follows the velocity */
};
OF_ASSERT_SIZE(struct DebrisSpin, 0x30);

/* one piece of debris: an item of the "debris" extra system (0xdc bytes) */
struct DebrisPiece {
	struct BExtraSystem *system;   /* 0x00 */
	float pos[4];                  /* 0x04 */
	float offset[4];               /* 0x14 spin offset added to pos */
	float drawPos[4];              /* 0x24 */
	float drawVel[4];              /* 0x34 */
	float vel[4];                  /* 0x44 */
	float unknown54[3];            /* 0x54 */
	float unknown60;               /* 0x60 1.0 */
	float unknown64;               /* 0x64 */
	float groundY;                 /* 0x68 */
	float groundBounce;            /* 0x6c */
	struct DebrisParams *params;   /* 0x70 */
	struct DebrisSpin *spin;       /* 0x74 */
	struct DebrisType *type;       /* 0x78 */
	float time;                    /* 0x7c left in the stage */
	float stageTime;               /* 0x80 */
	float age;                     /* 0x84 */
	float size;                    /* 0x88 */
	float scale;                   /* 0x8c */
	float trailTime;               /* 0x90 */
	unsigned char rgba[4];         /* 0x94 r, g, b, a */
	unsigned int colour;           /* 0x98 0xffffffff, or an 0x00rrggbb override */
	int stage;                     /* 0x9c -1 before the first */
	unsigned int flags;            /* 0xa0 the type's flags */
	unsigned char sprite[0x34];    /* 0xa4 CSprite (classlib_fx); +0x10 its position */
	int batch;                     /* 0xd8 CSpriteVBBatch of the debris vertex buffer */
};
OF_ASSERT_SIZE(struct DebrisPiece, 0xdc);
OF_ASSERT_OFFSET(struct DebrisPiece, flags, 0xa0);

/* the wind of the debris system (0x128 bytes, made by DebrisSys_InitWind) */
struct DebrisGust {
	float dir[3];      /* 0x00 */
	float unknown0c;   /* 0x0c */
	float period;      /* 0x10 */
	float minimum;     /* 0x14 */
	float maximum;     /* 0x18 */
};
struct DebrisSwirl {
	float pos[4];      /* 0x00 */
	float radius;      /* 0x10 */
	float maximum;     /* 0x14 */
	float minimum;     /* 0x18 */
	float strength;    /* 0x1c */
	float period;      /* 0x20 */
};
struct DebrisWind {
	float dir[4];                   /* 0x000 this frame */
	float base[4];                  /* 0x010 */
	struct DebrisGust gusts[4];     /* 0x020 */
	struct DebrisSwirl swirls[4];   /* 0x090 */
	int numGusts;                   /* 0x120 */
	int numSwirls;                  /* 0x124 */
};
OF_ASSERT_SIZE(struct DebrisWind, 0x128);

/* the user data of the debris system */
struct DebrisSysData {
	float time;               /* 0x0 */
	struct DebrisWind *wind;  /* 0x4 */
};

/* the ground test the game gives Debris_Init: from pos, moved by delta; writes the hit
point and the surface normal (w: bounce factor); returns the hit fraction (0: no hit) */
typedef float (*DebrisGroundFn)(float *pos, float *delta, float *hit, float *normal);

/* 0x002ed33c-0x002ed39c: the debris globals (block of_bca_debris in bca_data.c) */
extern DebrisGroundFn DAT_002ed33c;     /* the ground test */
extern int DAT_002ed340;                /* debris off */
extern int DAT_002ed344;                /* live pieces */
extern int DAT_002ed348;                /* kill pieces flagged 0x8000 too */
extern struct BExtraSystem *DAT_002ed34c; /* the "debris" system */
/* four colours (0x002ed350, 4 x 4 floats) and their "set" flags (0x002ed390) */
extern float DAT_002ed350[4][4];
extern int DAT_002ed390[4];

/* Debris.cpp functions missing from the function table (callbacks of the debris system;
the table functions are declared in of_decls.h from bumpercars_a.decls) */
int FUN_00080bf0(struct BExtraSystem *system, struct DebrisPiece *piece);           /* delete */
int FUN_00080c50(struct BExtraSystem *system, int reason);                          /* system */
int FUN_00080f40(struct BExtraSystem *system, struct DebrisPiece *piece, void *args); /* create */
int FUN_000812b0(struct BExtraSystem *system, int count, struct DebrisPiece **items, void *arg); /* draw */
int FUN_00082010(struct BExtraSystem *system, int count, struct DebrisPiece **items, void *arg); /* update */

/* ======================================================================== BumperCars: the game object */

/* a call through a cast: the shape the x86 code used (ECX `this` first, floats as floats).
Used for functions whose table prototype is still Ghidra's guess; the owner must agree. */
#define BCA_FN(fn, ret, params) ((ret(*) params)(fn))

/* a player slot of the game (0x2e0 bytes, 4 at CBCar_Game + 0x2750). The class is
bumpercars_b's (CBCar_Player); this module reaches its fields by offset (BCA_*). */
#define BCAR_PLAYER_SIZE 0x2e0

/* the bump-text reader of bumpercars_b (0x000a5be0 constructor, 0x000a5c00 open): a text file
in memory, read line by line (foxhunt 0x001386c0 / 0x00138720) */
struct BCarTextReader {
	const void *const *vtable;  /* 0x000 0x0026689c */
	char *text;                 /* 0x004 */
	char *cursor;               /* 0x008 */
	char *end;                  /* 0x00c */
	char name[0x200];           /* 0x010 */
	int line;                   /* 0x210 */
};
OF_ASSERT_SIZE(struct BCarTextReader, 0x214);

/* CBCar_Game (BumperCars/CBCar_Game.cpp): the minigame. One static object at 0x002ed458,
built by the static constructor 0x000872b0 (core crt) and used by CModule_BumperCars and
bumpercars_b's CGameMode_BumperCars (0x0026684c), which forward their mode calls here.
The sub-objects are other classes of BumperCars (most of them bumpercars_b's). */
struct CBCar_Game {
	const void *const *vtable;      /* 0x0000 0x00263b80 */
	int unknown0004;                /* 0x0004 */
	int variation;                  /* 0x0008 the "variation" setting (1-4) */
	void *mode;                     /* 0x000c the CGameMode_BumperCars (bumpercars_b, 0x2c bytes) */
	struct bkPackage *package;      /* 0x0010 packages\bumper */
	struct bkPackage *variantPackage; /* 0x0014 packages\bump-vNN */
	unsigned char debug[0x44];      /* 0x0018 CBCar_Debug (0x0008bf50) */
	unsigned char effects[0x25e4];  /* 0x005c the debris effects (constructor 0x00162f20, shared) */
	unsigned char camera[0x110];    /* 0x2640 CBCar_Camera (0x0008d1e0) */
	unsigned char players[4][BCAR_PLAYER_SIZE]; /* 0x2750 (0x00086b80) */
	unsigned char unknown32d0[0x40]; /* 0x32d0 */
	unsigned char goal[0x10];       /* 0x3310 CBCar_Goal (0x000919e0) */
	unsigned char sound[0x2e4];     /* 0x3320 CBCar_Sound (0x00091ef0) */
	unsigned char light[0x924];     /* 0x3604 CBCar_Light (0x000924e0) */
	unsigned char unknown3f28[0xc]; /* 0x3f28 (0x00094360) */
	void *floatyScores;             /* 0x3f34 the floating score system (bumpercars_b) */
	char worldInfo[0x200];          /* 0x3f38 game.cfg WORLDINFO */
	char playerInfo[0x200];         /* 0x4138 PLAYERINFO */
	char scoreInfo[0x200];          /* 0x4338 SCOREINFO */
	char lightInfo[0x200];          /* 0x4538 LIGHTINFO */
	char cameraInfo[0x200];         /* 0x4738 CAMERAINFO */
	char goalInfo[0x200];           /* 0x4938 GOALINFO */
	float gameTime;                 /* 0x4b38 GAMETIMER (seconds) */
	float timeLeft;                 /* 0x4b3c */
	int playerGoals;                /* 0x4b40 PLAYERGOALS */
	int state;                      /* 0x4b44 0: packages missing, 1: loaded/starting, 3: playing, 5 */
	int started;                    /* 0x4b48 */
	int unknown4b4c;                /* 0x4b4c -1 */
	unsigned char timer[0x60];      /* 0x4b50 the timer system (bumpercars_b 0x00096320) */
	int unknown4bb0[4];             /* 0x4bb0 */
	int paused;                     /* 0x4bc0 the game was paused */
	int list[3];                    /* 0x4bc4 a ClassLib list (CLList_Init) */
	float crowdTimer;               /* 0x4bd0 frames to the next crowd cheer */
	int unknown4bd4;                /* 0x4bd4 */
	int savedLight[4];              /* 0x4bd8 core's light colour (DAT_002ba940) while BumperCars runs */
	void *explosions;               /* 0x4be8 the explosion system (bumpercars_b, 8 bytes) */
	int unknown4bec;                /* 0x4bec */
};
OF_ASSERT_OFFSET(struct CBCar_Game, camera, 0x2640);
OF_ASSERT_OFFSET(struct CBCar_Game, players, 0x2750);
OF_ASSERT_OFFSET(struct CBCar_Game, goal, 0x3310);
OF_ASSERT_OFFSET(struct CBCar_Game, sound, 0x3320);
OF_ASSERT_OFFSET(struct CBCar_Game, light, 0x3604);
OF_ASSERT_OFFSET(struct CBCar_Game, floatyScores, 0x3f34);
OF_ASSERT_OFFSET(struct CBCar_Game, gameTime, 0x4b38);
OF_ASSERT_OFFSET(struct CBCar_Game, timer, 0x4b50);
OF_ASSERT_OFFSET(struct CBCar_Game, list, 0x4bc4);
OF_ASSERT_OFFSET(struct CBCar_Game, explosions, 0x4be8);
OF_ASSERT_SIZE(struct CBCar_Game, 0x4bf0);

/* 0x002ed458  the game (block of_bca_game in BumperCars/bca_data.c) */
extern struct CBCar_Game DAT_002ed458;
#define g_BCarGame (&DAT_002ed458)
/* a field of the game by its offset (other code reaches it by absolute address) */
#define BCG(off, type) BCA_AT(&DAT_002ed458, off, type)
/* the debris ground test of BumperCars (0x0008cc10, missing from the table) */
float FUN_0008cc10(float *pos, float *delta, float *point, float *normal);

/* CBCar_Game methods missing from the function table (reached by jumps) */
void FUN_00086b90(void *slot);                               /* player slot destructor */
int FUN_00086c80(struct CBCar_Game *self);                   /* one-time init */
void FUN_00086c90(struct CBCar_Game *self);                  /* mode enter */
void FUN_00086ea0(struct CBCar_Game *self);                  /* mode resume */
void FUN_00086ed0(struct CBCar_Game *self);                  /* mode suspend */
void FUN_00086f80(struct CBCar_Game *self, int top);         /* mode update */
void FUN_000874c0(struct CBCar_Game *self, int top);         /* mode draw */

/* a field of player slot i by its offset in the slot */
#define BCAR_PLAYER(game, i, off, type) BCA_AT((game)->players[i], off, type)


#endif
