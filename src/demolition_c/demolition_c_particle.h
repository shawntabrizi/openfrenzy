/*
demolition_c_particle.h - the particle systems of CDemolition_Particle.cpp (module demolition_c).

Seven classes built on the ClassLib particle system header (CPSHeader, CParticle in
classlib_fx.h) and its vtable layout of 6 slots:
  [0] Release  [1] Update(dt)  [2] Draw  [3] Reset  [4] Emit(particle)  [5] deleting destructor
Class names are inferred from what they do:

  vtable      class                  size   what
  0x0026b9ec  DcSparkSystem          0x78   sparks drawn as beams (CSparkSystem's Emit)
  0x0026ba04  DcDebrisSystem         0x5c4  3 bouncing, spinning actor chunks (variation models)
  0x0026ba1c  DcRubbleSystem         0x7a4  4 bouncing, spinning actor chunks (wall models)
  0x0026ba34  DcFlashSparkSystem     0x6c   sparks drawn as beams with a glow sprite each
  0x0026ba4c  DcGasSystem            0x68   rising point sprites (CGlowSystem's Release)
  0x0026ba64  DcTrailSystem          0xa8   a waving ribbon (polyline) that grows and shrinks
  0x0026ba7c  DcDustSystem           0x6c   rotating sprites that rise inside a shape

Owner: demolition_c.
*/

#ifndef DEMOLITION_C_PARTICLE_H
#define DEMOLITION_C_PARTICLE_H

#include "classlib_fx.h"

/* ---------- DcSparkSystem (vtable 0x0026b9ec) */

/* CSparkParams (classlib_fx) and one more float */
typedef struct DcSparkParams {
	byte active;         /* 0x44 */
	byte bounce;         /* 0x45 */
	short pad46;
	float speed;         /* 0x48 */
	float drag;          /* 0x4c */
	float gravity;       /* 0x50 */
	int lifeMin;         /* 0x54 */
	int lifeRange;       /* 0x58 */
	int sizeMin;         /* 0x5c */
	int sizeRange;       /* 0x60 */
	short colour[3];     /* 0x64 */
	short colour2[3];    /* 0x6a */
	float unknown70;     /* 0x70 0.6 */
} DcSparkParams;
OF_ASSERT_SIZE(DcSparkParams, 0x30);

typedef struct DcSparkSystem {
	CPSHeader h;         /* 0x00 */
	int unknown3c[2];    /* 0x3c */
	DcSparkParams p;     /* 0x44 */
	int vb;              /* 0x74 */
} DcSparkSystem;
OF_ASSERT_SIZE(DcSparkSystem, 0x78);

/* ---------- DcDebrisSystem (vtable 0x0026ba04) and DcRubbleSystem (vtable 0x0026ba1c) */

/* one flying chunk: an actor instance and its motion (0x1d0 bytes) */
typedef struct DcChunk {
	byte alive;          /* 0x000 */
	char pad001[0xf];
	char actor[0x170];   /* 0x010 BActorInstance: position at +0xc0, rotation at +0xe0 */
	float pos[4];        /* 0x180 */
	float vel[4];        /* 0x190 */
	float quat[4];       /* 0x1a0 */
	float radius;        /* 0x1b0 the model's radius (model + 0x5c) */
	float spin;          /* 0x1b4 */
	short age;           /* 0x1b8 */
	short life;          /* 0x1ba */
	int fade;            /* 0x1bc brightness << 7 while fading */
	int fadeTime;        /* 0x1c0 */
	int fadeStep;        /* 0x1c4 */
	short phase;         /* 0x1c8 0 flying, 1 fading */
	short pad1ca;
	int pad1cc;
} DcChunk;
OF_ASSERT_OFFSET(DcChunk, pos, 0x180);
OF_ASSERT_OFFSET(DcChunk, phase, 0x1c8);
OF_ASSERT_SIZE(DcChunk, 0x1d0);

typedef struct DcChunkParams {
	byte active;         /* 0x3c */
	byte bounce;         /* 0x3d */
	short pad3e;
	float speed;         /* 0x40 */
	float drag;          /* 0x44 */
	float gravity;       /* 0x48 */
	float restitution;   /* 0x4c */
} DcChunkParams;
OF_ASSERT_SIZE(DcChunkParams, 0x14);

typedef struct DcDebrisSystem {
	CPSHeader h;         /* 0x000 */
	DcChunkParams p;     /* 0x03c */
	DcChunk chunks[3];   /* 0x050 */
	int numChunks;       /* 0x5c0 chunks alive */
} DcDebrisSystem;
OF_ASSERT_OFFSET(DcDebrisSystem, numChunks, 0x5c0);

