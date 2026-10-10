/*
DEMOLITION_A.H

Module demolition_a: the first part of the Demolition minigame
(C:\BlitzParty\Game\Src\Demolition\, XBE 0x000e7af0-0x000f0b20): the game
entry (CModule_Demolition.cpp), the game object and its four variations
(CDemolition_Game.cpp), the small game subsystems that follow it (sfx,
rumble, platforms, info text, input, countdown, debug, physics), the lights
(CDemolition_Light.cpp), the spot effects, the overlay, the tokens
(CDemolition_Token.cpp) and the explosions (CDemolition_Explosion.cpp).

Owner: demolition_a. Other modules may include this header; only
demolition_a edits it.

Conventions (docs/ARCHITECTURE.md):
- Struct layouts keep the original (ILP32) offsets; offsets are in comments.
- Class and field names are ours (recovered from strings and use); methods
  take an explicit "self".
- The game object (0x7be0 bytes) holds sub-objects of every Demolition
  module. Only the parts this module reads are named; DG_* gives the byte
  offset of each sub-object, and DA_FIELD reads a field at a byte offset.
- Calls to other modules go through the DA_* wrapper macros below, with the
  prototype read from the x86 code (thiscall "this" first, floats as
  floats). Those prototypes are listed in the module report for the owners.
*/

#ifndef DEMOLITION_A_H
#define DEMOLITION_A_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ---------- helpers */

/* the field at byte offset OFF of an object whose layout is not decoded yet */
#define DA_FIELD(type, obj, off) (*(type *)((char *)(obj) + (off)))
/* the address of byte offset OFF of an object */
#define DA_AT(obj, off) ((void *)((char *)(obj) + (off)))

/* virtual calls: the vtable is the first word of the object; OFF is the byte offset of the slot */
#define DA_VTBL(obj) (*(void *const *const *)(obj))
#define DA_VFN(obj, off, fntype) ((fntype)DA_VTBL(obj)[(off) / 4])

/* the C library rand() of the game (bkernel owns the seed DAT_00285630) */
extern unsigned int DAT_00285630;
static inline int da_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}
/* the constant 0x00253740 (1/32767): rand() * DA_RAND_SCALE is 0..1 */
#define DA_RAND_SCALE 3.05185094e-05f

/* a 4-float vector (x, y, z, w) as the engine stores positions */
struct DA_Vec4 {
	float x, y, z, w;
};

/* ---------- the game object (CDemolition_Game, 0x7be0 bytes, vtable 0x00269aac) */

/* 0x002ff85c: the one game object (allocated by CModuleDemolition_Init) */
extern void *DAT_002ff85c;
#define DA_GAME ((char *)DAT_002ff85c)

/* fields of the game object */
#define DG_PACKAGE        0x10   /* bkPackage * "packages\demo-v<n>" */
#define DG_GENPACKAGE     0x14   /* bkPackage * "packages\demo-gen" */
#define DG_PACKAGEID      0x18   /* package id of DG_PACKAGE */
#define DG_GENPACKAGEID   0x1c   /* package id of DG_GENPACKAGE */
#define DG_MODE           0x20   /* the game mode pushed by Start (demolition_c) */
#define DG_PAUSED         0x24   /* char */
#define DG_ROUNDOVER      0x25   /* char: scores are frozen */
#define DG_VARIATION      0x28   /* int 0..3 */
#define DG_STATE          0x2c   /* short: 1 intro, 2 play, 3 results, 4 leave */
#define DG_SAVEDCLIP      0x30   /* 4 floats: DAT_00292c60..6c while the game runs */
#define DG_QUIT           0x40   /* char: leave the game */
#define DG_SAVEDCHARSEL   0x44   /* int: DAT_002bae70 at Initialise */
#define DG_SAVEDFOG       0x48   /* 4 ints: DAT_002ba940..4c */
#define DG_SAVEDFAR       0x58   /* 2 floats: DAT_0028ffa8, DAT_0028ffac */
#define DG_CHARLIB        0x64   /* CDemolition_CharLib (demolition_b) */
#define DG_PLAYERS        0x6c   /* first CDemolition_Player (list through DP_NEXT) */
#define DG_PLAYERBYINDEX  0x70   /* 4 player pointers */
#define DG_CHARCOLOURS    0x10c  /* int: DAT_00283ea4 last applied */
#define DG_WORLD          0x110  /* CDemolition_World (demolition_b) */
#define DG_CAMERA         0x1298 /* the camera (demolition_b) */
#define DG_CAMTARGET      0x12a0 /* vec4 camera target */
#define DG_CAMPOS         0x12b0 /* vec4 camera position */
#define DG_CAMERAMODE     0x12dc /* int: the camera state (1-2 intro, 3 play) */
#define DG_PAUSEA         0x12e8 /* int */
#define DG_PAUSEB         0x12ec /* int */
#define DG_ENDING         0x12f4 /* int */
#define DG_STARTING       0x12f8 /* int */
#define DG_STATETIMER     0x1310 /* timer (rollerball_a CTimer methods) */
#define DG_RESULTSTIME    0x136c /* float */
#define DG_SHAKEX         0x1370 /* float */
#define DG_SHAKEZ         0x1374 /* float */
#define DG_SKYA           0x138c /* float */
#define DG_SKYB           0x1390 /* float */
#define DG_ENTITIES       0x1398 /* entity system (demolition_b) */
#define DG_BOMBS          0x13a0 /* bomb system (demolition_b) */
#define DG_BOMBLIST       0x13e8 /* bomb system's physics list (demolition_b) */
#define DG_EXPLOSIONS     0x13f0 /* struct CDemolition_ExplosionSystem */
#define DG_TOKENS         0x14c8 /* struct CDemolition_TokenSystem */
#define DG_OVERLAY        0x14e4 /* struct CDemolition_Overlay */
#define DG_SPOTFX         0x1510 /* struct CDemolition_SpotFXSystem */
#define DG_TEXTURES       0x6c50 /* struct CDemolition_TextureSystem */
#define DG_LIGHTS         0x6ca4 /* struct CDemolition_LightSystem */
#define DG_SIMULATION     0x77d4 /* simulation (rollerball_a methods) */
#define DG_INPUT          0x77e0 /* struct CDemolition_Input */
#define DG_PLAYERFLAGS    0x77ec /* 4 pointers: the input channels (DG_INPUT + 0xc) */
#define DG_TIMER          0x7800 /* the game clock (rollerball_a methods) */
#define DG_COUNTDOWN      0x78c0 /* struct CDemolition_Countdown */
#define DG_ROUNDTIME      0x789c /* float: the frame step cap */
#define DG_DEBUG          0x78f8 /* struct CDemolition_Debug */
#define DG_INFO           0x7900 /* struct CDemolition_Info */
#define DG_VARIATIONOBJ   0x79a0 /* struct CDemolition_Variation * */
#define DG_PLATFORMS      0x79a4 /* struct CDemolition_PlatformSystem */
#define DG_DIRECT3D       0x79c8 /* direct3D helper (rollerball_a methods) */
#define DG_SFX            0x79d0 /* struct CDemolition_Sfx */
#define DG_RUMBLE         0x7a38 /* struct CDemolition_Rumble */
#define DG_RESPAWN        0x7b1c /* struct CDemolition_Respawn */
#define DG_UNK7B20        0x7b20 /* 0xa8 bytes, vtable 0x0026a4d4 */
#define DG_DECALS         0x7bc8 /* struct CDemolition_ObjectList (decals) */
#define DG_PARTICLES      0x7bd0 /* CParticleSystemList (classlib_fx) */
#define DG_SIZE           0x7be0

