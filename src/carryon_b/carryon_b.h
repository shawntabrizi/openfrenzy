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
#include "d3d8_extra.h"   /* src/d3d8: D3DDevice_InsertFence */

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

/* the root node of an actor instance (bactor BActorInstance +0x134) */
#define CB_ACTOR_ROOT(inst) CB_AT(inst, 0x134, struct BActorNode *)

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

/* the kernel debug break (xkernel: the port logs it and continues) */
void DbgBreakPoint(void);

/* ---------- Babel engine helpers (bkernel) */

#define CB_NEW(size, line) bkHeapAlloc((size), CB_FILE, (line), 0x1001)
#define CB_DELETE(p) bkHeapFree((void *)(p))

/* ---------- globals of other modules (fallbacks in CarryOn/carryon_b_globals.c) */

extern int DAT_002bae94;            /* number of players (core) */
/* per-player records of the core game state, 10 ints (0x28 bytes) each; [0] is the
   character of the player, DAT_002baed4 is the same table one record later */
extern int DAT_002baeac[];
extern int DAT_002baed4[];
/* +4 in the same records: 0 for a human player */
extern int DAT_002baed8[];
/* character colour table, 0x60 bytes per character: red, green, blue at +0, +1, +2
   (classlib_ui/charlib) */
extern unsigned char DAT_00290718[];
extern unsigned char DAT_00290719[];
extern unsigned char DAT_0029071a[];
extern char DAT_002855f0[];         /* the identity matrix (float[16]) */
extern float DAT_002be330[16];      /* the camera rotation (bdisplay) */
extern unsigned int DAT_002bc490[]; /* colour channel tables: red, green, blue, alpha */
extern unsigned int DAT_002bd640[];
extern unsigned int DAT_002bc898[];
extern unsigned int DAT_002bce18[];
/* single entries of those tables used as constants:
   DAT_002bca94 = DAT_002bc898[127], DAT_002bc590 = DAT_002bc490[64],
   DAT_002bcf18 = DAT_002bce18[64], DAT_002bcc94 = DAT_002bc898[255],
   DAT_002bd840 = DAT_002bd640[128], DAT_002bc690 = DAT_002bc490[128],
   DAT_002bd018 = DAT_002bce18[128] */
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
extern int DAT_00293928;            /* AI voice volume (charlib) */
/* launch rack tweakables (0x00294390-0x00294398) */
extern int DAT_00294108;            /* tweak "V3:Parts/Firework" 3 */
extern int DAT_0029410c;            /* tweak "V3:Part Types" 3 */
extern int DAT_00294110;            /* tweak "V3:Non-Random Ponk" 1 */
extern int DAT_00294114;            /* tweak "Fireworks" 1 (draw them) */
extern int DAT_00294118;            /* tweak "Detail" 2 */
extern float DAT_00294120;          /* tweak "V1:Probability" 0.06 */
extern int DAT_00294124;            /* the most rockets in the arena 10 */
extern int DAT_00294128[];          /* the most rockets of each type (carryon_a's tweaks
                                       0x00294128-0x00294138 in one block) */
extern int DAT_002943b4;            /* tweak "V1:Count" 15: rockets in use */
extern int DAT_00294390;            /* slots 20 */
extern float DAT_00294394;          /* fill time 600.0 */
extern float DAT_00294398;          /* time between launches 7.5 */

/* ---------- module objects */

/* a rocket in the arena (0x3c bytes; ctor 0x000cb5d0): thrown in from the edges or
   dropped from the sky, it bounces, lies on the ground and is picked up by a player */
struct CFire_Rocket {
	struct CFire_Rocket *next;            /* +0x00 */
	float phase;                          /* +0x04 0-1 animation phase */
	float pos[4];                         /* +0x08 */
	float vel[4];                         /* +0x18 */
	float groundY;                        /* +0x28 */
	float floorY;                         /* +0x2c */
	int state;                            /* +0x30 0: free, 2-5: bouncing, 6-7: lying, 8: unused */
	int type;                             /* +0x34 */
	struct BActorInstance *actor;         /* +0x38 */
};

/* a player's launch pad (0x68 bytes): the player puts rockets on it (one of each colour
   slot), then it launches them as one firework shell */
