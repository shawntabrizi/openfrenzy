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
__attribute__((weak)) int DAT_002844a0[3] = { 2, 4, 6 }; /* rounds per game length */
__attribute__((weak)) int DAT_002844dc[3] = { 0x60, 0x60, 0x60 }; /* menu box colour */
__attribute__((weak)) float DAT_0028fda8 = 1.0f;      /* frame time factor */
__attribute__((weak)) int DAT_0028ffb8 = -1;          /* current soundtrack */

/* ---------- .bss (tentative) */

int DAT_002b95cc;                 /* attract-mode game mode */
int DAT_002b95d0;                 /* the BG_Effects game mode */
/* the score table: the one strong definition here, so that the names of its fields that other
modules use (rank order 0x002b9c58[4] and its last entry 0x002b9c64, places 0x002b9c48[4], ties
0x002b9c68[4]) can alias into it. As separate common symbols they were separate storage that
CScoreTable_Rank never wrote: BumperCars started directly indexed its players with garbage
(minigame launcher triage, 2026-10-10). */
__attribute__((nocommon)) struct CScoreTable DAT_002b9968;
__asm__(
	".globl DAT_002b9c48\n.set DAT_002b9c48, DAT_002b9968 + 0x2e0\n"  /* place[4] */
	".globl DAT_002b9c58\n.set DAT_002b9c58, DAT_002b9968 + 0x2f0\n"  /* by_place[4] */
	".globl DAT_002b9c64\n.set DAT_002b9c64, DAT_002b9968 + 0x2fc\n"  /* by_place[3] */
	".globl DAT_002b9c68\n.set DAT_002b9c68, DAT_002b9968 + 0x300\n"  /* ties[4] */
);
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
int DAT_002baed4[4][10];          /* player records */
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

/* used by CGameMode_Map */
__attribute__((weak)) int DAT_00283ab8[21] = {
	0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2,
};
__attribute__((weak)) int DAT_00283d74 = 0;
__attribute__((weak)) int DAT_002844ac[6] = { 5, 4, 3, 2, 1, 0 };
int DAT_002bb020;
int DAT_002ba560;
int DAT_002ba63c[7];
int DAT_002ba658[21];
int DAT_002baeac;
int DAT_002baeb0;
int DAT_002baeb4;
int DAT_002baebc;
int DAT_002bae8c;
unsigned char DAT_002be370[0x40];
unsigned int DAT_002bc40c;
unsigned int DAT_002bc410;
__attribute__((weak)) unsigned int DAT_00285630 = 1;   /* rand() seed */

/* used by CGameMode_MetaVote */
int DAT_002ba228;                 /* the vote screen's background effect */
__attribute__((weak)) int DAT_0029074c[0x49] = { [0x00] = 10, [0x18] = 10, [0x30] = 20, [0x48] = 30 };
int DAT_003055e0;
int DAT_002ba99c;
float DAT_002be580;
float DAT_002be584;
float DAT_002be588;
float DAT_002be58c;
__attribute__((weak)) float DAT_002838c8 = 0.3f;    /* chip gravity */
__attribute__((weak)) float DAT_002838cc = -0.35f;  /* bounce */
__attribute__((weak)) float DAT_002838d0 = 0.08f;   /* stretch per speed */
__attribute__((weak)) float DAT_002838d4 = 0.16f;   /* chip scale */
__attribute__((weak)) int DAT_002844fc[3] = { 0xff, 0xff, 0xff };
__attribute__((weak)) float DAT_00283888[4] = { -1.0f, 1.0f, -1.0f, 1.0f };

/* used by CGameMode_MiniGameIntro */
int DAT_002ba220;
int DAT_002ba224;
int DAT_002baed0;
int DAT_002baeb8;
int DAT_002baea8;
int DAT_002ba6a4;
int DAT_002ba4e0;
int DAT_002f28ac;
int DAT_002bae90;
int DAT_002ecf90[4];
int DAT_002b95b0[6];
int _DAT_002ecd9c;
int _DAT_002ecda0;
int _DAT_002ecda4;
int _DAT_002ecda8;
int _DAT_002ecde8;
__attribute__((weak)) int DAT_00290960 = 1;
__attribute__((weak)) float DAT_00290be4[4] = { 50.0f, 50.0f, 50.0f, 50.0f };
__attribute__((weak)) int DAT_00290754[0x79];       /* classlib_ui owns it; zero in the XBE */
__attribute__((weak)) int DAT_00292c4c = 0x18;
__attribute__((weak)) int DAT_00292c50 = 0x18;
__attribute__((weak)) int DAT_00292c78 = 0x240;
__attribute__((weak)) int DAT_002838e4[3] = { 90, -25, -125 };
__attribute__((weak)) int DAT_002838f0 = 30;

/* used by CGameMode_MiniGameSelect */
struct CMiniGameEntry DAT_002ba238[0x30];
int DAT_002ba4dc;
int DAT_002ba4d0;
float _DAT_002ba4c0;
int DAT_002ba4d4;
int DAT_002ba4d8;
int DAT_002ba4c4;
int DAT_002ba230;
float _DAT_002ba4c8;
int _DAT_002ba4cc;
int DAT_002ba4e4;
void *DAT_002ba954[13];
__attribute__((weak)) int DAT_002839f0[0x31] = {
	1, -2, -2, -1, -1, 2, 2, 2, 2, -1, -2, 0, -1, 2, 2, 1, 2, 1, 1, 1, 1, -3, -2, -1, -1,
	1, 1, 1, 1, -2, -1, -1, -1, 2, 2, 0, 1, 1, 1, 1, 1, -1, -1, -1, 0, -1, -1, -1, -1,
};
__attribute__((weak)) int DAT_002838f4 = -1;
__attribute__((weak)) int DAT_002838f8 = -1;
__attribute__((weak)) int _DAT_0028198c = -1;
__attribute__((weak)) int DAT_00283984 = 5;
__attribute__((weak)) float _DAT_00283988 = 0.15f;
__attribute__((weak)) int DAT_0028452c[4] = { 0xff, 0xff, 0xff, 0xdc };

/* used by CGameMode_ObjectViewer and CGameMode_PressStart */
void *DAT_002ba4e8;
void *DAT_002ba4ec;
float DAT_002ea478[4];
__attribute__((weak)) int DAT_0028398c = 1;
__attribute__((weak)) float _DAT_00283990 = 3.05250319e-06f;
__attribute__((weak)) float _DAT_00283994 = 3.05250319e-06f;
__attribute__((weak)) float _DAT_00283998 = 3.05250319e-06f;
__attribute__((weak)) float _DAT_0028399c = 0.000305250316f;
__attribute__((weak)) float _DAT_002839a0 = 6.10500647e-05f;
__attribute__((weak)) float _DAT_002839a4 = 0.002f;
__attribute__((weak)) float _DAT_002839a8 = -0.01f;
__attribute__((weak)) float DAT_002839ac = 0.87266463f;
__attribute__((weak)) int _DAT_002839b0 = 1;
__attribute__((weak)) float DAT_0028fdd8[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
int DAT_002baadc;
char DAT_002ba538;
int DAT_002ba940;
int DAT_002ba944;
int DAT_002ba948;
int DAT_002ba94c;
