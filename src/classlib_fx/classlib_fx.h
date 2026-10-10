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

/* ======================================================================== bdisplay

Prototypes from src/bdisplay's .decls (bdDrawBeam3D, bdDrawBeamClipped, bdTextureLock,
bdTextureUnlock, bdDrawSprite3D, bdDrawPolyline3D/Clipped, bdDrawPointSprites), called
through casts so that texture and buffer handles can stay ints here. */

/* 0x000407d0 / 0x00040900  a textured beam between two points: (pos1, width1, length1,
colour1 (int[4]), pos2, width2, length2, colour2, texture, vertices (NULL: draw now), zBias) */
#define CLF_DrawBeam \
	((int (*)(float *, float, float, int *, float *, float, float, int *, int, float *, float))FUN_000407d0)
#define CLF_DrawBeam2 \
	((int (*)(float *, float, float, int *, float *, float, float, int *, int, float *, float))FUN_00040900)
/* 0x00041a40  lock a texture: returns the pixels; width, height, pitch (bytes), format, palette */
#define CLF_LockTexture \
	((void *(*)(int, int, int *, int *, int *, int *, int *))FUN_00041a40)
/* 0x00041ac0  unlock a texture */
#define CLF_UnlockTexture ((void (*)(int))FUN_00041ac0)
/* 0x00040750  a rotated sprite: (pos, width, height, angle, texture, flipU, flipV, r, g, b, a,
vertices, zBias) */
#define CLF_DrawSprite \
	((int (*)(float *, float, float, float, int, int, int, int, int, int, int, void *, float))FUN_00040750)
/* 0x00040880 / 0x00040cd0  a strip through n points: (n, points, widths, screenWidths,
colours, vertices, zBias) */
#define CLF_DrawPolyline \
	((int (*)(int, void *, void *, void *, void *, void *, float))FUN_00040880)
#define CLF_DrawPolylineClipped \
	((int (*)(int, void *, void *, void *, void *, void *, float))FUN_00040cd0)
/* 0x000406d0  point sprites from a vertex buffer: (vb, start, count, point size) */
#define CLF_DrawPointSprites ((void (*)(int, int, int, float))FUN_000406d0)

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

/* ======================================================================== CRay.cpp */

struct CRayControl;

/* one rotating ray of an orb (0x34 bytes) */
typedef struct CRay
{
	void *next;                  /* 0x00 list node */
	void *prev;                  /* 0x04 */
	struct CRayControl *control; /* 0x08 */
	float pos[4];                /* 0x0c the ray's end */
	float angle;                 /* 0x1c */
	float speed;                 /* 0x20 radians per tick */
	float length;                /* 0x24 */
	float length0;               /* 0x28 */
	float width0;                /* 0x2c at the orb */
	float width1;                /* 0x30 at the end */
} CRay;
OF_ASSERT_SIZE(CRay, 0x34);

/* an orb with rays that point at a target while a test passes (0x70 bytes) */
typedef struct CRayControl
{
	void *next;            /* 0x00 node in DAT_002f27c8 */
	void *prev;            /* 0x04 */
	CLFList rays;          /* 0x08 */
	int numRays;           /* 0x14 */
	void *test;            /* 0x18 float test(float *target, float *delta, float *a, float *b): <= 0 shows the rays */
	int visible;           /* 0x1c */
	float offset[4];       /* 0x20 added to the anchor */
	int texture;           /* 0x30 */
	float *anchor;         /* 0x34 */
	float centre[4];       /* 0x38 */
	float *target;         /* 0x48 */
	int colour0[4];        /* 0x4c */
	int colour1[4];        /* 0x5c */
	float angle;           /* 0x6c turn towards the target */
} CRayControl;
OF_ASSERT_SIZE(CRayControl, 0x70);

/* 0x002f27c8  every ray control (constructed by core's static constructor 0x001f2ba0) */
extern CLFList DAT_002f27c8;
/* 0x00292c44  turn rate of the rays towards their target (2.0) */
extern float DAT_00292c44;

/* ======================================================================== CParticleSystem.cpp

Five particle system classes share a header and a vtable layout of 6 slots:
  [0] Release  [1] Update(dt)  [2] Draw  [3] Reset  [4] Emit(particle)  [5] deleting destructor
Class names are inferred from what they draw:
  CParticleSystem  vtable 0x00266cd8  spiralling ring trails + point particles (used in arrays)
  CSparkSystem     vtable 0x00266cf0  particles drawn as beams from the previous position
  CSmokeSystem     vtable 0x00266d08  rotating sprites (alpha blended)
  CGlowSystem      vtable 0x00266d20  rotating sprites (additive)
  CSparkPointSystem vtable 0x00266d38 beams + point sprites
A CParticleSystemList (vtable 0x00266cd4) links systems through their word at +4. */

