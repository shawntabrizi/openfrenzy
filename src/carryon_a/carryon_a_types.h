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

/* ---------- CFire_Carry.cpp */

/* one carried object of one player (0x88 bytes): a model instance shown in the hands */
struct CoCarried
{
	int player;                 /* +0x00 */
	int type;                   /* +0x04 0..4 rockets, 5 glitter net, 6..10 parts, 0xb tapir */
	void *inst;                 /* +0x08 its model instance */
	void *anim;                 /* +0x0c "default" / "default_noloop" */
	int visible;                /* +0x10 in the player's hands */
	void *leftHand;             /* +0x14 the character's left palm node */
	void *rightHand;            /* +0x18 */
	void *handle;               /* +0x1c the "handle" node */
	void *flame;                /* +0x20 tapir: the "flame" node */
	void *vb;                   /* +0x24 net / trail vertex buffer */
	void *streak;               /* +0x28 alphatex\tapir_streak.bmp */
	float *trail;               /* +0x2c tapir: 30 trail points (float[4]) */
	float *trailVel;            /* +0x30 their velocities */
	int trailCount;             /* +0x34 */
	float phase[10];            /* +0x38 glitter net: wave phases */
	float speed[10];            /* +0x60 their speeds */
};
OF_ASSERT_OFFSET(struct CoCarried, trailCount, 0x34);
OF_ASSERT_SIZE(struct CoCarried, 0x88);

/* the carried objects of all players (0x68 bytes; also DAT_002f6d4c) */
struct CFireCarry
{
	int variant;                /* +0x00 */
	void *models[5];            /* +0x04 */
	struct CoCarried *items[4][5];  /* +0x18 per player and type */
};
OF_ASSERT_SIZE(struct CFireCarry, 0x68);

/* ---------- CFire_World.cpp: functions missing from the function table (ParseMap
callbacks; reported to the architect) */
int FUN_000c5610(int node, unsigned char *world);
int FUN_000c59a0(int node, unsigned char *world);
int FUN_000c6c00(int node, unsigned char *world);

/* a vertex of the debug lines, the glitter net and the tapir trail (0x24 bytes, FVF
0x152 / 0x144) */
struct CoVertex
{
	float x, y, z;
	float nx, ny, nz;
	unsigned int colour;
	float u, v;
};
OF_ASSERT_SIZE(struct CoVertex, 0x24);

/* ---------- CFire_Resource.cpp */

/* the resources and frame state of the game (0x3e0 bytes, g_fireRes = DAT_002f6d5c):
packages, fonts, the floor models, the sounds, the fog and the frame timer */
struct CFireResource
{
	void *levelPackage;         /* +0x000 packages\carryon_v%d */
	void *genericPackage;       /* +0x004 packages\carryon_gen */
	void *font;                 /* +0x008 gunship2.ttf */
	void *fontAlt;              /* +0x00c handgotb.ttf (Printf flag 2) */
	void *scoreFont;            /* +0x010 score.ttf */
	int pad014;                 /* +0x014 */
	unsigned char timer[0x234]; /* +0x018 carryon_b CFire_Timer (+0x98 ms, +0x9c frames:
	                               the game reads +0xb0 and +0xb4 of the resource) */
	int pad24c;                 /* +0x24c */
	int variant;                /* +0x250 0..3 */
	int numPlayers;             /* +0x254 */
	int testMesh;               /* +0x258 setting "testmesh" (variant 1 only) */
	float eye[4];               /* +0x25c the camera, copied each frame */
	float target[4];            /* +0x26c */
	void *floorModel[7];        /* +0x27c floor / rack models of the variant */
	void *floorInst[7];         /* +0x298 their instances */
	void *floorAnim[7];         /* +0x2b4 their "default" sub-objects */
	void *potModel[5];          /* +0x2d0 variant 2: the glow pots */
	void *potInst[5];           /* +0x2e4 */
	void **samples;             /* +0x2f8 46 sounds (DAT_002941a0) */
	int firstSample[15];        /* +0x2fc first sound of each kind */
	int numSamples[15];         /* +0x338 sounds of each kind */
	float cellSize;             /* +0x374 route cell size (CFire_Game) */
	int fogOn;                  /* +0x378 */
	int fogColour[3];           /* +0x37c */
	float fogNear;              /* +0x388 */
	float fogFar;               /* +0x38c */
	int frames;                 /* +0x390 */
	void *inputMap;             /* +0x394 */
	void *detailChannel;        /* +0x398 input "Detail": toggles the detail tweak */
	int numReserved;            /* +0x39c */
	int reserved[16];           /* +0x3a0 reserved sound channels */
};
OF_ASSERT_OFFSET(struct CFireResource, variant, 0x250);
OF_ASSERT_OFFSET(struct CFireResource, floorInst, 0x298);
OF_ASSERT_OFFSET(struct CFireResource, samples, 0x2f8);
OF_ASSERT_OFFSET(struct CFireResource, numSamples, 0x338);
OF_ASSERT_OFFSET(struct CFireResource, fogOn, 0x378);
OF_ASSERT_OFFSET(struct CFireResource, reserved, 0x3a0);
OF_ASSERT_SIZE(struct CFireResource, 0x3e0);