/* fields of the players (CDemolition_Player, demolition_b; linked through DP_NEXT) */
#define DP_ACTOR          0x30   /* the character's actor instance (charlib) */
#define DP_NEXT           0x1140 /* next player or 0 */
#define DP_POS            0x1144 /* vec4 */
#define DP_RADIUS         0x1164 /* float */
#define DP_HEIGHT         0x1168 /* float */
#define DP_ALIVE          0x116c /* char */
#define DP_ACTIVE         0x116d /* char */
#define DP_BLOCKED        0x116e /* char */
#define DP_WINNER         0x116f /* char */
#define DP_DONE           0x1171 /* char */
#define DP_CHEERED        0x1172 /* char */
#define DP_CANMOVE        0x1173 /* char */
#define DP_CONTROL        0x1174 /* char */
#define DP_COLOUR         0x1178 /* 3 ints r, g, b */
#define DP_INDEX          0x1184 /* int 0..3 */
#define DP_PAD            0x1188 /* int controller */
#define DP_STATS          0x118c /* stats object */
#define DP_SCORE          0x1194 /* int */
#define DP_HITTIMER       0x1198 /* float: invulnerable while > 0 */
#define DP_BOMB           0x11a0 /* the player's bomb object */
#define DP_PLACE          0x11a4 /* int: finishing place */
#define DP_DETONATOR      0x11a8 /* int */
#define DP_SPECTATE       0x11ac /* char */
#define DP_SHOWNSCORE     0x1208 /* float */
#define DP_PARTNER        0x1234 /* team partner */
#define DP_HAND           0x1238 /* bomb in hand */
#define DP_HANDSLOT       0x1240 /* int */
#define DP_HANDPOS        0x1270 /* vec4 */

#define DA_PLAYER_FIRST() DA_FIELD(char *, DA_GAME, DG_PLAYERS)
#define DA_PLAYER_NEXT(p) DA_FIELD(char *, (p), DP_NEXT)

/* ---------- the variation objects (CDemolition_Variation, vtables 0x00269ad8-0x00269b8c) */

/* vtable of a variation (15 slots) */
struct CDemolition_VariationVtbl {
	int (*init)(void *self, void *package);       /* +0x00 */
	void (*shutdown)(void *self);                  /* +0x04 */
	void (*reset)(void *self);                     /* +0x08 */
	void (*start)(void *self);                     /* +0x0c */
	void (*finish)(void *self);                    /* +0x10 */
	void (*postFinish)(void *self);                /* +0x14 */
	void (*update)(void *self, float dt);          /* +0x18 */
	void (*draw)(void *self);                      /* +0x1c */
	void (*drawOverlay)(void *self);               /* +0x20 */
	void (*begin)(void *self);                     /* +0x24 */
	void (*scores)(void *self);                    /* +0x28 */
	void (*damage)(void *self, float *pos, float radius); /* +0x2c */
	void *(*spawnPoint)(void *self, int player);   /* +0x30 */
	int (*unk34)(void *self);                      /* +0x34 */
	void *(*destroy)(void *self, unsigned int flags); /* +0x38 */
};
#define DA_VARIATION() DA_FIELD(void *, DA_GAME, DG_VARIATIONOBJ)
#define DA_VCALL(obj) (*(const struct CDemolition_VariationVtbl *const *)(obj))

/* ---------- small subsystems of the game object */

/* an object list: vtable, then the first object (each object: vtable, next at +4) */
struct CDemolition_ObjectList {
	const void *const *vtbl;  /* +0x00 */
	void *first;              /* +0x04 */
};

/* +0x77e0: controller input (vtable 0x0026a7e0, 0x20 bytes) */
struct CDemolition_Input {
	const void *const *vtbl;  /* +0x00 */
	void *map;                /* +0x04 BInputMap */
	int unk08;                /* +0x08 */
	void *channels[4];        /* +0x0c one per player */
	int unk1c;                /* +0x1c */
};

/* the countdown / timer (vtable 0x0026a7dc); its other methods are in rollerball_a */
struct CDemolition_Countdown {
	const void *const *vtbl;  /* +0x00 */
	int unk04;                /* +0x04 */
	char running;             /* +0x08 */
	char paused;              /* +0x09 */
	char expired;             /* +0x0a */
	char pad0b[5];
	int unk10;                /* +0x10 */
	int unk14;                /* +0x14 */
	uint32_t freqLo;          /* +0x18 DAT_002cd7a0 (tick frequency, low word) */
	uint32_t freqHi;          /* +0x1c DAT_002cd7a4 */
	int unk20[2];             /* +0x20 */
	int unk28;                /* +0x28 */
	int unk2c;                /* +0x2c */
	int unk30;                /* +0x30 */
	int unk34;                /* +0x34 */
};

/* +0x78f8: the debug text (vtable 0x0026a7d4) */
struct CDemolition_Debug {
	const void *const *vtbl;  /* +0x00 */
	void *font;               /* +0x04 handgotl.ttf */
};

/* +0x7900: the floating info texts (vtable 0x0026a7d0, 0xa0 bytes) */
struct CDemolition_Info {
	const void *const *vtbl;  /* +0x00 */
	int unk04;                /* +0x04 */
	void *font;               /* +0x08 handgotb.ttf */
	int unk0c;                /* +0x0c */
	unsigned char extras[0x80]; /* +0x10 BExtraSystem (bactor) */
	float riseSpeed;          /* +0x90 */
	float life;               /* +0x94 */
	float fadeTime;           /* +0x98 */
	int unk9c;                /* +0x9c */
};

