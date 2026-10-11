/*
CORE_MODES_A.H

Module core_modes_a: the first half of the front-end game modes
(C:\BlitzParty\Game\Src\Core\, XBE 0x00011000-0x0001b8e0): background text
effects (BG_Effects.cpp), the CGameMode base class and simple modes
(CGameMode_AskAgain.cpp), character select, credits, demo splash, full game,
game size menu and game winner screens.

Owner: core_modes_a. Other modules may include this header; only
core_modes_a edits it.

Conventions (docs/ARCHITECTURE.md):
- Struct layouts keep the original (ILP32) offsets; offsets are in comments.
- Class and field names are ours (recovered from strings and use), the
  original source used C++ classes; methods take an explicit "self".
- Calls to other modules use the prototype from of_decls.h when it matches
  the original call. When the generated (Ghidra) prototype is wrong for the
  call (missing "this", too few parameters, float parameters typed as
  integers), this header defines a CMA_* wrapper macro with the prototype
  read from the x86 code. Those prototypes are reported to the owners.
*/

#ifndef CORE_MODES_A_H
#define CORE_MODES_A_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/* ---------- basic types */

/* UTF-16 text as on the Xbox (the guest's wchar_t is 32 bits) */
typedef unsigned short wchar16;

/* a float's bit pattern, for a parameter the generated prototype still types as undefined4 */
static inline undefined4 cma_fbits(float value)
{
	undefined4 bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
}

/* Calls to a function of another module whose real parameters include floats while the table
   prototype (of_decls.h) still says undefined4: while the declaration is still TABLE_TYPE, pass
   the float bit patterns (an owner that kept that prototype reinterprets them); once the owner
   declares the real prototype in its .decls, pass real floats. */
#define CMA_DUAL(fn, table_type, real_type, table_args, real_args) \
	__builtin_choose_expr(__builtin_types_compatible_p(__typeof__(&(fn)), table_type), \
		((table_type)(fn))table_args, ((real_type)(fn))real_args)

/* the field at byte offset OFF of an object whose layout is not decoded yet */
#define CMA_FIELD(type, obj, off) (*(type *)((char *)(obj) + (off)))

/* virtual calls: the vtable is the first word of the object; OFF is the byte offset of the slot */
#define CMA_VTBL(obj) (*(void *const *const *)(obj))
#define CMA_VFN(obj, off, fntype) ((fntype)CMA_VTBL(obj)[(off) / 4])

/* ---------- ClassLib intrusive list (classlib_ui, 0x00071050-0x00071110) */

struct CMA_ListNode {
	struct CMA_ListNode *next; /* +0x00 */
	struct CMA_ListNode *prev; /* +0x04 */
};

struct CMA_List {
	struct CMA_ListNode *next; /* +0x00 head: next == prev == list when empty */
	struct CMA_ListNode *prev; /* +0x04 */
	int count;                 /* +0x08 */
};

/* FUN_00071050 (thiscall, returns this): empty list */
#define CMA_List_Init(list) ((void *(*)(struct CMA_List *))FUN_00071050)(list)
/* FUN_00071070 (thiscall): insert NODE at the tail (atTail != 0) or head */
#define CMA_List_Insert(list, node, atTail) \
	((void (*)(struct CMA_List *, void *, int))FUN_00071070)((list), (node), (atTail))
/* FUN_000710b0 (thiscall): unlink NODE, returns node->prev */
#define CMA_List_Remove(list, node) \
	((struct CMA_ListNode *(*)(struct CMA_List *, void *))FUN_000710b0)((list), (node))
/* FUN_00071110 (thiscall): the node after NODE (first when NODE is 0), 0 at the end */
#define CMA_List_Next(list, node) \
	((struct CMA_ListNode *(*)(struct CMA_List *, void *))FUN_00071110)((list), (node))

/* ---------- bKernel (bkernel module) */

