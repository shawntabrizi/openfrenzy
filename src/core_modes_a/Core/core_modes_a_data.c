/*
Core/core_modes_a_data.c  (module core_modes_a)

Globals the module uses, by their Ghidra names (docs/ARCHITECTURE.md section 12):

- Globals only this module uses: defined here (the .data ones with their XBE value).
- Uninitialised globals shared with other modules: tentative definitions. -fcommon merges
  them with the other modules' (and an owner's block alias wins over them).
- Initialised globals of other modules that have no owner definition yet: weak
  definitions with the XBE value; the owner's definition replaces them.
- Globals in the data blocks of module core (0x002839b8-0x0028450c, 0x002ba4f0-0x002bb138)
  and classlib_ui (0x0028fda8-0x00290cc0, 0x002ea490-0x002ed33c) are defined there under
  every Ghidra name: they are only declared (extern) in the .c files of this module.
*/

#include <stdint.h>

/* ---------- this module (.data) */

int DAT_00281990 = -1;                              /* 0x00281990 the last attract-mode game */

/* ---------- this module (.bss), also used by core / core_modes_b */

void *DAT_002b95d0;                                 /* 0x002b95d0 the BGEffects mode */
int DAT_002b95ac;                                   /* 0x002b95ac CharacterSel package loaded */
int DAT_002b95b0[6];                                /* 0x002b95b0 voice packages to load */
char DAT_002b95c8;                                  /* 0x002b95c8 */
char DAT_002b95d4;                                  /* 0x002b95d4 */
int DAT_002b9670;                                   /* 0x002b9670 the CharacterSel package */

/* ---------- other modules (.bss) */

uint32_t DAT_002bce18[256];                         /* bdisplay: colour tables */
uint32_t DAT_002bc490[256];
uint32_t DAT_002bd640[256];
uint32_t DAT_002bc898[256];
long long DAT_002cd7a0;                             /* bkernel: tick frequency */
char DAT_002d0758[0x150];                           /* bkernel: disc root path */
int DAT_002d02ac;                                   /* 0x002d02ac */
void *DAT_002db808;                                 /* engine_misc: DirectSound */

/* ---------- other modules (.data, XBE values) */

__attribute__((weak)) unsigned int DAT_00285630 = 1;   /* rand() seed */
__attribute__((weak)) float DAT_002855f0[16] = {       /* identity matrix */
	1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
	0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
};
__attribute__((weak)) int DAT_0028198c = -1;           /* 0x0028198c */
__attribute__((weak)) int DAT_002838f4 = -1;           /* 0x002838f4 */
__attribute__((weak)) int DAT_002838f8 = -1;           /* 0x002838f8 */
__attribute__((weak)) float DAT_00283760 = 1800.0f;    /* 0x00283760 */
__attribute__((weak)) int DAT_00292c4c = 0x18;         /* 0x00292c4c */
__attribute__((weak)) int DAT_00292c50 = 0x18;         /* 0x00292c50 */
__attribute__((weak)) int DAT_00292c78 = 0x240;        /* 0x00292c78 */

/* ---------- names inside the data blocks of module core (src/core/core_data.h)

   Core defines every Ghidra name of its blocks as an alias into the block (a defined symbol),
   which replaces the tentative / weak definitions below. They exist so that the image links
   even when core's generated alias list lacks a name this module uses; such a name would then
   be a separate zero object (reported to core). */

/* .bss block 0x002ba4f0-0x002bb138 */
unsigned char DAT_002ba74c[0x5c];                   /* 0x002ba74c, the mode stack at +4 */
int DAT_002ba544, DAT_002ba554, DAT_002ba558, DAT_002ba55c, DAT_002ba564;
int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c;
void *DAT_002ba954[13];                             /* the games */
float _DAT_002ba98c;                                /* refresh rate */
int DAT_002ba99c;                                   /* frame count */
int DAT_002ba9a4, DAT_002ba9a8, DAT_002ba9ac, DAT_002ba9b0;
float _DAT_002ba9c8;                                /* demo timer */
int DAT_002baadc, DAT_002baae8, DAT_002baaec;
int DAT_002bae6c, DAT_002bae70, DAT_002bae8c, DAT_002bae90, DAT_002bae94, DAT_002bae98;
int DAT_002bae9c, DAT_002baeac, DAT_002baeb0, DAT_002baeb8, DAT_002baebc;
int DAT_002baed0[40];                               /* 4 players of 0x28 bytes */
void *DAT_002baf78[36];                             /* 4 x 9 player input objects */

/* .data block 0x002839b8-0x0028450c (XBE values) */
__attribute__((weak)) int DAT_002844cc = 0;
__attribute__((weak)) int DAT_002844d0 = 0;
__attribute__((weak)) int DAT_002844d4 = 0x14;
__attribute__((weak)) int DAT_002844d8 = 0xff;
__attribute__((weak)) int DAT_00284408 = -1;
/* 0x002839e0: [game * 4 + variation]; the first four entries are string pointers in the XBE
   (never 0 or +-3), here 1 */
#define CMA_VARIATION_TABLE \
	1, 1, 1, 1, 1, -2, -2, -1, -1, 2, 2, 2, 2, -1, -2, 0, -1, 2, 2, 1, 2, 1, 1, 1, 1, -3, -2, -1, \
	-1, 1, 1, 1, 1, -2, -1, -1, -1, 2, 2, 0, 1, 1, 1, 1, 1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 1
__attribute__((weak)) int PTR_s_packages_english_002839e0[56] = { CMA_VARIATION_TABLE };
__attribute__((weak)) int PTR_s_packages_english_002839e4[55] = { 1, 1, 1, 1, -2, -2, -1, -1,
	2, 2, 2, 2, -1, -2, 0, -1, 2, 2, 1, 2, 1, 1, 1, 1, -3, -2, -1, -1, 1, 1, 1, 1, -2, -1, -1,
	-1, 2, 2, 0, 1, 1, 1, 1, 1, -1, -1, -1, 0, -1, -1, -1, -1, 0, 0, 1 };
