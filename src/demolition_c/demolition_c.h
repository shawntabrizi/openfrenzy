/*
demolition_c.h - the demolition_c module (Minigame Demolition, part 3 of 4: 0x000fcb70-0x00105970)

Original sources: C:\BlitzParty\Game\Src\Demolition\
  CDemolition_Lift.cpp       variation 1: the lift (cogs, worm shafts, the scrolling shaft,
                             the passing girders, the control box and its sparks)
  CDemolition_GasJets.cpp    variation 2: the gas jets (23 vents in 4 groups)
  CDemolition_LiftShaft.cpp  variation 3: the lift shaft (falling lift, platforms, brakes)
  CDemolition_ASEImport.cpp  a reader for 3D Studio ASE shapes (SHAPE_VERTEX_KNOT)
  CDemolition_Particle.cpp   the particle systems of the game (7 classes on the ClassLib
                             CParticleSystem.cpp classes, see classlib_fx.h)
  (unplaced)                 gap functions; most belong to the files above (file inferred)
  CGameMode_Demolition       the game mode (vtable 0x0026b96c, ctor 0x001019c0): thunks into
                             the game object

The game object (CDemolition_Game, demolition_a) is the global DAT_002ff85c. Each variation
object (allocated by CDemolition_Game::Initialise, 0x000eaf80) holds one of the classes here:
  variation 1  0x2340 bytes, vtable 0x00269b14, CDemolition_Lift at +0x10
  variation 2  0x02a0 bytes, vtable 0x00269b50, CDemolition_GasJets at +0x18
  variation 3  0x16d0 bytes, ctor 0x000e9140, CDemolition_LiftShaft at +0x40

Layouts are those of the original 32-bit game (docs/ARCHITECTURE.md, section 3). Objects of
other modules are reached through byte offsets (DC_I32(p, 0x10)).

Owner: demolition_c. Other modules may include this header; only demolition_c edits it.
*/

#ifndef DEMOLITION_C_H
#define DEMOLITION_C_H

#include <string.h>

/* ---------- raw field access (original offsets) */
#define DC_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define DC_I32(p, off) DC_AT(p, off, int)
#define DC_U32(p, off) DC_AT(p, off, unsigned int)
#define DC_F32(p, off) DC_AT(p, off, float)
#define DC_I16(p, off) DC_AT(p, off, short)
#define DC_U16(p, off) DC_AT(p, off, unsigned short)
#define DC_U8(p, off) DC_AT(p, off, unsigned char)
#define DC_PTR(p, off) DC_AT(p, off, void *)
#define DC_FP(p, off) ((float *)((char *)(p) + (off)))
#define DC_ADDR(p, off) ((void *)((char *)(p) + (off)))

/* virtual function at byte offset `off` of the object's vtable */
#define DC_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int dc_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

/* a float stored in an undefined4 */
static inline float dc_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* Call `fn` with the table prototype while that is still `table_type`, else with the real
   prototype. Callers compile against of_decls.h, whose prototype changes when the owner fixes
   it (a float parameter or result); this keeps both cases correct (as demolition_b does). */
#define DC_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn)) table_args, ((real_type)(fn)) real_args)

/* the engine's rand() (MSVC's, inlined by the game; seed DAT_00285630, shared) */
extern unsigned int DAT_00285630;
static inline int dc_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- globals of other modules (weak fallbacks in Demolition/demolition_c_globals.c) */

/* 0x002ff85c  the Demolition game object (CDemolition_Game; demolition_a) */
extern char *DAT_002ff85c;
/* 0x0028fda8  frame time factor (1.0 at 60 Hz) */
extern float DAT_0028fda8;

/* ---------- functions missing from src/include/of_functions.tsv (Ghidra merged them into
   their neighbours); defined in this module or weak fallbacks here, see the final report */

/* 0x000fcb30 / 0x000fcb50  constructor / destructor of a pair of lift girder strips
   (2 x 0x1c4, the demolition_d class with vtable 0x0026bef0); demolition_c */
void *FUN_000fcb30(void *self);
void FUN_000fcb50(void *self);
/* 0x0010db80 / 0x0010db90  constructor / destructor of one girder strip (sets vtable
   0x0026bef0); demolition_d range, weak fallbacks in demolition_c */
void *FUN_0010db80(void *self);
void FUN_0010db90(void *self);
/* CDemolition_Game methods (demolition_a range) reached by the game mode thunks:
   0x000e8010 Input(active), 0x000e9870 (no argument), 0x000e9920 Update(active) (prints
   "CDemolition_Game::PollPreRender: Unhandled game state!"), 0x000e9ac0 Render(active) */
void FUN_000e8010(void *game, int active);
void FUN_000e9870(void *game);
void FUN_000e9920(void *game, int active);
void FUN_000e9ac0(void *game, int active);

