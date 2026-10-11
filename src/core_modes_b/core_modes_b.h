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

#include <math.h>
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

/* bkernel FUN_0004bb30: find/load a resource by name (package, name, type, group) */
#define CMB_RESOURCE4 ((int (*)(int, const char *, int, int))FUN_0004bb30)

/* classlib_ui FUN_00071720: play a generic sample (id, volume, pan, pitch, loop) */
#define CMB_PLAY_SAMPLE ((int (*)(int, int, int, int, int))FUN_00071720)

/* core_modes_a BG_Effects text: select (FUN_000114b0, real prototype (this, float phase))
   and deselect (FUN_00011500) */
static inline void cmb_text_select(void *text, float phase)
{
	CMB_DUAL(FUN_000114b0, void (*)(int, undefined4), void (*)(void *, float),
		((int)text, cmb_fbits(phase)), (text, phase));
}
#define cmb_text_deselect(text) (((void (*)(void *))FUN_00011500)(text))

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
#define CMB_VT_SUSPEND 0x04   /* (this) another mode is pushed on top; also on pop */
#define CMB_VT_START 0x08     /* (this) pushed with start = 1 */
#define CMB_VT_EXIT 0x0c      /* (this) popped */
#define CMB_VT_ENTER 0x10     /* (this) pushed */
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
extern int DAT_002bb020;                  /* core: map/menu model shown */
extern int DAT_002ba560;                  /* core: the game mode after the map (meta game) */
extern int DAT_002ba63c[7];               /* core: per round, fixed game */
extern int DAT_002ba658[21];              /* core: fixed games [round][kind] */
extern int DAT_00283ab8[21];              /* core: fixed variants [round][kind] */
extern int DAT_002baeac, DAT_002baeb0;    /* chosen game and variant */
extern int DAT_002baeb4;                  /* site of the round */
extern int DAT_002baebc;
extern int DAT_002bae8c;
extern unsigned char DAT_002be370[];      /* bdisplay: view matrix */
extern int DAT_00283d74;
extern int DAT_002844ac[6];               /* fixed site order */
extern unsigned int DAT_002bc40c, DAT_002bc410;
extern float DAT_00283874[4];
extern int DAT_00283884;
extern unsigned char DAT_002ed324[]; /* CRumble (classlib_ui) */
extern float DAT_0028fda8;
extern int DAT_0028ffb8;
extern int DAT_002844a0[3];
extern int DAT_00284408;
extern int DAT_002ba570[0x31];
extern unsigned int DAT_002bc490[], DAT_002bc898[], DAT_002bce18[], DAT_002bd640[];

/* xkernel (HLE): logs and continues */
void DbgBreakPoint(void);

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

/* the per-player records (core): 4 x 0x28 bytes at 0x002baed4: [0] character,
   [1] human player, [5] score, [7] votes */
extern int DAT_002baed4[4][10];
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
	                               soundtrack, track, credits */
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

/* ---------- CGameMode_Map (0x5e8 bytes): the city map between rounds */

/* one site of the map (100 bytes); built by FUN_0001ddb0, destroyed by 0x0001ddc0 */
struct CMapSite {
	void *actor;                /* 0x00: site model */
	void *wire;                 /* 0x04: wireframe model (also the shadow caster) */
	int colour[4];              /* 0x08: wire sprite (core object at +8) colour */
	int sprite_10[4];
	int sprite_20;
	int sprite_24;
	int sprite_size;            /* 0x30 */
	unsigned char shadow[0x30]; /* 0x34: shadow object (bactor) */
};
OF_ASSERT_SIZE(struct CMapSite, 100);

