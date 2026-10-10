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

/* ---------- CMLaser.cpp */

struct MbLaserPoint
{
	float pos[4];
	int colour[4];              /* r, g, b, a */
};

/* a laser beam (0x1b0 bytes) */
struct CMLaser
{
	void *next;                 /* +0x000 node of the list 0x00308f1c */
	void *prev;                 /* +0x004 */
	float origin[4];            /* +0x008 */
	float life;                 /* +0x018 frames; 0 = for ever */
	int colour[4];              /* +0x01c fades with the life */
	int endColour[4];           /* +0x02c */
	float length;               /* +0x03c */
	float maxLength;            /* +0x040 */
	float width;                /* +0x044 */
	float spin[4];              /* +0x048 added to the angles per frame */
	float angles[4];            /* +0x058 */
	struct MbLaserPoint points[10]; /* +0x068 */
	int numPoints;              /* +0x1a8 segments */
	void *texture;              /* +0x1ac */
};
OF_ASSERT_OFFSET(struct CMLaser, points, 0x68);
OF_ASSERT_SIZE(struct CMLaser, 0x1b0);

extern void *DAT_00308f14;
extern void *DAT_00308f18;
extern struct MbList DAT_00308f1c;

/* ---------- CScorchMarks.cpp */

/* a scorch mark type (0x14 bytes; table at 0x002b0a64) */
struct MbScorchType
{
	const char *name;
	void *texture;
	float size;
	float alpha;
	float fade;
};
OF_ASSERT_SIZE(struct MbScorchType, 0x14);

/* a scorch mark (0xcc bytes) */
struct CScorchMark
{
	void *next;                 /* +0x00 node of the list 0x00308f2c */
	void *prev;                 /* +0x04 */
	struct MbVertex verts[4];   /* +0x08 */
	void *texture;              /* +0x98 */
	void *parent;               /* +0x9c an object with a heading at +0x1f8, or NULL */
	float pos[4];               /* +0xa0 */
	float normal[4];            /* +0xb0 */
	float size;                 /* +0xc0 */
	float alpha;                /* +0xc4 */
	float fade;                 /* +0xc8 per frame */
};
OF_ASSERT_SIZE(struct CScorchMark, 0xcc);

extern struct MbScorchType DAT_002b0a64[2];
extern int DAT_00308f28;
extern struct MbList DAT_00308f2c;

/* ---------- CBomb.cpp */

/* a bomb model (8 bytes; table at 0x002b0a90) */
struct MbBombModel
{
	const char *name;
	void *resource;
};

/* a bomb position of the level script (0x20 bytes) */
struct MbBombPos
{
	void *next;                 /* +0x00 node of its group's list */
	void *prev;                 /* +0x04 */
	float pos[4];               /* +0x08 */
	short used;                 /* +0x18 a bomb is there */
	short tag;                  /* +0x1a */
	short *blocker;             /* +0x1c no bomb while *blocker != 0 */
};
OF_ASSERT_SIZE(struct MbBombPos, 0x20);

/* a bomb group (0x20 bytes; 8 at 0x00308f58) */
struct MbBombGroup
{
	struct MbList positions;    /* +0x00 */
	unsigned short id;          /* +0x0c */
	unsigned short pad0e;
	int count;                  /* +0x10 bombs left to drop */
	int type;                   /* +0x14 kind of bomb */
	float interval;             /* +0x18 frames between bombs */
	float accum;                /* +0x1c */
};
OF_ASSERT_SIZE(struct MbBombGroup, 0x20);

/* a bomb (0x5c bytes) */
struct CBomb
{
	void *next;                 /* +0x00 node of the list 0x00308f48 */
	void *prev;                 /* +0x04 */
	void *actor;                /* +0x08 charlib actor (position +0xd0, velocity +0x194) */
	int type;                   /* +0x0c 0 bomb, 1 fused bomb, 2 mine, 3 missile, 4 dynamite */
	float timer;                /* +0x10 fuse time left; -1: not lit */
	float fuse;                 /* +0x14 */
	float radius;               /* +0x18 of the blast */
	float pushScale;            /* +0x1c */
	float maxPush;              /* +0x20 */
	float triggerDist;          /* +0x24 the mine's trigger distance */
	float unk28;                /* +0x28 */
	float height;               /* +0x2c above the ground at the start */
	void *trails[4];            /* +0x30 CProjTrail (missile) */
	int numTrails;              /* +0x40 */
	float dropInterval;         /* +0x44 frames between dropped particles */
	float dropAccum;            /* +0x48 */
	void *dropType;             /* +0x4c particle type */
	int tag;                    /* +0x50 */
	struct MbBombPos *bombPos;  /* +0x54 */
	struct CMetaExplosion *explosion; /* +0x58 */
};
OF_ASSERT_OFFSET(struct CBomb, numTrails, 0x40);
OF_ASSERT_SIZE(struct CBomb, 0x5c);

