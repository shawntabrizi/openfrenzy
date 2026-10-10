/*
CORE_FOREIGN.C

openfrenzy core module. Definitions of globals that belong to other modules but that core
code uses (declared in core_globals.h).

- Uninitialised (.bss) globals: tentative definitions. -fcommon merges them with the
  owner's definition; the largest size wins, so the size here covers what core touches.
- Initialised (.data) globals: weak definitions with the value in orig/default.xbe, so the
  image links before the owner defines them. The owner's (strong) definition replaces them.
*/

#include <stdint.h>

/* .bss of other modules */
int DAT_002ecdec;                 /* 0x002ecdec */
int DAT_002cb550;                 /* 0x002cb550 */
void *DAT_002ecc74;               /* 0x002ecc74 */
int DAT_002db824;                 /* 0x002db824 */
void *DAT_002db814[2];            /* 0x002db814 */
unsigned char DAT_002d08a8[0xc00]; /* 0x002d08a8 LAUNCH_DATA */
int DAT_002d0750;                 /* 0x002d0750 */
uint64_t DAT_002cd7a0;            /* 0x002cd7a0 */
uint32_t DAT_002be330[16];        /* 0x002be330 */
int DAT_002be57c;                 /* 0x002be57c */
float DAT_002be588;               /* 0x002be588 */
float DAT_002be58c;               /* 0x002be58c */
float DAT_002ecde8;               /* 0x002ecde8 */
unsigned char DAT_002ecd90[0x5c]; /* 0x002ecd90 (object; up to 0x002ecdec) */
int DAT_002b9718;                 /* 0x002b9718 core_modes_a: test map */
int DAT_002ed340;                 /* 0x002ed340 classlib_fx: debris off */
void *DAT_002b95cc, *DAT_002b95d0, *DAT_002b9c78;  /* core_modes_a/b mode objects */
uint32_t DAT_002bc898[256], DAT_002bd640[256], DAT_002bc490[256], DAT_002bce18[256];  /* bdisplay */

/* .data of other modules (XBE values) */
__attribute__((weak)) float DAT_0028fda8 = 1.0f;     /* 0x0028fda8 */
__attribute__((weak)) float DAT_002909b0 = 3600.0f;  /* 0x002909b0 (0x45610000) */
__attribute__((weak)) int DAT_0028198c = -1;         /* 0x0028198c */
/* 0x002855f0: identity matrix */
__attribute__((weak)) uint32_t DAT_002855f0[16] = {
	0x3f800000, 0, 0, 0, 0, 0x3f800000, 0, 0, 0, 0, 0x3f800000, 0, 0, 0, 0, 0x3f800000,
};
__attribute__((weak)) uint32_t DAT_00285630 = 1;    /* 0x00285630 libc rand seed */
__attribute__((weak)) float DAT_00292c84 = 4.0f;     /* 0x00292c84 (used by the tweak constructor) */
