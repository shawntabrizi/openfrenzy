/*
CLASSLIB_FX.H

Shared declarations of the classlib_fx module (ClassLib block 2, original source
C:\BlitzParty\Game\Src\ClassLib\: CProjector, RefBot, SpriteVB, Sprite, DebrisSys,
CRay, CParticleSystem, CollectableEffects, CToken, CWaterField, CSeagulls and the
gap functions around them).

Other modules may include this header. Only the classlib_fx agent edits it.

Conventions (docs/ARCHITECTURE.md):
- Layouts are the original 32-bit ones; offsets are checked with OF_ASSERT_OFFSET.
- Globals shared with other modules keep their Ghidra names (DAT_<addr>), so every
  module links to the same symbol. Uninitialised ones are tentative definitions
  (-fcommon merges them); initialised ones are defined by their owner only.
- Calls into other modules whose table prototype has the wrong types for float
  arguments go through the function-pointer types below (CLF_*). Each one names the
  prototype this module assumes; the owner module must agree (see the final report).
*/

#ifndef OPENFRENZY_CLASSLIB_FX_H
#define OPENFRENZY_CLASSLIB_FX_H

#include "xtl.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ======================================================================== helpers */

/* bKernel heap (bkernel): operator new is bkHeapAlloc(size, file, line, flags) */
#define CLF_Alloc(size, file, line, flags) \
	(((void *(*)(uint, const char *, int, uint))FUN_0004c480)((uint)(size), (file), (line), (flags)))
/* bkHeapCalloc(size, fill, file, line, flags) */
#define CLF_Calloc(size, fill, file, line, flags) \
	(((void *(*)(uint, byte, const char *, int, uint))FUN_0004c4b0)((uint)(size), (fill), (file), (line), (flags)))
#define CLF_Free(p) (((void (*)(void *))FUN_0004c110)((void *)(p)))
/* the game's debug print (bkernel bkPrintf): a printf format */
#define CLF_Printf ((void (*)(const char *, ...))FUN_0004b230)
/* bkernel 0x0004bb30: find a texture by name (package, name, flags, context) */
#define CLF_FindTexture(pkg, name, flags, ctx) \
	(((int (*)(int, const char *, int, uint))FUN_0004bb30)((pkg), (name), (flags), (ctx)))

/* MSVC rand(), inlined into the game: holdrand is the C library's seed at 0x00285630
(initial value 1, owned by the C runtime). */
extern unsigned int DAT_00285630;
static inline int CLF_Rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* x87 _ftol: truncating float to integer conversion */
#define CLF_FTOL(x) ((int)(longlong)(x))

/* ======================================================================== globals of other modules */

/* 0x0028fda8  frame time scale: 1.0 at the nominal frame rate (classlib_ui) */
extern float DAT_0028fda8;
/* 0x0028fdb8.. ClassLib unit vectors (classlib_ui data): (1,0,0,1) and (0,1,0) */
extern float DAT_0028fdb8[4];
extern float DAT_0028fdc8[3];

/* 0x002855f0  the identity matrix (bdisplay; tentative definition in ClassLib/clf_extern.c) */
extern float DAT_002855f0[16];

/* xkernel: the kernel's debug break (the port logs and continues) */
void DbgBreakPoint(void);

/* ======================================================================== intrusive list

The ClassLib list (classlib_ui 0x00071050-0x00071110): the head is itself a node. */
typedef struct CLFList
{
	void *next;  /* 0x0 */
	void *prev;  /* 0x4 */
	int count;   /* 0x8 */
} CLFList;
OF_ASSERT_SIZE(CLFList, 0xc);

/* CLList_Insert(list, node, at_tail), CLList_Remove(list, node) -> previous node,
CLList_Next(list, node or NULL) -> next node or NULL, CLList_Clear(list) */
#define CLF_ListInsert(list, node, tail) \
	(((void (*)(void *, void *, int))FUN_00071070)((void *)(list), (void *)(node), (tail)))