/* an air raid (0x3c bytes, allocated by meta_a) */
struct CAirRaid
{
	void *next;                 /* +0x00 node of the list 0x00308f3c */
	void *prev;                 /* +0x04 */
	float life;                 /* +0x08 */
	float x;                    /* +0x0c inner area */
	float z;                    /* +0x10 */
	float width;                /* +0x14 */
	float depth;                /* +0x18 */
	float outerX;               /* +0x1c inner area grown by the margin */
	float outerZ;               /* +0x20 */
	float outerWidth;           /* +0x24 */
	float outerDepth;           /* +0x28 */
	float velX;                 /* +0x2c of the missiles */
	float velZ;                 /* +0x30 */
	float interval;             /* +0x34 */
	float accum;                /* +0x38 */
};
OF_ASSERT_SIZE(struct CAirRaid, 0x3c);

extern struct MbBombModel DAT_002b0a90[5];
extern int DAT_00308f38;
extern struct MbList DAT_00308f3c;
extern struct MbList DAT_00308f48;
extern struct MbBombGroup DAT_00308f58[8];

/* ---------- BigRedButton.cpp */

/* an animation name and speed (0x44 bytes) */
struct MbAnimName
{
	char name[0x40];
	float speed;
};
OF_ASSERT_SIZE(struct MbAnimName, 0x44);

/* a big red button (allocated by the level loader) */
struct CBigRedButton
{
	int unk00[3];               /* +0x00 */
	float pos[4];               /* +0x0c the bone's position */
	float heading;              /* +0x1c the bone's heading */
	float radius2;              /* +0x20 squared reach */
	float tolerance;            /* +0x24 facing tolerance (radians) */
	int unk28;                  /* +0x28 */
	struct BActorNode *bone;    /* +0x2c */
	int enabled;                /* +0x30 */
	int mode;                   /* +0x34 */
	int presses;                /* +0x38 */
	int state;                  /* +0x3c */
	int prevState;              /* +0x40 */
	void *model;                /* +0x44 the button's model object */
};
OF_ASSERT_OFFSET(struct CBigRedButton, model, 0x44);

extern struct MbAnimName DAT_002b0be8[2];
extern int DAT_00309058;
extern int DAT_0030905c;
extern struct CBigRedButton *DAT_00309060[16];

/* ---------- Searchlights.cpp, BigGun.cpp */

/* the cutter's spark parameters (0x34 bytes at cutter +0x68) */
struct MbCutterParams
{
	unsigned char flags;        /* +0x00 1: sparks alive, 2: not below their start */
	unsigned char pad01[3];
	float speed;                /* +0x04 */
	float damping;              /* +0x08 */
	float gravity;              /* +0x0c per frame */
	float unk10;                /* +0x10 */
	float unk14;                /* +0x14 */
	int unk18;                  /* +0x18 */
	int unk1c;                  /* +0x1c */
	int sizeMin;                /* +0x20 */
	int sizeRange;              /* +0x24 */
	short colour[3];            /* +0x28 streak start colour */
	short endColour[3];         /* +0x2e streak end and glow colour */
};
OF_ASSERT_SIZE(struct MbCutterParams, 0x34);

/* a cutter spark (0x78 bytes); colours are 8.7 fixed point */
struct MbSpark
{
	unsigned char flags;        /* +0x00 1: alive */
	unsigned char pad01;
	short age;                  /* +0x02 */
	short life;                 /* +0x04 */
	short pad06;
	float pos[4];               /* +0x08 */
	float prevPos[4];           /* +0x18 */
	float vel[4];               /* +0x28 */
	float origin[4];            /* +0x38 */
	float size;                 /* +0x48 */
	int unk4c;                  /* +0x4c */
	int unk50[2];               /* +0x50 */
	short colour[3];            /* +0x58 */
	short pad5e;
	short endColour[3];         /* +0x60 */
	short pad66;
	short colourStep[3];        /* +0x68 */
	short pad6e;
	short endColourStep[3];     /* +0x70 */
	short pad76;
};
OF_ASSERT_SIZE(struct MbSpark, 0x78);

