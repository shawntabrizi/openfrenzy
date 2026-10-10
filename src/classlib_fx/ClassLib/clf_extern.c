/*
ClassLib/clf_extern.c  (classlib_fx)

Tentative definitions (-fcommon) of other modules' globals that classlib_fx uses, so the
image links before their owners define them. An owner's definition (or data-block alias)
replaces these at link time (docs/ARCHITECTURE.md section 12). Types follow the owners
where known.
*/

#include "classlib_fx.h"

float DAT_002855f0[16];  /* 0x002855f0 identity matrix (bdisplay) */
int DAT_002916dc[1];     /* 0x002916dc sparkle system of the spiral beams (bumpercars_a) */
int DAT_00291708[1];     /* 0x00291708 "STREAK2" effect (bumpercars_a) */
byte DAT_00290718[1];    /* 0x00290718 player colours, 0x60 bytes per player (classlib_ui) */
float DAT_002ea478[4];   /* 0x002ea478 zero velocity for sparkles */
int DAT_002ea4f4;        /* 0x002ea4f4 a looping sample handle (classlib_ui) */
int DAT_002ecdec;        /* 0x002ecdec the game is paused (classlib_ui) */
int DAT_002baeb8;        /* 0x002baeb8 this game mode has no RefBot (core) */
int DAT_003055e0;        /* 0x003055e0 a display flag (bdisplay/core) */
int DAT_002ed340;        /* 0x002ed340 debris disabled (DebrisSys, bumpercars_a) */
int DAT_002ed344;        /* 0x002ed344 live actor debris count (DebrisSys, bumpercars_a) */
unsigned int DAT_002bce18[256]; /* 0x002bce18 colour tables (bdisplay) */
unsigned int DAT_002bc490[256]; /* 0x002bc490 */
unsigned int DAT_002bd640[256]; /* 0x002bd640 */
unsigned int DAT_002bc898[256]; /* 0x002bc898 */
int DAT_00284b10;               /* 0x00284b10 ambient light (bactor; initial 30) */
int DAT_00284b14;               /* 0x00284b14 */
int DAT_00284b18;               /* 0x00284b18 */
int DAT_002ba99c;               /* 0x002ba99c game flags (core) */
int DAT_002bc48c;               /* 0x002bc48c triangle counter (bdisplay) */
int DAT_00302210[4];            /* 0x00302210 Fox Hunt: the AI of each player (foxhunt) */
int DAT_003022c8;               /* 0x003022c8 Fox Hunt: speech (foxhunt) */