struct CFire_Pad {
	struct CFire_Pad *next;               /* +0x00 */
	int count;                            /* +0x04 */
	int types[5];                         /* +0x08 */
	int player;                           /* +0x1c */
	float pos[4];                         /* +0x20 */
	float radius;                         /* +0x30 100 */
	int launchDelay;                      /* +0x34 */
	struct CFire_Fireworks *fireworks;    /* +0x38 */
	struct bkResource *model;             /* +0x3c */
	struct BActorInstance *actor;         /* +0x40 */
	struct BActorNode *top;               /* +0x44 node "c" */
	struct BActorNode *slots[3];          /* +0x48 nodes "a", "b", "c" */
	int unk54[2];                         /* +0x54 */
	struct BActorSubObject *launchAnim;   /* +0x5c "launch" */
	float addTime;                        /* +0x60 18 when a rocket is added */
	int lastType;                         /* +0x64 */
};

/* a place at the arena edge that throws rockets in (0x3c bytes) */
struct CFire_Spawner {
	struct CFire_Spawner *next;           /* +0x00 */
	float pos[4];                         /* +0x04 */
	int dir;                              /* +0x14 0, 1 or other: the throw direction */
	float timer;                          /* +0x18 */
	struct CFire_Rocket *slots[8];        /* +0x1c the rocket thrown at each of 8 angles */
};

/* the CarryOn arena (0x1c bytes; ctor 0x000ce110, DAT_002f7970) */
struct CFire_Arena {
	float *shadows;                       /* +0x00 6 vertices (0x24 bytes) per rocket */
	struct bkResource *shadowTexture;     /* +0x04 */
	int variant;                          /* +0x08 */
	struct CFire_Fireworks *fireworks;    /* +0x0c */
	struct CFire_Rocket *rockets;         /* +0x10 */
	struct CFire_Pad *pads;               /* +0x14 */
	struct CFire_Spawner *spawners;       /* +0x18 */
};

/* a pool of particles: count, the array, the used and the free list, and their
   sprites (CFire_Romans, CFire_FWorkStars, CFire_FWorkSmoke have this layout) */
struct CFire_Pool {
	int count;                            /* +0x00 */
	void *items;                          /* +0x04 */
	void *used;                           /* +0x08 */
	void *free;                           /* +0x0c */
	void *sprites;                        /* +0x10 CFire_PointSprites or CFire_QuadSprites */
};

/* a star of a firework burst (0x54 bytes): a point sprite that fades in, then out */
struct CFire_Star {
	struct CFire_Star *next;              /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[3];                         /* +0x14 */
	float unk20;                          /* +0x20 */
	float gravity;                        /* +0x24 */
	float size;                           /* +0x28 */
	float alpha;                          /* +0x2c 127 */
	float colour[3];                      /* +0x30 */
	float fade[3];                        /* +0x3c added to colour each step */
	float life;                           /* +0x48 */
	float fadeOutAt;                      /* +0x4c */
	int fadingOut;                        /* +0x50 */
};

/* the stars (0x14 bytes; init 0x000d77e0) */
struct CFire_FWorkStars {
	int count;                            /* +0x00 */
	struct CFire_Star *stars;             /* +0x04 */
	struct CFire_Star *used;              /* +0x08 */
	struct CFire_Star *free;              /* +0x0c */
	struct CFire_PointSprites *points;    /* +0x10 */
};

/* a smoke puff of a rocket trail (0x40 bytes) */
struct CFire_SmokePuff {
	struct CFire_SmokePuff *next;         /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[3];                         /* +0x14 */
	float unk20;                          /* +0x20 */
	float size;                           /* +0x24 */
	float alpha;                          /* +0x28 */
	float fade;                           /* +0x2c added to alpha each step */
	float grey[3];                        /* +0x30 */
	float life;                           /* +0x3c */
};

/* the rocket trail smoke (0x14 bytes; init 0x000d7c90) */
struct CFire_FWorkSmoke {
	int count;                            /* +0x00 */
	struct CFire_SmokePuff *puffs;        /* +0x04 */
	struct CFire_SmokePuff *used;         /* +0x08 */
	struct CFire_SmokePuff *free;         /* +0x0c */
	struct CFire_QuadSprites *quads;      /* +0x10 */
};