/* the laser's cutter (0x9c bytes) */
struct CBigGunCutter
{
	void *texScorch;            /* +0x00 cutterscorch.bmp */
	void *texGlow;              /* +0x04 cutterglow.bmp */
	float dir[4];               /* +0x08 sparks fly this way (0, 1, 0) */
	int unk18;                  /* +0x18 */
	struct MbSpark *sparks;     /* +0x1c */
	int numSparks;              /* +0x20 */
	int numActive;              /* +0x24 */
	void *texture;              /* +0x28 of the streaks (NULL) */
	float pos[4];               /* +0x2c */
	int unk3c[8];               /* +0x3c */
	struct bdVertexBuffer *vb;  /* +0x5c */
	int unk60[2];               /* +0x60 */
	struct MbCutterParams params; /* +0x68 */
};
OF_ASSERT_OFFSET(struct CBigGunCutter, vb, 0x5c);
OF_ASSERT_SIZE(struct CBigGunCutter, 0x9c);

/* a big gun's laser (0x74 bytes) */
struct CBigGunLaser
{
	struct CBigGunCutter *cutter; /* +0x00 */
	void *actor;                /* +0x04 the gun's actor */
	float hit[4];               /* +0x08 where the beam ends */
	float hitRaw[4];            /* +0x18 */
	float muzzle[4];            /* +0x28 */
	int unk38[8];               /* +0x38 */
	void *trail;                /* +0x58 CProjTrail (the spiral) */
	int unk5c;                  /* +0x5c */
	int alpha;                  /* +0x60 fades after firing */
	int firing;                 /* +0x64 */
	int visible;                /* +0x68 */
	float time;                 /* +0x6c */
	float duration;             /* +0x70 */
};
OF_ASSERT_SIZE(struct CBigGunLaser, 0x74);

/* a big gun (0x28 bytes) */
struct CBigGun
{
	int id;                     /* +0x00 */
	float pos[4];               /* +0x04 */
	void *actor;                /* +0x14 */
	struct BActorNode *bone;    /* +0x18 the laser bone that turns */
	struct CBigGunLaser *laser; /* +0x1c */
	void *next;                 /* +0x20 node of the list 0x00309268 */
	void *prev;                 /* +0x24 */
};
OF_ASSERT_SIZE(struct CBigGun, 0x28);

/* the key-framed path of a searchlight (a BKeyTrack3) */
struct MbPath
{
	short count;                /* +0x00 */
	short hasKeys;              /* +0x02 */
	void *keys;                 /* +0x04 */
};

/* a searchlight (allocated by the level loader) */
struct CSearchlight
{
	void *next;                 /* +0x00 node of the list 0x003090a8 */
	void *prev;                 /* +0x04 */
	int unk08;                  /* +0x08 */
	void *actor;                /* +0x0c the light model */
	struct MbPath path;         /* +0x10 */
	int unk18[6];               /* +0x18 */
	float pathTime[5];          /* +0x30 charlib timer (FUN_000b8240) */
	int unk44;                  /* +0x44 */
	float target[4];            /* +0x48 */
	int targetValid;            /* +0x58 */
	float cone;                 /* +0x5c half angle */
	float scale;                /* +0x60 */
	int unk64;                  /* +0x64 */
	int state;                  /* +0x68 0 on, 1 off, 2 growing, 3 shrinking */
	int arg6c;                  /* +0x6c */
	float detection[4];         /* +0x70 per player */
	float threshold;            /* +0x80 */
	int unk84;                  /* +0x84 */
	int flags;                  /* +0x88 1: draw as a beam */
	void *beamTexture;          /* +0x8c */
	float beamStart[4];         /* +0x90 */
	float beamEnd[4];           /* +0xa0 */
	float beamStartWidth;       /* +0xb0 */
	float beamStartLength;      /* +0xb4 */
	float beamEndWidth;         /* +0xb8 */
	float beamEndLength;        /* +0xbc */
	int beamStartColour[4];     /* +0xc0 */
	int beamColour[4];          /* +0xd0 */
	struct CBigGun *gun;        /* +0xe0 */
};
OF_ASSERT_OFFSET(struct CSearchlight, target, 0x48);
OF_ASSERT_OFFSET(struct CSearchlight, gun, 0xe0);

extern struct MbList DAT_003090a8;
extern void *DAT_00309258;
extern void *DAT_0030925c;
extern void *DAT_00309260;
extern struct MbList DAT_00309268;

/* ---------- CWater.cpp */

