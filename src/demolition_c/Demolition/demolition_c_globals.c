/*
Demolition/demolition_c_globals.c - weak fallbacks for globals and functions of other modules
that demolition_c uses (an owner's strong definition replaces a fallback at link time).
*/

#include "demolition_c_internal.h"

/* ---------- .data / .bss of other modules (weak, XBE values) */

__attribute__((weak)) unsigned int DAT_00285630 = 1;  /* rand() seed */
__attribute__((weak)) float DAT_0028fda8 = 1.0f;      /* frame time factor */
__attribute__((weak)) char *DAT_002ff85c;             /* the game object (demolition_a) */
__attribute__((weak)) float DAT_00285644 = 0.001f;    /* collision speed epsilon (engine_misc) */
__attribute__((weak)) int DAT_002ba99c;               /* frame counter (core) */
__attribute__((weak)) int *DAT_002ea488;              /* the CharLib lists (charlib) */

/* ---------- CDemolition_Wall (demolition_d): vtable 0x0026bef0, constructor and destructor
   (missing from the function table) */

__attribute__((weak)) const void *const vtable_0026bef0[1] = { (const void *)FUN_0010dff0 };

/* 0x0010db80  Demolition (CDemolition_Wall.cpp; demolition_d range, missing from the table)
Constructor: sets the vtable. */
__attribute__((weak)) void *FUN_0010db80(void *self)
{
	*(const void *const **)self = vtable_0026bef0;
	return self;
}

/* 0x0010db90  Demolition (CDemolition_Wall.cpp; demolition_d range, missing from the table)
Destructor: sets the vtable. */
__attribute__((weak)) void FUN_0010db90(void *self)
{
	*(const void *const **)self = vtable_0026bef0;
}

/* ---------- CDemolition_Game methods (demolition_a range, missing from the table) that the
   game mode thunks call; stubs until demolition_a defines them */

__attribute__((weak)) void FUN_000e8010(void *game, int active)
{
	(void)game;
	(void)active;
	of_unimplemented(0x000e8010, "FUN_000e8010");
}

__attribute__((weak)) void FUN_000e9870(void *game)
{
	(void)game;
	of_unimplemented(0x000e9870, "FUN_000e9870");
}

__attribute__((weak)) void FUN_000e9920(void *game, int active)
{
	(void)game;
	(void)active;
	of_unimplemented(0x000e9920, "FUN_000e9920");
}

__attribute__((weak)) void FUN_000e9ac0(void *game, int active)
{
	(void)game;
	(void)active;
	of_unimplemented(0x000e9ac0, "FUN_000e9ac0");
}