typedef struct DcRubbleSystem {
	CPSHeader h;         /* 0x000 */
	DcChunkParams p;     /* 0x03c */
	float dir[4];        /* 0x050 direction bias of the throw */
	DcChunk chunks[4];   /* 0x060 */
	int numChunks;       /* 0x7a0 */
} DcRubbleSystem;
OF_ASSERT_OFFSET(DcRubbleSystem, numChunks, 0x7a0);

/* ---------- DcFlashSparkSystem (vtable 0x0026ba34) */

typedef struct DcFlashSparkParams {
	byte active;         /* 0x50 */
	byte bounce;         /* 0x51 */
	short pad52;
	float speed;         /* 0x54 */
	float drag;          /* 0x58 */
	float gravity;       /* 0x5c */
	float unknown60;     /* 0x60 0.5 */
	float unknown64;     /* 0x64 1.0 */
} DcFlashSparkParams;
OF_ASSERT_SIZE(DcFlashSparkParams, 0x18);

typedef struct DcFlashSparkSystem {
	CPSHeader h;         /* 0x00 */
	int glowTexture;     /* 0x3c */
	int unknown40[4];    /* 0x40 */
	DcFlashSparkParams p;/* 0x50 */
	int vb;              /* 0x68 */
} DcFlashSparkSystem;
OF_ASSERT_SIZE(DcFlashSparkSystem, 0x6c);

/* ---------- DcGasSystem (vtable 0x0026ba4c) */

typedef struct DcGasParams {
	byte active;         /* 0x3c */
	byte emitting;       /* 0x3d */
	short pad3e;
	float speed;         /* 0x40 rise speed */
	float drag;          /* 0x44 */
	float unknown48;     /* 0x48 */
	float radius;        /* 0x4c emission radius (x 1/3) */
	int lifeMin;         /* 0x50 (halved) */
	int lifeRange;       /* 0x54 */
	int unknown58;       /* 0x58 */
	int maxEmit;         /* 0x5c particles emitted per update */
	float pointSize;     /* 0x60 */
} DcGasParams;
OF_ASSERT_SIZE(DcGasParams, 0x28);

typedef struct DcGasSystem {
	CPSHeader h;         /* 0x00 */
	DcGasParams p;       /* 0x3c */
	int vb;              /* 0x64 */
} DcGasSystem;
OF_ASSERT_SIZE(DcGasSystem, 0x68);

/* ---------- DcTrailSystem (vtable 0x0026ba64) */

typedef struct DcTrailParams {
	byte active;         /* 0x64 */
	byte unknown65;      /* 0x65 */
	byte growing;        /* 0x66 */
	byte pad67;
	float drag;          /* 0x68 */
	float gravity;       /* 0x6c */
	int length;          /* 0x70 points */
} DcTrailParams;
OF_ASSERT_SIZE(DcTrailParams, 0x10);

typedef struct DcTrailSystem {
	CPSHeader h;           /* 0x00 */
	float (*points)[4];    /* 0x3c */
	float (*vels)[4];      /* 0x40 */
	float *widths;         /* 0x44 */
	float *extra;          /* 0x48 */
	int (*colours)[4];     /* 0x4c */
	int head;              /* 0x50 points in use (end) */
	int tail;              /* 0x54 first point drawn */
	int texture;           /* 0x58 */
	int unknown5c[2];      /* 0x5c */
	DcTrailParams p;       /* 0x64 */
	int vb;                /* 0x74 */
	float emitVel[3];      /* 0x78 */
	int unknown84;         /* 0x84 */
	void *pointBuf;        /* 0x88 points and velocities */
	void *widthBuf;        /* 0x8c */
	void *extraBuf;        /* 0x90 */
	void *colourBuf;       /* 0x94 */
	float angle;           /* 0x98 */
	float angleSpeed;      /* 0x9c */
	float amplitude;       /* 0xa0 */
	float timer;           /* 0xa4 */
} DcTrailSystem;
OF_ASSERT_SIZE(DcTrailSystem, 0xa8);

/* ---------- DcDustSystem (vtable 0x0026ba7c) */

typedef struct DcDustParams {
	byte active;         /* 0x3c */
	byte emitting;       /* 0x3d */
	short pad3e;
	float speed;         /* 0x40 */
	float drag;          /* 0x44 */
	float rise;          /* 0x48 */
	float radius;        /* 0x4c */
	int lifeMin;         /* 0x50 (halved) */
	int lifeRange;       /* 0x54 */
	int unknown58;       /* 0x58 */
	float interval;      /* 0x5c */
} DcDustParams;
OF_ASSERT_SIZE(DcDustParams, 0x24);

typedef struct DcDustSystem {
	CPSHeader h;         /* 0x00 */
	DcDustParams p;      /* 0x3c */
	int vb;              /* 0x60 */
	float timer;         /* 0x64 */
	struct DcShape *shape; /* 0x68 the area the dust rises in */
} DcDustSystem;
OF_ASSERT_SIZE(DcDustSystem, 0x6c);

#endif
