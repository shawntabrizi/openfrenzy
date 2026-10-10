/*
demolition_d.h - the demolition_d module (Minigame Demolition, part 4 of 4: 0x001059e0-0x0010f5e0)

Original sources: C:\BlitzParty\Game\Src\Demolition\ (and one ClassLib file)
  Demolition/unplaced_ai.c          (gap) the computer players' brains: one class per game
                                    variation (vtables 0x0026bb58, 0x0026bb7c, 0x0026bba0,
                                    0x0026bbc4), created by demolition_b's CDemolition_CharLib
                                    setup (0x000f8dc0)
  Demolition/CDemolition_EntityEvent.c  the entity event queue (0x0010bd10-0x0010c030)
  ClassLib/unplaced_token.c         (gap; ClassLib/CToken.cpp by content) token assets, halo
                                    pass, list walk (0x0010c080-0x0010c200, 0x0010ce90)
  Demolition/CDemolition_Event.c    the CharLib event queue (0x0010cec0-0x0010d1d0)
  Demolition/CDemolition_CharLibAI.c (gap) the vcall thunks of the AI condition/state tables
                                    and the CharLib AI base of the Demolition players
  Demolition/CDemolition_Crate.c    crates (vtable 0x0026be9c)
  Demolition/CDemolition_Wall.c     brick walls that break into flying bricks (0x0010dba0-)

Layouts are those of the original 32-bit game (docs/ARCHITECTURE.md, section 3). The players
(CDemolition_CharLib, demolition_b) and the game object (DAT_002ff85c, demolition_a) belong
to other modules; their fields are reached through the byte offsets named below.

Other modules may include this header. Only demolition_d edits it.
*/

#ifndef DEMOLITION_D_H
#define DEMOLITION_D_H

#include <math.h>
#include <string.h>

/* ---------- raw field access (original offsets) */
#define DD_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define DD_I32(p, off) DD_AT(p, off, int)
#define DD_U32(p, off) DD_AT(p, off, unsigned int)
#define DD_F32(p, off) DD_AT(p, off, float)
#define DD_I16(p, off) DD_AT(p, off, short)
#define DD_U16(p, off) DD_AT(p, off, unsigned short)
#define DD_U8(p, off) DD_AT(p, off, unsigned char)
#define DD_PTR(p, off) DD_AT(p, off, char *)
#define DD_FP(p, off) ((float *)((char *)(p) + (off)))
#define DD_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable (the object is passed first,
   as the x86 code passed it in ECX) */
#define DD_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* call a function of another module with its real shape (the table prototype may still be
   Ghidra's guess, or may lack the ECX `this`) */
#define DD_FN(fn, ret, args) ((ret(*) args)(fn))

/* ---------- the engine's rand() (MSVC's, inlined by the game; seed DAT_00285630, bkernel) */
extern unsigned int DAT_00285630;
static inline int dd_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}
/* 0x00253740: 1 / 32767 (rand() to 0..1) */
#define DD_RAND_SCALE 3.05185094e-05f
/* a random number 0..1 */
static inline float dd_randf(void)
{
	return (float)dd_rand() * DD_RAND_SCALE;
}

/* ---------- globals of other modules */

/* 0x002ff85c  the Demolition game object (CDemolition_Game; demolition_a) */
extern char *DAT_002ff85c;
#define DD_GAME DAT_002ff85c
#define DG_VARIATION 0x28      /* int: game variation 0..3 */
#define DG_DIFFICULTY 0x44     /* int: AI difficulty 0..2 */
#define DG_PLAYERS 0x6c        /* first player (CDemolition_CharLib), linked at DP_NEXT */
#define DG_HOMEBASE 0x7c0      /* vec4 per player index, 0x20 bytes apart (variation 3) */
#define DG_BOMBLIST 0x13e8     /* bomb system list (demolition_b): first bomb at +4 */
#define DG_WALLTEXTURE 0x6c8c  /* the wall brick texture */
#define DG_VARIATIONOBJ 0x79a0 /* the variation object: vtable +0x30 Zone(i), +0x34 Arena() */
#define DG_CRATEACTOR 0x79ac   /* the crate actor */
#define DG_CRATECOLL 0x79b0    /* the crate collision actor */
#define DG_CRATECOLLRES 0x79c4 /* the crate's animating-collision resource */