/* a sound of the table at 0x002941a0 (46 entries) */
struct CoSoundDef
{
	const char *name;           /* +0x0 sounds\<name>.wav */
	unsigned char looped;       /* +0x4 loopedsounds\<name>.wav */
	unsigned char generic;      /* +0x5 from the generic package */
	signed char kind;           /* +0x6 0..14: CFire_Resource_FindSample's kind */
	unsigned char pad;
};
OF_ASSERT_SIZE(struct CoSoundDef, 0x8);

/* ---------- CFire_Camera.cpp */

/* the camera (0xd4 bytes, g_fireCam = DAT_002f7960). Modes: 0 intro (classlib_ui
CIntroCam), 1 follow the players (variant 2), 2 follow the fireworks (variant 3), 3 the
charlib CCamera (DAT_002f7964; variants 0 and 1), 4/5 the winner at the end. */
struct CFireCamera
{
	int snap;                   /* +0x00 mode 3: blend to the CCamera at once (carryon_b) */
	int mode;                   /* +0x04 */
	int fadeOut;                /* +0x08 the end fade-out runs */
	int frames;                 /* +0x0c frames drawn, up to 3 (snap the first ones) */
	void *inputMap;             /* +0x10 */
	void *skip;                 /* +0x14 input "skip" (+0x14: pressed) */
	int pad18;                  /* +0x18 */
	float zoom;                 /* +0x1c modes 1, 2 */
	float hold;                 /* +0x20 */
	int variant;                /* +0x24 */
	float blend;                /* +0x28 mode 3 */
	float blendTarget;          /* +0x2c */
	int wantFadeIn;             /* +0x30 */
	int wantFadeOut;            /* +0x34 */
	int fadeIn;                 /* +0x38 */
	float volume;               /* +0x3c 0..1: darkening and sound fade-in */
	float endTime;              /* +0x40 frames since the end */
	struct CCarryOnPlayer *winner;  /* +0x44 */
	float fade;                 /* +0x48 */
	int fadeCount;              /* +0x4c */
	float min[4];               /* +0x50 the players' bounds */
	float max[4];               /* +0x60 */
	float centre[4];            /* +0x70 */
	float mean[4];              /* +0x80 */
	float target[4];            /* +0x90 */
	float targetGoal[4];        /* +0xa0 */
	float eye[4];               /* +0xb0 */
	float eyeGoal[4];           /* +0xc0 */
	float phase;                /* +0xd0 mode 2: sway */
};
OF_ASSERT_OFFSET(struct CFireCamera, winner, 0x44);
OF_ASSERT_OFFSET(struct CFireCamera, target, 0x90);
OF_ASSERT_OFFSET(struct CFireCamera, eye, 0xb0);
OF_ASSERT_SIZE(struct CFireCamera, 0xd4);

/* ---------- the conveyor belts (variant 4) */

/* a belt (0x330 bytes, CFireGame.conveyor[]): `count` slots move along a two-point
path; the slots in the `window` are on the visible belt. A slot holds a player's
firework (type = player + 1). */
struct CFireBelt
{
	void *fireworks;            /* +0x000 carryon_b CFire_Fireworks */
	int count;                  /* +0x004 slots */
	int window;                 /* +0x008 */
	int points;                 /* +0x00c score for a launch */
	float pos;                  /* +0x010 0..count */
	float speed;                /* +0x014 slots per frame */
	int types[64];              /* +0x018 0: empty */
	float timers[64];           /* +0x118 a passed firework arrives when it runs out */
	int pending[64];            /* +0x218 its type */
	struct CoAsePath *path;     /* +0x318 the "Convey" path (two points) */
	int pad31c[5];              /* +0x31c */
};
OF_ASSERT_OFFSET(struct CFireBelt, timers, 0x118);
OF_ASSERT_OFFSET(struct CFireBelt, path, 0x318);
OF_ASSERT_SIZE(struct CFireBelt, 0x330);

/* a piece on a fuse / paint path (0x48 bytes). carryon_b owns the object
(struct CFire_ConveyorItem in carryon_b.h, same layout); this module updates and draws
it. */
struct CoConveyorItem
{
	int active;                 /* +0x00 */
	int player;                 /* +0x04 */
	float pos[4];               /* +0x08 the head */
	float pos2[4];              /* +0x18 the tail (starts after `delay`) */
	float delay;                /* +0x28 */
	int point;                  /* +0x2c the head's next path point */
	int point2;                 /* +0x30 the tail's */
	float speed;                /* +0x34 */
	int paint;                  /* +0x38 0: a lit fuse, 1: paint */
	struct CoAsePath *path;     /* +0x3c */
	void *vb;                   /* +0x40 the beam vertex buffer */
	void *texture;              /* +0x44 */
};
OF_ASSERT_SIZE(struct CoConveyorItem, 0x48);

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
