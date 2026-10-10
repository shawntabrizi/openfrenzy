/*
CharLib/charlib_extern.c  (charlib)

Definitions of the globals of other modules that charlib code uses (declared in charlib.h
and charlib_extern.h), so the image links before their owners define them
(docs/ARCHITECTURE.md section 12):
- uninitialised (.bss) globals: tentative definitions; -fcommon merges them with the
  owner's definition (the largest size wins, so the size here covers what charlib touches);
- initialised (.data) globals: weak definitions with the value in orig/default.xbe; the
  owner's strong definition replaces them.
*/

#include "charlib.h"

/* .bss of other modules */
int DAT_002bae94;                 /* 0x002bae94 core: number of players */
int *DAT_002ea488;                /* 0x002ea488 ClassLib: the CharLib actor list */
unsigned int DAT_002bc490[256];   /* bdisplay colour tables */
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];
int DAT_002baed4[4 * 10];         /* 0x002baed4 core: player table, 0x28 bytes per player */
unsigned char DAT_002ed418[0xc];  /* 0x002ed418 classlib_fx: the CProjector list object */
int DAT_002baea4;                 /* 0x002baea4 core: the game is paused */
void *DAT_002ecfac;               /* 0x002ecfac classlib_ui: the HUD object */
void *DAT_002ecc74;               /* 0x002ecc74 classlib_ui */
unsigned char DAT_002ea478[0x10]; /* 0x002ea478 ClassLib: effect parameters */

/* .data of other modules. Weak and zero: the contents point at other data and are not
reproduced here; the owner's definition (bumpercars) replaces them. */
__attribute__((weak)) int DAT_00290e8c = 0;                  /* 0x00290e8c */
__attribute__((weak)) unsigned char DAT_00291734[0x2c];      /* 0x00291734 RESPAWNCHAR effect */
unsigned char DAT_002ba750[0x58]; /* 0x002ba750 core: the game mode stack */
/* carryon .data: weak until carryon defines them. 0x00293ad8 holds the CFire_Game object
(its first word is a vtable pointer, 0x00267468 in the XBE: not reproduced). */
__attribute__((weak)) unsigned char DAT_00293ad8[0xc];
__attribute__((weak)) void *DAT_00293ae4 = 0;
__attribute__((weak)) int DAT_00293b28 = 0;
int DAT_002ba99c;                 /* 0x002ba99c core: frame counter */
