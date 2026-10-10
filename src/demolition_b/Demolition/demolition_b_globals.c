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
__attribute__((weak)) int DAT_00283ea4;                 /* the colour set (core) */
__attribute__((weak)) char DAT_00290738[6 * 0x60];      /* the characters (classlib_ui; zeros here) */
__attribute__((weak)) int DAT_00292c78 = 0x240;         /* route grid cells (charlib) */
__attribute__((weak)) int DAT_00293928 = 0xc8;          /* AI voice volume (charlib) */
__attribute__((weak)) float DAT_002b05a4 = 5.0f;        /* hurt animation speed (charlib) */
__attribute__((weak)) float DAT_002b05a8 = 1.5f;        /* knockout animation speed (charlib) */
__attribute__((weak)) int DAT_00297520[60];             /* variation settings (demolition_a; zeros
                                                           here, the owner's values replace them) */

/* ---------- CharLib / meta_a globals the player's states read (weak, XBE values) */
__attribute__((weak)) float DAT_0028fdd8[4] = { 0.0f, 0.0f, 1.0f, 1.0f }; /* (0, 0, 1, 1) */
__attribute__((weak)) char DAT_00290728[6 * 0x60 + 0x10]; /* the characters (classlib_ui; zeros here) */
__attribute__((weak)) float DAT_00292c98 = 50.0f;      /* jump speed */
__attribute__((weak)) float DAT_00293a38 = 1.5f;       /* reach scale with the power-up */
__attribute__((weak)) float DAT_002b059c = 10.0f;      /* walk -> run speed */
__attribute__((weak)) float DAT_002b05a0 = 8.0f;       /* run -> walk speed */
__attribute__((weak)) float DAT_002b074c = 180.0f;     /* attack angle */
__attribute__((weak)) float DAT_002b0750 = 0.2f;       /* velocity kept when stopping */
__attribute__((weak)) float DAT_002b0768 = 200.0f;     /* stand -> idle time */
__attribute__((weak)) float DAT_002b076c = 200.0f;     /* idle -> stand time */
__attribute__((weak)) int DAT_002b0770 = 0x400;
__attribute__((weak)) int DAT_002b0774 = 0x200;
__attribute__((weak)) float DAT_002b0778 = 0.02f;
__attribute__((weak)) float DAT_002b077c = -0.02f;
__attribute__((weak)) int DAT_002b0780 = 0x14;
__attribute__((weak)) float DAT_002b078c[4] = { 0.1f, 0.1f, 0.4f, 0.1f };  /* attack blend speeds */
__attribute__((weak)) float DAT_002b079c[4] = { 1.7f, 1.7f, 1.7f, 1.0f };  /* attack animation speeds */
__attribute__((weak)) float DAT_00293920 = 1.0f;      /* global animation speed factor */
__attribute__((weak)) float DAT_002b0784 = 14.0f;      /* attack step speed */
__attribute__((weak)) float DAT_002b0788 = 5.0f;       /* attack step time */
__attribute__((weak)) float DAT_002b07ac = 12.0f;      /* jump attack speed */
__attribute__((weak)) float DAT_002b07b0 = 350.0f;     /* jump attack range */
__attribute__((weak)) float DAT_002b07bc = 20.0f;      /* knockout slide time */
__attribute__((weak)) float DAT_002b07c0 = 5.0f;       /* knockout slide speed */
__attribute__((weak)) int DAT_002ba56c;                /* the debug free camera is on */
__attribute__((weak)) float DAT_00305654;
__attribute__((weak)) float DAT_002f2890;
__attribute__((weak)) float DAT_002f2894;
__attribute__((weak)) int DAT_00305650;

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
__attribute__((weak)) int DAT_002baebc;                 /* team mode (core) */
__attribute__((weak)) unsigned char DAT_002ed324[0x100]; /* the rumble manager (classlib_ui) */