/* fields of a player (CDemolition_CharLib: a CharLib AI player, 0x13e0 bytes; demolition_b) */
#define DP_ACTOR 0x30          /* the CharLib actor: +0x1d8 heading, +0x214 int */
#define DP_AISTATE 0x36        /* unsigned short: the CharLib AI state */
#define DP_BUTTON_JUMP 0x68    /* pad button object: +0x14 = 1 presses it */
#define DP_BUTTON_ACTION 0x6c  /* pad button object: +0x14 = 1 presses it */
#define DP_ROUTE 0x8c          /* struct CLRoute */
#define DP_ROUTE_ACTIVE 0x98   /* short (in the route) */
#define DP_ROUTE_NODE 0x1042   /* short (in the route): -1 when no route */
#define DP_NEXT 0x1140         /* next player or 0 */
#define DP_POS 0x1144          /* vec4 */
#define DP_RADIUS 0x1164       /* float */
#define DP_HEIGHT 0x1168       /* float */
#define DP_ALIVE 0x116c        /* char */
#define DP_ACTIVE 0x116d       /* char (the remote bomb: armed) */
#define DP_INDEX 0x1184        /* int 0..3 */
#define DP_STATS 0x118c        /* statistics: +0x3c int frags by player index */
#define DP_SCORE 0x1194        /* int */
#define DP_BOMB 0x11a0         /* the player's (first) bomb */
#define DP_BOMB2 0x11a4        /* the second bomb (variation 0) */
#define DP_CANJUMP 0x11ac      /* char */
#define DP_JUMPTIMER 0x11d0    /* timer (bumpercars_b BCarTimer_GetRemaining) */
#define DP_PARTNER 0x1234      /* team mate */
#define DP_CARRYING 0x1244     /* char: carries a bomb (variation 3) */

/* fields of a bomb (demolition_b) */
#define PB_ACTIVE 0x15         /* char */
#define PB_POS 0x43a4          /* vec4 */
#define PB_HEIGHT 0x4410       /* float */
#define PB_FUSE_TIMER 0x45f0   /* timer object: +0x30 time left */
#define PB_TIMER 0x4628        /* timer object: +0x30 time left */
#define PB_KIND 0x4454         /* int: 1 = not a threat */

/* rollerball_a 0x0018ea50: a timer's value (float at +0x30; ECX this) */
#define DD_TimerValue(t) DD_FN(FUN_0018ea50, float, (void *))((void *)(t))

/* ======================================================================== the AI brains */

/* a bomb (or other danger) the brain has seen */
typedef struct DemoAIThreat {
	float pos[4];     /* 0x00 */
	float dist;       /* 0x10 distance (variations 0-2: squared) */
	float fuse;       /* 0x14 time left on its fuse */
} DemoAIThreat;

/* the part shared by the four brain classes (sizes 0x1d8, 0x1c8, 0x1b4 and the last one);
   the fields after 0x168 differ per class and are reached by offset */
typedef struct DemoAI {
	const void *const *vtable; /* 0x000 */
	char *owner;               /* 0x004 the player it drives (CDemolition_CharLib) */
	int state;                 /* 0x008 */
	int mode;                  /* 0x00c target choice */
	float params[5];           /* 0x010 DAT_00298bb0[variation * 3 + difficulty] */
	float goal[4];             /* 0x024 where the route goes */
	float routeTimer;          /* 0x034 frames to the next route plan */
	unsigned char hasGoal;     /* 0x038 */
	unsigned char routing;     /* 0x039 a route is being followed */
	unsigned char newGoal;     /* 0x03a plan the route now */
	unsigned char arrived;     /* 0x03b the route ended */
	float stick[4];            /* 0x03c */
	DemoAIThreat threats[10];  /* 0x04c */
	int numThreats;            /* 0x13c */
	int maxThreats;            /* 0x140 */
	float scanRadius;          /* 0x144 */
	float f148;                /* 0x148 */
	float f14c;                /* 0x14c */
	float dangerFuse;          /* 0x150 a threat whose fuse is below this is urgent */
	char *target;              /* 0x154 the player it is after */
	char *lastTarget;          /* 0x158 */
	float f15c;                /* 0x15c (99999) */
	float f160;                /* 0x160 (pi / 4) */
	float arriveDist;          /* 0x164 */
} DemoAI;
OF_ASSERT_OFFSET(DemoAI, goal, 0x24);
OF_ASSERT_OFFSET(DemoAI, stick, 0x3c);
OF_ASSERT_OFFSET(DemoAI, threats, 0x4c);
OF_ASSERT_OFFSET(DemoAI, numThreats, 0x13c);
OF_ASSERT_OFFSET(DemoAI, target, 0x154);
OF_ASSERT_SIZE(DemoAI, 0x168);

/* 0x00298bb0  the brain parameters by variation * 3 + difficulty (5 floats each) */
extern float DAT_00298bb0[12][5];

/* ======================================================================== event queues */