#define CLF_ListRemove(list, node) \
	((void *)((int (*)(void *, void *))FUN_000710b0)((void *)(list), (void *)(node)))
#define CLF_ListNext(list, node) \
	((void *)((unsigned int (*)(void *, void *))FUN_00071110)((void *)(list), (void *)(node)))
#define CLF_ListClear(list) (((void (*)(void *))FUN_000710e0)((void *)(list)))
#define CLF_ListInit(list) (((void (*)(void *))FUN_00071050)((void *)(list)))

/* classlib_ui samples: CLSamples_PlayGeneric(index, volume, arg3, flags, pos) */
#define CLF_PlaySample(index, volume, a3, flags, pos) \
	(((int (*)(int, int, int, unsigned int, float *))FUN_00071720)((index), (volume), (a3), (flags), (pos)))

/* ======================================================================== bdisplay (assumed prototypes) */

/* 0x000407d0  draw a textured beam/strip between two points (like 0x00040900):
(p0, width0a, width0b, colour0 (int[4]), p1, width1a, width1b, colour1, texture, a, b) */
#define CLF_DrawBeam \
	((int (*)(float *, float, float, int *, float *, float, float, int *, int, int, int))FUN_000407d0)
#define CLF_DrawBeam2 \
	((int (*)(float *, float, float, int *, float *, float, float, int *, int, int, int))FUN_00040900)
/* 0x00041a40  lock a texture level: returns the pixels; width, height, pitch (bytes), format */
#define CLF_LockTexture \
	((void *(*)(int, int, int *, int *, int *, int *, int *))FUN_00041a40)
/* 0x00041ac0  unlock a texture level (texture, level, changed) */
#define CLF_UnlockTexture ((void (*)(int, int, int))FUN_00041ac0)

/* ======================================================================== bactor (assumed prototypes) */

/* 0x00035770  play an animation (actor, model, anim, flags, loops, a6, callback, user) */
#define CLF_PlayAnim \
	((void (*)(int, int, int, int, int, int, void *, int))FUN_00035770)
/* 0x000358d0  play an animation with a blend time (actor, model, anim, flags, loops, blend, a7, callback, user) */
#define CLF_PlayAnimBlend \
	((void (*)(int, int, int, int, int, float, int, void *, int))FUN_000358d0)
/* 0x000380f0  advance an actor's animation (actor, model, time scale, a4) */
#define CLF_UpdateAnim ((int (*)(int, int, float, int))FUN_000380f0)
/* 0x00035b90  find a node (bone) by name in a model: returns the node or 0 */
#define CLF_FindNode ((int (*)(int, const char *))FUN_00035b90)
/* 0x00035c70  find an animation by name: returns the anim (int[]: +4 first frame, +8 last frame) */
#define CLF_FindAnim ((int *(*)(int, const char *))FUN_00035c70)
/* 0x00035ce0  world position of a node: (actor, node, float out[4]) */
#define CLF_NodePosition ((void (*)(int, int, float *))FUN_00035ce0)
/* 0x00034e70  set an actor's animation frame (actor, node, frame, a4) */
#define CLF_SetFrame ((void (*)(int, int, int, int))FUN_00034e70)
/* 0x00034bf0  update an actor's matrices (actor, flag) */
#define CLF_UpdateActor ((void (*)(int, int))FUN_00034bf0)

/* ======================================================================== bumpercars_a effects (assumed prototypes) */

/* 0x00080d60  emit sparkles: (system, float pos[4], float vel[4], count, colour) */
#define CLF_EmitSparkles ((int (*)(int, float *, float *, int, uint))FUN_00080d60)
/* 0x00081880  emit a streak burst: (name ptr, count, pos, f, f, f, a7, colour) */
#define CLF_StreakBurst \
	((void (*)(void *, int, float *, float, float, float, int, int))FUN_00081880)
/* 0x00081cb0  emit a streak ring: (name ptr, count, f, pos, f, f, a7) */
#define CLF_StreakRing \
	((void (*)(void *, int, float, float *, float, float, int))FUN_00081cb0)

