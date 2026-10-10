/*
UNPLACED_AI.C  <-  gap functions 0x001586d0, 0x00158d90 (with its chunk at 0x00158260)
and 0x00159250 of C:\BlitzParty\Game\Src\Meta\ (probably CMetaPlayer.cpp)

The computer player's plan. AI modes (+0x04):
   1 go to a tile of my colour          2 go to a free tile        3 pick 2 or 9
   4 retaliate (+0x1088)                5 go to a collectable (+0x10)
   6 go to a target (+0x1c)             7 attack a player (+0x18)
   8 go to any tile                     9 go to an unowned flag tile (type 2)
  10 go to a tile of my character      11 go to the goal and score
  12 steal from a player (+0x18)       13 press a big red button (+0x0c)
Other AI fields: +0x08 target tile, +0x14 picky, +0x20 frames since the plan,
+0x24 plan time limit, +0x28, +0x8c route (CLRoute), +0x98 short: arrived.
*/

#include "meta_a.h"

/* the route to `to` (charlib CLRoute_Start(route, from, to, height)) */
static int ma_route(int ai, float *to)
{
	return CLRoute_Start((struct CLRoute *)MA_ADDR(ai, 0x8c), MA_FP(MA_CHAR(ai), 0xd0), to,
		MA_F32(MA_CHAR(ai), 0xd4));
}

/* the route to a tile's position (+0x34 its entity, +0xe0 there) */
static int ma_route_tile(int ai, int tile)
{
	return ma_route(ai, MA_FP(MA_PTR(tile, 0x34), 0xe0));
}

/* go for the tile; 1 when it is reached (or when the route is still on), see the modes */
static int ma_take_tile_under(int ai, int forget)
{
	if (FUN_00159380(MA_CHAR(ai) + 0xd0, 0.0f) != MA_I32(ai, 8))
		return 1;
	MA_I32(MA_PTR(ai, 0x6c), 0x14) = 1;
	if (forget)
		MA_I32(ai, 8) = 0;
	return 0;
}

