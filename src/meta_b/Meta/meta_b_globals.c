/*
Meta/meta_b_globals.c

Weak fallback definitions (original size, XBE value) of globals that the meta_b module
uses but other modules own. An owner's strong definition replaces them at link time.
The globals that meta_b owns are defined next to their code.
*/

#include "meta_b.h"
#include "meta_b_internal.h"

/* ---------- .data (weak, XBE values) */

__attribute__((weak)) unsigned int DAT_00285630 = 1;    /* rand() seed */
__attribute__((weak)) float DAT_0028fda8 = 1.0f;        /* frame time factor */
__attribute__((weak)) char DAT_002855f0[64] = {         /* identity matrix (float[16]) */
	0, 0, 0x80, 0x3f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0x80, 0x3f, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0x3f, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x80, 0x3f,
};

/* 0x002b0a4c-0x002b0a60  Meta tweakables (owner not known yet: meta_a or meta_b) */
__attribute__((weak)) float DAT_002b0a4c = 24.0f;
__attribute__((weak)) int DAT_002b0a50 = 10;
__attribute__((weak)) float DAT_002b0a54 = 120.0f;
__attribute__((weak)) float DAT_002b0a58 = 1.0f;
__attribute__((weak)) float DAT_002b0a5c = 1.0f;
__attribute__((weak)) float DAT_002b0a60 = 100.0f;

/* 0x00290dc8  the spiral beam textures (classlib_fx; 2 x 0xc bytes) */
__attribute__((weak)) char DAT_00290dc8[0x18];
/* 0x002b1ef4  the "SONIC_RING" particle type (the particle system's data) */
__attribute__((weak)) char DAT_002b1ef4[4];

__attribute__((weak)) float DAT_0028fdc8[3] = { 0.0f, 1.0f, 0.0f };
__attribute__((weak)) float DAT_0028fdd8[3] = { 0.0f, 0.0f, 1.0f };
__attribute__((weak)) float DAT_00292c84 = 4.0f;       /* gravity */
__attribute__((weak)) int DAT_00290d80[4] = { 0x7f, 0x6e, 0x28, 0x7f };  /* trail colours */
__attribute__((weak)) int DAT_00290d90[4] = { 0x3c, 0, 0, 0x28 };
/* 0x002b1ec8 "REDDOT", 0x002b215c "STREAK3": particle types */
__attribute__((weak)) char DAT_002b1ec8[4];
__attribute__((weak)) char DAT_002b215c[4];

/* ---------- .bss (weak) */

__attribute__((weak)) int DAT_002bae94;                 /* number of players */
__attribute__((weak)) int DAT_003055d0;                 /* display settings object */
__attribute__((weak)) int DAT_0030564c;                 /* game state object */
__attribute__((weak)) int DAT_003075bc[4];              /* meta players */
__attribute__((weak)) unsigned int DAT_002bc490[256];   /* colour tables (bdisplay) */
__attribute__((weak)) unsigned int DAT_002bd640[256];
__attribute__((weak)) unsigned int DAT_002bc898[256];
__attribute__((weak)) unsigned int DAT_002bce18[256];
__attribute__((weak)) long long DAT_002cd7a0;           /* timer ticks per second */
__attribute__((weak)) int DAT_002ecdec;                 /* the game is paused */
__attribute__((weak)) float DAT_002ea478[4];            /* a zero vector */
__attribute__((weak)) unsigned char DAT_002ed324[0x14]; /* the rumble manager */
__attribute__((weak)) int DAT_00290da4;                 /* CProjTrail texture */