/* ======================================================================== shapes

A closed outline in the XZ plane (vtable 0x0026b9a0, 0x40 bytes), read from an ASE file by
CDemolition_ASEShape; used for the areas of the arena (dust, bombs, players). */
struct DcShape {
	const void *const *vtable;  /* 0x00 vtable 0x0026b9a0 */
	float min[4];               /* 0x04 */
	float max[4];               /* 0x14 */
	float centre[4];            /* 0x24 */
	float radius;               /* 0x34 of the XZ bounding box */
	float (*points)[4];         /* 0x38 not owned */
	int count;                  /* 0x3c */
};
OF_ASSERT_OFFSET(struct DcShape, centre, 0x24);
OF_ASSERT_SIZE(struct DcShape, 0x40);

/* the points of a shape in an ASE file (vtable 0x0026b9a8, 0x0c bytes) */
struct DcASEShape {
	const void *const *vtable;  /* 0x0 vtable 0x0026b9a8 */
	float (*points)[4];         /* 0x4 */
	int count;                  /* 0x8 */
};

/* the ASE file reader (vtable 0x0026b9e8): a text buffer read line by line by the rowing_b /
   carryon_a reader methods (0x001b9e00 reads a line, 0x000c3a50 skips the line end) */
struct DcASEFile {
	const void *const *vtable;  /* 0x0 vtable 0x0026b9e8 */
	int unk04;                  /* 0x4 */
	char *pos;                  /* 0x8 */
	char *end;                  /* 0xc */
};

/* ======================================================================== CDemolition_Lift */

/* one cog or worm shaft of the lift (0x1a0 bytes) */
struct DcLiftPart {
	float pos[4];            /* 0x000 from the tables at 0x002988e0 / 0x00298958 */
	int unk10;               /* 0x010 */
	float angle;             /* 0x014 start angle (tables 0x00298940 / 0x00298998) */
	int unk18[2];            /* 0x018 */
	float quat[4];           /* 0x020 spin */
	char actor[0x170];       /* 0x030 BActorInstance (bactor): position at +0xc0, rotation +0xe0 */
};
OF_ASSERT_SIZE(struct DcLiftPart, 0x1a0);

/* a section of the scrolling shaft: a model and its height */
struct DcLiftSegment {
	void *model;             /* 0x0 bkResource */
	float height;            /* 0x4 */
};

/* a slot of the girder ring: the strip pair of that slot */
struct DcLiftGirderSlot {
	char *strips;            /* 0x00 two girder strips (2 x 0x1c4, demolition_d) */
	int unk04[5];            /* 0x04 */
};

struct CDemolition_Lift {
	const void *const *vtable;         /* 0x0000 vtable 0x0026b66c */
	int unk0004[3];                    /* 0x0004 */
	void *partModels[2];               /* 0x0010 cog.obe, worm shaft.obe */
	float speed;                       /* 0x0018 0 (stopped) .. -4 (full speed down) */
	int state;                         /* 0x001c 0 steady, 1 speeding up, 2 stopping */
	unsigned char soundOn;             /* 0x0020 */
	char pad0021[0xf];
	struct DcLiftPart cogs[6];         /* 0x0030 */
	struct DcLiftPart shafts[4];       /* 0x09f0 */
	void *segModels[3];                /* 0x1070 floor.obe, girders.obe, girders extra.obe */
	struct DcLiftSegment segs[3];      /* 0x107c */
	struct DcLiftSegment *ring[3];     /* 0x1094 shaft sections bottom up (ring buffer) */
	int segCounter;                    /* 0x10a0 sections added (picks girders / extra) */
	int ringCount;                     /* 0x10a4 */
	int ringHead;                      /* 0x10a8 */
	float drop;                        /* 0x10ac the first descent, down to -2500 */
	float scroll;                      /* 0x10b0 */
	char girders[5][0x388];            /* 0x10b4 5 x 2 girder strips (demolition_d) */
	struct DcLiftGirderSlot girderRing[5]; /* 0x225c */
	int girderCount;                   /* 0x22d4 */
	int girderHead;                    /* 0x22d8 */
	float girderScroll;                /* 0x22dc */
	int texCounter;                    /* 0x22e0 strips since the last random texture */
	int texEvery;                      /* 0x22e4 2 or 3 */
	char *controlBox;                  /* 0x22e8 the "control box01" object of the world */
	void *swingAnim;                   /* 0x22ec */
	void *sparksNode;                  /* 0x22f0 */
	float sparkPos[4];                 /* 0x22f4 */
	void *sparks;                      /* 0x2304 particle system (0x6c bytes, 0x00102b00) */
	int sparkTimer;                    /* 0x2308 */
	unsigned char sparking;            /* 0x230c */
	char pad230d[3];
	float shaftScroll;                 /* 0x2310 */
	int sound;                         /* 0x2314 */
	char pad2318[0x18];
};
OF_ASSERT_OFFSET(struct CDemolition_Lift, shafts, 0x9f0);
OF_ASSERT_OFFSET(struct CDemolition_Lift, girders, 0x10b4);
OF_ASSERT_OFFSET(struct CDemolition_Lift, girderRing, 0x225c);
OF_ASSERT_OFFSET(struct CDemolition_Lift, sound, 0x2314);
OF_ASSERT_SIZE(struct CDemolition_Lift, 0x2330);

