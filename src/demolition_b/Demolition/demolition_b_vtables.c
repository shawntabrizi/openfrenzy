/*
Demolition/demolition_b_vtables.c

The vtables of the demolition_b classes, rebuilt from the XBE with
`python3 -I scripts/xbe_data.py vtable ADDR N` (slot comments name the owning module).
Slots of 0x001e0323 (the CRT's _purecall: pure virtual) are 0.
*/

#include "demolition_b.h"

/* CDemolition_FragBomb */
/* 0x0026ae18 vtable */
const void *const vtable_0026ae18[] = {
	(const void *)FUN_000f2280, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f0fa0, /* [1] +0x04 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f51c0, /* [2] +0x08 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1a50, /* [3] +0x0c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f0fd0, /* [4] +0x10 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_RemoteBomb (message receiver at +0x1130) */
/* 0x0026ae2c vtable */
const void *const vtable_0026ae2c[] = {
	(const void *)FUN_000f21f0, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_RemoteBomb */
/* 0x0026ae30 vtable */
const void *const vtable_0026ae30[] = {
	(const void *)FUN_001d5950, /* [0] +0x00 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [1] +0x04 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [2] +0x08 tankedup_b TankedUp */
	(const void *)FUN_000b1860, /* [3] +0x0c charlib CharLib */
	(const void *)FUN_00151650, /* [4] +0x10 meta_a Meta */
	(const void *)FUN_001d5950, /* [5] +0x14 tankedup_b TankedUp */
	(const void *)FUN_001ec260, /* [6] +0x18 engine_misc LIB:engine_tail */
	(const void *)FUN_00151650, /* [7] +0x1c meta_a Meta */
	(const void *)FUN_000b0560, /* [8] +0x20 charlib CharLib */
	(const void *)FUN_000b0370, /* [9] +0x24 charlib CharLib */
	(const void *)FUN_000b0cb0, /* [10] +0x28 charlib CharLib */
	(const void *)FUN_001d5950, /* [11] +0x2c tankedup_b TankedUp */
	(const void *)FUN_000b06c0, /* [12] +0x30 charlib CharLib */
	(const void *)FUN_000a6fa0, /* [13] +0x34 bumpercars_b BumperCars */
	(const void *)FUN_001ec260, /* [14] +0x38 engine_misc LIB:engine_tail */
	(const void *)FUN_001ec260, /* [15] +0x3c engine_misc LIB:engine_tail */
	(const void *)FUN_000b0770, /* [16] +0x40 charlib CharLib */
	(const void *)FUN_001ec260, /* [17] +0x44 engine_misc LIB:engine_tail */
	(const void *)FUN_000f2600, /* [18] +0x48 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_00151650, /* [19] +0x4c meta_a Meta */
	(const void *)FUN_00151650, /* [20] +0x50 meta_a Meta */
	(const void *)FUN_000f1880, /* [21] +0x54 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_00151650, /* [22] +0x58 meta_a Meta */
	(const void *)FUN_00151650, /* [23] +0x5c meta_a Meta */
	(const void *)FUN_00151650, /* [24] +0x60 meta_a Meta */
	(const void *)FUN_00151650, /* [25] +0x64 meta_a Meta */
	(const void *)FUN_00151650, /* [26] +0x68 meta_a Meta */
	(const void *)FUN_00151650, /* [27] +0x6c meta_a Meta */
	(const void *)FUN_00151650, /* [28] +0x70 meta_a Meta */
	(const void *)FUN_00151650, /* [29] +0x74 meta_a Meta */
	(const void *)FUN_00151650, /* [30] +0x78 meta_a Meta */
	(const void *)FUN_00151650, /* [31] +0x7c meta_a Meta */
	(const void *)FUN_00151650, /* [32] +0x80 meta_a Meta */
	(const void *)FUN_00151650, /* [33] +0x84 meta_a Meta */
	(const void *)FUN_00151650, /* [34] +0x88 meta_a Meta */
	(const void *)FUN_00151650, /* [35] +0x8c meta_a Meta */
	(const void *)FUN_00151650, /* [36] +0x90 meta_a Meta */
	(const void *)FUN_000f18b0, /* [37] +0x94 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f8ab0, /* [38] +0x98 demolition_b Demolition */
	(const void *)FUN_000f2ea0, /* [39] +0x9c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f8ab0, /* [40] +0xa0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [41] +0xa4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [42] +0xa8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [43] +0xac demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [44] +0xb0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [45] +0xb4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [46] +0xb8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [47] +0xbc demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [48] +0xc0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [49] +0xc4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [50] +0xc8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [51] +0xcc demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [52] +0xd0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [53] +0xd4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [54] +0xd8 demolition_b Demolition */
	(const void *)FUN_000f2620, /* [55] +0xdc demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2710, /* [56] +0xe0 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2750, /* [57] +0xe4 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1590, /* [58] +0xe8 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2990, /* [59] +0xec demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f17d0, /* [60] +0xf0 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_TimedBomb */
/* 0x0026af2c vtable */
const void *const vtable_0026af2c[] = {
	(const void *)FUN_000f3070, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_CrossBomb */
/* 0x0026af30 vtable */
const void *const vtable_0026af30[] = {
	(const void *)FUN_000f3240, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1a20, /* [1] +0x04 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f48f0, /* [2] +0x08 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1a50, /* [3] +0x0c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f3790, /* [4] +0x10 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_BombSystem */
/* 0x0026af44 vtable */
const void *const vtable_0026af44[] = {
	(const void *)FUN_000f3b10, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_BombSystemVariationFrag */
/* 0x0026af48 vtable */
const void *const vtable_0026af48[] = {
	(const void *)FUN_000f3b90, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2130, /* [1] +0x04 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1c10, /* [2] +0x08 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1c70, /* [3] +0x0c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1ca0, /* [4] +0x10 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1ce0, /* [5] +0x14 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_001d5950, /* [6] +0x18 tankedup_b TankedUp */
	(const void *)FUN_000f1d50, /* [7] +0x1c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f3b70, /* [8] +0x20 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_BombSystemVariationRemote */
/* 0x0026af6c vtable */
const void *const vtable_0026af6c[] = {
	(const void *)FUN_000f3c90, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1d70, /* [1] +0x04 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1db0, /* [2] +0x08 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1df0, /* [3] +0x0c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1e30, /* [4] +0x10 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1e70, /* [5] +0x14 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_001d5950, /* [6] +0x18 tankedup_b TankedUp */
	(const void *)FUN_000f1f00, /* [7] +0x1c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f3c70, /* [8] +0x20 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_BombSystemVariationTimed */
/* 0x0026af90 vtable */
const void *const vtable_0026af90[] = {
	(const void *)FUN_000f3de0, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1fa0, /* [1] +0x04 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f4d70, /* [2] +0x08 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f1fe0, /* [3] +0x0c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2030, /* [4] +0x10 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2070, /* [5] +0x14 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f41a0, /* [6] +0x18 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2110, /* [7] +0x1c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f3dc0, /* [8] +0x20 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_BombSystemVariationCross */
/* 0x0026afb4 vtable */
const void *const vtable_0026afb4[] = {
	(const void *)FUN_000f4200, /* [0] +0x00 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2130, /* [1] +0x04 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f4e10, /* [2] +0x08 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2160, /* [3] +0x0c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f2190, /* [4] +0x10 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f43a0, /* [5] +0x14 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f21c0, /* [6] +0x18 demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f21d0, /* [7] +0x1c demolition_b Demolition/CDemolition_Bomb.cpp */
	(const void *)FUN_000f41e0, /* [8] +0x20 demolition_b Demolition/CDemolition_Bomb.cpp */
};

/* CDemolition_Object */
/* 0x0026b0d4 vtable */
const void *const vtable_0026b0d4[] = {
	(const void *)FUN_000f5460, /* [0] +0x00 demolition_b Demolition */
	(const void *)FUN_001d5950, /* [1] +0x04 tankedup_b TankedUp */
	(const void *)FUN_001ec260, /* [2] +0x08 engine_misc LIB:engine_tail */
	(const void *)FUN_001d5950, /* [3] +0x0c tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [4] +0x10 tankedup_b TankedUp */
};

/* (1 slot) */
/* 0x0026b0e8 vtable */
const void *const vtable_0026b0e8[] = {
	(const void *)FUN_000f5710, /* [0] +0x00 demolition_b Demolition */
};

/* (1 slot) */
/* 0x0026b0f0 vtable */
const void *const vtable_0026b0f0[] = {
	(const void *)FUN_000f59f0, /* [0] +0x00 demolition_b Demolition */
};

/* (1 slot) */
/* 0x0026b0f8 vtable */
const void *const vtable_0026b0f8[] = {
	(const void *)FUN_000f77a0, /* [0] +0x00 demolition_b Demolition */
};

/* CDemolition_CharLib */
/* 0x0026b2c0 vtable */
const void *const vtable_0026b2c0[] = {
	(const void *)FUN_001d5950, /* [0] +0x00 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [1] +0x04 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [2] +0x08 tankedup_b TankedUp */
	(const void *)FUN_000b1860, /* [3] +0x0c charlib CharLib */
	(const void *)FUN_00151650, /* [4] +0x10 meta_a Meta */
	(const void *)FUN_001d5950, /* [5] +0x14 tankedup_b TankedUp */
	(const void *)FUN_001ec260, /* [6] +0x18 engine_misc LIB:engine_tail */
	(const void *)FUN_00151650, /* [7] +0x1c meta_a Meta */
	(const void *)FUN_000b0560, /* [8] +0x20 charlib CharLib */
	(const void *)FUN_000b0370, /* [9] +0x24 charlib CharLib */
	(const void *)FUN_000b0cb0, /* [10] +0x28 charlib CharLib */
	(const void *)FUN_001d5950, /* [11] +0x2c tankedup_b TankedUp */
	(const void *)FUN_000b06c0, /* [12] +0x30 charlib CharLib */
	(const void *)FUN_000a6fa0, /* [13] +0x34 bumpercars_b BumperCars */
	(const void *)FUN_001ec260, /* [14] +0x38 engine_misc LIB:engine_tail */
	(const void *)FUN_001ec260, /* [15] +0x3c engine_misc LIB:engine_tail */
	(const void *)FUN_000b0770, /* [16] +0x40 charlib CharLib */
	(const void *)FUN_001ec260, /* [17] +0x44 engine_misc LIB:engine_tail */
	(const void *)FUN_000f8ca0, /* [18] +0x48 demolition_b Demolition */
	(const void *)FUN_00151650, /* [19] +0x4c meta_a Meta */
	(const void *)FUN_00151650, /* [20] +0x50 meta_a Meta */
	(const void *)FUN_00151650, /* [21] +0x54 meta_a Meta */
	(const void *)FUN_00151650, /* [22] +0x58 meta_a Meta */
	(const void *)FUN_00151650, /* [23] +0x5c meta_a Meta */
	(const void *)FUN_00151650, /* [24] +0x60 meta_a Meta */
	(const void *)FUN_00151650, /* [25] +0x64 meta_a Meta */
	(const void *)FUN_00151650, /* [26] +0x68 meta_a Meta */
	(const void *)FUN_00151650, /* [27] +0x6c meta_a Meta */
	(const void *)FUN_00151650, /* [28] +0x70 meta_a Meta */
	(const void *)FUN_00151650, /* [29] +0x74 meta_a Meta */
	(const void *)FUN_00151650, /* [30] +0x78 meta_a Meta */
	(const void *)FUN_00151650, /* [31] +0x7c meta_a Meta */
	(const void *)FUN_00151650, /* [32] +0x80 meta_a Meta */
	(const void *)FUN_00151650, /* [33] +0x84 meta_a Meta */
	(const void *)FUN_00151650, /* [34] +0x88 meta_a Meta */
	(const void *)FUN_00151650, /* [35] +0x8c meta_a Meta */
	(const void *)FUN_00151650, /* [36] +0x90 meta_a Meta */
	(const void *)FUN_000f8ab0, /* [37] +0x94 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [38] +0x98 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [39] +0x9c demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [40] +0xa0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [41] +0xa4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [42] +0xa8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [43] +0xac demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [44] +0xb0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [45] +0xb4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [46] +0xb8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [47] +0xbc demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [48] +0xc0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [49] +0xc4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [50] +0xc8 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [51] +0xcc demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [52] +0xd0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [53] +0xd4 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [54] +0xd8 demolition_b Demolition */
	(const void *)0, /* [55] 0x001e0323: not a game/engine function */
	(const void *)0, /* [56] 0x001e0323: not a game/engine function */
	(const void *)0, /* [57] 0x001e0323: not a game/engine function */
	(const void *)0, /* [58] 0x001e0323: not a game/engine function */
	(const void *)0, /* [59] 0x001e0323: not a game/engine function */
	(const void *)0, /* [60] 0x001e0323: not a game/engine function */
};

/* CDemolition_CharLib (message receiver at +0x1130) */
/* 0x0026b3b4 vtable */
const void *const vtable_0026b3b4[] = {
	(const void *)FUN_000f8c90, /* [0] +0x00 demolition_b Demolition */
};

/* (1 slot) */
/* 0x0026b3b8 vtable */
const void *const vtable_0026b3b8[] = {
	(const void *)FUN_000f8da0, /* [0] +0x00 demolition_b Demolition */
};

/* CDemolition_Player (message receiver at +0x1130) */
/* 0x0026b440 vtable */
const void *const vtable_0026b440[] = {
	(const void *)FUN_000fa500, /* [0] +0x00 demolition_b Demolition/CDemolition_Player.cpp */
};

/* CDemolition_Player */
/* 0x0026b448 vtable */
const void *const vtable_0026b448[] = {
	(const void *)FUN_001d5950, /* [0] +0x00 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [1] +0x04 tankedup_b TankedUp */
	(const void *)FUN_001d5950, /* [2] +0x08 tankedup_b TankedUp */
	(const void *)FUN_000b1860, /* [3] +0x0c charlib CharLib */
	(const void *)FUN_000fbc40, /* [4] +0x10 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_001d5950, /* [5] +0x14 tankedup_b TankedUp */
	(const void *)FUN_001ec260, /* [6] +0x18 engine_misc LIB:engine_tail */
	(const void *)FUN_00151650, /* [7] +0x1c meta_a Meta */
	(const void *)FUN_000b0560, /* [8] +0x20 charlib CharLib */
	(const void *)FUN_000b0370, /* [9] +0x24 charlib CharLib */
	(const void *)FUN_000b0cb0, /* [10] +0x28 charlib CharLib */
	(const void *)FUN_001d5950, /* [11] +0x2c tankedup_b TankedUp */
	(const void *)FUN_000b06c0, /* [12] +0x30 charlib CharLib */
	(const void *)FUN_000a6fa0, /* [13] +0x34 bumpercars_b BumperCars */
	(const void *)FUN_001ec260, /* [14] +0x38 engine_misc LIB:engine_tail */
	(const void *)FUN_001ec260, /* [15] +0x3c engine_misc LIB:engine_tail */
	(const void *)FUN_000b0770, /* [16] +0x40 charlib CharLib */
	(const void *)FUN_000fc0b0, /* [17] +0x44 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa510, /* [18] +0x48 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f9e40, /* [19] +0x4c demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_00151650, /* [20] +0x50 meta_a Meta */
	(const void *)FUN_000f9ee0, /* [21] +0x54 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f9f70, /* [22] +0x58 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f9fb0, /* [23] +0x5c demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fac10, /* [24] +0x60 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f9fe0, /* [25] +0x64 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_00151650, /* [26] +0x68 meta_a Meta */
	(const void *)FUN_00151650, /* [27] +0x6c meta_a Meta */
	(const void *)FUN_000fa010, /* [28] +0x70 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_00151650, /* [29] +0x74 meta_a Meta */
	(const void *)FUN_00151650, /* [30] +0x78 meta_a Meta */
	(const void *)FUN_00151650, /* [31] +0x7c meta_a Meta */
	(const void *)FUN_00151650, /* [32] +0x80 meta_a Meta */
	(const void *)FUN_00151650, /* [33] +0x84 meta_a Meta */
	(const void *)FUN_00151650, /* [34] +0x88 meta_a Meta */
	(const void *)FUN_00151650, /* [35] +0x8c meta_a Meta */
	(const void *)FUN_00151650, /* [36] +0x90 meta_a Meta */
	(const void *)FUN_000faec0, /* [37] +0x94 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fafe0, /* [38] +0x98 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fb080, /* [39] +0x9c demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fb270, /* [40] +0xa0 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa070, /* [41] +0xa4 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fb3e0, /* [42] +0xa8 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fb630, /* [43] +0xac demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f8ab0, /* [44] +0xb0 demolition_b Demolition */
	(const void *)FUN_000f8ab0, /* [45] +0xb4 demolition_b Demolition */
	(const void *)FUN_000fb820, /* [46] +0xb8 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa0e0, /* [47] +0xbc demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa160, /* [48] +0xc0 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f8ab0, /* [49] +0xc4 demolition_b Demolition */
	(const void *)FUN_000fb890, /* [50] +0xc8 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fb9c0, /* [51] +0xcc demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa1e0, /* [52] +0xd0 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fca70, /* [53] +0xd4 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa290, /* [54] +0xd8 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa530, /* [55] +0xdc demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa6a0, /* [56] +0xe0 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fc7e0, /* [57] +0xe4 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fa720, /* [58] +0xe8 demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000fc5d0, /* [59] +0xec demolition_b Demolition/CDemolition_Player.cpp */
	(const void *)FUN_000f9740, /* [60] +0xf0 demolition_b Demolition/CDemolition_Player.cpp */
};

