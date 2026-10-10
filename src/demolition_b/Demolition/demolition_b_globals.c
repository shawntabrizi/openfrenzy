/*
Demolition/demolition_b_globals.c

Weak fallback definitions (original size, XBE value) of globals that the demolition_b module
uses but other modules own. An owner's strong definition replaces them at link time.
The globals that demolition_b owns are defined next to their code.
*/

#include "demolition_b.h"

/* ---------- .data (weak, XBE values) */

__attribute__((weak)) unsigned int DAT_00285630 = 1;    /* rand() seed */
__attribute__((weak)) float DAT_0028fda8 = 1.0f;        /* frame time factor */
__attribute__((weak)) int DAT_00290750;                 /* the characters' table (classlib_ui) */
__attribute__((weak)) int DAT_00297520[60];             /* variation settings (demolition_a; zeros
                                                           here, the owner's values replace them) */

/* ---------- .bss (weak) */

__attribute__((weak)) char *DAT_002ff85c;               /* the Demolition game (demolition_a) */
__attribute__((weak)) int DAT_002bae70;                 /* difficulty */
__attribute__((weak)) int DAT_002bae94;                 /* number of players */
__attribute__((weak)) int DAT_002baed4[40];             /* player records (core) */
__attribute__((weak)) unsigned int DAT_002bce18[256];   /* colour channel tables (bdisplay) */
__attribute__((weak)) unsigned int DAT_002bc490[256];
__attribute__((weak)) unsigned int DAT_002bd640[256];
__attribute__((weak)) unsigned int DAT_002bc998;
__attribute__((weak)) int DAT_002f289c[4];              /* the players' CharLib AI objects */
__attribute__((weak)) int DAT_002baeb8;                 /* demo / no intro (core) */
__attribute__((weak)) int DAT_002ecfac;                 /* the HUD (classlib_ui) */
