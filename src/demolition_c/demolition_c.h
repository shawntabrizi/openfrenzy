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

#endif