/* +0x79a4: the crates and platforms (vtable 0x0026a7cc, 0x24 bytes) */
struct CDemolition_PlatformSystem {
	const void *const *vtbl;  /* +0x00 */
	void *first;              /* +0x04 first platform (struct CDemolition_Platform) */
	void *models[6];          /* +0x08 crate, crate_coll, ... */
	void *crateAC;            /* +0x20 crate_ac.obe */
};

/* a platform (vtable 0x0026a7b0; subclasses in demolition_d) */
struct CDemolition_Platform {
	const void *const *vtbl;  /* +0x00 */
	void *next;               /* +0x04 */
	float colour[4];          /* +0x08 */
	void *actor;              /* +0x18 */
	char enabled;             /* +0x1c */
	char visible;             /* +0x1d */
	char collides;            /* +0x1e */
	char dirty;               /* +0x1f */
	int unk20[2];             /* +0x20 */
	unsigned char collision[1]; /* +0x28 animating collision (engine_misc) */
};

/* +0x79d0: the sound effects (vtable 0x0026a6e4) */
#define DA_SFX_COUNT 25
struct CDemolition_Sfx {
	const void *const *vtbl;           /* +0x00 */
	void *samples[DA_SFX_COUNT];       /* +0x04 */
};

/* +0x7a38: the controller rumble (vtable 0x0026a4dc) */
struct CDemolition_Rumble {
	const void *const *vtbl;  /* +0x00 */
	int kind;                 /* +0x04 */
	int strength;             /* +0x08 */
	int duration;             /* +0x0c */
	int attack;               /* +0x10 */
	int release;              /* +0x14 */
	int unk18;                /* +0x18 */
	int unk1c;                /* +0x1c */
};

/* +0x14c8: the tokens of variation 2 (vtable 0x0026ab34, 0x1c bytes) */
struct CDemolition_TokenSystem {
	const void *const *vtbl;  /* +0x00 */
	char enabled;             /* +0x04 */
	char stopped;             /* +0x05 */
	char pad06[2];
	int maxBonus;             /* +0x08 */
	int maxTokens;            /* +0x0c */
	int area;                 /* +0x10 player area for the next token (0..3) */
	float areaTimer;          /* +0x14 */
	float bonusTimer;         /* +0x18 */
};

/* +0x14e4: the HUD overlay (vtable 0x0026ab30, 0x2c bytes) */
struct CDemolition_Overlay {
	const void *const *vtbl;  /* +0x00 */
	unsigned char hud[0xc];   /* +0x04 the HUD (classlib_ui CLHud) */
	int scores[4];            /* +0x10 */
	int unk20[3];             /* +0x20 */
};

/* +0x6c50: the textures (vtable 0x0026ab10) */
#define DA_TEXTURE_COUNT 20
struct CDemolition_TextureSystem {
	const void *const *vtbl;              /* +0x00 */
	void *textures[DA_TEXTURE_COUNT];     /* +0x04 */
};

/* ---------- lights (CDemolition_Light.cpp) */

/* a light (base vtable 0x0026a838, 7 slots) */
struct CDemolition_LightVtbl {
	void *(*destroy)(void *self, unsigned int flags); /* +0x00 */
	int (*init)(void *self);                          /* +0x04 */
	void (*shutdown)(void *self);                     /* +0x08 */
	void (*enable)(void *self, int on);               /* +0x0c */
	void (*update)(void *self, float dt);             /* +0x10 */
	void (*draw)(void *self);                         /* +0x14 */
	void (*setGroup)(void *self, int on);             /* +0x18 */
};
struct CDemolition_Light {
	const struct CDemolition_LightVtbl *vtbl; /* +0x00 */
	void *light;              /* +0x04 BLight (bactor) */
	int group;                /* +0x08 */
	struct CDemolition_Light *next; /* +0x0c */
	char on;                  /* +0x10 */
	char dirty;               /* +0x11 */
	char pad12[2];
	struct DA_Vec4 pos;       /* +0x14 position (point lights) */
	float range;              /* +0x24 (point lights) */
	int r, g, b;              /* +0x28 colour */
	/* +0x34: a directional light's direction (vec4); the hazard light's own BLight */
};

/* +0x6ca4: the light system and the spark sprites (vtable 0x0026a8a8, 0xb30 bytes) */
struct DA_Spark {
	struct DA_Vec4 pos;       /* +0x00 */
	int r, g, b;              /* +0x10 */
};
struct CDemolition_LightSystem {
	const void *const *vtbl;  /* +0x00 */
	void *models[3];          /* +0x04 hazbase01/02 ... */
	struct CDemolition_Light *first; /* +0x10 */
	int unk14[4];             /* +0x14 */
	struct CDemolition_Light *main;  /* +0x24 */
	struct DA_Spark sparks[100]; /* +0x28 */
	void *vbA;                /* +0xb18 bdVertexBuffer */
	void *vbB;                /* +0xb1c */
	int sparkCount;           /* +0xb20 */
	int posDirty;             /* +0xb24 */
	int colourDirty;          /* +0xb28 */
	float spin;               /* +0xb2c */
};

/* ---------- spot effects */

/* +0x1510: the spot effects (vtable 0x0026ab1c, 0x5740 bytes): 10 particle bursts (0x7e0) and
   12 smoke/steam puffs (0xb4) */
#define DA_SPOTFX_BURSTS 10
#define DA_SPOTFX_PUFFS 12
#define DA_SPOTFX_BURST_SIZE 0x7e0
#define DA_SPOTFX_PUFF_SIZE 0xb4
#define DA_SPOTFX_BURSTS_AT 0x10
#define DA_SPOTFX_PUFFS_AT 0x4ed0

/* ---------- explosions (CDemolition_Explosion.cpp) */

/* a light flash of an explosion (vtable 0x0026ab84, 0x60 bytes): a point light whose range
   and colour fade with its countdown (demolition_b starts it) */
struct CDemolition_Flash {
	const void *const *vtbl;  /* +0x00 */
	int unk04;                /* +0x04 */
	struct CDemolition_Light *light; /* +0x08 a point light (CDemolition_Light.cpp) */
	int unk0c;                /* +0x0c */
	struct CDemolition_Countdown timer; /* +0x10 */
	char active;              /* +0x48 */
	char pad49[3];
	struct DA_Vec4 pos;       /* +0x4c (written by demolition_b) */
	float age;                /* +0x5c */
};