/* ======================================================================== CDemolition_GasJets */

/* a gas jet (vtable 0x0026b74c, 0x9c bytes): a DcGasSystem (CDemolition_Particle.c) and a
   flame drawn as two beams */
struct DcGasJet {
	const void *const *vtable;  /* 0x00 */
	float pos[4];               /* 0x04 */
	char gas[0x68];             /* 0x14 DcGasSystem: active at +0x3c (0x50), emitting +0x3d (0x51) */
	unsigned char enabled;      /* 0x7c */
	unsigned char visible;      /* 0x7d */
	unsigned char firing;       /* 0x7e burns players */
	unsigned char pad7f;
	float fireTime;             /* 0x80 */
	unsigned char flameOn;      /* 0x84 growing */
	unsigned char flameDraw;    /* 0x85 */
	short pad86;
	float flameHeight;          /* 0x88 */
	float flameMax;             /* 0x8c */
	float flameWidth;           /* 0x90 */
	float flameJitter;          /* 0x94 */
	int flameAlpha;             /* 0x98 */
};
OF_ASSERT_SIZE(struct DcGasJet, 0x9c);

/* a light that follows the jets of a group (vtable 0x0026b748, 0x48 bytes) */
struct DcJetLight {
	const void *const *vtable;  /* 0x00 */
	int unk04;                  /* 0x04 */
	void *light;                /* 0x08 CDemolition_Light (demolition_a, 0x34 bytes) */
	int mode;                   /* 0x0c 1 fading in, 2 fading out */
	char timer[0x38];           /* 0x10 timer (demolition_a 0x000ec2f0) */
};
OF_ASSERT_SIZE(struct DcJetLight, 0x48);

/* a row of jets from the outside to the centre (the centre jet is the last of every row) */
struct DcJetGroup {
	int count;                  /* 0x00 */
	struct DcGasJet *jets[10];  /* 0x04 */
};
OF_ASSERT_SIZE(struct DcJetGroup, 0x2c);

struct CDemolition_GasJets {
	const void *const *vtable;      /* 0x000 vtable 0x0026b750 */
	int unk004;                     /* 0x004 */
	struct DcGasJet *jets[23];      /* 0x008 */
	unsigned char unk064;           /* 0x064 */
	unsigned char stopped;          /* 0x065 */
	unsigned char active;           /* 0x066 a wave is running */
	unsigned char pad067;
	struct DcJetGroup groups[4];    /* 0x068 */
	struct DcJetLight lights[4];    /* 0x118 */
	int state;                      /* 0x238 0 wait, 1-2 outside in, 3-4 centre out */
	float waitTime;                 /* 0x23c */
	char timer[0x48];               /* 0x240 timer (demolition_a 0x000ec2f0) */
};
OF_ASSERT_OFFSET(struct CDemolition_GasJets, groups, 0x68);
OF_ASSERT_OFFSET(struct CDemolition_GasJets, lights, 0x118);
OF_ASSERT_OFFSET(struct CDemolition_GasJets, timer, 0x240);
OF_ASSERT_SIZE(struct CDemolition_GasJets, 0x288);

/* ======================================================================== CDemolition_LiftShaft */

/* a platform of the lift in the shaft (0x430 bytes): its actor, 4 spark systems at its brakes
   (ClassLib CSparkPointSystem, 0x98 bytes each) and the brake animations */
struct DcPlatform {
	unsigned char enabled;      /* 0x000 */
	unsigned char visible;      /* 0x001 */
	unsigned char soundStarted; /* 0x002 */
	unsigned char pad003;
	int sound;                  /* 0x004 the falling sound */
	int falling;                /* 0x008 1 once it falls */
	int unk00c;                 /* 0x00c */
	char actor[0x170];          /* 0x010 BActorInstance */
	float pos[4];               /* 0x180 */
	float quat[4];              /* 0x190 */
	float fallSpeed;            /* 0x1a0 */
	void *sparkNodes[4];        /* 0x1a4 "sparks03".."sparks06" */
	unsigned char braking;      /* 0x1b4 */
	char pad1b5[3];
	char sparks[4][0x98];       /* 0x1b8 CSparkPointSystem */
	void *anims[2];             /* 0x418 "brake_in", "brake_out" */
	char *worldObject;          /* 0x420 "platform0N" of the world */
	int pad424[3];
};
OF_ASSERT_OFFSET(struct DcPlatform, pos, 0x180);
OF_ASSERT_OFFSET(struct DcPlatform, sparks, 0x1b8);
OF_ASSERT_SIZE(struct DcPlatform, 0x430);