/* 0x001586d0  vtable +0x1c: carry on with the plan (0) or make a new one (1) */
undefined4 FUN_001586d0(int ai)
{
	int saved;
	int tile;
	int goal;
	int target;
	int found;
	float delta[4];

	if ((float)DAT_002b0864 < MA_F32(ai, 0x20))
		return 1;
	if (MA_I32(ai, 0x24) != -1 && (float)MA_I32(ai, 0x24) < MA_F32(ai, 0x20))
		return 1;
	if (3 < MA_I32(ai, 0x1140) && MA_I32(ai, 4) != 0xb)
		return 1;
	/* now and then: take a tile in passing */
	if (ma_rand() % 1000 < (DAT_002bae70 + 1) * 0x19) {
		FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), -1, -1, 0, -1, MA_U16(MA_CHAR(ai), 0x230), 0);
		saved = MA_I32(MA_PTR(ai, 0x6c), 0x14);
		MA_I32(MA_PTR(ai, 0x6c), 0x14) = 1;
		if (FUN_00151550(ai) == 0)
			MA_I32(MA_PTR(ai, 0x6c), 0x14) = saved;
	}
	switch (MA_I32(ai, 4)) {
	case 0:
		return 1;
	case 1:
		tile = MA_I32(ai, 8);
		if (tile != 0) {
			if (MA_I16(tile, 0x10) != -1) {
				MA_I32(ai, 8) = 0;
				if ((int)MA_I16(tile, 0x10) != (int)MA_U16(MA_CHAR(ai), 0x230))
					return 1;
			}
			if (MA_I16(ai, 0x98) != 0)
				return ma_take_tile_under(ai, 1);
		}
		goal = MA_I32(ai, 0x1134);
		if (goal == 0)
			found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), -1, -1, 0, -1, MA_U16(MA_CHAR(ai), 0x230), 0);
		else if (MA_I32(ai, 0x14) == 0)
			found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), MA_I32(goal, 0x1c), (short)MA_U16(goal, 0x12),
				0, -1, -1, 0);
		else
			found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), 0, -1, 0, 1, -1, 0);
		goto found_tile;
	case 2:
		if (MA_I32(ai, 8) != 0 && MA_I16(ai, 0x98) != 0)
			return ma_take_tile_under(ai, 0);
		found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), 0, -1, 0, 1, -1, 0);
		if (found == 0)
			return 1;
		goto new_tile;
	case 3:
		if (FUN_001591b0(0, -1, 1, -1) != 0) {
			MA_I32(ai, 4) = 2;
			return 0;
		}
		if (FUN_001591b0(2, -1, 0, -1) != 0) {
			MA_I32(ai, 4) = 9;
			return 0;
		}
		/* fall through */
	case 7:
		FUN_00156460(ai, 1);
		if (MA_I16(ai, 0x98) != 0)
			return 1;
		if (MA_I16(ai, 0x36) == 5)
			return 1;
		if (MA_I32(ai, 0x18) == -1 || MA_I16(DAT_003075bc[MA_I32(ai, 0x18)], 0x36) == 0xd) {
			MA_U16(ai, 0x1086) = 0xffff;
			if ((int)Core_Rand() % 100 < 0x32)
				MA_I32(ai, 0x18) = (int)FUN_001573d0(ai, 1);
			else
				MA_I32(ai, 0x18) = (int)FUN_001563e0(ai, 1);
		}
		if ((int)MA_I16(ai, 0x1086) == MA_I32(ai, 0x18)) {
			MA_I32(ai, 0x18) = -1;
			MA_U16(ai, 0x1086) = 0xffff;
			return 1;
		}
		if (MA_I16(ai, 0x1042) == -2)
			return 0;
		ma_route(ai, MA_FP(MA_CHAR(DAT_003075bc[MA_I32(ai, 0x18)]), 0xd0));
		return 0;
	case 4:
		FUN_00156460(ai, 1);
		target = MA_I16(ai, 0x1088);
		if (target == -1)
			return 1;
		if (MA_I16(ai, 0x1086) == target) {
			MA_U16(ai, 0x1088) = 0xffff;
			MA_U16(ai, 0x1086) = 0xffff;
			return 1;
		}
		if ((float)DAT_002b0860 <= MA_F32(ai, 0x20)) {
			MA_U16(ai, 0x1088) = 0xffff;
			return 1;
		}
		if (MA_I16(ai, 0x1042) == -2 && MA_I16(ai, 0x98) == 0)
			return 0;
		ma_route(ai, MA_FP(MA_CHAR(DAT_003075bc[target]), 0xd0));
		return 0;
	case 5:
		if (MA_I32(ai, 0x10) == 0)
			return 1;
		if (MA_I16(ai, 0x98) != 0)
			return 1;
		found = FUN_000b9c90((undefined4)(MA_CHAR(ai) + 0xd0), -1);
		if (found == MA_I32(ai, 0x10))
			return 0;
		MA_I32(ai, 0x10) = found;
		ma_route(ai, MA_FP(found, 0xd0));
		return 0;
	case 6:
		if (MA_I32(ai, 0x1c) == 0)
			return 1;
		Core_Vec3Sub(delta, MA_FP(MA_CHAR(ai), 0xd0), (float *)MA_PTR(ai, 0x1c));
		if (delta[0] == 0.0f && delta[2] == 0.0f)
			delta[2] = 1.0f;
		AI_SetStick((void *)ai, delta, 1.0f);
		return 0;
	case 8:
		/* (no zero check here in the original) */
		found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), -1, -1, -1, -1, -1, 0);
		goto new_tile;
	case 9:
		if (MA_I32(ai, 8) != 0 && MA_I16(ai, 0x98) != 0)
			return ma_take_tile_under(ai, 1);
		found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), 2, -1, 0, -1, -1, 0);
		goto found_tile;
	case 10:
		if (MA_I32(ai, 8) != 0 && MA_I16(ai, 0x98) != 0)
			return ma_take_tile_under(ai, 1);
		found = FUN_00159020(MA_FP(MA_CHAR(ai), 0xd0), 4, (short)MA_U16(MA_CHAR(ai), 0x230), 0, -1, -1, 0);
		goto found_tile;
	case 0xb:
		if (MA_I32(ai, 0x1140) != 0 && MA_I16(ai, 0x98) == 0)
			return 0;
		return 1;
	case 0xc:
		FUN_00156460(ai, 1);
		if (MA_I16(ai, 0x98) != 0 && (int)Core_Rand() % 100 < 10)
			return 1;
		target = MA_I32(ai, 0x18);
		if (target != -1 && MA_I32(DAT_003075bc[target], 0x1140) == 0)
			return 1;
		if (MA_I16(ai, 0x1042) == -2)
			return 0;
		ma_route(ai, MA_FP(MA_CHAR(DAT_003075bc[target]), 0xd0));
		return 0;
	case 0xd:
		tile = MA_I32(ai, 0xc);
		if (tile != 0 && MA_I32(tile, 0x30) != 0 && MA_U8(MA_PTR(tile, 0x44), 0x3d5) != 0 &&
			((MA_I32(tile, 0x3c) == 1 && MA_I32(tile, 0x34) == 0) || MA_I32(tile, 0x3c) == 0)) {
			if (MA_I16(ai, 0x98) == 0 && FUN_001621d0((undefined4)MA_CHAR(ai)) == 0)
				return 0;
			MA_I32(MA_PTR(ai, 0x6c), 0x14) = 1;
			if (FUN_00151500(ai) == 0)
				return 0;
			MA_F32(ai, 0x1130) = 240.0f;
			return 1;
		}
		return 1;
	default:
		return 0;
	}

