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
