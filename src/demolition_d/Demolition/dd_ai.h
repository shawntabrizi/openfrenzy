/*
Demolition/dd_ai.h - helpers shared by the demolition_d brain files only (unplaced_ai_*.c).
*/

#ifndef DEMOLITION_D_AI_H
#define DEMOLITION_D_AI_H

#include "demolition_d.h"

/* the player a brain drives, and the target it is after */
#define AI_OWNER(s) ((s)->owner)
#define AI_POS(p) DD_FP((p), DP_POS)

/* class fields after 0x168 (they differ per class) */
#define AI_F(s, off) DD_F32((s), (off))
#define AI_I(s, off) DD_I32((s), (off))
#define AI_P(s, off) DD_PTR((s), (off))

/* the owner's pad buttons: 1 presses, 0 releases */
#define AI_PressJump(s, v) (DD_I32(DD_PTR(AI_OWNER(s), DP_BUTTON_JUMP), 0x14) = (v))
#define AI_PressAction(s, v) (DD_I32(DD_PTR(AI_OWNER(s), DP_BUTTON_ACTION), 0x14) = (v))

/* the owner's facing (the CharLib actor's heading) */
#define AI_HEADING(s) DD_F32(DD_PTR(AI_OWNER(s), DP_ACTOR), 0x1d8)

/* the first player of the game, and the next one */
#define AI_FIRST_PLAYER() DD_PTR(DD_GAME, DG_PLAYERS)
#define AI_NEXT_PLAYER(p) DD_PTR((p), DP_NEXT)

/* the variation object's arena (virtual +0x34: centre at +0x24/+0x2c, radius at +0x34)
   and zones (virtual +0x30: zone i, centre at +0x24/+0x2c) */
#define AI_VARIATION() DD_PTR(DD_GAME, DG_VARIATIONOBJ)
#define AI_Arena() DD_VFN(AI_VARIATION(), 0x34, char *(*)(void *))(AI_VARIATION())
#define AI_Zone(i) DD_VFN(AI_VARIATION(), 0x30, char *(*)(void *, int))(AI_VARIATION(), (i))

/* demolition_c: the arena region tests (ECX this) and the turning helper */
#define AI_RegionContains(region, pos) DD_FN(FUN_00101ac0, int, (void *, float *))((region), (pos))
#define AI_RegionClear(region, pos, radius, flag) \
	DD_FN(FUN_00102120, int, (void *, float *, float, int))((region), (pos), (radius), (flag))
#define AI_TurnTowards(cur, target, step) DD_FN(FUN_00101f50, float, (float, float, float))((cur), (target), (step))

/* charlib: drive the owner's stick, follow its route; bumpercars_b: a timer's time left */
#define AI_SetStick(player, dir, amount) FUN_000b0780((void *)(player), (dir), (amount))
#define AI_FollowRoute(player) FUN_000b1190((void *)(player))
#define AI_TimeLeft(timer) FUN_000965f0((struct BCarTimer *)(timer))

/* core 0x00011730: a rand() of the core module (0..32767) */
#define AI_CoreRandF() ((float)(int)FUN_00011730() * DD_RAND_SCALE)

/* the random time patterns of the brains (one rand() each) */
static inline float ai_vary(float t)          /* 0.5 .. 1.5 times t */
{
	return (dd_randf() + 0.5f) * t;
}
static inline float ai_vary_mid(float t)      /* 0.75 .. 1.25 times t */
{
	return (dd_randf() * 0.5f + 0.75f) * t;
}

/* the brain parameters of the current variation and difficulty */
static inline float *ai_params(void)
{
	return DAT_00298bb0[DD_I32(DD_GAME, DG_VARIATION) * 3 + DD_I32(DD_GAME, DG_DIFFICULTY)];
}

/* the reset shared by the four classes' Reset methods (the stores of their first lines) */
static inline void ai_reset_base(DemoAI *self)
{
	self->stick[0] = 0.0f;
	self->stick[1] = 0.0f;
	self->stick[2] = 0.0f;
	self->stick[3] = 0.0f;
	self->state = 0;
	self->params[0] = ai_params()[0];
	self->params[1] = ai_params()[1];
	self->params[2] = ai_params()[2];
	self->params[3] = ai_params()[3];
	self->params[4] = ai_params()[4];
	self->routing = 0;
	self->hasGoal = 0;
	self->newGoal = 0;
	self->arrived = 0;
	self->routeTimer = 0.0f;
	self->target = NULL;
	self->f15c = 99999.0f;
}

/* set the goal and ask for a new route */
static inline void ai_set_goal(DemoAI *self, float x, float y, float z, float w)
{
	self->goal[0] = x;
	self->goal[1] = y;
	self->goal[2] = z;
	self->goal[3] = w;
	self->hasGoal = 1;
	self->newGoal = 1;
}

#endif
