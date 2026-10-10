/*
Drillers/drillers_chunks.c

Six table entries of this module are not functions: they are continuation chunks (code
after a jump inside a function) that the function table lists as function starts. Their
code is decompiled as part of the function that owns it. Nothing calls these addresses as
functions; the definitions exist only so each table entry has a body. Each logs through
of_unimplemented and does nothing.
*/

#include "drillers.h"
#include "of_game.h"

/* 0x00110000  Drillers: not a function. A chunk of 0x0010ffa0 (CDrillers_Game.c). */
void FUN_00110000(void)
{
	/* STUB: there is no function here to decompile */
	of_unimplemented(0x00110000, "FUN_00110000 (not a function: chunk of 0x0010ffa0)");
}

/* 0x00110840  Drillers: not a function. A chunk of the render body 0x00110270
   (CDrillers_Game.c). */
void FUN_00110840(void)
{
	/* STUB: there is no function here to decompile */
	of_unimplemented(0x00110840, "FUN_00110840 (not a function: chunk of 0x00110270)");
}

/* 0x00115443  Drillers: not a function. A chunk of 0x00115410 (Drillers/unplaced_map.c). */
undefined4 FUN_00115443(int a, undefined4 b, undefined4 c, int d)
{
	(void)a;
	(void)b;
	(void)c;
	(void)d;
	/* STUB: there is no function here to decompile */
	of_unimplemented(0x00115443, "FUN_00115443 (not a function: chunk of 0x00115410)");
	return 0;
}

/* 0x00120000  Drillers: not a function. A chunk of DrPads_Create 0x0011fd50
   (Drillers/unplaced_pads.c). */
void FUN_00120000(void)
{
	/* STUB: there is no function here to decompile */
	of_unimplemented(0x00120000, "FUN_00120000 (not a function: chunk of 0x0011fd50)");
}

/* 0x001214f0  Drillers: not a function. Inside the chunk 0x00121150 of 0x00120d00 (the
   results table, Drillers/unplaced_hud.c). */
void FUN_001214f0(void)
{
	/* STUB: there is no function here to decompile */
	of_unimplemented(0x001214f0, "FUN_001214f0 (not a function: inside 0x00120d00)");
}

/* 0x001246e0  Drillers: not a function. A chunk of 0x001245f0 (CDrillers_Particle.c). */
void FUN_001246e0(int a)
{
	(void)a;
	/* STUB: there is no function here to decompile */
	of_unimplemented(0x001246e0, "FUN_001246e0 (not a function: chunk of 0x001245f0)");
}
