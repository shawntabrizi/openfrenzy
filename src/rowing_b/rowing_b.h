/*
ROWING_B.H

Module rowing_b: the Rowing (jet boats) minigame, part 2 (0x001b12c0-0x001ba160), from
C:\BlitzParty\Game\Src\Rowing\ (CRowing_Sprites.cpp, CRowing_World.cpp, CRowing_Spark.cpp,
CRowing_Lights.cpp, CRowing_Tokens.cpp, CRowing_objects.cpp, Rowing_Sound.cpp,
CRowing_ASEImport.cpp and gap functions between them). Part 1 is rowing_a.

Owner: rowing_b. Other modules may include this header; only rowing_b edits it.

Conventions (docs/ARCHITECTURE.md; the same as carryon_b):

- Object layouts keep the original (Xbox, ILP32) offsets. Fields that are not understood
  yet are reached with the RB_I32/RB_F32/... accessors and the original byte offset.
- x86 `this` (ECX) is the first parameter. Ghidra often drops a `this` that a caller loads
  into ECX; tools/ecx.py shows it for every call.
- The Rowing globals (both Rowing modules) live in two blocks with the original layout:
  rowing_b_data.h, RW_VAR(type, original address).
- Calls to functions of other modules whose table prototype (of_decls.h) is still a Ghidra
  guess go through RB_CALL (a cast to the shape read from the x86 code), so they stay right
  when the owner fixes the prototype.
- Read-only .rdata constants are written as literals with their address in a comment.
- x87 compares: Ghidra's "(a < b) == (a == b)" means a > b, and "(a < b) != (a == b)"
  means a <= b. They are written as plain compares.
*/

#ifndef ROWING_B_H
#define ROWING_B_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "xtl.h"
#include "d3d8_extra.h"   /* src/d3d8: D3DDevice_InsertFence, SetTextureState_Deferred */
#include "rowing_b_data.h"

/* ---------- raw field access at original offsets */

#define RB_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define RB_I32(p, off) RB_AT(p, off, int)
#define RB_U32(p, off) RB_AT(p, off, unsigned int)
#define RB_F32(p, off) RB_AT(p, off, float)
#define RB_I16(p, off) RB_AT(p, off, short)
#define RB_U16(p, off) RB_AT(p, off, unsigned short)
#define RB_U8(p, off) RB_AT(p, off, unsigned char)
#define RB_I8(p, off) RB_AT(p, off, signed char)
#define RB_PTR(p, off) RB_AT(p, off, void *)
#define RB_FP(p, off) ((float *)((char *)(p) + (off)))
#define RB_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define RB_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* call a function of another module with the shape read from the x86 code */
#define RB_CALL(fn, real_type) ((real_type)(fn))

static inline unsigned int rb_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