/* ======================================================================== CProjector.cpp */

/* one trail type of the projector rays (0x4c bytes; table at 0x00290cc8, 3 entries).
classlib_ui (0x0007a550/0x0007a900) reads and writes the colours of entry 1. */
typedef struct CProjRayType
{
	int beamColour0[4]; /* 0x00 RGBA 0..127 */
	int beamColour1[4]; /* 0x10 */
	int trailHead[4];   /* 0x20 colour at the head of the trail */
	int trailTail[4];   /* 0x30 colour at the tail */
	char *textureName;  /* 0x40 */
	int texture;        /* 0x44 found by CProjector_LoadTextures */
	uint flags;         /* 0x48 bit 0: the bone animation plays once */
} CProjRayType;
OF_ASSERT_SIZE(CProjRayType, 0x4c);

/* a texture for a spiral beam (0xc bytes; table at 0x00290dc8, 2 entries) */
typedef struct CProjSpiralType
{
	char *textureName; /* 0x0 */
	int texture;       /* 0x4 */
	int sparkles;      /* 0x8 the sparkle system (bumpercars_a data at 0x002916dc) */
} CProjSpiralType;
OF_ASSERT_SIZE(CProjSpiralType, 0xc);

/* a trail: a strip of up to maxPoints points that fades out (0x584c bytes) */
typedef struct CProjTrail
{
	void *next;            /* 0x00 list node */
	void *prev;            /* 0x04 */
	CLFList *list;         /* 0x08 &DAT_002ed424 or &DAT_002ed430 */
	short dirty;           /* 0x0c a point was added this frame */
	short dying;           /* 0x0e the owner is gone: free when empty */
	int (*colours)[4];     /* 0x10 per point RGBA (count + 1) */
	float *widths;         /* 0x14 per point */
	float *extra;          /* 0x18 per point, zeroed */
	float (*points)[4];    /* 0x1c per point x, y, z, 1 */
	short maxPoints;       /* 0x20 */
	short numPoints;       /* 0x22 */
	short numDrawn;        /* 0x24 */
	short pad26;
	int headColour[4];     /* 0x28 */
	int tailColour[4];     /* 0x38 */
	float headWidth;       /* 0x48 */
	float tailWidth;       /* 0x4c */
	float fade;            /* 0x50 alpha level, starts at headColour[3] */
	float fadeRate;        /* 0x54 127 / maxPoints */
	byte vertices[0x57f0]; /* 0x58 built by bdisplay 0x00040cd0 */
	int texture;           /* 0x5848 */
} CProjTrail;
OF_ASSERT_OFFSET(CProjTrail, fade, 0x50);
OF_ASSERT_OFFSET(CProjTrail, texture, 0x5848);
OF_ASSERT_SIZE(CProjTrail, 0x584c);

/* a projector ray: follows a bone animation and leaves a trail (0x54 bytes) */
typedef struct CProjector
{
	void *next;            /* 0x00 list node */
	void *prev;            /* 0x04 */
	CLFList *list;         /* 0x08 &DAT_002ed418 or &DAT_002ed43c */
	float source[4];       /* 0x0c where the beam starts */
	float target[4];       /* 0x1c position of the traced bone */
	float life;            /* 0x2c frames left */
	float width;           /* 0x30 */
	int actor;             /* 0x34 */
	int node;              /* 0x38 the traced bone */
	int *anim;             /* 0x3c [1] first frame, [2] last frame */
	int frame;             /* 0x40 */
	int speed;             /* 0x44 frames per tick */
	CProjTrail *trail;     /* 0x48 */
	int step;              /* 0x4c trail point spacing in frames */
	CProjRayType *type;    /* 0x50 */
} CProjector;
OF_ASSERT_OFFSET(CProjector, trail, 0x48);
OF_ASSERT_SIZE(CProjector, 0x54);