/* one particle (0x68 bytes); colours are 8.7 fixed point shorts */
typedef struct CParticle
{
	byte alive;          /* 0x00 */
	byte pad01;
	short age;           /* 0x02 ticks */
	short life;          /* 0x04 ticks */
	short pad06;
	float pos[4];        /* 0x08 */
	float prevPos[4];    /* 0x18 */
	float vel[4];        /* 0x28 */
	float size;          /* 0x38 */
	float sizeSpeed;     /* 0x3c */
	float angle;         /* 0x40 */
	float spin;          /* 0x44 */
	short colour[4];     /* 0x48 */
	short colour2[4];    /* 0x50 */
	short dColour[4];    /* 0x58 */
	short dColour2[4];   /* 0x60 */
} CParticle;
OF_ASSERT_SIZE(CParticle, 0x68);

/* the header shared by the five classes */
typedef struct CPSHeader
{
	const void *const *vtable;   /* 0x00 */
	struct CPSHeader *next;      /* 0x04 link in a CParticleSystemList */
	int unknown08;               /* 0x08 */
	CParticle *particles;        /* 0x0c */
	int maxParticles;            /* 0x10 */
	int numAlive;                /* 0x14 */
	int texture;                 /* 0x18 */
	float pos[4];                /* 0x1c emitter position (y is also the floor) */
	int unknown2c[4];            /* 0x2c */
} CPSHeader;
OF_ASSERT_SIZE(CPSHeader, 0x3c);

/* a ring trail of CParticleSystem (0x2c bytes) */
typedef struct CPSRing
{
	float (*points)[4];  /* 0x00 ringLength points */
	float *widths;       /* 0x04 */
	float *extra;        /* 0x08 */
	int (*colours)[4];   /* 0x0c */
	float vel[3];        /* 0x10 only [1] (rise speed) is non-zero */
	int unknown1c;       /* 0x1c */
	float angle;         /* 0x20 */
	float radius;        /* 0x24 */
	int count;           /* 0x28 points in use */
} CPSRing;
OF_ASSERT_SIZE(CPSRing, 0x2c);

/* CParticleSystem parameters (0x40 bytes at +0x50) */
typedef struct CPSParams
{
	byte active;         /* 0x50 */
	byte unknown51;
	short pad52;
	float angleSpeed;    /* 0x54 ring turn per tick */
	float riseAccel;     /* 0x58 ring rise acceleration */
	float gravity;       /* 0x5c particle gravity */
	float riseSpeed;     /* 0x60 ring rise speed */
	float speed;         /* 0x64 particle speed */
	float drag;          /* 0x68 */
	float radiusSpeed;   /* 0x6c */
	float radius;        /* 0x70 */
	int numRings;        /* 0x74 */
	int ringLength;      /* 0x78 */
	int maxEmit;         /* 0x7c particles emitted per tick */
	int lifeMin;         /* 0x80 */
	int lifeRange;       /* 0x84 */
	float pointSize;     /* 0x88 */
	float emitTime;      /* 0x8c */
} CPSParams;
OF_ASSERT_SIZE(CPSParams, 0x40);

typedef struct CParticleSystem
{
	CPSHeader h;           /* 0x00 */
	float emitTimer;       /* 0x3c */
	CPSRing *rings;        /* 0x40 */
	int texture2;          /* 0x44 ring texture */
	short colour[3];       /* 0x48 */
	short pad4e;
	CPSParams p;           /* 0x50 */
	int vb;                /* 0x90 ring strips */
	int pointVB;           /* 0x94 point sprites */
	float (*ringPoints)[4];/* 0x98 */
	float *ringWidths;     /* 0x9c */
	float *ringExtra;      /* 0xa0 */
	int (*ringColours)[4]; /* 0xa4 */
} CParticleSystem;
OF_ASSERT_OFFSET(CParticleSystem, p, 0x50);
OF_ASSERT_SIZE(CParticleSystem, 0xa8);

/* CSparkSystem parameters (0x2c bytes at +0x44) */
typedef struct CSparkParams
{
	byte active;         /* 0x44 */
	byte bounce;         /* 0x45 bounce on the floor */
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
} CSparkParams;
OF_ASSERT_SIZE(CSparkParams, 0x2c);

