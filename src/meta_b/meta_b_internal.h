/*
META_B_INTERNAL.H

Object layouts of the meta_b module (original offsets, ILP32). Other modules may include
it; only meta_b edits it.
*/

#ifndef META_B_INTERNAL_H
#define META_B_INTERNAL_H

#include "meta_b.h"

/* a ClassLib intrusive list head (CLList_*) */
struct MbList
{
	void *next;                 /* +0x00 */
	void *prev;                 /* +0x04 */
	int count;                  /* +0x08 */
};
OF_ASSERT_SIZE(struct MbList, 0xc);

/* ---------- CLightning.cpp */

/* a lightning bolt (0x48 bytes, node of a group list) */
struct CLightningBolt
{
	void *next;                 /* +0x00 */
	void *prev;                 /* +0x04 */
	float pos[4];               /* +0x08 where it strikes */
	int arg;                    /* +0x18 from the level script */
	float start[4];             /* +0x1c set by meta_a 0x0015ca40/0x0015cb80 */
	float end[4];               /* +0x2c */
	float brightness;           /* +0x3c */
	short tag;                  /* +0x40 */
	short pad42;
	float life;                 /* +0x44 frames left; 0 = not alive */
};
OF_ASSERT_SIZE(struct CLightningBolt, 0x48);

/* the lightning manager (the global at 0x00308460) */
struct CLightningMgr
{
	unsigned short numGroups;   /* +0x00 */
	unsigned short pad02;
	struct MbList groups[8];    /* +0x04 */
	short groupIds[8];          /* +0x64 */
	float strikeTime;           /* +0x74 automatic strikes while > 0 */
	float strikeAccum;          /* +0x78 */
	float strikeInterval;       /* +0x7c */
	unsigned short strikeGroup; /* +0x80 */
	unsigned short pad82;
};
OF_ASSERT_OFFSET(struct CLightningMgr, groupIds, 0x64);
OF_ASSERT_OFFSET(struct CLightningMgr, strikeGroup, 0x80);

extern short DAT_00308458;
extern struct CLightningMgr DAT_00308460;

/* ---------- vents (gap functions after CLightning.cpp, Meta/unplaced.c) */

/* a particle vent: emits up to 5 particle types, switches on and off in a cycle and
   pushes players inside its radius (a sphere, or a cylinder along `dir`). */
struct CMetaVent
{
	void *next;                 /* +0x00 node of the vent list 0x00308e60 */
	void *prev;                 /* +0x04 */
	void *type[5];              /* +0x08 particle types (bumpercars_a particle system) */
	int colour[5];              /* +0x1c -1 = the type's own colour */
	short tag;                  /* +0x30 */
	short active;               /* +0x32 */
	short on;                   /* +0x34 phase of the cycle: emitting */
	short numCycle;             /* +0x36 */
	short offTime[6];           /* +0x38 */
	short onTime[6];            /* +0x44 */
	short timer;                /* +0x50 frames left in this phase */
	short cycle;                /* +0x52 */
	float rate[5];              /* +0x54 particles per frame */
	float accum[5];             /* +0x68 */
	int numTypes;               /* +0x7c */
	unsigned int flags;         /* +0x80 1/2: push while on, 4: push while off,
	                               8: push along dir, 0x10: cylinder */
	float radius;               /* +0x84 */
	float strength;             /* +0x88 */
	float length;               /* +0x8c */
	float dir[4];               /* +0x90 */
	float vel[4];               /* +0xa0 particle velocity */
	float pos[4];               /* +0xb0 */
	float offset[4];            /* +0xc0 position relative to the parent */
	float posSpreadXZ;          /* +0xd0 */
	float posSpreadY;           /* +0xd4 */
	float velSpreadXZ;          /* +0xd8 */
	float velSpreadY;           /* +0xdc */
	void *parent;               /* +0xe0 an object with position +0xe0 and yaw +0x1f8 */
};
OF_ASSERT_OFFSET(struct CMetaVent, rate, 0x54);
OF_ASSERT_OFFSET(struct CMetaVent, flags, 0x80);
OF_ASSERT_OFFSET(struct CMetaVent, parent, 0xe0);

extern struct MbList DAT_00308e60;  /* the vents */

