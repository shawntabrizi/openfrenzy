/*
CORE_MODES_B.H

Module core_modes_b: front-end game modes, part 2 (0x0001bd30-0x00026ce0):
CGameMode_MainMenu, CGameMode_MainOptions, CGameMode_Map, CGameMode_MetaVote,
CGameMode_MiniGameIntro, CGameMode_MiniGameSelect, CGameMode_ObjectViewer,
CGameMode_PressStart, and the player score table (CScoreTable, unplaced).

Owner: core_modes_b. Other modules may include this header; only core_modes_b edits it.

Conventions used in this module (see docs/ARCHITECTURE.md):

- Object layouts keep the original (Xbox, ILP32) offsets. Fields not understood yet are
  reached with the CMB_I32/CMB_F32/... accessors and the original offset.
- Virtual calls go through CMB_VFN(obj, byte offset of the slot, function type), and always
  pass the object as the first argument (the x86 code passed it in ECX).
- Calls into other modules whose table prototype has the wrong shape (missing ECX `this`,
  extra stack arguments, a void return that is used) go through a cast to the real shape.
  Integer and pointer arguments are ABI-safe that way on arm64.
- Calls whose real prototype has float parameters that the table still declares as
  undefined4 go through CMB_DUAL: it passes float bit patterns while the table prototype is
  unchanged, and real floats once the owner declares the float prototype in its .decls.
*/

#ifndef CORE_MODES_B_H
#define CORE_MODES_B_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

/* ---------- raw field access at original offsets */

#define CMB_AT(p, off, type) (*(type *)((char *)(p) + (off)))
#define CMB_I32(p, off) CMB_AT(p, off, int)
#define CMB_U32(p, off) CMB_AT(p, off, unsigned int)
#define CMB_F32(p, off) CMB_AT(p, off, float)
#define CMB_PTR(p, off) CMB_AT(p, off, void *)
#define CMB_U8(p, off) CMB_AT(p, off, unsigned char)

/* virtual function at byte offset `off` of the object's vtable */
#define CMB_VFN(obj, off, type) ((type)((*(void ***)(obj))[(off) / 4]))

/* a float's bit pattern, for an undefined4 parameter that carries a float */
static inline unsigned int cmb_fbits(float value)
{
	unsigned int bits;
	memcpy(&bits, &value, sizeof bits);
	return bits;
}

/* a float stored in an undefined4 */
static inline float cmb_bitsf(unsigned int bits)
{
	float value;
	memcpy(&value, &bits, sizeof value);
	return value;
}

/* see the header comment */
#define CMB_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn)) table_args, ((real_type)(fn)) real_args)