struct CGameMode_Map {
	struct cmb_CGameMode base;
	float exit_timer;           /* 0x098: counts down, then pops the mode */
	int font_bold;              /* 0x09c */
	int font_light;             /* 0x0a0 */
	struct CMapSite sites[6];   /* 0x0a4 */
	float pos[6][4];            /* 0x2fc: site positions */
	float far_pos[6][4];        /* 0x35c */
	int alpha[6];               /* 0x3bc: site brightness, 0x100 = full */
	int alpha_min[6];           /* 0x3d4 */
	float shadow_alpha;         /* 0x3ec */
	int city_model;             /* 0x3f0 */
	void *city;                 /* 0x3f4 */
	float light_dir[4];         /* 0x3f8 */
	unsigned char light[0x70];  /* 0x408: BLight */
	float cam_pos[4];           /* 0x478 */
	float cam_target[4];        /* 0x488 */
	float pos_from[4];          /* 0x498 */
	float pos_to[4];            /* 0x4a8 */
	float target_from[4];       /* 0x4b8 */
	float target_to[4];         /* 0x4c8 */
	float zoom[4];              /* 0x4d8 */
	float zoom_from[4];         /* 0x4e8 */
	float zoom_to[4];           /* 0x4f8 */
	float blend;                /* 0x508 */
	int node_texture;           /* 0x50c */
	int line_texture;           /* 0x510 */
	int order[6];               /* 0x514: site of each round */
	int round_of[6];            /* 0x52c: round of each site, -1 = none */
	int rounds;                 /* 0x544 */
	int first_label;            /* 0x548 */
	int current;                /* 0x54c: current round */
	int path_alpha;             /* 0x550 */
	float path_done;            /* 0x554 */
	float path_current;         /* 0x558 */
	float pulse;                /* 0x55c: 0..6 */
	int kind;                   /* 0x560: why the map is shown */
	int state;                  /* 0x564 */
	int game_name;              /* 0x568 */
	int game;                   /* 0x56c: chosen game */
	int variant;                /* 0x570 */
	float timer;                /* 0x574 */
	float timer2;               /* 0x578 */
	float angle;                /* 0x57c */
	int package;                /* 0x580 */
	float fade;                 /* 0x584 */
	float zfar;                 /* 0x588 */
	void *selector;             /* 0x58c: game selector (core FUN_0002e1b0) */
	unsigned char list[0xc];    /* 0x590: overlay list (classlib_ui) */
	void *title;                /* 0x59c: overlays */
	void *subtitle;             /* 0x5a0 */
	void *label_left;           /* 0x5a4 */
	void *label_right;          /* 0x5a8 */
	void *vs;                   /* 0x5ac */
	void *vs_text;              /* 0x5b0 */
	void *prompt;               /* 0x5b4 */
	int grid;                   /* 0x5b8: vertex buffer of the grid */
	int logo;                   /* 0x5bc: logo animation 1-7 */
	int samples[8];             /* 0x5c0 */
	int sample_playing;         /* 0x5e0 */
	int hum;                    /* 0x5e4 */
};
OF_ASSERT_OFFSET(struct CGameMode_Map, pos, 0x2fc);
OF_ASSERT_OFFSET(struct CGameMode_Map, light, 0x408);
OF_ASSERT_OFFSET(struct CGameMode_Map, order, 0x514);
OF_ASSERT_OFFSET(struct CGameMode_Map, list, 0x590);
OF_ASSERT_SIZE(struct CGameMode_Map, 0x5e8);

/* the engine's rand() (MSVC's, inlined by the game; seed DAT_00285630) */
extern unsigned int DAT_00285630;
static inline int cmb_rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- CGameMode_MetaVote: the players spend their round score (chips) on votes */

struct CMetaToken {
	float gravity;              /* 0x00: +0.3 falls down onto the stack, -0.3 goes back */
	float y;                    /* 0x04 */
	float z;                    /* 0x08 */
	float floor;                /* 0x0c: rest height */
	float base;                 /* 0x10 */
	float velocity;             /* 0x14 */
	float f18;
	void *actor;                /* 0x1c: tile model */
};
OF_ASSERT_SIZE(struct CMetaToken, 0x20);