/* a rising firework rocket (0x34 bytes) */
struct CFire_FWRocket {
	struct CFire_FWRocket *next;          /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[4];                         /* +0x14 */
	float colour[3];                      /* +0x24 */
	float fuse;                           /* +0x30 */
};

/* a firework shell (0x7c bytes): rises, leaves a trail and bursts */
struct CFire_Shell {
	struct CFire_Shell *next;             /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[4];                         /* +0x14 */
	float gravity;                        /* +0x24 */
	float axis[4];                        /* +0x28 the spin axis of spiral trails */
	signed char types[5];                 /* +0x38 burst, ring, split, trail, spiral */
	char pad3d[3];
	int unk40;                            /* +0x40 */
	float alpha;                          /* +0x44 127 */
	float colourVel[4];                   /* +0x48 added to colour and alpha */
	float colour[3];                      /* +0x58 */
	float burstColour[3];                 /* +0x64 */
	float fuse;                           /* +0x70 */
	int player;                           /* +0x74 */
	int strength;                         /* +0x78 */
};

/* the fireworks (0x3c bytes) */
struct CFire_Fireworks {
	struct CFire_FWorkSmoke *smoke;       /* +0x00 */
	struct CFire_Glitter *glitter;        /* +0x04 */
	struct CFire_Sparks *sparks;          /* +0x08 */
	struct CFire_FWorkStars *stars;       /* +0x0c */
	struct CFire_FWorkStars *stars2;      /* +0x10 */
	int numRockets;                       /* +0x14 */
	struct CFire_FWRocket *rockets;       /* +0x18 */
	struct CFire_FWRocket *usedRockets;   /* +0x1c */
	struct CFire_FWRocket *freeRockets;   /* +0x20 */
	int numShells;                        /* +0x24 */
	struct CFire_Shell *shells;           /* +0x28 */
	struct CFire_Shell *usedShells;       /* +0x2c */
	struct CFire_Shell *freeShells;       /* +0x30 */
	struct CFire_QuadSprites *quads;      /* +0x34 */
	void *clFireworks;                    /* +0x38 classlib_ui CLFireworks (0x1c bytes) */
};

/* a smoke puff (0x4c bytes) */
struct CFire_Puff {
	struct CFire_Puff *next;              /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[3];                         /* +0x14 */
	float unk20;                          /* +0x20 */
	float size;                           /* +0x24 */
	float grow;                           /* +0x28 */
	float alpha;                          /* +0x2c 255 */
	float colour[3];                      /* +0x30 */
	float colourVel[3];                   /* +0x3c */
	float life;                           /* +0x48 */
};

/* smoke (DAT_002f798c) */
struct CFire_Smoke {
	struct CFire_QuadSprites *quads;      /* +0x00 */
	struct CFire_Puff *puffs;             /* +0x04 */
	struct CFire_Puff *used;              /* +0x08 */
	struct CFire_Puff *free;              /* +0x0c */
};

/* a section of the frame timer (0x18 bytes) */
struct CFire_TimerSection {
	unsigned long long now;               /* +0x00 */
	unsigned long long start;             /* +0x08 */
	float ms;                             /* +0x10 */
	int pad;                              /* +0x14 */
};

/* the frame timer of the CarryOn game (vtable 0x00268b2c; in the game object at +0x18) */
struct CFire_Timer {
	const void *const *vtbl;              /* +0x000 */
	int unk04;                            /* +0x004 */
	struct CFire_TimerSection sections[5]; /* +0x008 */
	unsigned long long ticksPerSecond;    /* +0x080 */
	unsigned long long now;               /* +0x088 */
	unsigned long long last;              /* +0x090 */
	float ms;                             /* +0x098 */
	float frames;                         /* +0x09c */
	int marks;                            /* +0x0a0 */
	unsigned int markColours[50];         /* +0x0a4 */
	float markTimes[50];                  /* +0x16c */
};

/* the CarryOn game mode (vtable 0x00268b38): a CGameMode (core_modes_a, 0x28 bytes) and
   the clear colour that it saves */
struct CGameMode_CarryOn {
	unsigned char base[0x28];             /* +0x00 CGameMode: +0x04 unk04, +0x10 flags, +0x14 */
	unsigned int savedColour[4];          /* +0x28 */
};


