/*
CARRYON_A_TYPES.H

Object layouts of the carryon_a module (original offsets, ILP32). Included by
carryon_a.h. Other modules may include it; only carryon_a edits it.
*/

#ifndef CARRYON_A_TYPES_H
#define CARRYON_A_TYPES_H

/* ---------- CFire_Game.cpp */

/* the game: one object in .data at 0x00293ad8 (CarryOn/carryon_a_data.c). charlib's
CModule_CarryOn sets the variant (CFire_Game_SetVariant), the carryon_b game mode calls
CFire_Game_BeginLoad, _InitOnce, _InitObjects, _Start, _Update, _Draw and _Shutdown. */
struct CFireGame
{
	const void *const *vtable;  /* +0x000 0x00267468 */
	unsigned int flags;         /* +0x004 bits 1-2: variant 0..3 (CO_VARIANT) */
	int pad008;                 /* +0x008 */
	void *mode;                 /* +0x00c the CarryOn game mode (carryon_b 0x000d2290) */
	void *level;                /* +0x010 the level data (0x23c8 bytes, CFire_World_Load) */
	void *fireworks;            /* +0x014 carryon_b fireworks (0x3c bytes, 0x000cf200) */
	void *launcher;             /* +0x018 variant 1: launcher (carryon_b, 0x130 bytes) */
	void *conveyor[2];          /* +0x01c variant 4: conveyor belts (0x330 bytes) */
	void *paint;                /* +0x024 variant 4: paint (carryon_b, 0x24 bytes) */
	void *spawner;              /* +0x028 carryon_b (0x1c bytes, 0x000ce110) */
	void *obj02c;               /* +0x02c carryon_b (0x000d1a80 draws it) */
	void *sky;                  /* +0x030 carryon_b (0x5c bytes, 0x000d18a0) */
	void *glitter;              /* +0x034 variant 2: glitter (carryon_b, 0x128 bytes) */
	int pad038[5];              /* +0x038 */
	int int04c;                 /* +0x04c */
	int frame;                  /* +0x050 frames since the start (DAT_00293b28) */
	float timeLeft;             /* +0x054 */
	float endTime;              /* +0x058 frames since the end */
	float savedBounds[4];       /* +0x05c route grid bounds of the previous game */
	int state;                  /* +0x06c CO_STATE_* (DAT_00293b44) */
	struct CTweakMenu *tweaks;  /* +0x070 "FireTweaks" */
	int pad074;                 /* +0x074 */
	int *overlays;              /* +0x078 a ClassLib list (0xc bytes) */
	void *rampTex[4];           /* +0x07c variant 3: alphatex\p%dramp_anim.gif */
	float rampFrame;            /* +0x08c */
	void *animTex[8];           /* +0x090 animated textures (CFire_Game_AddAnimTexture) */
	float animSpeed[8];         /* +0x0b0 frames per frame */
	float animFrame[8];         /* +0x0d0 */
	unsigned int animFrames[8]; /* +0x0f0 */
	int numAnim;                /* +0x110 */
	void **projector;           /* +0x114 0x90 bytes: [0] a bactor extra system, +4 a float */
	void *fuses;                /* +0x118 variant 4: paths\fuses.ase node "Fuse" */
	int numFuses;               /* +0x11c */
	void *convey;               /* +0x120 node "Convey" */
	int numConvey;              /* +0x124 */
	void *paintPaths;           /* +0x128 node "Paint" */
	int numPaintPaths;          /* +0x12c */
	int ended;                  /* +0x130 2 once the time ran out */
	int scores[4];              /* +0x134 */
	float barMax;               /* +0x144 the HUD bars' full value */
	float barGrow;              /* +0x148 */
	int winnerShown;            /* +0x14c */
	void *carry;                /* +0x150 CFire_Carry (0x68 bytes) */
	int pad154[29];             /* +0x154 */
	void *collision;            /* +0x1c8 the collision actor (map\collide.obe) */
	void *panels[4];            /* +0x1cc variant 1/4: player panels (overlays) */
	int order[4];               /* +0x1dc players by rank */
	int shuffle[4];             /* +0x1ec players in random order */
	int spawn[4];               /* +0x1fc spawn point of each player */
	int rumble;                 /* +0x20c the rumble setting it was set up for */
	void *lights[3];            /* +0x210 carryon_b 0x000d10d0 */
	float savedNear;            /* +0x21c */
	float savedFar;             /* +0x220 */
};
OF_ASSERT_OFFSET(struct CFireGame, frame, 0x50);
OF_ASSERT_OFFSET(struct CFireGame, state, 0x6c);
OF_ASSERT_OFFSET(struct CFireGame, numAnim, 0x110);
OF_ASSERT_OFFSET(struct CFireGame, carry, 0x150);
OF_ASSERT_OFFSET(struct CFireGame, collision, 0x1c8);
OF_ASSERT_OFFSET(struct CFireGame, spawn, 0x1fc);
OF_ASSERT_SIZE(struct CFireGame, 0x224);

/* the variant 0..3 ("Let's do Launch", "Up the glitter", "Firework Dismay",
"Carnival Knowledge") */
#define CO_VARIANT(game) (((game)->flags >> 1) & 3)

/* CFireGame.state */
#define CO_STATE_LOADING 0
#define CO_STATE_LOADED 1
#define CO_STATE_INTRO 2
#define CO_STATE_COUNTDOWN 3
#define CO_STATE_PLAY 4
#define CO_STATE_END 5

/* character model table at 0x00293a68 (6 characters, 0x10 bytes each) */
struct CoCharModel
{
	const char *name;           /* +0x0 */
	void *actor;                /* +0x4 loaded from the generic package */
	const char *animName;       /* +0x8 */
	void *anim;                 /* +0xc */
};
OF_ASSERT_SIZE(struct CoCharModel, 0x10);

/* ---------- the text reader used by the ASE import (vtable 0x00267bb8; other minigames
use it too: rollerball_b 0x001989c0 sets it up on a file buffer) */
struct CoTextReader
{
	const void *const *vtable;  /* +0x0 */
	char *start;                /* +0x4 */
	char *cursor;               /* +0x8 */
	char *end;                  /* +0xc */
};
OF_ASSERT_SIZE(struct CoTextReader, 0x10);

/* ---------- CFire_ASEImport.cpp: no data, only a vtable (0x00267bbc) */
struct CFireASEImport
{
	const void *const *vtable;  /* +0x0 */
};

/* a point read from an ASE shape (0xc bytes): the vertices and whether it is closed */
struct CoAsePath
{
	float *points;              /* +0x0 float[4] per vertex */
	int numPoints;              /* +0x4 */
	unsigned char closed;       /* +0x8 */
	unsigned char pad[3];
};
OF_ASSERT_SIZE(struct CoAsePath, 0xc);

#endif