/* ---------- CGameMode_MiniGameIntro: the rules and controls of the next minigame */

struct CGameMode_MiniGameIntro {
	struct cmb_CGameMode base;
	unsigned char list[0xc];    /* 0x098: overlay list */
	void *title;                /* 0x0a4 */
	void *text[3];              /* 0x0a8: description lines */
	void *controls[2];          /* 0x0b4 */
	unsigned char pad_bc[8];
	void *logo;                 /* 0x0c4: animlogo.gif overlay */
	void *choice[2];            /* 0x0c8: play / practice */
	int loading_model;          /* 0x0d0 */
	unsigned char pad_d4[0x250 - 0xd4];
	int selection;              /* 0x250 */
	int ready;                  /* 0x254 */
	int game;                   /* 0x258: text index of the game */
	char name[0x80];            /* 0x25c */
	int frames;                 /* 0x2dc */
	int fade_start;             /* 0x2e0 */
	int field_2e4;
	float saved_fov[2];         /* 0x2e8 */
	void *backdrop;             /* 0x2f0: CBGTilingBackdrop */
	int loaded;                 /* 0x2f4 */
};
OF_ASSERT_OFFSET(struct CGameMode_MiniGameIntro, selection, 0x250);
OF_ASSERT_OFFSET(struct CGameMode_MiniGameIntro, frames, 0x2dc);
OF_ASSERT_OFFSET(struct CGameMode_MiniGameIntro, loaded, 0x2f4);

extern int DAT_002ba220, DAT_002ba224;     /* fonts of the minigame intro */
extern int DAT_002baed0;                   /* practice (no score) */
extern int DAT_002baeb8;                   /* demo mode: CPU players only */
extern int DAT_002baea8;                   /* chosen intro option */
extern int DAT_002ba6a4;                   /* attract mode */
extern int DAT_002ba4e0, DAT_002f28ac, DAT_002bae90;
extern int DAT_00290960;
extern float DAT_00290be4[4];
extern int DAT_002ecf90[4];
extern int DAT_002b95b0[6];                /* per character: package to load */
extern int DAT_00290754[0x79];             /* classlib_ui: per character +0x60 apart */
extern int DAT_00292c4c, DAT_00292c50, DAT_00292c78;
extern int DAT_002838e4[3];                /* description line heights */
extern int DAT_002838f0;                   /* fade frames */
extern int _DAT_002ecd9c, _DAT_002ecda0, _DAT_002ecda4, _DAT_002ecda8; /* text ids for the pause menu */
extern int _DAT_002ecde8;

/* ---------- CGameMode_MiniGameSelect: the tournament's minigame carousel */

/* one carousel entry (12 bytes, DAT_002ba238): game 1-12, variant 1-4, name text */
struct CMiniGameEntry {
	int game;
	int variant;
	int text;
};
OF_ASSERT_SIZE(struct CMiniGameEntry, 12);

struct CGameMode_MiniGameSelect {
	struct cmb_CGameMode base;
	int leave;                  /* 0x098: back pressed: pop */
	int field_9c;
	void *items[0x36];          /* 0x0a0: one text per entry */
	int up_texture;             /* 0x178 */
	int down_texture;           /* 0x17c */
	unsigned char pad_180[8];
	void *up_glow;              /* 0x188 */
	void *down_glow;            /* 0x18c */
	void *up_arrow;             /* 0x190 */
	void *down_arrow;           /* 0x194 */
	void *heading_left;         /* 0x198 */
	void *heading_right;        /* 0x19c */
	unsigned char list[0xc];    /* 0x1a0: overlay list */
	void *info;                 /* 0x1ac: the game info panel (core FUN_0002e0d0) */
	int totals[4];              /* 0x1b0: tournament points per player */
};
OF_ASSERT_OFFSET(struct CGameMode_MiniGameSelect, items, 0xa0);
OF_ASSERT_OFFSET(struct CGameMode_MiniGameSelect, list, 0x1a0);
OF_ASSERT_OFFSET(struct CGameMode_MiniGameSelect, totals, 0x1b0);

