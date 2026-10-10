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
int DAT_002d02ac;                 /* 0x002d02ac */
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
int DAT_002b95ac;                 /* 0x002b95ac core_modes_a */
void *DAT_002b9670;               /* 0x002b9670 core_modes_a */
int DAT_002b9c58, DAT_002b9c64;   /* core_modes_b */
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
__attribute__((weak)) int DAT_00281b0c[3] = { 0x3c, 0x3c, 0x3c };  /* 0x00281b0c core_modes_a */
__attribute__((weak)) int DAT_00281b18[3] = { 0xff, 0xff, 0xff };  /* 0x00281b18 core_modes_a */
__attribute__((weak)) float DAT_00292c84 = 4.0f;     /* 0x00292c84 (used by the tweak constructor) */

/* placeholders for the objects and vtables the exit functions of crt/static_init.c write */
__attribute__((weak)) unsigned char DAT_002ba7a4[4];  /* 0x002ba7a4 */
__attribute__((weak)) const void *const vtable_000[1] = { 0 };  /* 0x000 */
__attribute__((weak)) unsigned char DAT_00293ad8[4];  /* 0x00293ad8 */
__attribute__((weak)) const void *const vtable_00267468[1] = { 0 };  /* 0x00267468 */
__attribute__((weak)) unsigned char DAT_00296ee4[4];  /* 0x00296ee4 */
__attribute__((weak)) const void *const vtable_00269648[1] = { 0 };  /* 0x00269648 */
__attribute__((weak)) unsigned char DAT_00298da0[4];  /* 0x00298da0 */
__attribute__((weak)) const void *const vtable_0026c294[1] = { 0 };  /* 0x0026c294 */
__attribute__((weak)) unsigned char DAT_00298e70[4];  /* 0x00298e70 */
__attribute__((weak)) const void *const vtable_0026c2ec[1] = { 0 };  /* 0x0026c2ec */
__attribute__((weak)) unsigned char DAT_002a54d8[4];  /* 0x002a54d8 */
__attribute__((weak)) const void *const vtable_0026c310[1] = { 0 };  /* 0x0026c310 */
__attribute__((weak)) unsigned char DAT_002aa830[4];  /* 0x002aa830 */
__attribute__((weak)) const void *const vtable_0026c568[1] = { 0 };  /* 0x0026c568 */
__attribute__((weak)) unsigned char DAT_002aa900[4];  /* 0x002aa900 */
__attribute__((weak)) const void *const vtable_0026c5a0[1] = { 0 };  /* 0x0026c5a0 */
__attribute__((weak)) unsigned char DAT_002aaaf8[4];  /* 0x002aaaf8 */
__attribute__((weak)) const void *const vtable_0026cb20[1] = { 0 };  /* 0x0026cb20 */
__attribute__((weak)) unsigned char DAT_002aab08[4];  /* 0x002aab08 */
__attribute__((weak)) const void *const vtable_0026cb7c[1] = { 0 };  /* 0x0026cb7c */
__attribute__((weak)) unsigned char DAT_002aadf8[4];  /* 0x002aadf8 */
__attribute__((weak)) const void *const vtable_0026cbac[1] = { 0 };  /* 0x0026cbac */
__attribute__((weak)) unsigned char DAT_002aae60[4];  /* 0x002aae60 */
__attribute__((weak)) const void *const vtable_0026cc44[1] = { 0 };  /* 0x0026cc44 */
__attribute__((weak)) unsigned char DAT_002ae878[4];  /* 0x002ae878 */
__attribute__((weak)) const void *const vtable_0026e330[1] = { 0 };  /* 0x0026e330 */
__attribute__((weak)) unsigned char DAT_002ae948[4];  /* 0x002ae948 */
__attribute__((weak)) const void *const vtable_0026e510[1] = { 0 };  /* 0x0026e510 */
__attribute__((weak)) unsigned char DAT_002aebd8[4];  /* 0x002aebd8 */
__attribute__((weak)) const void *const vtable_0026edb8[1] = { 0 };  /* 0x0026edb8 */
__attribute__((weak)) unsigned char DAT_002aec10[4];  /* 0x002aec10 */
__attribute__((weak)) const void *const vtable_0026edd4[1] = { 0 };  /* 0x0026edd4 */
__attribute__((weak)) unsigned char DAT_002afe80[4];  /* 0x002afe80 */
__attribute__((weak)) const void *const vtable_0026eeb4[1] = { 0 };  /* 0x0026eeb4 */
__attribute__((weak)) unsigned char DAT_002affa0[4];  /* 0x002affa0 */
__attribute__((weak)) const void *const vtable_0026eeb8[1] = { 0 };  /* 0x0026eeb8 */
__attribute__((weak)) unsigned char DAT_002b0038[4];  /* 0x002b0038 */
__attribute__((weak)) const void *const vtable_0026f038[1] = { 0 };  /* 0x0026f038 */
__attribute__((weak)) unsigned char DAT_002b0378[4];  /* 0x002b0378 */
__attribute__((weak)) const void *const vtable_0026f05c[1] = { 0 };  /* 0x0026f05c */