/* point sprites in vertex buffers (0x9c bytes; the same layout as classlib_fx's
   CPointSprites; ctor 0x000d2750, init 0x000d27a0) */
struct CFire_PointSprites {
	int vertsPerBuffer;                   /* +0x00 at most 5688 */
	struct bdVertexBuffer *buffers[32];   /* +0x04 at most 28 are created */
	float *write;                         /* +0x84 the locked vertices of the current buffer */
	int vertsLeft;                        /* +0x88 */
	int buffer;                           /* +0x8c the current buffer */
	int blendMode;                        /* +0x90 0 opaque, 1-4: Blend_SetMode 0, 1, 3, 2 */
	float pointSize;                      /* +0x94 */
	struct bkResource *texture;           /* +0x98 bound to stage 3 */
};

/* camera-facing quads in vertex buffers, 6 vertices each (0xc8 bytes; ctor 0x000d24c0,
   init 0x000d24e0) */
struct CFire_QuadSprites {
	int vertsPerBuffer;                   /* +0x00 */
	struct bdVertexBuffer *buffers[32];   /* +0x04 */
	float *write;                         /* +0x84 */
	int vertsLeft;                        /* +0x88 */
	int buffer;                           /* +0x8c */
	int blendMode;                        /* +0x90 */
	struct bkResource *texture;           /* +0x94 */
	float corners[12];                    /* +0x98 the camera's right +- up, x, y, z */
};

/* a glitter bit (0x24 bytes) */
struct CFire_GlitterBit {
	struct CFire_GlitterBit *next;        /* +0x00 */
	float pos[3];                         /* +0x04 */
	float size;                           /* +0x10 */
	float vel[3];                         /* +0x14 */
	float life;                           /* +0x20 */
};

/* glitter (0x118 bytes): point sprites that fall and slow down */
struct CFire_Glitter {
	unsigned int colours[64];             /* +0x000 */
	int colour;                           /* +0x100 next colour */
	int count;                            /* +0x104 */
	struct CFire_GlitterBit *bits;        /* +0x108 */
	struct CFire_GlitterBit *used;        /* +0x10c */
	struct CFire_GlitterBit *free;        /* +0x110 */
	struct CFire_PointSprites *points;    /* +0x114 */
};

/* a spark (0xe8 bytes): a short trail of up to 9 positions drawn as one beam from the
   oldest to the newest position; colours are 8.7 fixed point and fade each update */
struct CFire_Spark {
	int active;                           /* +0x00 */
	short colour[3];                      /* +0x04 tail colour (a quarter of the head's) */
	short pad0a;
	short colour2[3];                     /* +0x0c head colour */
	short pad12;
	short fade[3];                        /* +0x14 added to colour each update */
	short pad1a;
	short fade2[3];                       /* +0x1c added to colour2 each update */
	short pad22;
	short age;                            /* +0x24 */
	short life;                           /* +0x26 */
	short fadeIn;                         /* +0x28 */
	short pad2a;
	float trail[9][4];                    /* +0x2c the newest first */
	int length;                           /* +0xbc */
	float tail[4];                        /* +0xc0 */
	float vel[3];                         /* +0xd0 */
	float unkdc;                          /* +0xdc */
	float width;                          /* +0xe0 */
	signed char maxLength;                /* +0xe4 */
	char pad[3];
};

/* the sparks (0x20 bytes; vtable 0x00268bd8, ctor 0x000d2870) */
struct CFire_Sparks {
	const void *const *vtbl;              /* +0x00 */
	float drag;                           /* +0x04 0.97 */
	float gravity;                        /* +0x08 -0.2 */
	float spread;                         /* +0x0c 45.0 */
	struct bdVertexBuffer *vb;            /* +0x10 */
	struct bkResource *texture;           /* +0x14 */
	int count;                            /* +0x18 */
	struct CFire_Spark *sparks;           /* +0x1c */
};