/* a ring of lights falling past (a barrier, 0x68 bytes): 2 or 4 glows at (x, z) on a level */
struct DcBarrier {
	float centre[4];            /* 0x00 (0, y, 0): y rises with the lift */
	float points[4][4];         /* 0x10 x at [0], z at [2] */
	int count;                  /* 0x50 */
	short colour[3];            /* 0x54 faded */
	short baseColour[3];        /* 0x5a */
	float size;                 /* 0x60 */
	float zBias;                /* 0x64 */
};
OF_ASSERT_SIZE(struct DcBarrier, 0x68);

/* a slot of the shaft section ring */
struct DcShaftSlot {
	struct DcLiftSegment *seg;  /* 0x0 */
	unsigned char opaqueOnly;   /* 0x4 draw pass 1 only */
	char pad5[3];
};

struct CDemolition_LiftShaft {
	const void *const *vtable;        /* 0x0000 vtable 0x0026b880 */
	int unk0004[3];                   /* 0x0004 */
	void *shaftModels[4];             /* 0x0010 walls top start, walls, walldoor, glass walls */
	void *platformModel;              /* 0x0020 cross\objects\lift.obe */
	unsigned char soundOn;            /* 0x0024 */
	char pad0025[3];
	float speed;                      /* 0x0028 */
	float scroll;                     /* 0x002c */
	float drop;                       /* 0x0030 up to 2000 */
	int state;                        /* 0x0034 0 wait, 1 slow down, 2 wait, 3 fall, 4 brake */
	int unk0038;                      /* 0x0038 */
	int sectionCounter;               /* 0x003c */
	unsigned char started;            /* 0x0040 */
	char pad0041[3];
	struct DcLiftSegment sections[4]; /* 0x0044 */
	struct DcShaftSlot ring[10];      /* 0x0064 */
	int ringCount;                    /* 0x00b4 */
	int ringHead;                     /* 0x00b8 */
	float timer;                      /* 0x00bc */
	struct DcPlatform platforms[4];   /* 0x00c0 */
	int pattern;                      /* 0x1180 barrier pattern 0-3 */
	int shape;                        /* 0x1184 0, 1: two glows across x or z; 2: four */
	int kind;                         /* 0x1188 */
	int barrierCounter;               /* 0x118c */
	int barrierEvery;                 /* 0x1190 */
	int fadeIn;                       /* 0x1194 the ambient light fades in */
	float barrierDist;                /* 0x1198 */
	int unk119c[3];                   /* 0x119c */
	short colour[3];                  /* 0x11a8 of the next barrier */
	short pad11ae;
	char lights[3][0x34];             /* 0x11b0 CDemolition_Light (demolition_a) */
	struct DcBarrier barriers[10];    /* 0x124c */
	int barrierCount;                 /* 0x165c */
	int barrierHead;                  /* 0x1660 */
	float distance;                   /* 0x1664 fallen so far */
	float camTimerX;                  /* 0x1668 */
	float camTimerY;                  /* 0x166c */
	float shake;                      /* 0x1670 */
	float shakeVel;                   /* 0x1674 */
	int sound;                        /* 0x1678 */
	int pad167c[5];
};
OF_ASSERT_OFFSET(struct CDemolition_LiftShaft, platforms, 0xc0);
OF_ASSERT_OFFSET(struct CDemolition_LiftShaft, pattern, 0x1180);
OF_ASSERT_OFFSET(struct CDemolition_LiftShaft, lights, 0x11b0);
OF_ASSERT_OFFSET(struct CDemolition_LiftShaft, barriers, 0x124c);
OF_ASSERT_OFFSET(struct CDemolition_LiftShaft, sound, 0x1678);
OF_ASSERT_SIZE(struct CDemolition_LiftShaft, 0x1690);

/* functions of CDemolition_LiftShaft.cpp missing from src/include/of_functions.tsv (defined in
   Demolition/CDemolition_LiftShaft.c) */
void *FUN_000ffe90(void *platform);                    /* DcPlatform constructor */
void FUN_000ffec0(void *platform);                     /* DcPlatform destructor */
void FUN_00100ef0(struct CDemolition_LiftShaft *self); /* Draw: shaft and platforms */
void FUN_00100f60(struct CDemolition_LiftShaft *self); /* DrawEffects: sparks and barriers */

#endif