static inline float rb_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* the C library rand() of the XBE (MSVC's, inlined by the game; seed DAT_00285630) */
extern unsigned int DAT_00285630;
static inline int rb_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

#define RB_RAND01 3.0518509e-05f    /* 1/32767 (0x00253740) */

/* rand() / 32767: 0 to 1 */
static inline float rb_randf(void)
{
	return (float)rb_rand() * RB_RAND01;
}

/* the kernel debug break (xkernel: the port logs it and continues) */
void DbgBreakPoint(void);

/* ---------- Babel engine helpers (bkernel) */

#define RB_NEW(size, file, line, flags) bkHeapAlloc((uint)(size), (file), (line), (flags))
#define RB_DELETE(p) bkHeapFree((void *)(p))

#define RB_FILE_SPRITES "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_Sprites.cpp"
#define RB_FILE_WORLD "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_World.cpp"
#define RB_FILE_SPARK "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_Spark.cpp"
#define RB_FILE_LIGHTS "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_Lights.cpp"
#define RB_FILE_TOKENS "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_Tokens.cpp"
#define RB_FILE_OBJECTS "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_objects.cpp"
#define RB_FILE_SOUND "C:\\BlitzParty\\Game\\Src\\Rowing\\Rowing_Sound.cpp"
#define RB_FILE_ASE "C:\\BlitzParty\\Game\\Src\\Rowing\\CRowing_ASEImport.cpp"

/* ---------- globals of other modules */

extern char DAT_002855f0[];         /* the identity matrix (float[16], bkernel) */
extern float DAT_002be330[16];      /* the camera rotation (bdisplay) */
extern float DAT_0028fda8;          /* frame time factor, 1.0 = 1/60 s (core) */
extern int DAT_002bae94;            /* number of players (core) */
extern int DAT_002b9c58[];          /* the players in race order (core) */
/* colour channel tables (bdisplay): byte -> D3DCOLOR bits of red, green, blue, alpha */
extern unsigned int DAT_002bc490[];  /* green */
extern unsigned int DAT_002bd640[];  /* blue */
extern unsigned int DAT_002bce18[];  /* red */
extern unsigned int DAT_002bc898[];  /* alpha */

/* the D3DCOLOR of 0-255 channels */
#define RB_RGBA(r, g, b, a) \
	(DAT_002bce18[(r)] | DAT_002bc490[(g)] | DAT_002bd640[(b)] | DAT_002bc898[(a)])

/* ---------- CRowing_Sprites.cpp */

/* 0x0030fb40  five sprite lists (one per sprite type): a vertex array of 0x1800 vertices
   (0x1c bytes each) that bdDrawSprite3D fills, its vertex count and its texture */
struct RowSpriteList {
	void *verts;                /* +0x00 */
	int count;                  /* +0x04 vertices used */
	void *texture;              /* +0x08 the texture of the last sprite added */
};
#define g_RowSpriteLists ((struct RowSpriteList *)RW_ADDR(0x0030fb40))

/* camera-facing quads (6 vertices each, a triangle strip with degenerate joins) in up to 8
   vertex buffers; the Rowing copy of CFire_QuadSprites (0x68 bytes, allocated by
   CRowingParticles_Init) */
struct RowQuadSprites {
	int vertsPerBuffer;                 /* +0x00 */
	struct bdVertexBuffer *buffers[8];  /* +0x04 */
	float *write;                       /* +0x24 next vertex in the locked buffer */
	int vertsLeft;                      /* +0x28 */
	int buffer;                         /* +0x2c the locked buffer */
	int blendMode;                      /* +0x30 0 opaque, 1-4 Blend_SetMode 0, 1, 3, 2 */
	void *texture;                      /* +0x34 */
	float corners[12];                  /* +0x38 quad corners from the camera's axes */
};

/* a point of a helix trail */
struct RowHelixPoint {
	float pos[4];
};

/* the twisting trail behind a boat (0x68 bytes, in the rowing_a player): up to 18 points
   that drift and spread, drawn as a polyline with the helixtwist texture */
struct RowHelix {
	float flash;                        /* +0x00 >0: the colour fades to white (counts down) */
	int colour[3];                      /* +0x04 red, green, blue */
	void *texture;                      /* +0x10 tex_special\helixtwist.bmp */
	struct RowHelixPoint *points;       /* +0x14 18 points, newest first */
	struct RowHelixPoint *vels;         /* +0x18 18 velocities */
	int unused1c[4];                    /* +0x1c */
	float axis[4];                      /* +0x2c 0, 1, 0, 1 */
	int unused3c[7];                    /* +0x3c */
	int arg58;                          /* +0x58 the 4th constructor argument */
	int *alphas;                        /* +0x5c 18 alphas */
	int count;                          /* +0x60 points used */
	struct bdVertexBuffer *vb;          /* +0x64 40 vertices */
};

/* the shared polyline arrays of the helix trails (0x0030fb7c, 0x0030fb80, 0x0030fb84):
   18 widths, 18 screen widths, 18 colours (r, g, b, a ints) */
#define g_RowHelixWidths RW_VAR(float *, 0x0030fb7c)
#define g_RowHelixScreenWidths RW_VAR(float *, 0x0030fb80)
#define g_RowHelixColours RW_VAR(int *, 0x0030fb84)

/* a steam puff (0x54 bytes; free list at particles +0x10, active list at +0x0c) */
struct RowSteam {
	struct RowSteam *next;      /* +0x00 */
	float pos[4];               /* +0x04 */
	float vel[3];               /* +0x14 */
	float pad20;                /* +0x20 */
	float rise;                 /* +0x24 added to vel[1] every update */
	float angle;                /* +0x28 */
	float spin;                 /* +0x2c */
	float size;                 /* +0x30 */
	float grow;                 /* +0x34 */
	int colour[3];              /* +0x38 red, green, blue */
	int pad44;                  /* +0x44 */
	float alpha;                /* +0x48 */
	float fade;                 /* +0x4c alpha step (rises, then falls at a third) */
	float life;                 /* +0x50 updates left */
};

/* a spray blob (0x4c bytes; free list at particles +0x20, active list at +0x1c) */
struct RowBlob {
	struct RowBlob *next;       /* +0x00 */
	float pos[4];               /* +0x04 */
	float vel[3];               /* +0x14 */
	float pad20;                /* +0x20 */
	float size;                 /* +0x24 */
	float grow;                 /* +0x28 */
	float colour[4];            /* +0x2c alpha, red, green, blue */
	float fade[3];              /* +0x3c red, green, blue steps */
	float life;                 /* +0x48 updates left */
};

/* the particles of the race (rowing_a game object, 0x28 bytes from CRowingParticles_Init):
   steam puffs and spray blobs, each drawn as quads */
struct RowParticles {
	void *steamTexture;             /* +0x00 tex_special\steam_01.bmp */
	void *blobTexture;              /* +0x04 tex_special\blob.bmp */
	struct RowSteam *steamPool;     /* +0x08 */
	struct RowSteam *steam;         /* +0x0c active */
	struct RowSteam *steamFree;     /* +0x10 */
	struct RowQuadSprites *steamQuads;  /* +0x14 */
	struct RowBlob *blobPool;       /* +0x18 */
	struct RowBlob *blobs;          /* +0x1c active */
	struct RowBlob *blobFree;       /* +0x20 */
	struct RowQuadSprites *blobQuads;   /* +0x24 */
};

/* ---------- CRowing_ASEImport.cpp, CRowing_World.cpp */

/* a 2D path read from an ASE shape (0xc bytes): x and the second coordinate of each knot */
struct RowPath {
	int count;                  /* +0x00 knots */
	float *points;              /* +0x04 count x 2 floats */
	char closed;                /* +0x08 */
};

/* the text reader of the ASE import (the CarryOn CoTextReader layout; vtable 0x00276728) */
struct RowAseText {
	const void *const *vtable;  /* +0x00 */
	char *start;                /* +0x04 */
	char *cursor;               /* +0x08 */
	char *end;                  /* +0x0c */
};
struct RowAseFile {
	struct RowAseText text;
};

/* the importer (vtable 0x0027672c; no fields) */
struct RowAseImport {
	const void *const *vtable;  /* +0x00 */
};

/* the reader's open on a buffer (rollerball_b 0x001989c0, thiscall) */
#define RowAseText_Open(text, data, size) \
	RB_CALL(FUN_001989c0, void (*)(struct RowAseText *, void *, unsigned int))((text), (data), (size))

extern const void *const vtable_00276728[];
extern const void *const vtable_0027672c[];

/* functions missing from the function table (defined in this module; reported to the
   architect) */
int FUN_001b3560(int node, void *world);    /* "World" ParseMap callback */
int FUN_001b3a10(int node, void *world);    /* "Instance" ParseMap callback */
int FUN_001b42f0(int node, void *world);    /* "Object" ParseMap callback */

/* ---------- CRowing_Spark.cpp */

/* a spark (0xe4 bytes): a short trail of positions drawn as a beam from its tail to its
   head; colours in 8.7 fixed point */
struct RowSpark {
	int active;                 /* +0x00 */
	short colour[3];            /* +0x04 the tail colour */
	short pad0a;
	short colour2[3];           /* +0x0c the head colour */
	short pad12;
	short fade[3];              /* +0x14 colour steps */
	short pad1a;
	short fade2[3];             /* +0x1c colour2 steps */
	short pad22;
	short age;                  /* +0x24 */
	short life;                 /* +0x26 */
	float trail[9][4];          /* +0x28 newest first */
	int length;                 /* +0xb8 */
	float tail[4];              /* +0xbc */
	float vel[3];               /* +0xcc */
	float pad_d8;               /* +0xd8 */
	float width;                /* +0xdc */
	signed char maxLength;      /* +0xe0 */
	char pad_e1[3];
};

/* the sparks (vtable 0x00275f34; 0x20 bytes) */
struct RowSparks {
	const void *const *vtable;  /* +0x00 */
	float drag;                 /* +0x04 0.97 */
	float gravity;              /* +0x08 -0.2 */
	float spread;               /* +0x0c 45 */
	struct bdVertexBuffer *vb;  /* +0x10 */
	void *texture;              /* +0x14 */
	int count;                  /* +0x18 */
	struct RowSpark *sparks;    /* +0x1c */
};

extern const void *const vtable_00275f34[];

/* ---------- CRowing_Lights.cpp */

/* a light (0x3c bytes): flags 1 on (owns an enabled BLight), 2 off (switched off by the
   game), 4 the BLight has specular colour */
struct RowLight {
	struct BLight *blight;      /* +0x00 */
	int group;                  /* +0x04 */
	int type;                   /* +0x08 0 point, 1 directional */
	unsigned int flags;         /* +0x0c */
	float pos[4];               /* +0x10 position or direction */
	int r, g, b;                /* +0x20 */
	float range;                /* +0x2c */
	void *sprite;               /* +0x30 the glow sprite's texture */
	float spriteSize;           /* +0x34 */
	struct RowLight *next;      /* +0x38 */
};

/* the light manager (0x48 bytes, the one object at 0x0030fc04): at most 8 lights own a
   BLight */
struct RowLights {
	struct RowLight *slots[8];  /* +0x00 */
	struct BLight *blights[8];  /* +0x20 */
	struct RowLight *list;      /* +0x40 */
	int rawRange;               /* +0x44 the D3D attenuation is the range, not its root */
};
#define g_RowLights RW_VAR(struct RowLights *, 0x0030fc04)

/* byte -> float (/255) colour table (bdisplay) */
extern float DAT_002bd220[];

/* ---------- the Rowing game (rowing_a) as seen from rowing_b */

/* 0x0030f0b0  the Rowing game object (rowing_a; static); its first word holds the variant
   flags: bit 0 ?, bit 1 the next game is chosen, bits 2-3 the layout (0 race, 1 arrows and
   the multi-line race, 2 no whirlpools: the tokens, 3 the floating obstacles) */
#define RW_GAME ((void *)RW_ADDR(0x0030f0b0))
#define g_RowVariant RW_VAR(unsigned int, 0x0030f0b0)
#define RW_LAYOUT() ((g_RowVariant >> 2) & 3)
/* 0x0030f0b4  the game state (1 count down, 5 racing, 6 finished) */
#define g_RowState RW_VAR(int, 0x0030f0b4)
/* 0x0030f0b8  frames in this state */
#define g_RowStateFrames RW_VAR(int, 0x0030f0b8)
/* 0x0030f12c  the level (CRowing_World.cpp object, in the game object) */
#define g_RowWorld RW_VAR(unsigned char *, 0x0030f12c)
/* 0x0030f130  the water (classlib_fx water field; vtable slot 0x28 makes a ripple) */
#define g_RowWater RW_VAR(void *, 0x0030f130)
/* 0x0030f138  the sprite list manager (its methods ignore it) */
/* 0x0030f140  the race particles (struct RowParticles *) */
#define g_RowParticles RW_VAR(struct RowParticles *, 0x0030f140)
/* 0x0030f148  the frame timer: +0x98 milliseconds of the frame, +0x9c the frame step */
#define g_RowTimer RW_VAR(unsigned char *, 0x0030f148)
#define RW_FRAME_MS() RB_F32(g_RowTimer, 0x98)
#define RW_FRAME_STEP() RB_F32(g_RowTimer, 0x9c)
/* 0x0030f178  the floating objects (100 struct RowFloaty *) */
#define g_RowFloaties ((unsigned char **)RW_ADDR(0x0030f178))
/* 0x0030f490  the players (rowing_a CRowing_Player *, 4): +0x110 gate states (signed
   chars), +0x2b4 position, +0x304/+0x30c speeds, +0x37c the AI may target it */
#define g_RowPlayers ((unsigned char **)RW_ADDR(0x0030f490))
/* 0x002b5630, 0x002b5634  the river width and centre (x) */
#define g_RowRiverWidth RW_VAR(float, 0x002b5630)
#define g_RowRiverCentre RW_VAR(float, 0x002b5634)

/* a ripple on the water at (x, z) */
static inline void rb_water_ripple(float x, float z, float height, float inner, float outer)
{
	void *water = g_RowWater;

	if (water != NULL)
		RB_VFN(water, 0x28, void (*)(void *, float, float, float, float, float))
			(water, x, z, height, inner, outer);
}

/* rowing_a functions with their shapes read from the x86 code (the table prototypes are
   still Ghidra guesses) */
/* 0x001a6b50  a boat hits a sphere at pos of radius r: 1 when it does */
#define rb_BoatHitsSphere(a, pos, r, b) \
	RB_CALL(FUN_001a6b50, int (*)(void *, float *, float, void *))((a), (pos), (r), (b))
/* 0x001a73c0  registers a collision sphere (type, radius, pos, vel, r, callback, context);
   returns its id */
#define rb_AddCollider(type, radius, pos, vel, r, cb, ctx) \
	RB_CALL(FUN_001a73c0, int (*)(int, float, float *, float *, float, void *, void *)) \
		((type), (radius), (pos), (vel), (r), (void *)(cb), (ctx))
/* 0x001a5990  switches a collision sphere on or off */
#define rb_EnableCollider(id, on) RB_CALL(FUN_001a5990, void (*)(int, int))((id), (on))
/* 0x001a6f20  resets a collision sphere */
#define rb_ResetCollider(id) RB_CALL(FUN_001a6f20, void (*)(int))(id)

/* ---------- CRowing_objects.cpp */

/* a scenery object (vtable 0x00276224; 0x20 bytes; the base of the special objects) */
struct RowObject {
	const void *const *vtable;      /* +0x00 */
	struct RowObject *next;         /* +0x04 the model's instance ring */
	struct RowObject *prev;         /* +0x08 */
	struct BActorInstance *inst;    /* +0x0c */
	int type;                       /* +0x10 the model's type (RowModel_Load) */
	unsigned int passes;            /* +0x14 the draw passes it has */
	unsigned int flags;             /* +0x18 1 active */
	void *anim;                     /* +0x1c its looping animation */
};

/* a model (0x1c bytes) in the ring of models (list head at the world's +0x26c) */
struct RowModel {
	struct RowModel *next;          /* +0x00 */
	struct RowModel *prev;          /* +0x04 */
	unsigned int crc;               /* +0x08 the file name's CRC */
	void *actor;                    /* +0x0c */
	int shared;                     /* +0x10 loaded without a package (not freed) */
	struct RowObject *instances;    /* +0x14 */
	int type;                       /* +0x18 */
};

extern const void *const vtable_00276224[];
extern const void *const vtable_002762c4[];
extern const void *const vtable_0027658c[];
extern const void *const vtable_002765d4[];
extern const void *const vtable_002765ec[];

/* the callbacks of the special objects (missing from the function table) */
int FUN_001b7000(void *self, void *boat, void *a, void *b, float *out);
int FUN_001b71f0(void *self, void *boat, void *a, void *b, float *out);
int FUN_001b72b0(void *self, void *boat, void *a, void *b, float *out);
int FUN_001b7700(int a, int b, int c, int d, void *context);
int FUN_001b8680(void *self, void *boat, void *a, void *b, float *out);

#endif