/* FUN_0004c480: Babel operator new (size, source file, line, flags); returns the block */
#define CMA_New(size, file, line, flags) \
	((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)((size), (file), (line), (flags))
/* FUN_0004c110: Babel operator delete */
#define CMA_Delete(p) FUN_0004c110((void *)(p))
/* FUN_0004b230: debug printf (varargs) */
#define CMA_DebugPrintf ((void (*)(const char *, ...))FUN_0004b230)
/* FUN_0004b110: wcslen for UTF-16 text */
#define CMA_WStrLen(s) FUN_0004b110((short *)(s))
/* FUN_0004c5f0: 64-bit tick counter; DAT_002cd7a0 is its frequency */
#define CMA_GetTicks() ((long long)FUN_0004c5f0())

/* ---------- BDISPLAY (bdisplay module) */

/* FUN_00043440: print UTF-16 text at x, y with an RGBA colour (font, text, length, x, y, r, g, b, a,
   per-glyph callback, callback context). x and y are floats. */
#define CMA_PrintText(font, text, len, x, y, r, g, b, a, cb, ctx) \
	CMA_DUAL(FUN_00043440, \
		void (*)(undefined4, undefined4, undefined4, undefined4, undefined4, undefined4, undefined4, \
			undefined4, undefined4, undefined4, undefined4), \
		void (*)(int, const wchar16 *, int, float, float, int, int, int, int, void *, void *), \
		((undefined4)(font), (undefined4)(text), (undefined4)(len), cma_fbits(x), cma_fbits(y), \
			(undefined4)(r), (undefined4)(g), (undefined4)(b), (undefined4)(a), (undefined4)(cb), \
			(undefined4)(ctx)), \
		((int)(font), (text), (len), (x), (y), (r), (g), (b), (a), (cb), (ctx)))
/* FUN_00043400: print UTF-16 text inside a CMA_TextLayout box */
#define CMA_PrintTextBox(font, text, len, layout, r, g, b, a, cb, ctx) \
	((void (*)(int, const wchar16 *, int, void *, int, int, int, int, void *, void *))FUN_00043400)( \
		(int)(font), (text), (len), (layout), (r), (g), (b), (a), (cb), (ctx))
/* FUN_000720e0 (classlib_ui): print text in a box with a glow/scale effect */
#define CMA_PrintTextBoxFx(font, text, len, layout, r, g, b, a, glow, scale, ctx) \
	((void (*)(int, const wchar16 *, int, void *, int, int, int, int, float, float, void *))FUN_000720e0)( \
		(int)(font), (text), (len), (layout), (r), (g), (b), (a), (glow), (scale), (ctx))

/* FUN_00044810: near and far clip planes (floats) */
#define CMA_SetClipPlanes(n, f) \
	CMA_DUAL(FUN_00044810, void (*)(undefined4, undefined4), void (*)(float, float), \
		(cma_fbits(n), cma_fbits(f)), ((n), (f)))

/* the text box FUN_00043400 and FUN_000720e0 read (8 dwords) */
struct CMA_TextLayout {
	float x;      /* +0x00 */
	float y;      /* +0x04 */
	float width;  /* +0x08 */
	float height; /* +0x0c */
	int hAlign;   /* +0x10 */
	int vAlign;   /* +0x14 */
	int wrap;     /* +0x18 */
	int unk1c;    /* +0x1c */
};

/* the vertex FUN_0003f2d0 draws (9 dwords, pre-transformed 2D) */
struct CMA_Vertex {
	float x, y, z;      /* +0x00 */
	uint32_t unk0c[3];  /* +0x0c */
	uint32_t color;     /* +0x18 ARGB from the colour tables */
	float u, v;         /* +0x1c */
};

/* bdisplay's colour tables: colour = R[r] | G[g] | B[b] | A[a] (r, g, b, a in 0..255) */
extern uint32_t DAT_002bce18[256]; /* red */
extern uint32_t DAT_002bc490[256]; /* green */
extern uint32_t DAT_002bd640[256]; /* blue */
extern uint32_t DAT_002bc898[256]; /* alpha */
#define CMA_RGBA(r, g, b, a) \
	(DAT_002bc490[(g)] | DAT_002bd640[(b)] | DAT_002bce18[(r)] | DAT_002bc898[(a)])

/* ---------- ClassLib sound helpers (classlib_ui) */

/* FUN_000713a0: play SAMPLE (volume, pan, priority, 3D position or 0); returns a voice or -1 */
#define CMA_PlaySample(sample, vol, pan, prio, pos) \
	((int (*)(int, int, int, int, int))FUN_000713a0)((int)(sample), (vol), (pan), (prio), (pos))
/* FUN_00071720: play the generic sample with index N (volume, pan, priority, 3D position) */
#define CMA_PlayGeneric(n, vol, pan, prio, pos) \
	((int (*)(int, int, int, int, int))FUN_00071720)((n), (vol), (pan), (prio), (pos))

/* 0x00072060 (classlib_ui, missing from the function table): the glyph callback that reveals a
   CBGTypeText one character at a time. Weak: 0 (no reveal effect) until classlib_ui defines it. */
int FUN_00072060(int glyph, void *ctx) __attribute__((weak));

/* ---------- globals of other modules (see core_modes_a_data.c) */

extern float DAT_0028fda8;           /* frame time step (1.0 = one 60 Hz frame) */
extern unsigned int DAT_00285630;    /* the C library rand() seed */
extern long long DAT_002cd7a0;       /* tick frequency of FUN_0004c5f0 */
extern float DAT_002855f0[16];       /* identity matrix */

/* ---------- BG_Effects.cpp: background text effects */

/* CBGEffect (vtable 0x00253744): base of every background effect.
   vtable: [0] scalar deleting dtor (self, flags), [1] Init, [2] Update -> int (0: finished),
   [3] Render, [4] Show(float), [5] Hide(float) */
struct CBGEffect {
	const void *const *vtbl;  /* +0x00 */
	struct CMA_ListNode node; /* +0x04 link in the owning effect list */
	unsigned int flags;       /* +0x0c 3 when built; 4 shown, 8 hidden, 0x10 finished */
};

/* CBGMatrixText (vtable 0x00253788): a grid of random characters that scrolls up */
struct CBGMatrixText {
	struct CBGEffect base;  /* +0x00 */
	int cursor;             /* +0x10 */
	wchar16 *buffer;        /* +0x14 cols * rows characters */
	float x;                /* +0x18 */
	float y;                /* +0x1c */
	int cols;               /* +0x20 */
	int rows;               /* +0x24 */
	int scroll;             /* +0x28 characters moved up per step */
	int color[4];           /* +0x2c */
	float interval;         /* +0x3c seconds per step */
	float timer;            /* +0x40 */
	float scale;            /* +0x44 */
};

/* CBGText (vtable 0x002537d8): a line of text */
struct CBGText {
	struct CBGEffect base;  /* +0x00 */
	float x;                /* +0x10 */
	float y;                /* +0x14 */
	const wchar16 *source;  /* +0x18 */
	const wchar16 *text;    /* +0x1c */
	int length;             /* +0x20 */
	int color[4];           /* +0x24 halves of the colour given */
	float scale;            /* +0x34 */
	int font;               /* +0x38 */
};

/* CBGTextBuf (vtable 0x002537f4): a CBGText that owns a copy of its text */
struct CBGTextBuf {
	struct CBGText text;    /* +0x00 */
	wchar16 *buffer;        /* +0x3c */
	int capacity;           /* +0x40 characters, with the terminator */
};

/* CBGTypeText (vtable 0x00253810): text that types itself, fades and pulses */
struct CBGTypeText {
	struct CBGTextBuf buf;  /* +0x00 */
	float glow;             /* +0x44 */
	float glowTarget;       /* +0x48 */
	float glowRate;         /* +0x4c */
	float alpha;            /* +0x50 follows (float)color[3] */
	float alphaRate;        /* +0x54 */
	float charDelay;        /* +0x58 0: show all text at once */
	float startDelay;       /* +0x5c */
	float charTimer;        /* +0x60 */
	int voice;              /* +0x64 typing sound voice, -1 none, -2 stopped */
	int typeSound;          /* +0x68 */
	int beepVolume;         /* +0x6c 0: no "computertext" beeps */
	int typed;              /* +0x70 characters typed */
	int lines;              /* +0x74 */
	float pulsePhase;       /* +0x78 */
	float boxWidth;         /* +0x7c */
	int wrap;               /* +0x80 */
	int revealed;           /* +0x84 read by the glyph callback 0x00072060 */
	int glyphCount;         /* +0x88 written by the glyph callback */
	unsigned int style;     /* +0x8c 1 pulse, 2 underline bar, 4 shadow, 8/0x10 h-align,
	                                 0x20/0x40 v-align */
};

/* CBGTypeTextBox (vtable 0x00253858): CBGTypeText in a fixed box */
struct CBGTypeTextBox {
	struct CBGTypeText type;      /* +0x00 */
	struct CMA_TextLayout layout; /* +0x90 */
};

/* CBGTilingBackdrop (vtable 0x00253880): scrolling tiled background (ff_tile.bmp) */
struct CBGTilingBackdrop {
	struct CBGEffect base;  /* +0x00 */
	float tileU;            /* +0x10 */
	float tileV;            /* +0x14 */
	float scrollU;          /* +0x18 */
	float scrollV;          /* +0x1c */
	int texture;            /* +0x20 */
};

extern const void *const vtable_00253744[];
extern const void *const vtable_00253788[];
extern const void *const vtable_002537d8[];
extern const void *const vtable_002537f4[];
extern const void *const vtable_00253810[];
extern const void *const vtable_00253858[];
extern const void *const vtable_00253880[];

#endif