extern struct CMiniGameEntry DAT_002ba238[0x30]; /* the carousel entries, sorted by name */
extern int DAT_002ba4dc;                  /* number of entries */
extern int DAT_002ba4d0;                  /* scroll direction, 0 = still */
extern float _DAT_002ba4c0;               /* scroll position 0..1 */
extern int DAT_002ba4d4;                  /* first visible entry */
extern int DAT_002ba4d8;                  /* selected entry */
extern int DAT_002ba4c4, DAT_002ba230;    /* fonts */
extern float _DAT_002ba4c8;               /* arrow offset */
extern int DAT_002ba4cc;                  /* scores to add when back from a game (set by classlib_ui's HUD) */
extern int DAT_002ba4e4;                  /* fade in on the next render */
extern int DAT_002839f0[0x31];            /* core: variant availability [game * 4 + variant - 4] */
extern int DAT_002838f4, DAT_002838f8;    /* last game and variant */
extern int DAT_0028198c;                  /* core: the AskAgain answer (1 play again, -1 no) */
extern int DAT_00283984;                  /* frames before input is accepted */
extern float _DAT_00283988;               /* scroll speed */
extern int DAT_0028452c[4];               /* heading colour */
extern void *DAT_002ba954[13];            /* core: the module of each game */

/* ---------- CGameMode_ObjectViewer: a debug viewer for every actor resource. Its base is
   core_modes_a's FUN_00012800 class (not CGameMode); fields from 0x28. */

struct CGameMode_ObjectViewer {
	const void *vtable;         /* 0x00 */
	unsigned int field_04;
	unsigned int package;       /* 0x08 */
	int refs;                   /* 0x0c */
	unsigned int flags;         /* 0x10 */
	unsigned char pad_14[0x14];
	void *camera;               /* 0x28: previous camera */
	void *input;                /* 0x2c: input map */
	void *next_actor;           /* 0x30: input channels */
	void *prev_actor;           /* 0x34 */
	void *next_anim;            /* 0x38 */
	void *prev_anim;            /* 0x3c */
	void *zoom;                 /* 0x40 */
	void *rot_x;                /* 0x44 */
	void *rot_y;                /* 0x48 */
	void *rot_z;                /* 0x4c */
	void *cam_y;                /* 0x50 */
	void *resource;             /* 0x54: current actor resource */
	void *actor;                /* 0x58 */
	unsigned int anim;          /* 0x5c */
	float target[4];            /* 0x60 */
	float distance;             /* 0x70 */
};
OF_ASSERT_OFFSET(struct CGameMode_ObjectViewer, camera, 0x28);
OF_ASSERT_OFFSET(struct CGameMode_ObjectViewer, distance, 0x70);

extern void *DAT_002ba4e8, *DAT_002ba4ec;     /* the viewer's two lights */
extern float DAT_002ea478[4];                 /* camera target */
extern int DAT_0028398c;                      /* lights on */
extern float _DAT_00283990, _DAT_00283994, _DAT_00283998, _DAT_0028399c, _DAT_002839a0;
extern float DAT_0028fdd8[4];

/* ---------- CGameMode_PressStart: the title screen */

struct CGameMode_PressStart {
	struct cmb_CGameMode base;
	int saved_flag;             /* 0x098: DAT_00284408 */
	void *press_start;          /* 0x09c: texts */
	void *legal;                /* 0x0a0 */
	void *trademark;            /* 0x0a4 */
	float fade;                 /* 0x0a8 */
	float idle_time;            /* 0x0ac */
	int pressed;                /* 0x0b0 */
	unsigned char list_b4[0xc]; /* 0x0b4 */
	unsigned char list[0xc];    /* 0x0c0 */
	int field_cc;
	unsigned char logo[0x134];  /* 0x0d0: BActorInstance of fusion_frontend.obe */
	void *logo_node;            /* 0x204: the instance's node (+0x134) */
	unsigned char pad_208[0x38];
	float scroll;               /* 0x240: texture scroll */
	float shine;                /* 0x244 */
};
OF_ASSERT_OFFSET(struct CGameMode_PressStart, logo_node, 0x204);
OF_ASSERT_OFFSET(struct CGameMode_PressStart, shine, 0x244);