typedef struct CSparkSystem
{
	CPSHeader h;         /* 0x00 */
	int unknown3c[2];    /* 0x3c */
	CSparkParams p;      /* 0x44 */
	int vb;              /* 0x70 */
} CSparkSystem;
OF_ASSERT_OFFSET(CSparkSystem, vb, 0x70);

/* CSmokeSystem parameters (0x20 bytes at +0x3c) */
typedef struct CSmokeParams
{
	byte active;         /* 0x3c */
	byte pad3d[3];
	float speed;         /* 0x40 */
	float drag;          /* 0x44 */
	float gravity;       /* 0x48 */
	float radius;        /* 0x4c */
	int lifeMin;         /* 0x50 */
	int lifeRange;       /* 0x54 */
	float spin;          /* 0x58 */
} CSmokeParams;
OF_ASSERT_SIZE(CSmokeParams, 0x20);

typedef struct CSmokeSystem
{
	CPSHeader h;         /* 0x00 */
	CSmokeParams p;      /* 0x3c */
	int vb;              /* 0x5c */
} CSmokeSystem;
OF_ASSERT_OFFSET(CSmokeSystem, vb, 0x5c);

/* CGlowSystem parameters (0x28 bytes at +0x3c) */
typedef struct CGlowParams
{
	byte active;         /* 0x3c */
	byte pad3d[3];
	float speed;         /* 0x40 */
	float drag;          /* 0x44 */
	float gravity;       /* 0x48 */
	float unknown4c;     /* 0x4c */
	float spin;          /* 0x50 */
	int sizeMin;         /* 0x54 */
	int sizeRange;       /* 0x58 */
	int lifeMin;         /* 0x5c */
	int lifeRange;       /* 0x60 */
} CGlowParams;
OF_ASSERT_SIZE(CGlowParams, 0x28);

typedef struct CGlowSystem
{
	CPSHeader h;         /* 0x00 */
	CGlowParams p;       /* 0x3c */
	int vb;              /* 0x64 */
} CGlowSystem;
OF_ASSERT_OFFSET(CGlowSystem, vb, 0x64);

/* CSparkPointSystem parameters (0x38 bytes at +0x58) */
typedef struct CSparkPointParams
{
	byte active;         /* 0x58 */
	byte bounce;         /* 0x59 */
	short pad5a;
	float speed;         /* 0x5c */
	float drag;          /* 0x60 */
	float gravity;       /* 0x64 */
	float unknown68;     /* 0x68 */
	float unknown6c;     /* 0x6c */
	int lifeMin;         /* 0x70 */
	int lifeRange;       /* 0x74 */
	int sizeMin;         /* 0x78 */
	int sizeRange;       /* 0x7c */
	short colour[3];     /* 0x80 */
	short colour2[3];    /* 0x86 */
	float pointSize;     /* 0x8c */
} CSparkPointParams;
OF_ASSERT_SIZE(CSparkPointParams, 0x38);

typedef struct CSparkPointSystem
{
	CPSHeader h;         /* 0x00 */
	int unknown3c[2];    /* 0x3c */
	int texture2;        /* 0x44 point sprite texture */
	float dir[3];        /* 0x48 emission direction */
	int unknown54;       /* 0x54 */
	CSparkPointParams p; /* 0x58 */
	int vb;              /* 0x90 */
	int pointVB;         /* 0x94 */
} CSparkPointSystem;
OF_ASSERT_OFFSET(CSparkPointSystem, vb, 0x90);

/* a list of systems linked through CPSHeader.next */
typedef struct CParticleSystemList
{
	const void *const *vtable;  /* 0x0 */
	CPSHeader *first;           /* 0x4 */
} CParticleSystemList;

/* an emitter of a CParticleEffect: an array of CParticleSystem, an actor (0x40 bytes or more) */
typedef struct CParticleEmitter
{
	int on;                        /* 0x00 */
	int id;                        /* 0x04 */
	uint colour;                   /* 0x08 0xRRGGBB or -1 */
	int next;                      /* 0x0c next system of the array to fire */
	float scale;                   /* 0x10 */
	float time;                    /* 0x14 seconds while on */
	int actor;                     /* 0x18 */
	int anim;                      /* 0x1c */
	CParticleSystem *systems;      /* 0x20 */
	int texture;                   /* 0x24 */
	float *drift;                  /* 0x28 added to every particle's velocity */
	float pos[4];                  /* 0x2c */
	struct CParticleEmitter *link; /* 0x3c */
} CParticleEmitter;
OF_ASSERT_OFFSET(CParticleEmitter, link, 0x3c);

