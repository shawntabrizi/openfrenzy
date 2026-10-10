/*
TankedUp/unplaced_debris.c  (gap functions of C:\BlitzParty\Game\Src\TankedUp\, file unknown)

The debris system (CTank_Game +0xfc0, "debrisSystem"): a thin layer over the shared debris
code (bumpercars_a 0x000827f0-0x00080d60, classlib_fx sprites). It loads the debris
sprites, gives the debris code the world's ground test, and adds debris of one of the 15
TankedUp debris types (0x002b7210, 0x2c bytes each, in tu_data.c).
*/

#include "tankedup_a.h"

/* bumpercars_a: the "aisteam1" sprite definitions (6) */
extern unsigned char DAT_00290f00[];

/* the TankedUp debris sprite definitions (7: aisteam, spark, sparkburst, aisteam, xone,
   xthree, xfive) and debris types */
#define TU_DEBRIS_SPRITES ((struct CSpriteDef *)TU_ADDR(0x002b6950))
#define TU_DEBRIS_TYPE(i) ((struct DebrisType *)((char *)TU_ADDR(0x002b7210) + (i) * 0x2c))
/* the world that the ground test asks (set by Initialise) */
#define TU_DEBRIS_WORLD TU_VAR(void *, 0x00315328)

/* 0x001c3490  TankedUp (not in the function table: Ghidra left it as a label)
The debris code's ground test: asks the world (CTankWorld_SweepGround, 0x001cb560). */
float FUN_001c3490(float *pos, float *delta, float *hit, float *normal)
{
	return CTankWorld_SweepGround(TU_DEBRIS_WORLD, pos, delta, hit, normal);
}

/* 0x001c34b0  TankedUp (gap)
Initialise: keeps the world, loads both sprite sets, starts the debris code with room for
1000 pieces. Always returns 1. */
int TankDebris_Initialise(void *self, void *package, void *world)
{
	(void)self;
	(void)package;
	TU_DEBRIS_WORLD = world;
	Sprite_LoadDefs(TU_DEBRIS_SPRITES, 7, 0);
	Sprite_LoadDefs((struct CSpriteDef *)DAT_00290f00, 6, 0);
	Debris_Init((void *)FUN_001c3490, 1000);
	return 1;
}

/* 0x001c34f0  TankedUp (gap)
Free: the sprites, then the debris code. */
void TankDebris_Free(void *self)
{
	(void)self;
	Sprite_FreeDefs(TU_DEBRIS_SPRITES, 7);
	Sprite_FreeDefs((struct CSpriteDef *)DAT_00290f00, 6);
	FUN_00080bb0();
}

/* 0x001c3510  TankedUp (gap)
Update: moves the debris by `step` frames (in seconds for the debris code). Returns
`state` (no state change). */
int TankDebris_Update(void *self, float step, int state)
{
	(void)self;
	Debris_UpdateSystems(step * 0.0166666675f);
	return state;
}

/* 0x001c3530  TankedUp (gap; a jump to 0x00080cc0)
Draw: the debris code's draw. */
void TankDebris_Draw(void *self)
{
	(void)self;
	FUN_00080cc0();
}

/* 0x001c3540  TankedUp (gap)
Adds one piece of debris of TankedUp debris type `type` (0-14). */
void TankDebris_Add(void *self, int type, float *pos, float *vel, int life, unsigned int colour)
{
	(void)self;
	Debris_Add(TU_DEBRIS_TYPE(type), pos, vel, life, colour);
}
