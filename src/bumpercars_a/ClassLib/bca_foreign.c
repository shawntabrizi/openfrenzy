/*
ClassLib/bca_foreign.c  (module bumpercars_a)

Weak fallbacks for data that the debris code uses but other modules own. A strong
definition (or alias) in the owner replaces each of these at link time.
*/

#include "bumpercars_a.h"

/* 0x00290ea8  (classlib_fx SpriteVB.cpp data) "debrisVB": a CSpriteVB whose batch list is
   empty (next and prev point at the object). Same fallback as meta_a's. */
__attribute__((weak, aligned(4))) unsigned int DAT_00290ea8[15] = {
	(unsigned int)DAT_00290ea8, (unsigned int)DAT_00290ea8,
};

/* 0x00290ed8  (classlib_fx data, after debrisVB) debris drawing on: 0 in the XBE */
__attribute__((weak)) int DAT_00290ed8 = 0;

/* 0x00290cb4 (25.0, debris sprite size scale) and 0x00290cb8 (1, debris sprites on) lie in
   classlib_ui's block of_clu_data (0x0028fda8-0x00290cc0), which names no alias for them:
   weak aliases into that block. */
__asm__(
	".weak DAT_00290cb4\n.set DAT_00290cb4, of_clu_data + 0xf0c\n"
	".weak DAT_00290cb8\n.set DAT_00290cb8, of_clu_data + 0xf10\n"
);

/* ---------- BumperCars globals after the game object (0x002f2048-): bumpercars_b's .bss
   block of_bcb_bss names them; these tentative definitions (-fcommon) only keep the image
   linking without it. Same types as the users here. */
int DAT_002f2048;           /* engine minimum frequency change (CBCar_Debug) */
int DAT_002f204c;           /* engine maximum volume */
int DAT_002f2050;           /* engine minimum volume */
int DAT_002f2054;           /* engine frequency change */
int DAT_002f2058;           /* engine turning volume */
float DAT_002f2068[4][5];   /* start positions: x, y, z, w, heading */
float DAT_002f2170[4];      /* computer drivers' speed per player */
void *DAT_002f26c4;         /* the camera path editor (bumpercars_b 0x000a5ca0) */
float DAT_002f2028;         /* frames to the crowd cheer */
int DAT_002f202c;           /* winners shown */
float DAT_002f205c;         /* the winner view's last time */
int DAT_002f2060;           /* the player the winner view shows */