/* a set of emitters with the same number of systems each */
typedef struct CParticleEffect
{
	int numSystems;              /* 0x00 per emitter */
	float scale;                 /* 0x04 */
	CParticleEmitter *emitters;  /* 0x08 */
	int unknown0c[2];            /* 0x0c */
	int actor;                   /* 0x14 drawn at each emitter in mode 0 */
	int texture1;                /* 0x18 */
	int texture2;                /* 0x1c */
} CParticleEffect;

/* the vtables (rebuilt with symbols) */
extern const void *const vtable_00266cd4[1];
extern const void *const vtable_00266cd8[6];
extern const void *const vtable_00266cf0[6];
extern const void *const vtable_00266d08[6];
extern const void *const vtable_00266d20[6];
extern const void *const vtable_00266d38[6];

/* bdisplay colour tables: red << 16, green << 8, blue, alpha << 24 by 0..255 */
extern unsigned int DAT_002bce18[256];
extern unsigned int DAT_002bc490[256];
extern unsigned int DAT_002bd640[256];
extern unsigned int DAT_002bc898[256];
/* 0x002bc48c  triangles drawn this frame (bdisplay) */
extern int DAT_002bc48c;
/* 0x00284b10  ambient light colour (bactor) */
extern int DAT_00284b10;
extern int DAT_00284b14;
extern int DAT_00284b18;
/* 0x002ba99c  game flags (core): bit 0 keeps the previous particle positions */
extern int DAT_002ba99c;

/* ======================================================================== CollectableEffects.cpp */

/* one ray of a collectable effect (0x54 bytes) */
typedef struct CCollectRay
{
	float base[4];     /* 0x00 */
	float tip[4];      /* 0x10 base + direction * radius */
	int unknown20[4];  /* 0x20 */
	float rot[4];      /* 0x30 Euler angles that turn (0, 0, 1) into the direction */
	int colour[4];     /* 0x40 random 0..126 */
	int pad50;
} CCollectRay;
OF_ASSERT_SIZE(CCollectRay, 0x54);

/* rays that burst out of a collected item, then fade (charlib uses it) */
typedef struct CCollectEffect
{
	CCollectRay *rays;   /* 0x00 */
	int colour[4];       /* 0x04 random 0..126 */
	int unknown14[4];    /* 0x14 */
	int iter;            /* 0x24 the update loop counter (a member in the original) */
	int numRays;         /* 0x28 */
	float unknown2c;     /* 0x2c 6.0 */
	float pos[4];        /* 0x30 */
	float spin[4];       /* 0x40 random */
	float rot[4];        /* 0x50 random */
	int texture;         /* 0x60 "projray3.bmp" */
	float top[4];        /* 0x64 pos raised by 70 */
	int fadeColour[4];   /* 0x74 */
	int unknown84;       /* 0x84 */
	int state;           /* 0x88 0 grow, 1 hold, 2 shrink and fade, 3 done */
	float radius;        /* 0x8c */
	float maxRadius;     /* 0x90 */
	float progress;      /* 0x94 */
} CCollectEffect;
OF_ASSERT_OFFSET(CCollectEffect, progress, 0x94);

/* ======================================================================== CToken.cpp */

/* a damped spring on up to 4 values (0x3c bytes) */
typedef struct CSpring
{
	float cur[4];      /* 0x00 */
	float target[4];   /* 0x10 */
	float vel[4];      /* 0x20 */
	float stiffness;   /* 0x30 */
	float damping;     /* 0x34 */
	int count;         /* 0x38 values used */
} CSpring;
OF_ASSERT_SIZE(CSpring, 0x3c);

/* a shadow quad vertex (0x24 bytes) */
typedef struct CTokenVertex
{
	float pos[3];      /* 0x00 */
	float unknown0c[3];/* 0x0c */
	uint colour;       /* 0x18 */
	float uv[2];       /* 0x1c */
} CTokenVertex;
OF_ASSERT_SIZE(CTokenVertex, 0x24);