/* 0x00290cc0 / 0x00290cc4  names of the traced anim and bone ("tracey", "traceme") */
extern char *DAT_00290cc0;
extern char *DAT_00290cc4;
/* 0x00290cc8  the ray types */
extern CProjRayType DAT_00290cc8[3];
/* 0x00290dac  life of a ray (frames); 0x00290db0 its speed; 0x00290db4 its width */
extern float DAT_00290dac;
extern float DAT_00290db0;
extern float DAT_00290db4;
/* 0x00290db8-0x00290dc4  debug switches: draw beams, draw trail strips, draw trails, update trails */
extern int DAT_00290db8;
extern int DAT_00290dbc;
extern int DAT_00290dc0;
extern int DAT_00290dc4;
/* 0x00290dc8  spiral textures */
extern CProjSpiralType DAT_00290dc8[2];
/* 0x00290de0..0x00290df4  spiral beam settings (height, radius, turns, sparkle rate, sparkle
size, trail width) */
extern float DAT_00290de0;
extern float DAT_00290de4;
extern int DAT_00290de8;
extern float DAT_00290dec;
extern float DAT_00290df0;
extern float DAT_00290df4;

/* 0x002ed418 / 0x002ed43c  projector lists (normal / attached to the RefBot);
0x002ed430 / 0x002ed424  trail lists (normal / second pass). Constructed by core's static
constructors 0x001f2af0-0x001f2b50. */
extern CLFList DAT_002ed418;
extern CLFList DAT_002ed424;
extern CLFList DAT_002ed430;
extern CLFList DAT_002ed43c;

/* 0x00290df8  "refrobot2.obe"; 0x00290e18 animation names; 0x00290e28 actor settings */
extern char *DAT_00290df8;
extern char *DAT_00290e18[4];
extern float DAT_00290e28[4];
/* 0x002ed3a0  saved render state block (bdisplay 0x00046d40 / 0x00046d60) */
extern int DAT_002ed3a0[30];
/* 0x002ed44c  the "actordebris" system (bactor 0x0003c780); 0x002ed450 kill it */
extern int DAT_002ed44c;
extern int DAT_002ed450;

/* ---------- other modules' globals (tentative definitions in ClassLib/clf_extern.c) */
extern int DAT_002916dc[1];
extern int DAT_00291708[1];
extern byte DAT_00290718[1];
extern float DAT_002ea478[4];
extern int DAT_002ea4f4;
extern int DAT_002ecdec;
extern int DAT_002baeb8;
extern int DAT_003055e0;
extern int DAT_002ed340;
extern int DAT_002ed344;

/* ======================================================================== RefBot.cpp */

/* the referee robot that carries a projector (0x30 bytes) */
typedef struct CRefBot
{
	int actor;         /* 0x00 */
	int laserNode;     /* 0x04 "refrobo_hole" */
	int active;        /* 0x08 */
	int anims[4];      /* 0x0c names at 0x00290e18 */
	int sample;        /* 0x1c */
	CLFList textures;  /* 0x20 animated screen textures (CAnimTex) */
	float texSpeed;    /* 0x2c */
} CRefBot;
OF_ASSERT_SIZE(CRefBot, 0x30);

/* 0x002ed448  the RefBot, or 0 */
extern CRefBot *DAT_002ed448;

/* an animated texture (0x18 bytes): its frame advances every tick */
typedef struct CAnimTex
{
	void *next;     /* 0x00 list node */
	void *prev;     /* 0x04 */
	int texture;    /* 0x08 */
	float frame;    /* 0x0c */
	float speed;    /* 0x10 frames per tick */
	CLFList *list;  /* 0x14 */
} CAnimTex;
OF_ASSERT_SIZE(CAnimTex, 0x18);

/* RefBot animation callbacks (not in the function table: Ghidra missed them) */
int FUN_00084880(int event, int actor, int model, int anim, int user);
int FUN_00084970(int event, int actor, int model, int anim, int user);

