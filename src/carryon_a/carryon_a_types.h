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

/* ---------- CCarryOn_Player.cpp */

/* a player: a CharLib AI player (0x1130 bytes, src/charlib/charlib_ai.h; the AI_* offsets
there name its fields) with the Fireworks state. vtable 0x00267b70. One per player slot in
DAT_002f6d28 (and charlib's AI player table DAT_002f289c). */
struct CCarryOnPlayer
{
	unsigned char ai[0x1130];   /* +0x0000 the CharLib AI player (vtable, +0x04 AI mode,
	                               +0x30 actor, +0x36 AI state, +0x8c route, ...) */
	int fireworks;              /* +0x1130 fireworks (variant 1/3/4) carried, 0..5 */
	int glitter;                /* +0x1134 variant 2: glitter collected, 0..100 */
	float glitterTimer;         /* +0x1138 variant 2 */
	float timer113c;            /* +0x113c */
	float glitterHeld;          /* +0x1140 variant 2: frames since the last glitter */
	int int1144;                /* +0x1144 */
	int score;                  /* +0x1148 */
	int scoreText;              /* +0x114c index of the next score text (0..8) */
	int pad1150[2];             /* +0x1150 */
	int partTypes[5];           /* +0x1158 variant 3: the type of each part carried */
	int hasTarget;              /* +0x116c the AI walks to `target` */
	float target[4];            /* +0x1170 */
	int routeAge;               /* +0x1180 frames since the route was planned */
	void *pickup;               /* +0x1184 variant 2: the glitter pick-up it walks to */
	void *goal;                 /* +0x1188 the launch pad / spawner item it walks to */
	int pad118c[2];             /* +0x118c */
	int hasRoute;               /* +0x1194 */
	int attacker;               /* +0x1198 the last player who hit it (-1) */
	int *station;               /* +0x119c variant 3: its launch station (+4 parts asked, +8 types, +0x20 position) */
	void *belt;                 /* +0x11a0 variant 2/4: the belt item it walks to */
	int altModel;               /* +0x11a4 1: the alternative model instance is shown */
	int chase;                  /* +0x11a8 the player it chases (-1) */
	int chaseFireworks;         /* +0x11ac `fireworks` when the chase started */
	int chaseGlitter;           /* +0x11b0 `glitter` when the chase started */
	int chaseTargetGlitter;     /* +0x11b4 the target's glitter then */
	int wantLaunch;             /* +0x11b8 */
	int glitterFull;            /* +0x11bc */
	int collected;              /* +0x11c0 fireworks picked up */
	int taken;                  /* +0x11c4 fireworks taken from others */
	int sample;                 /* +0x11c8 variant 2: the jar's looping sound channel */
	int volume;                 /* +0x11cc its volume 0..255 */
	int lastGlitter;            /* +0x11d0 */
	void *jarNode;              /* +0x11d4 the node the jar hangs on */
	void *jarObject;            /* +0x11d8 the "glitter" sub-object */
	int jarAnim;                /* +0x11dc */
	float jarPos[4];            /* +0x11e0 */
	void *instance[2];          /* +0x11f0 the actor's two model instances (normal,
	                               alternative); [1] is a 0x170-byte copy */
	int wantType[2];            /* +0x11f8 variant 3: the part types it still needs */
	int pad1200;                /* +0x1200 */
};
OF_ASSERT_OFFSET(struct CCarryOnPlayer, fireworks, 0x1130);
OF_ASSERT_OFFSET(struct CCarryOnPlayer, score, 0x1148);
OF_ASSERT_OFFSET(struct CCarryOnPlayer, partTypes, 0x1158);
OF_ASSERT_OFFSET(struct CCarryOnPlayer, hasRoute, 0x1194);
OF_ASSERT_OFFSET(struct CCarryOnPlayer, chase, 0x11a8);
OF_ASSERT_OFFSET(struct CCarryOnPlayer, sample, 0x11c8);
OF_ASSERT_OFFSET(struct CCarryOnPlayer, instance, 0x11f0);
OF_ASSERT_SIZE(struct CCarryOnPlayer, 0x1204);

/* CharLib AI fields (src/charlib/charlib_ai.h) used here */
#define CP_MODE 0x04            /* int: the AI mode of the derived class (CP_MODE_*) */
#define CP_THINK_TIME 0x20      /* float: frames since the last plan */
#define CP_ACTOR 0x30           /* the actor */
#define CP_STATE 0x36           /* unsigned short: the AI state (7 hurt, 9 knocked out) */
#define CP_STATE_FRAMES 0x3c    /* int */
#define CP_ROUTE 0x8c           /* struct CLRoute */
#define CP_ARRIVED 0x98         /* short: the route ended */
#define CP_ROUTE_NODE 0x1042    /* short: the route's first node, -1: no route (CLRoute +0xfb6) */
#define CP_TARGET_PLAYER 0x1086 /* short */
#define CP_HIT_BY 0x1088        /* short: who hit it last (-1) */
#define CP_HITS 0x108a          /* short[4]: hits taken from each player */

/* the actor (charlib Actor.c) */
#define CA_POS 0xd0             /* float[4] */
#define CA_PLAYER 0x230         /* unsigned short: the player slot */
#define CA_YAW 0x1e8            /* float */
#define CA_PLAYER_OF(p) (CO_U16(CO_PTR(p, CP_ACTOR), CA_PLAYER))

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
