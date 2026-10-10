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