/* an explosion (vtable 0x0026ab90, 0x790 bytes): four particle systems and a countdown */
struct CDemolition_Explosion {
	const void *const *vtbl;  /* +0x000 */
	int unk04[3];             /* +0x004 */
	struct CDemolition_Explosion *next; /* +0x010 */
	struct DA_Vec4 pos;       /* +0x014 */
	float age;                /* +0x024 */
	char active;              /* +0x028 */
	char visible;             /* +0x029 */
	char pad2a[2];
	unsigned char sparks[0x78];  /* +0x02c DcSparkSystem (demolition_c) */
	unsigned char smoke[0x60];   /* +0x0a4 smoke particles (classlib_fx) */
	unsigned char glow[0x6c];    /* +0x104 glow particles (classlib_fx) */
	unsigned char debris[0x5d0]; /* +0x170 DcDebrisSystem (demolition_c) */
	struct CDemolition_Countdown timer; /* +0x740 */
	float power;              /* +0x778 the flash size */
	void *owner;              /* +0x77c */
	void *source;             /* +0x780 the object that caused it (itself when none) */
	int unk784;               /* +0x784 */
	int unk788[2];            /* +0x788 */
};

/* +0x13f0: the explosion system (vtable 0x0026ab94) */
struct CDemolition_ExplosionSystem {
	const void *const *vtbl;  /* +0x00 */
	int unk04;                /* +0x04 */
	struct CDemolition_Explosion *first; /* +0x08 12 explosions, made by Init */
	int unk0c;                /* +0x0c */
	struct CDemolition_Flash flashes[2]; /* +0x10 */
};

/* a vertex of FVF 0x152 (position, normal, diffuse colour, texture coordinates; 0x24 bytes):
   the skeleton line (0x002ff860) and the explosion flash quad (0x002ff8a8) */
struct DA_LineVertex {
	float x, y, z;       /* +0x00 */
	float nx, ny, nz;    /* +0x0c */
	uint32_t colour;     /* +0x18 */
	float u, v;          /* +0x1c */
};

/* ---------- the physics of the bombs and pieces (demolition_b objects, solved here) */

/* a physics body of the bomb system's list (fields used here; the class is demolition_b's) */
#define PB_NEXT       0x10
#define PB_ACTIVE     0x15
#define PB_HIT        0x19
#define PB_ENABLED    0x1c
#define PB_CONTACTS   0x20   /* flags: 1 world, 2 bodies, 3 platforms */
#define PB_MESHHIT    0x24   /* int[] per world mesh */
#define PB_MESHHITS   0x4c   /* int[] hit count per mesh */
#define PB_MESHDATA   0x74   /* hits per mesh, 0x618 bytes each */
#define PB_BODYCOUNT  0x3d64
#define PB_BODYHIT    0x3d68 /* other bodies hit */
#define PB_BODYDATA   0x3d78 /* 0xd0 bytes each */
#define PB_PLATCOUNT  0x40b8
#define PB_PLATDATA   0x40bc /* 0xf0 bytes each */
#define PB_SPHERE     0x438c /* sphere: vec4 centre (w: radius) */
#define PB_POS        0x43a4 /* vec4 */
#define PB_OLDPOS     0x43b4 /* vec4 */
#define PB_VEL        0x43c4 /* vec4 */
#define PB_NORMAL     0x43e4 /* vec4 last contact normal */
#define PB_BOUNCE     0x4440 /* float */
#define PB_PUSH       0x4444 /* float */
#define PB_FRICTION   0x4448 /* float */
#define PB_AIRTIME    0x444c /* float */
#define PB_KIND       0x4454 /* int */

/* the collision epsilon of the engine's sweep tests (engine_misc owns the value) */
extern float DAT_00285644;

/* ---------- globals of other modules */

extern float DAT_0028fda8;              /* frame time step (core) */
extern int DAT_002bae70;                /* character select entry (core) */
extern int DAT_002bae98;                /* 1: party, 2: tournament (core) */
extern int DAT_002baebc;                /* team game (core) */
extern int DAT_002baed8[];              /* players: 10 ints each (core) */
#define DA_PLAYER_PRESENT(i) DAT_002baed8[(i) * 10]
#define DA_PLAYER_PADBITS(i) DAT_002baed8[(i) * 10 + 2]
#define DA_PLAYER_POINTS(i) DAT_002baed8[(i) * 10 + 4]
extern int DAT_002b9c58[];              /* the four players' characters (core) */
extern int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c; /* fog (core) */
extern float DAT_0028ffa8, DAT_0028ffac;  /* far planes (classlib_ui) */
extern float DAT_00292c60, DAT_00292c64, DAT_00292c68, DAT_00292c6c; /* world clip (charlib) */
extern int DAT_00292c78;                /* (core_modes_b) */
extern int DAT_00283ea4;                /* character colour set (core) */
extern int DAT_00283e58;                /* turbo cheat (core) */
extern int DAT_002ecdec;                /* a controller is missing (core) */
extern void *DAT_002ba9a0;              /* the settings (core) */
extern unsigned char DAT_002ba750[];    /* the game mode stack (core) */
extern unsigned char DAT_002ed324[];    /* the rumble manager (classlib_ui) */
extern unsigned char DAT_002ecd90[];    /* the HUD pause screen (classlib_ui) */
extern unsigned char DAT_002b9968[];    /* the team/character data (core) */
extern void *DAT_002ecfac;              /* the HUD data (classlib_ui) */
extern float DAT_00290be4[4];           /* HUD bar heights (classlib_ui) */
extern uint32_t DAT_002cd7a0, DAT_002cd7a4; /* tick frequency (bkernel) */

/* bdisplay's colour tables: colour = R[r] | G[g] | B[b] | A[a] */
extern uint32_t DAT_002bce18[256]; /* red */
extern uint32_t DAT_002bc490[256]; /* green */
extern uint32_t DAT_002bd640[256]; /* blue */
extern uint32_t DAT_002bc898[256]; /* alpha */
#define DA_RGBA(r, g, b, a) \
	(DAT_002bd640[(b)] | DAT_002bce18[(r)] | DAT_002bc490[(g)] | DAT_002bc898[(a)])

/* ---------- data of this module (Demolition/demolition_a_data.c) */

/* 0x00297510: per variation settings (0x3c bytes each), after the four level names */
struct DA_VariationSettings {
	int light;          /* +0x00 0x00297520 */
	int unk04;          /* +0x04 */
	int unk08;          /* +0x08 */
	int fog;            /* +0x0c 0x0029752c */
	int fogR, fogG, fogB; /* +0x10 */
	float fogStart, fogEnd; /* +0x1c */
	int ambR, ambG, ambB;   /* +0x24 */
	int clearR, clearG, clearB; /* +0x30 -> DAT_002ba940..48 */
};
extern const char *const DAT_00297510[4];               /* level path per variation */
extern struct DA_VariationSettings DAT_00297520[4];
extern const char *const DAT_00297750[DA_SFX_COUNT];  /* sfx\explosion01.wav ... */
extern const char *const DAT_002977b4[6];             /* models\crate.obe ... */
extern const char *const DAT_002977cc[1];             /* models\crate_ac.obe */
extern const char *const DAT_002977d0[3];             /* models\hazbase01.obe ... */
extern struct DA_Vec4 DAT_002977e0[4];                /* main light per variation */
extern const char *const DAT_00297820[DA_TEXTURE_COUNT]; /* spotfx\sprites\... */