/* an entity event (0x34 bytes, vtable 0x0026bc00) */
typedef struct DemoEntityEvent {
	const void *const *vtable; /* 0x00 */
	short id;                  /* 0x04 an event with the same id replaces the data */
	short pad06;
	int kind;                  /* 0x08 0: never merged */
	int priority;              /* 0x0c */
	unsigned int data[9];      /* 0x10 */
} DemoEntityEvent;
OF_ASSERT_SIZE(DemoEntityEvent, 0x34);

/* a CharLib event (0x24 bytes, vtable 0x0026bd20); demolition_b's struct DbMessage */
typedef struct DemoCharEvent {
	const void *const *vtable; /* 0x00 */
	short id;                  /* 0x04 */
	short pad06;
	int kind;                  /* 0x08 */
	int priority;              /* 0x0c */
	unsigned int data[5];      /* 0x10 */
} DemoCharEvent;
OF_ASSERT_SIZE(DemoCharEvent, 0x24);

/* a priority ring of events (vtables 0x0026bc04 and 0x0026bd24; 0x10 bytes) */
typedef struct DemoEventQueue {
	const void *const *vtable; /* 0x00 */
	void *events;              /* 0x04 the array (count at [-1]) */
	unsigned short capacity;   /* 0x08 a power of 2 */
	unsigned short head;       /* 0x0a */
	unsigned short count;      /* 0x0c */
	unsigned short pad0e;
} DemoEventQueue;
OF_ASSERT_SIZE(DemoEventQueue, 0x10);

/* ======================================================================== walls */

/* a brick of a wall (0x88 bytes) */
typedef struct DemoBrick {
	float matrix[16];          /* 0x00 rotation of a flying brick */
	float pos[4];              /* 0x40 flying brick position */
	float vel[4];              /* 0x50 */
	float rot[4];              /* 0x60 x, y, z angles */
	float spin[4];             /* 0x70 */
	unsigned char solid;       /* 0x80 still in the wall */
	unsigned char flying;      /* 0x81 */
	unsigned char faceTop;     /* 0x82 draw the top face */
	unsigned char faceBottom;  /* 0x83 */
	unsigned char faceLeft;    /* 0x84 */
	unsigned char faceRight;   /* 0x85 */
	unsigned char pad86[2];
} DemoBrick;
OF_ASSERT_SIZE(DemoBrick, 0x88);

/* a wall vertex (D3DFVF_XYZ | NORMAL | DIFFUSE | TEX1, 0x24 bytes) */
typedef struct DemoWallVertex {
	float pos[3];
	float normal[3];
	unsigned int colour;
	float uv[2];
} DemoWallVertex;
OF_ASSERT_SIZE(DemoWallVertex, 0x24);

/* a wall (vtable 0x0026bef0); the vtable pointer is at 0 */
typedef struct DemoWall {
	const void *const *vtable; /* 0x000 */
	float width;               /* 0x004 */
	float height;              /* 0x008 */
	float depth;               /* 0x00c */
	int rows;                  /* 0x010 */
	int cols;                  /* 0x014 */
	float brickW;              /* 0x018 */
	float brickH;              /* 0x01c */
	float depth2;              /* 0x020 */
	float halfW;               /* 0x024 */
	float halfH;               /* 0x028 */
	float halfD;               /* 0x02c */
	float pos[4];              /* 0x030 */
	float angle;               /* 0x040 */
	float world[16];           /* 0x044 */
	float translation[16];     /* 0x084 */
	float rotation[16];        /* 0x0c4 */
	void *texture;             /* 0x104 */
	int unknown108;            /* 0x108 */
	unsigned char dirty;       /* 0x10c */
	unsigned char posDirty;    /* 0x10d */
	unsigned char angleDirty;  /* 0x10e */
	unsigned char pad10f;
	DemoBrick *bricks;         /* 0x110 rows * cols */
	struct bdVertexBuffer *vb; /* 0x114 */
	int numWallVerts;          /* 0x118 */
	int numFlyingVerts;        /* 0x11c */
	int numFlying;             /* 0x120 */
	float corners[8][4];       /* 0x124 the corners of a brick */
	float unknown1a4[4];       /* 0x1a4 */
	float lightDir[4];         /* 0x1b4 */
} DemoWall;
OF_ASSERT_OFFSET(DemoWall, world, 0x44);
OF_ASSERT_OFFSET(DemoWall, texture, 0x104);
OF_ASSERT_OFFSET(DemoWall, bricks, 0x110);
OF_ASSERT_OFFSET(DemoWall, corners, 0x124);
OF_ASSERT_OFFSET(DemoWall, lightDir, 0x1b4);

#endif
