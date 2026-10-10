/*
ClassLib/classlib_ui_extern.c  (classlib_ui)

Weak fallbacks for globals of other modules that classlib_ui code uses and that no
module defines yet (checked by linking the game image without this file). A strong
definition or alias in the owner replaces each of these at link time.

Only weak definitions here: a tentative (common) definition would take precedence over
another module's weak fallback with real values (lld prefers a common symbol to a weak
definition), e.g. the identity matrix at 0x002855f0 or the rand() seed at 0x00285630.
All other globals of other modules are declared extern where they are used
(classlib_ui.h and the .c files) and are defined by their owners.
*/

#include "classlib_ui.h"

/* 0x00283898  core .data: results images of the characters */
__attribute__((weak)) const char *DAT_00283898[6] = {
	"dub_g.bmp", "fly_g.bmp", "geena_g.bmp", "zak_g.bmp", "jade_g.bmp", "jet_g.bmp",
};

__attribute__((weak)) int DAT_002b9c48[4];         /* core .bss: players out of the results */
__attribute__((weak)) int DAT_002b9c68;            /* core .bss: number of result boxes */
__attribute__((weak)) unsigned int DAT_002ba4cc;   /* core .bss */
__attribute__((weak)) unsigned int DAT_002bc960;   /* bdisplay .bss: alpha component of the ground quad */
__attribute__((weak)) unsigned int DAT_002bca94;   /* bdisplay .bss: alpha component of the AI ring */
__attribute__((weak)) unsigned int DAT_002bcc94[256]; /* bdisplay .bss: colour table */
__attribute__((weak)) int DAT_002ed348;            /* debris system (bumpercars_a / classlib_fx) */
__attribute__((weak)) int DAT_002ed34c;            /* debris system */
__attribute__((weak)) float DAT_002ed350[16];      /* per-player world positions (4 x xyzw) */