/* a trail (0xac bytes): beams from the trail positions, relative to an origin */
struct CFire_Trail {
	int active;                           /* +0x00 */
	float *origin;                        /* +0x04 */
	short colour[3];                      /* +0x08 tail colour */
	short pad0e;
	short colour2[3];                     /* +0x10 head colour */
	short pad16;
	short age;                            /* +0x18 */
	short life;                           /* +0x1a */
	short fadeIn;                         /* +0x1c */
	short pad1e;
	float trail[8][4];                    /* +0x20 */
	int length;                           /* +0xa0 */
	float width;                          /* +0xa4 */
	signed char maxLength;                /* +0xa8 */
	char pad[3];
};

/* the trails (0x10 bytes; init 0x000d2b00) */
struct CFire_Trails {
	struct bdVertexBuffer *vb;            /* +0x00 */
	struct bkResource *texture;           /* +0x04 */
	int count;                            /* +0x08 */
	struct CFire_Trail *trails;           /* +0x0c */
};

/* a Roman candle (0x88 bytes) */
struct CFire_Roman {
	struct CFire_Roman *next;             /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[3];                         /* +0x14 */
	float unk20;                          /* +0x20 */
	float gravity;                        /* +0x24 */
	float size;                           /* +0x28 */
	float brightness;                     /* +0x2c 96 at the start */
	float fade;                           /* +0x30 */
	float colour[3];                      /* +0x34 */
	float timer;                          /* +0x40 */
	int state;                            /* +0x44 0: rising, 1: burning, 2: falling, 3: out */
	float fallPos[4];                     /* +0x48 */
	float burnPos[4];                     /* +0x58 */
	float burnVel;                        /* +0x68 */
	float fallStart[4];                   /* +0x6c */
	float fallVel;                        /* +0x7c */
	float ground;                         /* +0x80 */
	struct CFire_Light *light;            /* +0x84 */
};

/* the Roman candles (0x14 bytes) */
struct CFire_Romans {
	int count;                            /* +0x00 */
	struct CFire_Roman *romans;           /* +0x04 */
	struct CFire_Roman *used;             /* +0x08 */
	struct CFire_Roman *free;             /* +0x0c */
	struct CFire_QuadSprites *quads;      /* +0x10 */
};

/* a glow pot of the V2 world (0x34 bytes, ctor 0x000cc870): the players bring their
   rockets to it */
struct CFire_GlowPot {
	struct CFire_GlowPot *next;           /* +0x00 */
	int score;                            /* +0x04 glitter collected */
	int player;                           /* +0x08 */
	float pos[4];                         /* +0x0c */
	float radius;                         /* +0x1c 300 */
	struct bkResource *model;             /* +0x20 */
	struct BActorInstance *actor;         /* +0x24 */
	struct BActorSubObject *fill;         /* +0x28 */
	float fillFrac;                       /* +0x2c */
	float fillFrac2;                      /* +0x30 */
};

/* a falling glitter particle of the V2 world (0x28 bytes) */
struct CFire_V2Bit {
	struct CFire_V2Bit *next;             /* +0x00 */
	float pos[4];                         /* +0x04 */
	float vel[3];                         /* +0x14 */
	float bounced;                        /* +0x20 */
	unsigned char rgb[3];                 /* +0x24 */
	unsigned char player;                 /* +0x27 */
};

/* the V2 (glitter) world of CarryOn (0x128 bytes, DAT_002f7968) */
struct CFire_V2Glitter {
	struct CFire_Romans *romans;          /* +0x000 */
	struct CFire_Glitter *glitter;        /* +0x004 */
	struct CFire_Sparks *sparks;          /* +0x008 */
	struct CFire_Trails *trails;          /* +0x00c */
	struct CFire_GlowPot *pots[4];        /* +0x010 */
	int frame;                            /* +0x020 */
	struct CFire_GlowPot *potList;        /* +0x024 */
	int unk28;                            /* +0x028 */
	struct CFire_Light *beamLights[4];    /* +0x02c */
	struct CFire_Light *spots[2];         /* +0x03c */
	struct CFire_Light *flash;            /* +0x044 */
	float spin;                           /* +0x048 */
	float beamPos[4][4];                  /* +0x04c */
	float beamVel[4][4];                  /* +0x08c */
	float beamTime[4];                    /* +0x0cc */
	int unkdc[6];                         /* +0x0dc */
	float flashTime;                      /* +0x0f4 */
	int flashPlayer;                      /* +0x0f8 */
	int pending[4];                       /* +0x0fc glitter still to drop per player */
	struct bkResource *dishModel;         /* +0x10c */
	struct BActorInstance *dish;          /* +0x110 */
	struct BActorSubObject *dishy;        /* +0x114 */
	struct CFire_PointSprites *points;    /* +0x118 */
	struct CFire_V2Bit *bits;             /* +0x11c */
	struct CFire_V2Bit *used;             /* +0x120 */
	struct CFire_V2Bit *free;             /* +0x124 */
};