/* ---------- functions of other modules, with the prototypes read from the x86 code */

/* core */
#define DA_GameCtor(self, name, a, b, c, d) \
	((void (*)(void *, const char *, const char *, const char *, const char *, const char *))FUN_0002bf30)( \
		(self), (name), (a), (b), (c), (d))
#define DA_GameDtor(self) ((void (*)(void *))FUN_0002bf60)(self)
#define DA_PushMode(mode, resume) ((int (*)(void *, void *, int))FUN_0002b640)(DAT_002ba750, (mode), (resume))
#define DA_PopMode(resume) ((int (*)(void *, int))FUN_0002b6f0)(DAT_002ba750, (resume))
#define DA_SetCharacterColours(actor, set) ((void (*)(void *, int))FUN_0002bb00)((actor), (set))
#define DA_Gain(gain, t) ((float (*)(float, float))FUN_0002bf90)((gain), (t))
/* core_modes_a / core_modes_b */
#define DA_LoadCharacterVoices(flush) ((void (*)(int))FUN_00012a60)(flush)
#define DA_MiniGameIntroCtor(self, game, picture) \
	((void *(*)(void *, int, const char *))FUN_00023440)((self), (game), (picture))
#define DA_TeamData_Reset() ((void (*)(void *))FUN_0001bd70)(DAT_002b9968)

