/*
CLASSLIB_UI.H

Shared declarations of the classlib_ui module (ClassLib block 1, original source
C:\BlitzParty\Game\Src\ClassLib\: GenericSamples, COverlay, Hud, CIntroCam,
ParseMap, CFireworks, CRumble and the math helpers in front of them).

Other modules may include this header. Only the classlib_ui agent edits it.

The module's globals live in data blocks with the original layout
(classlib_ui_data.h); other modules reach them by their Ghidra names (DAT_<addr>),
which are aliases into the blocks. Globals of other modules that this module uses
get tentative definitions in ClassLib/classlib_ui_extern.c (-fcommon merges them
with the owner's definition, docs/ARCHITECTURE.md section 12).
*/

#ifndef OPENFRENZY_CLASSLIB_UI_H
#define OPENFRENZY_CLASSLIB_UI_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "classlib_ui_data.h"

/* ---------- helpers for decompiled code */

/* FPATAN: atan(ST1 / ST0), as Ghidra writes fpatan(st1, st0) */
#define CLU_FPATAN(y, x) atan2((double)(y), (double)(x))

/* The game's debug print (bkernel 0x0004b230) takes a printf format. The table
prototype has one parameter; call it through a variadic pointer. */
#define CLU_DebugPrintf ((void (*)(const char *, ...))FUN_0004b230)

/* operator new of the game: bkHeapAlloc (bkernel 0x0004c480; the table prototype
returns void) */
#define CLU_Alloc(size, file, line, flags) \
	((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)((size), (file), (line), (flags))

/* MSVC rand(), inlined by the compiler into the game: holdrand is the C library's
seed at 0x00285630 (initial value 1, owned by the C runtime). */
extern unsigned int DAT_00285630;
static inline int CLU_Rand(void)
{
	DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
	return (int)((DAT_00285630 >> 16) & 0x7fff);
}

/* ---------- types */

/* a pre-transformed vertex of the 2D quads drawn with FUN_0003f2d0 (0x24 bytes) */
struct CLQuadVertex {
	float x, y, z;       /* 0x00 */
	float unused[3];     /* 0x0c */
	unsigned int colour; /* 0x18 ARGB */
	float u, v;          /* 0x1c */
};
OF_ASSERT_SIZE(struct CLQuadVertex, 0x24);

/* ---------- globals of other modules used here (tentative definitions) */

/* 0x002ba544  language folder name (char *), e.g. "Packages\English" (core) */
extern char *DAT_002ba544;
/* 0x002ba750  the game mode stack object (core; FUN_0002b5b0 is a method of it) */
extern unsigned char DAT_002ba750[0x58];
/* 0x00283d74  Welsh commentary selected (core) */
extern int DAT_00283d74;
/* 0x00283e0c  commentator lines delay by their length (core .data, 0) */
extern int DAT_00283e0c;

/* ---------- globals owned by classlib_ui that other modules use */

/* 0x002906f8  the six playable characters (Dub, Fly, Geena, Zak, Jade, Jet).
Other modules read single fields under Ghidra names: DAT_00290700 is
DAT_002906f8[0].portrait, DAT_00290718 is DAT_002906f8[0].colour, and so on
(field address = 0x002906f8 + 0x60 * character + offset). */
struct CLCharacterInfo {
	const char *name;          /* 0x00 also the voice folder: "Dub" */
	const unsigned short *display_name; /* 0x04 u"Dub" (UTF-16) */
	const char *portrait;      /* 0x08 "char_dub.bmp" */
	const char *select_image;  /* 0x0c "selchar_dub.bmp" */
	const char *bone_a;        /* 0x10 "lpalm" / "rpalm" */
	const char *bone_b;        /* 0x14 */
	const char *bone_shoulder; /* 0x18 "rshoulder" / "lshoulder" */
	const char *mallet_model;  /* 0x1c "malletdub.obe" */
	unsigned int colour;       /* 0x20 bytes r, g, b, 0 */
	float values[8];           /* 0x24 */
	int ints[7];               /* 0x44 */
};
OF_ASSERT_SIZE(struct CLCharacterInfo, 0x60);
extern const struct CLCharacterInfo DAT_002906f8[6];

/* 0x0028fda8  frame time scale: 1.0 at the nominal frame rate */
extern float DAT_0028fda8;
/* 0x0028ffa8 / 0x0028ffac  3D sample min / max distance */
extern float DAT_0028ffa8;
extern float DAT_0028ffac;
/* 0x0028ffb8  index of the music track that plays now, -1 for none */
extern int DAT_0028ffb8;

/* ---------- functions missing from of_functions.tsv (defined here, report to the architect) */

/* 0x00072060  draw callback of CLSprite_DrawBlurred (ClassLib/unplaced.c) */
int FUN_00072060(int pass, int *info);
/* 0x00074c10  draw function of the pause menu volume bars (ClassLib/COverlay.c) */
void FUN_00074c10(int self, int shadow);
/* 0x0007d6c0 / 0x0007e340  parse map callbacks of the intro camera ("Camera",
"AnimStream" blocks; ClassLib/CIntroCam.c) */
int FUN_0007d6c0(int node, undefined4 unused);
int FUN_0007e340(int node, undefined4 unused);
/* 0x0007d890 / 0x0007da30 / 0x0007db00  "FrNum" callbacks: position, roll, FOV keys */
int FUN_0007d890(int property, int *counter);
int FUN_0007da30(int property, int *counter);
int FUN_0007db00(int property, int *counter);
/* 0x0007fdb0  default burst callback of the fireworks (ClassLib/CFireworks.c) */
void FUN_0007fdb0(int fireworks, int rocket);
/* 0x00080bf0 / 0x00080c50  debris system callbacks (ClassLib/unplaced.c) */
int FUN_00080bf0(undefined4 system, int particle);
void FUN_00080c50(int system, int reason);

#endif