extern struct CFire_V2Glitter *DAT_002f7968;
extern float DAT_002943d0[4];         /* the arena centre (400, -200, -900) */
extern void *DAT_00293ae8;            /* the arena data (+0x2378 pot positions, +0x23b8 dish) */
extern void *DAT_002ecfac;            /* the HUD (classlib_ui) */

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
	float unk10;                          /* +0x10 (0.314 from carryon_a) */
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



/* ---------- functions missing from of_functions.tsv (defined here; reported) */

/* 0x000cf190  the burst callback of CarryOn's classlib_ui fireworks (CarryOn/CFire_Fireworks.c) */
void FUN_000cf190(void *fireworks, float *rocket);

/* ---------- layout checks */

OF_ASSERT_SIZE(struct CFire_ConveyorItem, 0x48);
OF_ASSERT_SIZE(struct CFire_Conveyor, 0x24);
OF_ASSERT_SIZE(struct CFire_LaunchRack, 0x130);
OF_ASSERT_SIZE(struct CFire_Light, 0x50);
OF_ASSERT_SIZE(struct CFire_Lights, 0x5c);
OF_ASSERT_SIZE(struct CFire_PointSprites, 0x9c);
OF_ASSERT_SIZE(struct CFire_QuadSprites, 0xc8);
OF_ASSERT_SIZE(struct CFire_GlitterBit, 0x24);
OF_ASSERT_SIZE(struct CFire_Glitter, 0x118);
OF_ASSERT_SIZE(struct CFire_Spark, 0xe8);
OF_ASSERT_SIZE(struct CFire_Sparks, 0x20);
OF_ASSERT_SIZE(struct CFire_Trail, 0xac);
OF_ASSERT_SIZE(struct CFire_Trails, 0x10);
OF_ASSERT_SIZE(struct CFire_Roman, 0x88);
OF_ASSERT_SIZE(struct CFire_Romans, 0x14);
OF_ASSERT_SIZE(struct CFire_GlowPot, 0x34);
OF_ASSERT_SIZE(struct CFire_V2Bit, 0x28);
OF_ASSERT_SIZE(struct CFire_V2Glitter, 0x128);
OF_ASSERT_SIZE(struct CFire_Rocket, 0x3c);
OF_ASSERT_SIZE(struct CFire_Pad, 0x68);
OF_ASSERT_SIZE(struct CFire_Spawner, 0x3c);
OF_ASSERT_SIZE(struct CFire_Arena, 0x1c);
OF_ASSERT_SIZE(struct CFire_Star, 0x54);
OF_ASSERT_SIZE(struct CFire_SmokePuff, 0x40);
OF_ASSERT_SIZE(struct CFire_FWRocket, 0x34);
OF_ASSERT_SIZE(struct CFire_Shell, 0x7c);
OF_ASSERT_SIZE(struct CFire_Fireworks, 0x3c);
OF_ASSERT_SIZE(struct CFire_Puff, 0x4c);
OF_ASSERT_OFFSET(struct CFire_Timer, markTimes, 0x16c);
OF_ASSERT_OFFSET(struct CFire_Timer, ticksPerSecond, 0x80);
OF_ASSERT_SIZE(struct CGameMode_CarryOn, 0x38);
OF_ASSERT_OFFSET(struct CFire_Spark, trail, 0x2c);
OF_ASSERT_OFFSET(struct CFire_Spark, maxLength, 0xe4);
OF_ASSERT_OFFSET(struct CFire_Trail, trail, 0x20);
OF_ASSERT_OFFSET(struct CFire_Roman, light, 0x84);
OF_ASSERT_OFFSET(struct CFire_V2Glitter, flashTime, 0xf4);
OF_ASSERT_OFFSET(struct CFire_Shell, fuse, 0x70);

#endif