extern int DAT_002baadc;                      /* skip the title */
extern char DAT_002ba538;                     /* the map was shown once */
extern int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c; /* clear colour */
extern float _DAT_002839a4, _DAT_002839a8, DAT_002839ac;
extern int _DAT_002839b0;
extern const void *const vtable_00256010[13];     /* CGameMode_Root (core), built here */

struct CGameMode_MetaVote {
	struct cmb_CGameMode base;
	int state;                  /* 0x098 */
	float timer;                /* 0x09c */
	float blink;                /* 0x0a0 */
	int place[4];               /* 0x0a4: place by score */
	int keep[4];                /* 0x0b4: chips a CPU player keeps */
	int chips[4];               /* 0x0c4: chips left */
	int spent[4];               /* 0x0d4: chips voted */
	int active[4];              /* 0x0e4: still voting */
	int voting;                 /* 0x0f4: players still voting */
	float saved_fov[2];         /* 0x0f8: from bdisplay 0x002be588 */
	float saved_clip[2];        /* 0x100: from bdisplay 0x002be580 */
	unsigned char saved_state[0x78]; /* 0x108: render state (bdisplay FUN_00046d40) */
	int rain;                   /* 0x180: chingching1.wav */
	int chink;                  /* 0x184: orbchink.wav */
	unsigned char pad_188[8];
	void *hint;                 /* 0x190: overlays */
	void *title;                /* 0x194 */
	void *portrait[4];          /* 0x198 */
	void *button[4];            /* 0x1a8 */
	void *chips_text[4];        /* 0x1b8 */
	void *spent_text[4];        /* 0x1c8 */
	void *left_arrow[4];        /* 0x1d8 */
	void *right_arrow[4];       /* 0x1e8 */
	unsigned char list[0xc];    /* 0x1f8: overlay list */
	int anim;                   /* 0x204: "default" animation */
	int tile_model[4];          /* 0x208 */
	unsigned char light[0x70];  /* 0x218: BLight */
	struct CMetaToken *tokens[4]; /* 0x288: 30 tokens each */
	int token_count[4];         /* 0x298 */
	int package;                /* 0x2a8: Packages\ScoreScreen */
	int field_2ac;
	int font_bold;              /* 0x2b0 */
	int font_light;             /* 0x2b4 */
};
OF_ASSERT_OFFSET(struct CGameMode_MetaVote, saved_state, 0x108);
OF_ASSERT_OFFSET(struct CGameMode_MetaVote, list, 0x1f8);
OF_ASSERT_OFFSET(struct CGameMode_MetaVote, tokens, 0x288);
OF_ASSERT_OFFSET(struct CGameMode_MetaVote, font_light, 0x2b4);

extern int DAT_002ba228;                  /* MetaVote background effect object */
extern int DAT_0029074c[0x49];            /* classlib_ui: per player, +0x60 apart */
extern int DAT_003055e0;
extern int DAT_002ba99c;                  /* frame counter */
extern float DAT_002be580, DAT_002be584, DAT_002be588, DAT_002be58c; /* bdisplay clip, fov */
extern float DAT_002838c8, DAT_002838cc, DAT_002838d0, DAT_002838d4;
extern int DAT_002844fc[3];               /* text colour */
extern float DAT_00283888[4];             /* MetaVote light direction */

void CGameMode_MainMenu_CreateBoxes(struct CGameMode_MainMenu *self);

#endif