/* a bronze, silver or gold token that springs up, spins and vanishes when collected */
typedef struct CToken
{
	void *next;               /* 0x000 node in DAT_002ff9d4 */
	void *prev;               /* 0x004 */
	int unknown08[2];         /* 0x008 */
	byte actor[0x170];        /* 0x010 the token's actor instance (bactor BActorInstance) */
	float pos[4];             /* 0x180 */
	float up[4];              /* 0x190 */
	CSpring scale;            /* 0x1a0 the actor's scale */
	float size;               /* 0x1dc */
	float spinSpeed;          /* 0x1e0 */
	float angle;              /* 0x1e4 */
	int state;                /* 0x1e8 1 appearing, 2 idle, 4 vanishing */
	int type;                 /* 0x1ec 0 bronze, 1 silver, 2 gold */
	int glow;                 /* 0x1f0 drawn in the glow pass */
	CTokenVertex shadow[4];   /* 0x1f4 */
} CToken;
OF_ASSERT_OFFSET(CToken, pos, 0x180);
OF_ASSERT_OFFSET(CToken, shadow, 0x1f4);

/* a token model ("bronzetoken.obe", ...) and its loaded actor */
typedef struct CTokenModel
{
	char *name;   /* 0x0 */
	int actor;    /* 0x4 loaded by demolition_d */
} CTokenModel;

/* 0x00298d1c-0x00298d48  token tunables (scale spring damping, stiffness, start height,
glow factor, shadow size, models, actor height) */
extern float DAT_00298d1c;
extern float DAT_00298d20;
extern float DAT_00298d24;
extern uint DAT_00298d28;
extern float DAT_00298d2c;
extern CTokenModel DAT_00298d30[3];
extern float DAT_00298d48;
/* 0x002ff9c8  shadow mode (0 none, 1 alpha, else additive); 0x002ff9d0 shadow texture */
extern int DAT_002ff9c8;
extern int DAT_002ff9d0;
/* 0x002ff9d4  every token (constructed by core's static constructor 0x001f2c20) */
extern CLFList DAT_002ff9d4;

/* ======================================================================== CWaterField.cpp */

/* the parameters of a water field (0x40 bytes; defaults at 0x002ae578) */
typedef struct CWaterParams
{
	float x0;            /* 0x04 corner */
	float y;             /* 0x08 surface height */
	float z0;            /* 0x0c */
	float sizeX;         /* 0x10 */
	float sizeZ;         /* 0x14 recomputed from the cell counts */
	int nx;              /* 0x18 cells across (made even) */
	int nz;              /* 0x1c cells deep (made even) */
	int wrap;            /* 0x20 the left and right edges wrap */
	float damping;       /* 0x24 */
	float tension;       /* 0x28 */
	float heightScale;   /* 0x2c */
	float normalScale;   /* 0x30 slope to normal table index */
	float slopeUV;       /* 0x34 slope to texture offset */
	uint colour;         /* 0x38 */
	char *textureName;   /* 0x3c */
	float uvScale;       /* 0x40 */
} CWaterParams;
OF_ASSERT_SIZE(CWaterParams, 0x40);

/* a water vertex (FVF 0x152: position, normal, colour, uv; 0x24 bytes) */
typedef struct CWaterVertex
{
	float pos[3];
	float normal[3];
	uint colour;
	float uv[2];
} CWaterVertex;
OF_ASSERT_SIZE(CWaterVertex, 0x24);

/* a rippling water surface (vtable 0x0026e05c) */
typedef struct CWaterField
{
	const void *const *vtable;   /* 0x0000 */
	CWaterParams p;              /* 0x0004 */
	byte *flags;                 /* 0x0044 per cell: 0 water, 1 fixed, 2 left edge, 3 right edge */
	CWaterVertex edges[4][5];    /* 0x0048 the four edge fans */
	byte unknown318[0x9048 - 0x318];
	int texture;                 /* 0x9048 */
	float cellX;                 /* 0x904c */
	float cellZ;                 /* 0x9050 */
	int numCells;                /* 0x9054 */
	float uOffset;               /* 0x9058 */
	float vOffset;               /* 0x905c */
	float unknown9060;           /* 0x9060 */
	int unknown9064;             /* 0x9064 */
	int unknown9068;             /* 0x9068 */
	float unknown906c;           /* 0x906c */
	float uvPerUnit;             /* 0x9070 */
	float uvStep;                /* 0x9074 per cell */
	float (*cells)[2];           /* 0x9078 height, velocity */
	int numIndices;              /* 0x907c */
	int numVerts;                /* 0x9080 */
	int vb;                      /* 0x9084 */
	int ib;                      /* 0x9088 */
	float (*normals)[3];         /* 0x908c 32 x 32 table */
} CWaterField;
OF_ASSERT_OFFSET(CWaterField, texture, 0x9048);
OF_ASSERT_OFFSET(CWaterField, normals, 0x908c);

extern const void *const vtable_0026e05c[11];
/* 0x002ae578  default parameters */
extern CWaterParams DAT_002ae578;

#endif