/* the Babel allocator (bkernel FUN_0004c480: size, file, line, flags), operator new */
#define CMB_NEW(size, file, line) \
	(((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)( \
		(size), (file), (line), 0x1001))

/* operator delete (bkernel FUN_0004c110) */
#define CMB_DELETE(p) (((void (*)(void *))FUN_0004c110)(p))

/* bkernel FUN_0004b230: debug printf */
#define CMB_DEBUG_PRINTF ((void (*)(const char *, ...))FUN_0004b230)

/* bkernel FUN_0004ae80: wide sprintf of the Babel engine (destination, format, ...) */
#define CMB_WSPRINTF ((int (*)(unsigned short *, const unsigned short *, ...))FUN_0004ae80)

/* bkernel FUN_0004bb30: find/load a resource by name (package, name, type, tag[, ...]) */
#define CMB_RESOURCE4 ((int (*)(int, const char *, int, int))FUN_0004bb30)
#define CMB_RESOURCE5 ((int (*)(int, const char *, int, int, int))FUN_0004bb30)

/* classlib_ui FUN_00071720: play a generic sample (id, volume, pan, pitch, loop) */
#define CMB_PLAY_SAMPLE ((int (*)(int, int, int, int, int))FUN_00071720)

/* the text database: DAT_002ecc74 points at the language table, +0x20 holds 16-byte
   entries whose first word is the (wide) string */
#define CMB_TEXT(index) (*(unsigned short **)(CMB_I32(DAT_002ecc74, 0x20) + (index) * 0x10))

/* ---------- the game-mode stack (core: CGameModeStack at 0x002ba750) */

/* The stack lives in the core module's .bss block (src/core/core_data.h: of_core_bss holds
   0x002ba4f0-0x002bb138 with the original layout). Ghidra never names 0x002ba750, so there
   is no DAT_ alias for it; reach it through the block. */
extern unsigned char of_core_bss[];
#define CMB_CORE_BSS_START 0x002ba4f0u
#define CMB_MODESTACK ((void *)(of_core_bss + (0x002ba750u - CMB_CORE_BSS_START)))

/* core FUN_0002b640 Push(stack, mode, start) and FUN_0002b6f0 Pop(stack, n) */
#define CMB_PUSH_MODE(mode, start) \
	(((int (*)(void *, void *, int))FUN_0002b640)(CMB_MODESTACK, (void *)(mode), (start)))
#define CMB_POP_MODE(count) (((int (*)(void *, int))FUN_0002b6f0)(CMB_MODESTACK, (count)))

/* ---------- CGameMode (base class; core and core_modes_a own it). Only the fields this
   module uses, at their original offsets. */

struct cmb_CGameMode {
	const void *vtable;       /* 0x00 */
	unsigned int field_04;    /* 0x04 */
	unsigned int package;     /* 0x08: package handle */
	int refs;                 /* 0x0c */
	unsigned int flags;       /* 0x10 */
	int state;                /* 0x14 */
	unsigned char pad_18[0x10];
	void *menu_model;         /* 0x28 */
	unsigned char held[4][9]; /* 0x2c: button held per pad */
	int repeat_delay[9];      /* 0x50 */
	int repeat_rate[9];       /* 0x74 */
};
OF_ASSERT_SIZE(struct cmb_CGameMode, 0x98);

/* CGameMode vtable slots (byte offsets) */
#define CMB_VT_DTOR 0x00      /* (this, flags) scalar deleting destructor */
#define CMB_VT_STOP 0x04      /* (this) leave / release */
#define CMB_VT_START 0x08     /* (this) enter */
#define CMB_VT_RESUME 0x0c    /* (this) back on top of the stack */
#define CMB_VT_LOAD 0x10      /* (this) pushed */
#define CMB_VT_BUTTONS 0x14   /* (this, int) */
#define CMB_VT_UPDATE 0x18    /* (this, int active) */
#define CMB_VT_RENDER 0x1c    /* (this, int active) */
#define CMB_VT_ONBUTTON 0x34  /* (this, int button, int pad) */

/* core: base constructor (this, name) and destructor */
#define CMB_GAMEMODE_CTOR(self, name) \
	(((void *(*)(void *, const char *))FUN_00030090)((self), (name)))
#define CMB_GAMEMODE_DTOR(self) (((void (*)(void *))FUN_00030190)(self))

/* ---------- globals of other modules (tentative definitions in Core/cmb_globals.c) */

/* see Core/cmb_globals.c for what each one is */
extern int DAT_002b95cc, DAT_002b95d0, DAT_002b9c78;
extern int DAT_002ba550, DAT_002ba554;
extern int DAT_002bae68, DAT_002bae6c, DAT_002bae74, DAT_002bae7c, DAT_002bae84, DAT_002bae88;
extern int DAT_002bae94, DAT_002bae98, DAT_002bae9c;
extern int DAT_002baad8, DAT_002baae8;
extern float _DAT_002ba9c8;
extern int DAT_002ecc74, DAT_002ecdec, DAT_002ecfac;
extern unsigned char DAT_002ed324[]; /* CRumble (classlib_ui) */
extern float DAT_0028fda8;
extern int DAT_0028ffb8;
extern int DAT_002844a0;
extern int DAT_00284408;
extern int DAT_002ba570[0x31];
extern unsigned int DAT_002bc490[], DAT_002bc898[], DAT_002bce18[], DAT_002bd640[];

/* ---------- D3D_KickOffAndWaitForIdle has no HLE yet (docs/LIBRARY_CALLS.md): call it when
   the d3d8 module provides it */

extern void D3D_KickOffAndWaitForIdle(void) __attribute__((weak));
static inline void cmb_kick_off_and_wait_for_idle(void)
{
	if (D3D_KickOffAndWaitForIdle)
		D3D_KickOffAndWaitForIdle();
}

/* ---------- CScoreTable (unplaced, 0x0001bd30-0x0001c200): the party/tournament scores.
   One instance, g_score_table (DAT_002b9968). */

struct CScoreRound {
	int kind;                   /* 0x00: which scores[] row this round filled (3: bonus) */
	int scores[4][4];           /* 0x04: [kind][player] */
	int sum[4];                 /* 0x44: per player, kinds 0-2 */
	int extra[4];               /* 0x54 */
};
OF_ASSERT_SIZE(struct CScoreRound, 100);

struct CScoreTable {
	int round;                  /* 0x000: rounds played */
	struct CScoreRound rounds[7]; /* 0x004 */
	int totals[4];              /* 0x2c0: sum of kinds 0-2 */
	int bonus[4];               /* 0x2d0: sum of kind 3 */
	int place[4];               /* 0x2e0: place of each player (0 = first) */
	int by_place[4];            /* 0x2f0: players in place order, -1 = none */
	int ties[4];                /* 0x300: players sharing each place */
};
OF_ASSERT_OFFSET(struct CScoreTable, totals, 0x2c0);
OF_ASSERT_SIZE(struct CScoreTable, 0x310);

extern struct CScoreTable DAT_002b9968;
#define g_score_table DAT_002b9968

/* the per-player game records (core): 4 x 0x28 bytes at 0x002baed8; the score is at +0x10 */
extern int DAT_002baed8[4][10];
/* tournament data (core): +0x1b0 holds an int per player */
extern int DAT_002ba55c;

/* ---------- CGameMode_MainMenu (0xc8 bytes or more) */

struct CGameMode_MainMenu {
	struct cmb_CGameMode base;
	void *items[4];             /* 0x98: menu texts (BG_Effects text boxes) */
	void *group_a8;             /* 0xa8: background text boxes */
	void *group_ac;             /* 0xac: menu texts */
	int field_b0;
	float idle_time;            /* 0xb4: frames without input */
	int field_b8;
	int field_bc;
	int font;                   /* 0xc0 */
	int selection;              /* 0xc4 */
};
OF_ASSERT_OFFSET(struct CGameMode_MainMenu, selection, 0xc4);

/* ---------- CGameMode_MainOptions */

struct CGameMode_MainOptions {
	struct cmb_CGameMode base;
	void *group;                /* 0x98: all texts */
	void *items[6];             /* 0x9c: effects volume, music volume, vibration,
	                               soundtrack, track, language (CharVoice) */
	void *hint;                 /* 0xb4: help line (text buffer at +0x3c) */
	int effects_level;          /* 0xb8: 0-10 */
	int music_level;            /* 0xbc: 0-10 */
	int sample;                 /* 0xc0: frontend_02b.wav */
	int package;                /* 0xc4: loaded package */
	int field_c8;
	int option;                 /* 0xcc: selected item */
};
OF_ASSERT_OFFSET(struct CGameMode_MainOptions, option, 0xcc);

/* the level meter's vertices (DAT_002b9c80, 10 quads) */
struct cmb_meter_vertex {
	float x, y, z;
	float f0c, f10, f14;
	unsigned int colour;
	float u, v;
};
OF_ASSERT_SIZE(struct cmb_meter_vertex, 0x24);
extern struct cmb_meter_vertex DAT_002b9c80[10][4];
extern int DAT_00283768[8];
extern float DAT_00283760;
extern int DAT_002844dc[3];
extern int DAT_002baf78[4][9];

void CGameMode_MainMenu_CreateBoxes(struct CGameMode_MainMenu *self);

#endif