/* bkernel */
#define DA_New(size, file, line) \
	((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)((size), (file), (line), 0x1001)
#define DA_Delete(p) ((void (*)(void *))FUN_0004c110)((void *)(p))
#define DA_Printf ((void (*)(const char *, ...))FUN_0004b230)
#define DA_LoadResource(pkg, name, type, group) \
	((void *(*)(void *, const char *, int, int))FUN_0004bb30)((pkg), (name), (type), (group))
#define DA_DeleteResource(res) ((void (*)(void *))FUN_0004b6e0)(res)
#define DA_LoadPackageResources(pkg, mask, group, tag) \
	((int (*)(void *, unsigned int, int, unsigned int))FUN_0004b870)((pkg), (mask), (group), (tag))
#define DA_DeletePackageResources(id, mask) ((void (*)(unsigned int, unsigned int))FUN_0004b9e0)((id), (mask))
#define DA_LoadFile(pkg, name, buf, size, tags, extra) \
	((void *(*)(void *, const char *, void *, unsigned int *, unsigned int *, int))FUN_0004c6b0)( \
		(pkg), (name), (buf), (size), (tags), (extra))
#define DA_ClosePackage(pkg) ((void (*)(void *))FUN_0004db40)(pkg)
#define DA_PackageId(pkg) ((unsigned int (*)(void *))FUN_0004dce0)(pkg)
#define DA_MatrixTranslation(m, x, y, z) ((void (*)(float *, float, float, float))FUN_00050250)((m), (x), (y), (z))
#define DA_MatrixInverseAffine(out, m) ((int (*)(float *, const float *))FUN_000502f0)((out), (m))
#define DA_VectorNormalise(out, v) ((void (*)(float *, const float *))FUN_000514a0)((out), (v))
#define DA_VectorDistance(a, b) ((float (*)(const float *, const float *))FUN_00051500)((a), (b))
#define DA_RotateY(out, v, angle) ((void (*)(float *, const float *, float))FUN_00051170)((out), (v), (angle))
#define DA_QuatFromAxisAngle(out, aa) ((void (*)(float *, const float *))FUN_00051df0)((out), (aa))
#define DA_InputCreateMap(n) ((void *(*)(int))FUN_000488c0)(n)
#define DA_InputCreateChannel(map, name, type, flags, scale, acc, dec) \
	((void *(*)(void *, const void *, int, unsigned int, int, int, int))FUN_00048a10)( \
		(map), (name), (type), (flags), (scale), (acc), (dec))
#define DA_InputBind(ch, input, other) ((int (*)(void *, unsigned int, unsigned int))FUN_00048b20)((ch), (input), (other))

/* engine_misc */
#define DA_SweepSphereSphere(a, b, vec, hit) \
	((int (*)(void *, void *, float *, void *))FUN_00053af0)((a), (b), (vec), (hit))
#define DA_SweepSphereMesh(sphere, mesh, delta, count, hits) \
	((int (*)(void *, void *, float *, void *, void *))FUN_00053720)((sphere), (mesh), (delta), (count), (hits))
#define DA_CollideAnimating(sphere, coll, delta, count, hits) \
	((int (*)(void *, void *, float *, int *, void *))FUN_00052590)((sphere), (coll), (delta), (count), (hits))
#define DA_UpdateAnimatingCollision(coll) ((void (*)(void *))FUN_000526f0)(coll)
#define DA_SoundStopChannel(ch) ((void (*)(int))FUN_000546e0)(ch)
#define DA_SoundStopAll() ((void (*)(void))FUN_00054730)()
#define DA_Nop1EC260(self) ((void (*)(void *))FUN_001ec260)(self)

/* bactor */
#define DA_DeleteActorInstance(inst) ((void (*)(void *))FUN_00034930)(inst)
#define DA_PrepActorInstance(inst, flags) ((void (*)(void *, int))FUN_00034bf0)((inst), (flags))
#define DA_DrawActorInstance(inst, passes) ((int (*)(void *, int))FUN_00034d50)((inst), (passes))
#define DA_GetNodePosition(inst, node, out) ((void (*)(void *, void *, float *))FUN_00035ce0)((inst), (node), (out))
#define DA_SetBrightness(b) ((void (*)(int))FUN_00035d50)(b)
#define DA_InitActorInstance(inst, actor, slots, file, line) \
	((void *(*)(void *, void *, int, const char *, int))FUN_00036b20)((inst), (actor), (slots), (file), (line))
#define DA_CreateLight(light, flags) ((void *(*)(void *, uint32_t))FUN_000382f0)((light), (flags))
#define DA_EnableLight(light, on) ((void (*)(void *, int))FUN_00038370)((light), (on))
#define DA_SetAmbientLight(r, g, b) ((void (*)(int, int, int))FUN_00038440)((r), (g), (b))
#define DA_SetLightColour(light, r, g, b) ((void (*)(void *, int, int, int))FUN_00038470)((light), (r), (g), (b))
#define DA_DeleteLight(light) ((void (*)(void *))FUN_000384d0)(light)
#define DA_SetDirectionalLight(light, dir) ((void (*)(void *, const float *))FUN_00038550)((light), (dir))
#define DA_SetPointLight(light, pos, range) ((void (*)(void *, const float *, float))FUN_000385a0)((light), (pos), (range))
#define DA_SetSpotLight(light, pos, dir, inner, outer) \
	((void (*)(void *, const float *, const float *, float, float))FUN_000385e0)((light), (pos), (dir), (inner), (outer))
/* baUpdateExtras takes the frame time as raw float bits in its pointer argument (the original
   pushes the float); a float argument would go in a float register on arm64 */
static inline void DA_UpdateExtras(void *sys, float dt)
{
	unsigned int bits;

	__builtin_memcpy(&bits, &dt, sizeof bits);
	baUpdateExtras((struct BExtraSystem *)sys, (void *)(uintptr_t)bits);
}
#define DA_DrawExtras(sys, arg) ((void (*)(void *, void *))FUN_0003c690)((sys), (arg))
#define DA_AddExtra ((void *(*)(void *, void *, double, double, double, int, int, int, const char *))FUN_0003c920)

/* bdisplay */
#define DA_DrawPrimitive(type, verts, count, fvf) ((void (*)(int, void *, int, int))FUN_0003f3f0)((type), (verts), (count), (fvf))
#define DA_VBCreate(out, n, fvf, flags, bufs) ((int (*)(void *, int, int, int, int))FUN_0003f500)((out), (n), (fvf), (flags), (bufs))
#define DA_VBDestroy(vb) ((void (*)(void *))FUN_0003f650)(vb)
#define DA_VBLock(vb, flags) ((void *(*)(void *, unsigned int))FUN_0003f710)((vb), (flags))
#define DA_VBUnlock(vb) ((int (*)(void *))FUN_0003f790)(vb)
#define DA_DrawPointSprites(vb, start, count, size) ((void (*)(void *, int, int, float))FUN_000406d0)((vb), (start), (count), (size))
#define DA_DrawSprite3D(pos, w, h, angle, tex, fu, fv, r, g, b, a, verts, zbias) \
	((int (*)(float *, float, float, float, void *, int, int, int, int, int, int, float *, float))FUN_00040750)( \
		(pos), (w), (h), (angle), (tex), (fu), (fv), (r), (g), (b), (a), (verts), (zbias))
#define DA_SetTexture(stage, tex) ((int (*)(int, void *))FUN_000416c0)((stage), (tex))
#define DA_SetWorldIdentity() ((void (*)(void))FUN_000438b0)()
#define DA_SetWorldMatrix(m) ((void (*)(float *))FUN_00043930)(m)
#define DA_SetProjectionMode(mode) ((void (*)(int))FUN_00043b70)(mode)
#define DA_SetDepthOffset(o) ((void (*)(float))FUN_00044550)(o)
#define DA_SetNearFar(n, f) ((void (*)(float, float))FUN_00044810)((n), (f))
#define DA_SetRenderState(state, value, extra) ((void (*)(int, unsigned int, unsigned int))FUN_00044b00)((state), (value), (extra))
#define DA_SetFogRange(start, end) ((void (*)(float, float))FUN_00044dd0)((start), (end))
#define DA_SetFogColour(r, g, b) ((void (*)(unsigned int, unsigned int, unsigned int))FUN_00044e20)((r), (g), (b))
#define DA_PushRenderState(state, value, extra) ((void (*)(int, unsigned int, unsigned int))FUN_00046cb0)((state), (value), (extra))
#define DA_PopRenderState(state) ((void (*)(int))FUN_00046d00)(state)

/* classlib_ui */
#define DA_AngleDelta(a, b) ((float (*)(float, float))FUN_000703d0)((a), (b))
#define DA_WrapAngle(a) ((float (*)(float))FUN_000703a0)(a)
#define DA_ListInit(list) ((void (*)(void *))FUN_00071050)(list)
#define DA_ListRemove(list, node) ((void *(*)(void *, void *))FUN_000710b0)((list), (node))
#define DA_ListNext(list, node) ((void *(*)(void *, void *))FUN_00071110)((list), (node))
#define DA_CommentaryIdle() ((int (*)(void))FUN_000716f0)()
#define DA_PlayCommentary(kind, choice, vol, flags, rep, pos) \
	((int (*)(int, int, int, unsigned int, int, float *))FUN_000719a0)((kind), (choice), (vol), (flags), (rep), (pos))
#define DA_AddPackage(name, cutscene) ((void *(*)(const char *, int))FUN_00071f90)((name), (cutscene))
#define DA_OverlayDrawAll(noz) ((void (*)(int))FUN_00073900)(noz)
#define DA_OverlayDeleteAll() ((void (*)(void))FUN_00074ba0)()
#define DA_OverlayUpdateAll() ((void (*)(void))FUN_00074be0)()
#define DA_HudCanPause(hud) ((int (*)(void *))FUN_000772e0)(hud)
#define DA_RumbleStopAll() ((void (*)(void *))FUN_00080250)(DAT_002ed324)
#define DA_RumbleModify(player, kind, params) ((int (*)(void *, int, int, void *))FUN_000806d0)(DAT_002ed324, (player), (kind), (params))
#define DA_RumbleCreate() ((int (*)(void *))FUN_00080a20)(DAT_002ed324)
#define DA_RumbleShutdown() ((void (*)(void *))FUN_00080b60)(DAT_002ed324)
#define DA_DebrisStart() ((void (*)(void))FUN_00080bb0)()

/* the HUD (classlib_ui CLHud, the object at DAT_002ecfac) */
#define DA_HudInit(self) ((void (*)(void *))FUN_00072720)(self)
#define DA_HudSetScores(scores) ((void (*)(void *, int *))FUN_00078960)(DAT_002ecfac, (scores))
#define DA_HudAddSeconds(dt) ((void (*)(void *, float))FUN_00078c70)(DAT_002ecfac, (dt))
#define DA_HudSetClock(seconds, sound) ((void (*)(void *, float, int))FUN_000792f0)(DAT_002ecfac, (seconds), (sound))
#define DA_HudCreateClock(seconds) ((void (*)(void *, float))FUN_00079490)(DAT_002ecfac, (seconds))
#define DA_HudCreateScoreBoxes() ((void (*)(void *))FUN_000796c0)(DAT_002ecfac)
#define DA_HudCreateBars() ((void (*)(void *))FUN_0007a080)(DAT_002ecfac)
#define DA_HudCreateScoreTexts(scores) ((void (*)(void *, int *))FUN_0007a200)(DAT_002ecfac, (scores))
#define DA_HudCreateWinnerText(scores, type) ((int (*)(void *, int *, int))FUN_0007ad50)(DAT_002ecfac, (scores), (type))
#define DA_HudUpdateBars() ((void (*)(void *))FUN_0007bd80)(DAT_002ecfac)
#define DA_HudInitBarGeometry() ((void (*)(void *))FUN_0007bfd0)(DAT_002ecfac)
#define DA_HudSetBarLevels(a, b, c, d, scale) \
	((void (*)(void *, float, float, float, float, float))FUN_0007c080)(DAT_002ecfac, (a), (b), (c), (d), (scale))

/* bumpercars_a / bumpercars_b */
#define DA_DebrisInit(ground, n) ((void (*)(void *, int))FUN_000827f0)((ground), (n))
#define DA_SettingsGet(key) ((char *(*)(void *, const char *))FUN_00082870)(DAT_002ba9a0, (key))
#define DA_Return1() ((int (*)(void))FUN_000a6fa0)()
#define DA_TimerFraction(t) ((float (*)(void *))FUN_000965f0)(t)
#define DA_InfoSetFont(self, size) ((void (*)(void *, int))FUN_000a74d0)((self), (size))

/* classlib_fx */
#define DA_ProjectorReset() ((void (*)(void))FUN_00083220)()
#define DA_ProjectorUpdateAll() ((void (*)(void))FUN_000845d0)()
#define DA_ProjectorDeleteAll() ((void (*)(void))FUN_00084620)()
#define DA_RefBotCreate() ((void (*)(void))FUN_00084a30)()
#define DA_RefBotUpdate() ((void (*)(void))FUN_00084e90)()
#define DA_RefBotDelete() ((void (*)(void))FUN_00084ee0)()
#define DA_PSSetTexture(ps, tex) ((void (*)(void *, void *))FUN_000a8ab0)((ps), (tex))
#define DA_PSListCtor(list) ((void (*)(void *))FUN_000a8ac0)(list)
#define DA_PSListDtor(list) ((void (*)(void *))FUN_000a8ad0)(list)
#define DA_SmokeCtor(s) ((void (*)(void *))FUN_000a8df0)(s)
#define DA_SmokeDtor(s) ((void (*)(void *))FUN_000a8e00)(s)
#define DA_GlowCtor(s) ((void (*)(void *))FUN_000a8fa0)(s)
#define DA_GlowDtor(s) ((void (*)(void *))FUN_000a8fb0)(s)
#define DA_PSSetPosition(ps, pos) ((void (*)(void *, float *))FUN_000a9550)((ps), (pos))
#define DA_PSListDraw(list) ((void (*)(void *))FUN_000a8ae0)(list)
#define DA_PSListDeleteAll(list) ((void (*)(void *))FUN_000a9590)(list)
#define DA_SmokeInit(s, params, n) ((int (*)(void *, void *, int))FUN_000aa750)((s), (params), (n))
#define DA_GlowInit(s, params, n) ((int (*)(void *, void *, int))FUN_000aab30)((s), (params), (n))
#define DA_TokenDrawGlowAll() ((void (*)(void))FUN_0010c420)()
#define DA_TokenInit(self, pos, type, size, up) ((void *(*)(void *, float *, int, float, float *))FUN_0010c690)((self), (pos), (type), (size), (up))
#define DA_TokenCollect(self) ((void (*)(void *))FUN_0010cbb0)(self)
#define DA_TokenDrawShadowAll() ((void (*)(void))FUN_0010cc30)()

/* charlib */
#define DA_RouteSetHeights(h) ((void (*)(void *))FUN_000ad5c0)(h)
#define DA_RouteHeightsCtor(self) ((void *(*)(void *))FUN_000ad7d0)(self)
#define DA_RouteHeightsDtor(self) ((void (*)(void *))FUN_000ad810)(self)
#define DA_RouteRebuild() ((void (*)(void))FUN_000ad980)()
#define DA_PlayAnim(actor, index, loop, queue, speed, blend, sync) \
	((void (*)(void *, int, int, int, float, float, int))FUN_000af690)((actor), (index), (loop), (queue), (speed), (blend), (sync))
#define DA_OverlayInit(self, pkg) ((int (*)(void *, void *))FUN_000b06c0)((self), (pkg))
#define DA_GroundProbe(pos, radius, tri, platform, mode) \
	((float (*)(float *, float, void **, void **, int))FUN_000b3ff0)((pos), (radius), (tri), (platform), (mode))
#define DA_ActorUpdateMatrix(actor) ((void (*)(void *))FUN_000b90f0)(actor)
#define DA_CharLibUpdateAll() ((void (*)(void))FUN_000b9ae0)()
#define DA_CharLibDeleteAll() ((void (*)(void))FUN_000b9b80)()

#define DA_DrawShadows(arg) ((void (*)(void *))FUN_000bef00)(arg)
#define DA_DrawShadowsEnd() ((void (*)(void))FUN_000beed0)()

/* carryon_a */
#define DA_FireShowPlayer(player, show) ((void (*)(int, int))FUN_000be5d0)((player), (show))

/* rollerball_a: the shared timer, simulation, decal and light helper classes */
#define DA_TimerSet(t, seconds) ((void (*)(void *, float))FUN_0018ea10)((t), (seconds))
#define DA_TimerStart(t) ((void (*)(void *))FUN_0018ea30)(t)
#define DA_TimerGet(t) ((float (*)(void *))FUN_0018ea50)(t)
#define DA_TimerStop(t) ((void (*)(void *))FUN_0018ea60)(t)
#define DA_TimerUpdate(t) ((void (*)(void *))FUN_0018e8e0)(t)
#define DA_TimerReset(t) ((void (*)(void *))FUN_0018e880)(t)
#define DA_ClockTick(t) ((void (*)(void *))FUN_0018e770)(t)
#define DA_SimulationInit(s) ((int (*)(void *))FUN_0018ec50)(s)
#define DA_TimerInit(t) ((int (*)(void *))FUN_0018e6f0)(t)
#define DA_Direct3DInit(d) ((int (*)(void *))FUN_0018e510)(d)
#define DA_DecalsInit(d) ((int (*)(void *))FUN_0018e000)(d)
#define DA_PSListAdd(list, ps) ((void (*)(void *, void *))FUN_0018e010)((list), (ps))
#define DA_InfoShutdown(i) ((void (*)(void *))FUN_0018dc50)(i)
#define DA_InputShutdown(i) ((void (*)(void *))FUN_0018eb70)(i)
#define DA_PlaySfx(sfx, sample, vol, pan, pos, flags) \
	((void (*)(void *, int, int, int, void *, int))FUN_0018e450)((sfx), (sample), (vol), (pan), (pos), (flags))
/* 0x0018e540 is a thiscall whose ECX (the Direct3D helper) is unused: only the alpha is passed */
#define DA_SetFactorAlphaStage(alpha) FUN_0018e540(alpha)
#define DA_LightSetColour(l, r, g, b) ((void (*)(void *, int, int, int))FUN_0018fa10)((l), (r), (g), (b))
#define DA_LightSetRange(l, range) ((void (*)(void *, float))FUN_0018fa30)((l), (range))

/* rollerball_b, scratchmatch, tankedup_b */
#define DA_ASEParse(ase, text, size) ((void (*)(void *, void *, unsigned int))FUN_001989c0)((ase), (text), (size))
#define DA_CameraShutdown(c) ((void (*)(void *))FUN_001bb790)(c)
#define DA_Nop(self) ((void (*)(void *))FUN_001d5950)(self)

/* demolition_b (CDemolition_CharLib, _World, _Player, _Bomb, camera, entities) */
#define DA_BombFrag(bomb, source, owner) ((void (*)(void *, void *, void *))FUN_000f2570)((bomb), (source), (owner))
#define DA_BombRemote(bomb, source, owner) ((void (*)(void *, void *, void *))FUN_000f2c20)((bomb), (source), (owner))
#define DA_BombMine(bomb) ((void (*)(void *))FUN_000f31b0)(bomb)
#define DA_BombKnock(bomb, source, owner) ((void (*)(void *, void *, void *))FUN_000f39a0)((bomb), (source), (owner))
#define DA_BodyPreStep(body, dt) ((void (*)(void *, float))FUN_000f54c0)((body), (dt))
#define DA_BodyStep(body, dt) ((void (*)(void *, float))FUN_000f5370)((body), (dt))
#define DA_BodyPostStep(body) ((void (*)(void *))FUN_000f53c0)(body)
#define DA_CameraSetMode(cam, mode) ((void (*)(void *, int))FUN_000f5760)((cam), (mode))
#define DA_CameraPause(cam, speed) ((void (*)(void *, float))FUN_000f5780)((cam), (speed))
#define DA_CharLibSetFrozen(cl, frozen) ((void (*)(void *, int))FUN_000f8c60)((cl), (frozen))
#define DA_CharLibUpdateInput(cl) ((void (*)(void *))FUN_000f8c30)(cl)
#define DA_PlayerDropBomb(p) ((void (*)(void *))FUN_000f9a00)(p)
#define DA_PlayerCelebrate(p) ((void (*)(void *))FUN_000f9a90)(p)
#define DA_PlayerPlaceBomb(p, bomb, hand) ((void (*)(void *, void *, int))FUN_000f99e0)((p), (bomb), (hand))
#define DA_PlayerHold(p, actorBomb, hand, pos, slot) \
	((void (*)(void *, void *, void *, void *, int))FUN_000f9a40)((p), (actorBomb), (hand), (pos), (slot))
#define DA_PlayerSetCharacter(p, data) ((void (*)(void *, void *))FUN_000fa7e0)((p), (data))
#define DA_BombDisarm(bomb) ((void (*)(void *))FUN_000f10d0)(bomb)
#define DA_BombsStart(b) ((void (*)(void *))FUN_000f1bf0)(b)
#define DA_BombsStop(b) ((void (*)(void *))FUN_000f1c00)(b)

/* demolition_c (variation worlds, ASE paths, particles) */
#define DA_ASECtor(a) ((void (*)(void *))FUN_001025a0)(a)
#define DA_ASEDtor(a) ((void (*)(void *))FUN_001025b0)(a)
#define DA_ASEListCtor(l) ((void (*)(void *))FUN_001023f0)(l)
#define DA_ASEListDtor(l) ((void (*)(void *))FUN_00102410)(l)
#define DA_ASEFind(list, ase, name) ((void (*)(void *, void *, const char *))FUN_00102440)((list), (ase), (name))
#define DA_RegionCtor(r) ((void *(*)(void *))FUN_00101aa0)(r)
#define DA_RegionBuild(r, points, count) ((void (*)(void *, void *, int))FUN_00102000)((r), (points), (count))
#define DA_RegionContains(r, pos) ((int (*)(void *, float *))FUN_00101ac0)((r), (pos))

/* demolition_d (crates, walls, events) */
#define DA_ContactsCtor(c) ((void (*)(void *))FUN_0010bd10)(c)
#define DA_ContactsDtor(c) ((void (*)(void *))FUN_0010bd20)(c)
#define DA_ContactAdd(body, contact) ((void (*)(void *, void *))FUN_0010bea0)((body), (contact))
#define DA_EventsReset() ((void (*)(void))FUN_0010c080)()
#define DA_EventsCreate(a, b) ((void (*)(int, int))FUN_0010c190)((a), (b))
#define DA_TokenNext(tok) ((void *(*)(void *))FUN_0010c200)(tok)
#define DA_TokensUpdate() ((void (*)(void))FUN_0010ce90)()
#define DA_HitCtor(h) ((void (*)(void *))FUN_0010cec0)(h)
#define DA_HitDtor(h) ((void (*)(void *))FUN_0010ced0)(h)
#define DA_HitSend(system, h) ((void (*)(int, int))FUN_0010d050)((int)(system), (int)(h))


/* ---------- functions of this module missing from the function table (docs/ARCHITECTURE.md
   section 11): defined here under their FUN_ names, which other modules call */

void FUN_000e8010(void *game, int active);    /* CDemolition_Game: mode lost focus */
void FUN_000e8400(void *self, float dt);      /* variation 0: play state */
void FUN_000e9870(void *game);                /* CDemolition_Game::Reset */
void FUN_000e9920(void *game, int active);    /* CDemolition_Game::PollPreRender */
void FUN_000e9ac0(void *game, int active);    /* CDemolition_Game::Render */
void FUN_000ee120(void *self);                /* the light system's spark sprites */
#define CDemolitionGame_LostFocus FUN_000e8010
#define CDemolitionGame_Reset FUN_000e9870
#define CDemolitionGame_PollPreRender FUN_000e9920
#define CDemolitionGame_Render FUN_000e9ac0

#endif