/* the debug cylinder's vertices (0x24 bytes: position, normal, colour, uv) */
struct MbVertex
{
	float x, y, z;
	float nx, ny, nz;
	unsigned int colour;
	float u, v;
};
OF_ASSERT_SIZE(struct MbVertex, 0x24);

/* ---------- CMeta_ExplSmoke.cpp */

/* a smoke puff (0x44 bytes) */
struct MbSmokePuff
{
	float spin;                 /* +0x00 degrees (in radians) per frame */
	float angle;                /* +0x04 */
	float speed;                /* +0x08 (second set only) */
	float decel;                /* +0x0c (second set only) */
	int r, g, b;                /* +0x10 */
	int alpha;                  /* +0x1c */
	float life;                 /* +0x20 frames until it fades */
	float pos[4];               /* +0x24 */
	float dir[4];               /* +0x34 (second set only) */
};
OF_ASSERT_SIZE(struct MbSmokePuff, 0x44);

/* explosion smoke (0x40 bytes): a ring of puffs that stay, and puffs that fly out */
struct CMetaExplSmoke
{
	void *texture;              /* +0x00 myaismoke00.tga */
	struct bdVertexBuffer *vb;  /* +0x04 */
	char *write;                /* +0x08 lock pointer while drawing */
	char *base;                 /* +0x0c */
	int numStill;               /* +0x10 */
	int numMoving;              /* +0x14 */
	float radiusStill;          /* +0x18 */
	float radiusMoving;         /* +0x1c */
	struct MbSmokePuff *still;  /* +0x20 */
	struct MbSmokePuff *moving; /* +0x24 */
	float pos[4];               /* +0x28 */
	void *next;                 /* +0x38 node of the list 0x00308e6c */
	void *prev;                 /* +0x3c */
};
OF_ASSERT_SIZE(struct CMetaExplSmoke, 0x40);

extern struct MbList DAT_00308e6c;  /* explosion smoke, nodes at +0x38 */

/* ---------- CMeta_Explosion.cpp */

/* a timer in seconds that stops while the game is paused (0x30 bytes; gap functions
   0x0016bab0-0x0016bc60) */
struct MbTimer
{
	unsigned char running;      /* +0x00 */
	unsigned char paused;       /* +0x01 */
	unsigned char frozen;       /* +0x02 */
	unsigned char pad03[5];
	unsigned long long start;   /* +0x08 bkTimerRead() ticks */
	long long frequency;        /* +0x10 ticks per second */
	unsigned long long pauseStart; /* +0x18 */
	float duration;             /* +0x20 seconds */
	float durationMs;           /* +0x24 */
	float remaining;            /* +0x28 seconds */
	float progress;             /* +0x2c 0..1 */
};
OF_ASSERT_SIZE(struct MbTimer, 0x30);

/* an explosion (0x1a8 bytes): three particle effects of ClassLib (sparks, flames,
   debris; their classes belong to classlib_fx) and a shock wave ring */
struct CMetaExplosion
{
	unsigned int flags;         /* +0x000 4: active */
	int unk04;                  /* +0x004 */
	float unk08;                /* +0x008 100.0 */
	float alpha;                /* +0x00c of the shock wave */
	float alphaMax;             /* +0x010 */
	void *texExplosion;         /* +0x014 explosion.bmp */
	void *texShockwave;         /* +0x018 shockwave.bmp */
	void *texSparks;            /* +0x01c sparkburst01.bmp */
	void *texExplosion2;        /* +0x020 explosion2.bmp */
	char sparks[0x74];          /* +0x024 classlib_fx effect (vtable first) */
	char flames[0x60];          /* +0x098 */
	char debris[0x68];          /* +0x0f8 */
	struct MbTimer timer;       /* +0x160 shock wave */
	float pos[4];               /* +0x190 */
	void *next;                 /* +0x1a0 node of the list 0x00308f08 */
	void *prev;                 /* +0x1a4 */
};
OF_ASSERT_OFFSET(struct CMetaExplosion, timer, 0x160);
OF_ASSERT_SIZE(struct CMetaExplosion, 0x1a8);

extern struct MbList DAT_00308f08;  /* explosions, nodes at +0x1a0 */

#endif