/* a drip that splashes at random times (0x20 bytes) */
struct MbDrip
{
	float x;                    /* +0x00 */
	float z;                    /* +0x04 */
	float depth;                /* +0x08 splash */
	float inner;                /* +0x0c */
	float outer;                /* +0x10 */
	float timer;                /* +0x14 */
	float timeMin;              /* +0x18 */
	float timeRange;            /* +0x1c */
};
OF_ASSERT_SIZE(struct MbDrip, 0x20);

/* a water area (0x174 bytes, allocated by the level loader) */
struct CMetaWater
{
	void *field;                /* +0x000 CWaterField (classlib_fx) */
	float minX;                 /* +0x004 or the centre x when round */
	float minZ;                 /* +0x008 or the centre z */
	float maxX;                 /* +0x00c or the squared radius */
	float maxZ;                 /* +0x010 */
	float level;                /* +0x014 */
	float targetLevel;          /* +0x018 */
	float speed;                /* +0x01c of the level */
	short tag;                  /* +0x020 */
	short round;                /* +0x022 */
	int numDrips;               /* +0x024 */
	struct MbDrip drips[10];    /* +0x028 */
	float dropDepth;            /* +0x168 */
	float dropAccum;            /* +0x16c */
	float dropInterval;         /* +0x170 */
};
OF_ASSERT_SIZE(struct CMetaWater, 0x174);

extern int DAT_003090b4;
extern struct CMetaWater *DAT_003090b8[6];

/* ---------- particle emitters (Meta/unplaced.c) */

/* an emitter (0x70 bytes, allocated by the level loader) */
struct CMetaEmitter
{
	void *next;                 /* +0x00 node of the list 0x003090d8 */
	void *prev;                 /* +0x04 */
	float p0[4];                /* +0x08 line start or circle centre */
	float p1[4];                /* +0x18 line end, or (0, 0, radius, 1) */
	float vel[4];               /* +0x28 */
	float spread[4];            /* +0x38 */
	float accum;                /* +0x48 */
	float rate;                 /* +0x4c */
	float targetRate;           /* +0x50 */
	float perFrame;             /* +0x54 */
	float lengthFactor;         /* +0x58 */
	int arg;                    /* +0x5c */
	unsigned int colour;        /* +0x60 */
	int type;                   /* +0x64 0 line, 1 water drip line, 2 circle */
	void *ptype;                /* +0x68 particle type (a float strength for type 1) */
	int flags;                  /* +0x6c 1: on the water, 2: with velocity */
};
OF_ASSERT_SIZE(struct CMetaEmitter, 0x70);

extern float DAT_003090d0;
extern float DAT_003090d4;
extern struct MbList DAT_003090d8;

/* ---------- CMeta_Rain.cpp */

/* a rain splash sprite (0x40 bytes; a block of DAT_002b0cac at 0x003090f4) */
struct MbSplash
{
	int active;                 /* +0x00 */
	float life;                 /* +0x04 */
	float life0;                /* +0x08 */
	float alpha;                /* +0x0c */
	float alpha0;               /* +0x10 */
	float size;                 /* +0x14 */
	float pos[4];               /* +0x18 */
	float normal[4];            /* +0x28 */
	void *next;                 /* +0x38 node of the list 0x00309100 */
	void *prev;                 /* +0x3c */
};
OF_ASSERT_SIZE(struct MbSplash, 0x40);

/* the rain (inside meta_a's Meta mode object at +0xa0) */
struct CMetaRain
{
	struct BExtraSystem *system; /* +0x00 the drops */
	int unk04;                  /* +0x04 */
	float flashTimer;           /* +0x08 frames to the next lightning */
	float flashTime;            /* +0x0c frames the lightning lasts */
};

extern struct MbSplash *DAT_003090f4;
extern struct MbList DAT_00309100;
extern struct BLight *DAT_003090fc;

/* ---------- functions missing from src/include/of_functions.tsv (defined here with their
   FUN_ address in the comment; listed in the module report for the architect) */
int FUN_00163df0(struct BExtraSystem *system, int count, void **items, void *arg);
int FUN_001642f0(struct BExtraSystem *system, void *item, void *data);
int FUN_00164460(struct BExtraSystem *system, int count, void **items, void *arg);
#define MetaRain_DrawDrops FUN_00163df0     /* 0x00163df0 */
#define MetaRain_InitDrop FUN_001642f0      /* 0x001642f0 */
#define MetaRain_UpdateDrops FUN_00164460   /* 0x00164460 */

#endif