found_tile:
	if (found == 0)
		return 1;
	if (found != MA_I32(ai, 8)) {
		MA_I32(ai, 8) = found;
		ma_route_tile(ai, found);
	}
	if (MA_I16(ai, 0x98) != 0)
		return 1;
	return 0;

new_tile:
	if (found == MA_I32(ai, 8))
		return 0;
	MA_I32(ai, 8) = found;
	ma_route_tile(ai, found);
	return 0;
}

/* the nearest goal (meta_b FUN_00166a60, a thiscall whose ECX is not used) */
static int ma_nearest_goal(int ai)
{
	return MA_DUAL(FUN_00166a60, int (*)(float, undefined4), int (*)(void *, float *),
		(0.0f, (undefined4)(MA_CHAR(ai) + 0xd0)), ((void *)0, MA_FP(MA_CHAR(ai), 0xd0)));
}

/* 0x00158d90  vtable +0x18: set the AI mode (-1: choose one, the chunk at 0x00158260) */
uint FUN_00158d90(int ai, uint mode)
{
	int seed_roll;
	int goal;
	int button;
	int target;
	int chance;
	uint roll;
	int half;
	int timer_half_check;

	seed_roll = ma_rand() % 100;
	MA_I32(ai, 0x24) = -1;
	MA_I16(ai, 0x98) = 1;
	MA_I32(ai, 8) = 0;
	MA_I32(ai, 0x10) = 0;
	MA_I32(ai, 0x20) = 0;
	MA_I32(ai, 0x28) = 0;
	MA_I32(ai, 0x14) = seed_roll < DAT_002bae70 * 0x14;
	if (mode != 0xffffffff) {
		MA_U32(ai, 4) = mode;
		return mode;
	}

	/* 0x00158260: a goal: a safe one by caution, else the nearest */
	if (ma_rand() % 100 < *(int *)((char *)&DAT_0029074c +
			DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60)) {
		goal = FUN_00166ad0(MA_U16(MA_CHAR(ai), 0x230));
		if (goal != 0)
			goto have_goal;
		MA_PRINTF("\n\n*** NO SAFE GOAL!!\n\n");
		goal = ma_nearest_goal(ai);
	} else {
		goal = ma_nearest_goal(ai);
	}
	if (goal == 0) {
		MA_PRINTF("\n\n*** NO NEAR GOAL!!\n\n");
		goto button;
	}
have_goal:
	if (MA_I32(ai, 0x1140) != 0 && MA_I16(ai, 0x36) != 0xc && MA_F32(ai, 0x1130) == 0.0f) {
		half = DAT_002b0854 / 2;
		timer_half_check = (float)half <= MA_F32(DAT_0030564c, 0x7c);
		if (timer_half_check) {
			if (MA_F32(DAT_0030564c, 0x7c) < (float)DAT_002b0854 &&
				(int)Core_Rand() % 100 < DAT_002bae70 * 0x32)
				goto score;
			if (MA_I32(ai, 0x1140) < 4 &&
				*(int *)((char *)&DAT_0029074c + DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60) *
					MA_I32(ai, 0x1140) <= (int)Core_Rand() % 100)
				goto button;
		}
	score:
		MA_I32(ai, 4) = 0xb;
		return (uint)ma_route(ai, MA_FP(goal, 0xe0));
	}
button:
	button = (int)((int (*)(int))FUN_00161c70)(MA_CHAR(ai));
	MA_I32(ai, 0xc) = button;
	if (button != 0 && ma_rand() % 100 < DAT_002b0858) {
		MA_I32(ai, 4) = 0xd;
		roll = (uint)ma_route(ai, MA_FP(MA_PTR(MA_PTR(ai, 0xc), 0x44), 0xe0));
		MA_U32(ai, 0x107c) = DAT_002b085c;
		return roll;
	}
	/* steal from the richest player, by its score and how far behind this one is */
	target = (int)FUN_001573d0(ai, 0);
	if (target != -1) {
		chance = (int)((double)(int)FUN_001519a0(MA_I32(DAT_003075bc[target], 0x1140)) *
			((double)DAT_002baee8[target * 10] /
				((double)DAT_002baee8[MA_U16(MA_CHAR(ai), 0x230) * 10] + 1.0) + 1.0));
		if (MA_F32(ai, 0x10a4) != 0.0f)
			chance = chance * 2 + 10;
		roll = (uint)ma_rand();
		if ((int)(roll % 0x96) < chance) {
			MA_I32(ai, 0x18) = target;
			MA_U16(ai, 0x1086) = 0xffff;
			MA_I32(ai, 4) = 0xc;
			MA_I16(ai, 0x98) = 0;
			return roll / 0x96;
		}
	}
	if (FUN_001562a0(ai) != 0)
		return 1;
	/* greed: a collectable */
	if (ma_rand() % 0x96 < *(int *)((char *)&DAT_00290740 + 4 +
			DAT_002baed4[MA_U16(MA_CHAR(ai), 0x230) * 10] * 0x60) &&
		FUN_000b9c40(-1) != 0) {
		MA_I32(ai, 0x10) = FUN_000b9c90((undefined4)(MA_CHAR(ai) + 0xd0), -1);
		if (MA_I32(ai, 0x10) != 0) {
			roll = (uint)ma_route(ai, MA_FP(MA_PTR(ai, 0x10), 0xd0));
			MA_I32(ai, 4) = 5;
			return roll;
		}
	}
	/* a tile of my character */
	roll = (uint)FUN_001591b0(-1, -1, 0, MA_U16(MA_CHAR(ai), 0x230));
	if (roll != 0) {
		MA_I16(ai, 0x98) = 0;
		MA_I32(ai, 4) = 1;
		return roll;
	}
	if (MA_I32(ai, 0x1140) != 0 && MA_F32(ai, 0x1130) == 0.0f && goal != 0) {
		MA_I32(ai, 4) = 0xb;
		return (uint)ma_route(ai, MA_FP(goal, 0xe0));
	}
	MA_I32(ai, 0x18) = -1;
	MA_I32(ai, 4) = 7;
	MA_I16(ai, 0x98) = 0;
	return 0;
}

/* 0x00159250  add to the player's score */
void FUN_00159250(int ai, int points)
{
	DAT_002baee8[MA_U16(MA_CHAR(ai), 0x230) * 10] = DAT_002baee8[MA_U16(MA_CHAR(ai), 0x230) * 10] + points;
}
