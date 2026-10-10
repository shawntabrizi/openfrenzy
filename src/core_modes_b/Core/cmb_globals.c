/*
CMB_GLOBALS.C

Globals that core_modes_b code uses, under the names Ghidra gives them (DAT_<address>, or
_DAT_<address> where Ghidra used that spelling; other modules alias exactly those names).

Every definition here is a fallback, never a strong definition:

- .data globals: weak definitions with the XBE value.
- .bss globals: tentative definitions (common symbols, -fcommon).

A module that owns the memory (core's data blocks in src/core/crt/core_data.c alias
DAT_ names into one block with the original layout) replaces them at link time, so this
module's code then reads the owner's memory. Until then the image still links and the
values are the original ones.
*/

#include "core_modes_b.h"

/* ---------- .data (weak, XBE values) */

__attribute__((weak)) float DAT_00283760 = 1800.0f;   /* idle frames before the attract mode */
__attribute__((weak)) int DAT_00283768[8] = {         /* level meter colour channels */
	0xff, 0xff, 0xff, 0xe6, 0xff, 0xff, 0xff, 0x5a,
};
__attribute__((weak)) int DAT_00284408 = -1;
__attribute__((weak)) int DAT_002844a0 = 2;
__attribute__((weak)) int DAT_002844dc[3] = { 0x60, 0x60, 0x60 }; /* menu box colour */
__attribute__((weak)) float DAT_0028fda8 = 1.0f;      /* frame time factor */
__attribute__((weak)) int DAT_0028ffb8 = -1;          /* current soundtrack */

/* ---------- .bss (tentative) */

int DAT_002b95cc;                 /* attract-mode game mode */
int DAT_002b95d0;                 /* the BG_Effects game mode */
struct CScoreTable DAT_002b9968;  /* the score table */
int DAT_002b9c78;                 /* the options game mode */
struct cmb_meter_vertex DAT_002b9c80[10][4];
int DAT_002ba550;                 /* party game mode */
int DAT_002ba554;                 /* tournament game mode */
int DAT_002ba55c;                 /* tournament data */
int DAT_002ba570[0x31];
float _DAT_002ba9c8;
int DAT_002baad8;
int DAT_002baae8;
int DAT_002bae68;                 /* music volume 0-255 */
int DAT_002bae6c;                 /* sound effect volume 0-255 */
int DAT_002bae74;
int DAT_002bae7c;                 /* vibration on */
int DAT_002bae84;                 /* soundtrack database (custom soundtracks) */
int DAT_002bae88;
int DAT_002bae94;                 /* number of players */
int DAT_002bae98;                 /* 1: tournament */
int DAT_002bae9c;
int DAT_002baed8[4][10];          /* player records */
int DAT_002baf78[4][9];           /* per player, per button: input objects */
unsigned int DAT_002bc490[256];   /* colour channel tables */
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];
int DAT_002ecc74;                 /* text database */
int DAT_002ecdec;

/* the core module's .bss block (src/core/crt/core_data.c defines it) */
__attribute__((weak, aligned(16))) unsigned char of_core_bss[0xc48];

int DAT_002ecfac;                 /* (classlib_ui) +0x90: rumble strength */
unsigned char DAT_002ed324[4];    /* CRumble object (classlib_ui) */