/* ======================================================================== point sprites (gap 0x00084f30-0x00085060) */

/* a set of vertex buffers for point sprites (0x9c bytes) */
typedef struct CPointSprites
{
	int vertsPerBuffer;    /* 0x00 at most 16000 */
	int buffers[30];       /* 0x04 vertex buffers (at most 28 are created) */
	int unknown7c[2];      /* 0x7c */
	void *write;           /* 0x84 locked vertices of the current buffer */
	int vertsLeft;         /* 0x88 */
	int buffer;            /* 0x8c current buffer */
	int blendMode;         /* 0x90 0 opaque, 1-4 blend table entries 0, 1, 3, 2 */
	int param94;           /* 0x94 */
	int texture;           /* 0x98 bound to stage 3 */
} CPointSprites;
OF_ASSERT_SIZE(CPointSprites, 0x9c);

/* 0x00290e38  blend modes: { source blend, destination blend, blend op } */
extern uint DAT_00290e38[][3];

/* ======================================================================== SpriteVB.cpp */

/* one texture's batch of quads in the debris vertex buffer (0x20 bytes) */
typedef struct CSpriteVBBatch
{
	struct CSpriteVBBatch *next; /* 0x00 */
	struct CSpriteVBBatch *prev; /* 0x04 */
	int key;                     /* 0x08 the sprite type: +0x0c model, +0x24 flag, +0x28 flags */
	byte *write;                 /* 0x0c next vertex to write */
	byte *start;                 /* 0x10 first vertex */
	int count;                   /* 0x14 sprites this frame */
	int firstVertex;             /* 0x18 */
	int numVertices;             /* 0x1c */
} CSpriteVBBatch;
OF_ASSERT_SIZE(CSpriteVBBatch, 0x20);

/* the debris vertex buffer ("debrisVB"): a list of batches */
typedef struct CSpriteVB
{
	CSpriteVBBatch *next;  /* 0x00 list head */
	CSpriteVBBatch *prev;  /* 0x04 */
	int unknown08[6];      /* 0x08 */
	CSpriteVBBatch *last;  /* 0x20 batch found last */
	int count;             /* 0x24 batches */
	int vb;                /* 0x28 vertex buffer */
	int vbSize;            /* 0x2c vertices */
	int locked;            /* 0x30 */
	byte *lockEnd;         /* 0x34 */
	byte *lockStart;       /* 0x38 */
} CSpriteVB;
OF_ASSERT_OFFSET(CSpriteVB, lockStart, 0x38);

/* ======================================================================== Sprite.cpp */

/* a sprite definition: frames "<name>NN.bmp" (or .tga) (0x48 bytes) */
typedef struct CSpriteDef
{
	char name[0x40];   /* 0x00 */
	int *frames;       /* 0x40 textures */
	short numFrames;   /* 0x44 */
	ushort flags;      /* 0x46 bit 2: .tga files */
} CSpriteDef;
OF_ASSERT_SIZE(CSpriteDef, 0x48);

/* a sprite animation: a definition played at a frame time */
typedef struct CSpriteAnim
{
	CSpriteDef *def;   /* 0x0 */
	float frameTime;   /* 0x4 */
	ushort flags;      /* 0x8 1 paused, 2 random first frame, 4 loop */
} CSpriteAnim;

/* a sprite instance (0x34 bytes) */
typedef struct CSprite
{
	CSpriteAnim *anim;  /* 0x00 */
	int texture;        /* 0x04 current frame's texture */
	int frame;          /* 0x08 */
	float timer;        /* 0x0c */
	float pos[4];       /* 0x10 */
	short animFlags;    /* 0x20 */
	byte unknown22;     /* 0x22 */
	byte unknown23;     /* 0x23 */
	int user;           /* 0x24 */
	byte colour[4];     /* 0x28 */
	float angle;        /* 0x2c */
	float spin;         /* 0x30 */
} CSprite;
OF_ASSERT_SIZE(CSprite, 0x34);

#endif
