/*
RollerBall/unplaced_ai.c  (module rollerball_b)

Gap functions 0x0019a4e0-0x0019ce50 (original file not known; their .rdata lies between
CRollerBall_Player.cpp's and CRollerBall_Particle.cpp's):

- the RollerBall vertex shader fragment (vtable 0x00274c74; made by rollerball_a's shader
  system): a fog/rim term from the camera position.
- the player controllers (CRollerball_Entity +0x4204), one class per variation, that drive
  the computer players (and are set up for all players):
    base  (vtable 0x00274d50, 0x184 bytes)  chase the nearest lit goal
    sumo  (vtable 0x00274d74, 0x98 bytes)   attack, stay on the arena, dodge
    race2 (vtable 0x00274d98, 0x54 bytes)   follow the path point +0x3c (set by the game)
    race  (vtable 0x00274dbc, 0x5c bytes)   the same, with a random lane +0x54
    drop  (vtable 0x00274de0, 0x98 bytes)   like sumo, with the drop arena's sizes
  Virtual functions: [0] init (the entity), [1] update, [3] shutdown, [4] reset,
  [5] re-roll the timers, [7] hit by a ball, [8] scalar deleting destructor.
  0x0019bdd0 (race2's slow approach) is missing from the function table.

Common controller fields: +0x04..+0x14 the AI parameters (data 0x002b5328: 5 floats per
variation and difficulty; the tweak names are "AI: Aggressiveness", "Vindictiveness",
"Cleverness", "Accuracy", "Awareness"), +0x18 the entity, +0x1c the state, +0x24 the
target player. Sumo/drop: +0x3c centre, +0x4c arena radius, +0x50/+0x58 margin terms,
+0x54 margin, +0x5c distance from the centre, +0x60/+0x64 to the centre (x, z), +0x68 time
limit, +0x6c re-target timer, +0x70..+0x7c steering jitter (period, timer, range, angle),
+0x80..+0x94 dodge (period, timer, duration, side, start, avoiding).
*/

#include "rollerball_b.h"

#define RBC_P0 0x04         /* aggressiveness */
#define RBC_P1 0x08         /* vindictiveness */
#define RBC_P2 0x0c         /* cleverness */
#define RBC_P3 0x10         /* accuracy */
#define RBC_P4 0x14         /* awareness */
#define RBC_ENTITY 0x18
#define RBC_STATE 0x1c
#define RBC_TARGET 0x24

/* sumo / drop */
#define RBA_CENTRE 0x3c
#define RBA_RADIUS 0x4c
#define RBA_MARGIN_BASE 0x50
#define RBA_MARGIN 0x54
#define RBA_MARGIN_SPEED 0x58
#define RBA_DIST 0x5c
#define RBA_TO_CENTRE 0x60
#define RBA_TIME_LIMIT 0x68
#define RBA_RETARGET 0x6c
#define RBA_JITTER_PERIOD 0x70
#define RBA_JITTER_TIMER 0x74
#define RBA_JITTER_RANGE 0x78
#define RBA_JITTER 0x7c
#define RBA_DODGE_PERIOD 0x80
#define RBA_DODGE_TIMER 0x84
#define RBA_DODGE_TIME 0x88
#define RBA_DODGE_SIDE 0x8c
#define RBA_DODGE_START 0x90
#define RBA_AVOIDING 0x94

/* base */
#define RBB_GOALS 0x3c       /* 9 records of 0x20: pos[4], dx, dz, distance, lit */
#define RBB_GOAL_TARGET 0x15c
#define RBB_GOAL_LAST 0x160
#define RBB_DECIDE_PERIOD 0x164
#define RBB_DECIDE_TIMER 0x168
#define RBB_JITTER_PERIOD 0x16c
#define RBB_JITTER_TIMER 0x170
#define RBB_JITTER_RANGE 0x17c
#define RBB_JITTER 0x180

/* race */
#define RBR_LANE 0x54
#define RBR_LANE_TIMER 0x58

#define RBC_E(ctl) ((char *)RBB_P(ctl, RBC_ENTITY))

/* the controller vtables (defined below) */
extern const void *const rbb_vtable_00274d50[9];
extern const void *const rbb_vtable_00274d74[9];
extern const void *const rbb_vtable_00274d98[9];
extern const void *const rbb_vtable_00274dbc[9];
extern const void *const rbb_vtable_00274de0[9];

/* bdisplay: the camera position (x, y, z) and the word after it */
extern float DAT_002be570[3];
extern unsigned int DAT_002be57c;

/* rand() / 32767 + 0.5 */
#define RBB_RAND_HALF() (RBB_RANDF() + 0.5f)

/* the AI parameters of the variation and difficulty (game + 0x20) */
static void rbb_LoadParams(void *ctl)
{
	int row = rbb_Variation * 3 + RBB_I(rbb_Game, 0x20);

	RBB_F(ctl, RBC_P0) = RBD_F(0x002b5328 + row * 0x14);
	RBB_F(ctl, RBC_P1) = RBD_F(0x002b532c + row * 0x14);
	RBB_F(ctl, RBC_P2) = RBD_F(0x002b5330 + row * 0x14);
	RBB_F(ctl, RBC_P3) = RBD_F(0x002b5334 + row * 0x14);
	RBB_F(ctl, RBC_P4) = RBD_F(0x002b5338 + row * 0x14);
}

/* the roll force of the entity: in the air or on the ground */
static float rbb_RollForce(char *e)
{
	return RBB_C(e, 0x18) == 0 ? RBB_F(e, 0x4170) : RBB_F(e, 0x416c);
}

/* steer the entity along heading `a` with its roll force */
static void rbb_SteerAngle(void *ctl, float a)
{
	float fwd[3];
	float v[4];
	float force;

	fwd[0] = 0.0f;
	fwd[1] = 0.0f;
	fwd[2] = 1.0f;
	bmVectorRotateY(v, fwd, a);
	bmVectorNormalise(v, v);
	force = rbb_RollForce(RBC_E(ctl));
	v[0] = v[0] * force;
	v[1] = v[1] * force;
	v[2] = v[2] * force;
	v[3] = 0.0f;   /* not set in the original */
	CRollerballPlayer_SetTargetHeading(RBC_E(ctl), a);
	CRollerballEntity_SetPush(RBC_E(ctl), v);
}

/* an opponent: another active player that is not the team mate */
static int rbb_IsOpponent(void *ctl, char *e)
{
	return e != RBC_E(ctl) && RBB_C(e, RBE_ACTIVE) != 0 && RBB_P(RBC_E(ctl), 0x426c) != e;
}

/* ======================================================================== shader fragment */

/* 0x00274c74  shader fragment vtable */
const void *const rbb_vtable_00274c74[4] = {
	(const void *)FUN_0019a590, /* +0x00 set the constants */
	(const void *)FUN_0019a520, /* +0x04 begin */
	(const void *)FUN_0019a560, /* +0x08 end */
	(const void *)FUN_0003b310, /* +0x0c bactor */
};

/* 0x0019a4e0  (file unknown)
Shader fragment constructor: 3 constants. */
void *CRollerBallShaderFragment_Ctor(void *self)
{
	bShaderFragmentInit((struct BShaderFragment *)self, 0, 3,
		"add r9, %c0, -r0\ndp3 r9.w, r9, r9\nrsq r9.w, r9.w\nmul r9, r9, r9.w\ndp3 r9.w, r9, r2\n"
		"add r9.x, r9.w, %c1.x\nmov oT0.y, %c1.y\nmov oT0.x, r9.x\nmov oD0, %c2.x\n"
		"mul r4, r2, %c1.zzzw\nadd r0, r0, r4\n");
	*(const void *const **)self = rbb_vtable_00274c74;
	RBB_F(self, 0x14) = 0.0f;
	RBB_F(self, 0x18) = 0.0f;
	RBB_F(self, 0x1c) = 10.0f;
	RBB_F(self, 0x20) = 1.0f;
	return self;
}

/* 0x0019a520  (file unknown) */
void CRollerBallShaderFragment_Begin(void)
{
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(2, 1, 0);
	bdPushRenderState(5, 0, 0);
}

/* 0x0019a560  (file unknown) */
void CRollerBallShaderFragment_End(void)
{
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(2);
	bdPopRenderState(5);
}

/* 0x0019a590  (file unknown)
Set the constants at register ctx[2]: c0 the camera position in object space (the matrix
at *ctx + 0x80), c1 = (+0x14, +0x18, +0x1c, 0), c2.x = +0x20 (c2.yzw are not set in the
original; 0 here). */
void CRollerBallShaderFragment_SetConstants(void *self, int *ctx)
{
	float c[12];

	memset(c, 0, sizeof c);
	c[2] = DAT_002be570[2];
	c[1] = DAT_002be570[1];
	c[0] = DAT_002be570[0];
	c[3] = rbb_bitsf(DAT_002be57c);
	bmTransformVector4InPlace((const float *)((char *)(intptr_t)ctx[0] + 0x80), c);
	c[5] = RBB_F(self, 0x18);
	c[6] = RBB_F(self, 0x1c);
	c[4] = RBB_F(self, 0x14);
	c[8] = RBB_F(self, 0x20);
	c[7] = 0.0f;
	D3DDevice_SetVertexShaderConstant(ctx[2], c, 3);
}

/* ======================================================================== shared */

/* 0x0019a620  (file unknown)
Controller [0]: attach to `entity` and reset (virtual +0x10). */
int CRollerBallControl_Init(void *self, void *entity)
{
	RBB_P(self, RBC_ENTITY) = entity;
	RBB_VFN(self, 0x10, void (*)(void *))(self);
	return 1;
}

/* 0x0019a640  (file unknown)
Sumo controller constructor. */
void *CRollerballControlSumo_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274d74;
	return self;
}

/* 0x0019a650  (file unknown)
Race2 controller constructor. */
void *CRollerballControlRace2_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274d98;
	return self;
}

/* 0x0019a660  (file unknown)
Race2 controller reset. */
void CRollerBallControlRace2_Reset(void *self)
{
	RBB_I(self, 0x2c) = 0;
	RBB_I(self, 0x30) = 0;
	RBB_I(self, 0x34) = 0;
	RBB_I(self, 0x38) = 0;
	RBB_I(self, RBC_STATE) = 1;
	rbb_LoadParams(self);
}

/* 0x0019a710  (file unknown)
Race controller constructor. */
void *CRollerballControlRace_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274dbc;
	return self;
}

/* 0x0019a720  (file unknown)
Drop controller constructor. */
void *CRollerballControlDrop_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274de0;
	return self;
}

/* 0x0019a730  (file unknown)
A random opponent (else the current target). */
void *CRollerBallControl_PickRandomTarget(void *self)
{
	char *e;
	int count = 0;
	int r;
	int i;

	for (e = rbb_FirstPlayer; e != NULL; e = RBB_P(e, RBE_NEXT))
		if (rbb_IsOpponent(self, e))
			count++;
	r = rbb_rand();
	/* x86 idiv traps for count 0; then no opponent can be picked either */
	if (count != 0)
		r = r % count;
	i = 0;
	for (e = rbb_FirstPlayer; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		if (rbb_IsOpponent(self, e)) {
			if (r <= i)
				return e;
			i++;
		}
	}
	return RBB_P(self, RBC_TARGET);
}

/* 0x0019a7c0  (file unknown)
The opponent nearest to `pos` (x/z; the first player when none is within 99999). */
void *CRollerBallControl_Nearest(void *self, float *pos)
{
	char *first = rbb_FirstPlayer;
	char *best = first;
	char *e;
	float best_d = 99999.0f;

	for (e = first; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		float dx;
		float dz;
		float d;
		if (!rbb_IsOpponent(self, e))
			continue;
		dx = pos[0] - RBB_F(e, RBE_POS + 0);
		dz = pos[2] - RBB_F(e, RBE_POS + 8);
		d = sqrtf(dx * dx + dz * dz);
		if (d < best_d) {
			best = e;
			best_d = d;
		}
	}
	return best;
}

/* 0x0019a840  (file unknown)
The opponent farthest from `pos` (x/z; the first player when none). */
void *CRollerBallControl_Farthest(void *self, float *pos)
{
	char *first = rbb_FirstPlayer;
	char *best = first;
	char *e;
	float best_d = -99999.0f;

	for (e = first; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		float dx;
		float dz;
		float d;
		if (!rbb_IsOpponent(self, e))
			continue;
		dx = pos[0] - RBB_F(e, RBE_POS + 0);
		dz = pos[2] - RBB_F(e, RBE_POS + 8);
		d = sqrtf(dx * dx + dz * dz);
		if (best_d < d) {
			best = e;
			best_d = d;
		}
	}
	return best;
}

/* 0x0019a8c0  (file unknown)
The nearest opponent within 10 degrees of `heading` (null when none). */
void *CRollerBallControl_InSights(void *self, float heading)
{
	char *e;
	char *best = NULL;
	float best_d = 99999.0f;

	for (e = rbb_FirstPlayer; e != NULL; e = RBB_P(e, RBE_NEXT)) {
		char *me;
		float dx;
		float dz;
		float d;
		if (RBB_C(e, RBE_ACTIVE) == 0)
			continue;
		me = RBC_E(self);
		if (e == me || RBB_P(me, 0x426c) == e)
			continue;
		dx = RBB_F(e, RBE_POS + 0) - RBB_F(me, RBE_POS + 0);
		dz = RBB_F(e, RBE_POS + 8) - RBB_F(me, RBE_POS + 8);
		if (fabsf(CLMath_AngleDelta((float)atan2((double)dx, (double)dz), heading)) < 0.17453292f) {
			d = sqrtf(dx * dx + dz * dz);
			if (d <= best_d) {
				best = e;
				best_d = d;
			}
		}
	}
	return best;
}

/* 0x0019a9b0  (file unknown)
Base controller constructor. */
void *CRollerballControlBase_Ctor(void *self)
{
	*(const void *const **)self = rbb_vtable_00274d50;
	return self;
}

/* 0x0019a9c0  (file unknown)
Base controller scalar deleting destructor. */
void *CRollerBallControlBase_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274d50;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* ======================================================================== base */

/* 0x00274d50  base controller vtable */
const void *const rbb_vtable_00274d50[9] = {
	(const void *)FUN_0019a620, /* +0x00 init */
	(const void *)FUN_0019ccd0, /* +0x04 update */
	(const void *)FUN_001d5950, /* +0x08 (empty) */
	(const void *)FUN_001d5950, /* +0x0c shutdown (empty) */
	(const void *)FUN_0019aee0, /* +0x10 reset */
	(const void *)FUN_0019b0e0, /* +0x14 re-roll the timers */
	(const void *)FUN_001ec260, /* +0x18 (empty) */
	(const void *)FUN_001ec260, /* +0x1c hit (empty) */
	(const void *)FUN_0019a9c0, /* +0x20 scalar deleting destructor */
};

/* 0x0019a9e0  (file unknown)
Base: the way to each lit goal; the steering jitter; now and then pick the nearest lit
goal. (The jitter angle doubles its own old value, not its range: as the original.) */
void CRollerBallControlBase_Think(void *self, float dt)
{
	char *rec;
	char *goal;
	char *pick;
	float best;
	int i;

	for (i = 0; i < 9; i++) {
		rec = (char *)self + RBB_GOALS + i * 0x20;
		if (RBB_C(rec, 0x1c) != 0) {
			RBB_F(rec, 0x10) = RBB_F(rec, 0x00) - RBB_F(RBC_E(self), RBE_POS + 0);
			RBB_F(rec, 0x14) = RBB_F(rec, 0x08) - RBB_F(RBC_E(self), RBE_POS + 8);
			RBB_F(rec, 0x18) = sqrtf(RBB_F(rec, 0x14) * RBB_F(rec, 0x14) + RBB_F(rec, 0x10) * RBB_F(rec, 0x10));
		}
	}
	RBB_F(self, RBB_JITTER_TIMER) = RBB_F(self, RBB_JITTER_TIMER) - dt;
	if (RBB_F(self, RBB_JITTER_TIMER) < 0.0f) {
		RBB_F(self, RBB_JITTER_TIMER) = RBB_RAND_HALF() * RBB_F(self, RBB_JITTER_PERIOD);
		RBB_F(self, RBB_JITTER) = RBB_RANDF() * (RBB_F(self, RBB_JITTER) + RBB_F(self, RBB_JITTER)) -
			RBB_F(self, RBB_JITTER_RANGE);
	}
	RBB_F(self, RBB_DECIDE_TIMER) = RBB_F(self, RBB_DECIDE_TIMER) - dt;
	if (RBB_F(self, RBB_DECIDE_TIMER) >= 0.0f)
		return;
	RBB_F(self, RBB_DECIDE_TIMER) = RBB_RAND_HALF() * RBB_F(self, RBB_DECIDE_PERIOD);
	rec = (char *)self + RBB_GOALS;
	for (goal = RBB_P(rbb_GoalSystem, RBGS_GOAL_LIST); goal != NULL; goal = RBB_P(goal, RBG_NEXT)) {
		RBB_B(rec, 0x1c) = RBB_S(goal, RBG_STATE) != RBG_STATE_IDLE;
		rec += 0x20;
	}
	RBB_P(self, RBB_GOAL_TARGET) = NULL;
	pick = NULL;
	best = 99999.0f;
	for (i = 0; i < 9; i++) {
		rec = (char *)self + RBB_GOALS + i * 0x20;
		if (RBB_C(rec, 0x1c) != 0 && RBB_F(rec, 0x18) < best) {
			pick = rec;
			best = RBB_F(rec, 0x18);
		}
	}
	if (pick != NULL)
		RBB_P(self, RBB_GOAL_TARGET) = pick;
}

/* 0x0019acc0  (file unknown)
Base state 1: roll at the target goal (with the jitter); within 250 go to state 2. */
void CRollerBallControlBase_Chase(void *self)
{
	char *rec = RBB_P(self, RBB_GOAL_TARGET);

	if (rec == NULL)
		return;
	RBB_P(self, RBB_GOAL_LAST) = rec;
	if (RBB_F(rec, 0x18) < 250.0f) {
		RBB_I(self, RBC_STATE) = 2;
		return;
	}
	if (0.0f < RBB_F(rec, 0x18) && (RBB_F(rec, 0x10) != 0.0f || RBB_F(rec, 0x14) != 0.0f))
		rbb_SteerAngle(self, (float)(atan2((double)RBB_F(rec, 0x10), (double)RBB_F(rec, 0x14)) +
			(double)RBB_F(self, RBB_JITTER)));
}

/* 0x0019ade0  (file unknown)
Base state 2: close to the goal, roll straight at it; beyond 250 go back to state 0. */
void CRollerBallControlBase_Approach(void *self)
{
	char *rec = RBB_P(self, RBB_GOAL_TARGET);
	float v[4];
	float force;

	if (rec == NULL)
		return;
	RBB_P(self, RBB_GOAL_LAST) = rec;
	if (250.0f < RBB_F(rec, 0x18)) {
		RBB_I(self, RBC_STATE) = 0;
		return;
	}
	if (0.0f < RBB_F(rec, 0x18) && (RBB_F(rec, 0x10) != 0.0f || RBB_F(rec, 0x14) != 0.0f)) {
		v[0] = RBB_F(rec, 0x10);
		v[1] = 0.0f;
		v[2] = RBB_F(rec, 0x14);
		bmVectorNormalise(v, v);
		force = rbb_RollForce(RBC_E(self));
		v[0] = v[0] * force;
		v[1] = v[1] * force;
		v[2] = force * v[2];
		v[3] = 0.0f;   /* not set in the original */
		CRollerballPlayer_SetTargetHeading(RBC_E(self), (float)atan2((double)v[0], (double)v[2]));
		CRollerballEntity_SetPush(RBC_E(self), v);
	}
}

/* the base controller's random timers (decision, jitter) from the parameters */
static void rbb_BaseRollTimers(void *self, float p_decide)
{
	RBB_F(self, RBB_DECIDE_PERIOD) = (1.0f - p_decide) * 120.0f;
	RBB_F(self, RBB_DECIDE_TIMER) = RBB_RAND_HALF() * RBB_F(self, RBB_DECIDE_PERIOD);
	RBB_F(self, RBB_JITTER_PERIOD) = (1.0f - RBB_F(self, RBC_P3)) * 150.0f;
	RBB_F(self, RBB_JITTER_TIMER) = RBB_RAND_HALF() * RBB_F(self, RBB_JITTER_PERIOD);
	RBB_F(self, RBB_JITTER_RANGE) = (1.0f - RBB_F(self, RBC_P3)) * 0.0174532924f;
	RBB_F(self, RBB_JITTER) = (RBB_F(self, RBB_JITTER_RANGE) + RBB_F(self, RBB_JITTER_RANGE)) * RBB_RANDF() -
		RBB_F(self, RBB_JITTER_RANGE);
}

/* 0x0019aee0  (file unknown)
Base reset: the goal positions, state 0, the parameters and timers. */
void CRollerBallControlBase_Reset(void *self)
{
	char *goal;
	char *rec;

	RBB_I(self, 0x2c) = 0;
	RBB_I(self, 0x30) = 0;
	RBB_I(self, 0x34) = 0;
	RBB_I(self, 0x38) = 0;
	rec = (char *)self + RBB_GOALS;
	for (goal = RBB_P(rbb_GoalSystem, RBGS_GOAL_LIST); goal != NULL; goal = RBB_P(goal, RBG_NEXT)) {
		memcpy(RBB_V(rec, 0x00), RBB_V(goal, RBG_POS), 16);
		RBB_F(rec, 0x14) = 0.0f;
		RBB_F(rec, 0x10) = 0.0f;
		RBB_F(rec, 0x18) = 0.0f;
		RBB_B(rec, 0x1c) = 0;
		rec += 0x20;
	}
	RBB_P(self, RBB_GOAL_TARGET) = NULL;
	RBB_I(self, RBC_STATE) = 0;
	rbb_LoadParams(self);
	rbb_BaseRollTimers(self, RBB_F(self, RBC_P4));
}

/* 0x0019b0e0  (file unknown)
Base: re-roll the timers. */
void CRollerBallControlBase_RollTimers(void *self)
{
	rbb_BaseRollTimers(self, RBB_F(self, RBC_P4));
}

/* 0x0019ccd0  (file unknown)
Base update: think, then the state (0 starts the chase). */
void CRollerBallControlBase_Update(void *self, float dt)
{
	CRollerBallControlBase_Think(self, dt);
	switch (RBB_I(self, RBC_STATE)) {
	case 0:
		RBB_I(self, RBC_STATE) = 1;
		break;
	case 1:
		CRollerBallControlBase_Chase(self);
		break;
	case 2:
		CRollerBallControlBase_Approach(self);
		break;
	}
}

/* ======================================================================== sumo and drop */

/* 0x00274d74  sumo controller vtable */
const void *const rbb_vtable_00274d74[9] = {
	(const void *)FUN_0019a620, /* +0x00 init */
	(const void *)FUN_0019cd20, /* +0x04 update */
	(const void *)FUN_001d5950, /* +0x08 (empty) */
	(const void *)FUN_001d5950, /* +0x0c shutdown (empty) */
	(const void *)FUN_0019b830, /* +0x10 reset */
	(const void *)FUN_0019baa0, /* +0x14 re-roll the timers */
	(const void *)FUN_001ec260, /* +0x18 (empty) */
	(const void *)FUN_0019b7a0, /* +0x1c hit by a ball */
	(const void *)FUN_0019b1f0, /* +0x20 scalar deleting destructor */
};

/* 0x00274de0  drop controller vtable */
const void *const rbb_vtable_00274de0[9] = {
	(const void *)FUN_0019a620, /* +0x00 init */
	(const void *)FUN_0019cde0, /* +0x04 update */
	(const void *)FUN_001d5950, /* +0x08 (empty) */
	(const void *)FUN_001d5950, /* +0x0c shutdown (empty) */
	(const void *)FUN_0019c420, /* +0x10 reset */
	(const void *)FUN_0019baa0, /* +0x14 re-roll the timers */
	(const void *)FUN_001ec260, /* +0x18 (empty) */
	(const void *)FUN_0019b7a0, /* +0x1c hit by a ball */
	(const void *)FUN_0019c000, /* +0x20 scalar deleting destructor */
};

/* moving fast at an opponent closer to the centre: attack it (returns 1) */
static int rbb_ArenaSpotVictim(void *self)
{
	char *me = RBC_E(self);
	double vx = (double)RBB_F(me, RBE_VEL + 0);
	double vz = (double)RBB_F(me, RBE_VEL + 8);
	double speed = sqrt(vx * vx + vz * vz);
	char *victim;

	if (!((double)RBB_F(me, 0x416c) * 10.0 <= speed))
		return 0;
	victim = CRollerBallControl_InSights(self, (float)atan2(vx, vz));
	if (victim == NULL || !(RBB_F(RBB_P(victim, RBE_CONTROLLER), RBA_DIST) < RBB_F(self, RBA_DIST)))
		return 0;
	RBB_P(self, RBC_TARGET) = victim;
	RBB_F(self, RBA_RETARGET) = RBB_RAND_HALF() * 200.0f;
	return 1;
}

/* steer at the target */
static void rbb_ArenaSteerAtTarget(void *self)
{
	char *t = RBB_P(self, RBC_TARGET);
	char *me = RBC_E(self);
	double a = atan2((double)RBB_F(t, RBE_POS + 0) - (double)RBB_F(me, RBE_POS + 0),
		(double)RBB_F(t, RBE_POS + 8) - (double)RBB_F(me, RBE_POS + 8));

	rbb_SteerAngle(self, (float)(a + (double)RBB_F(self, RBA_JITTER)));
}

/* steer towards the centre, turned by `offset` */
static void rbb_ArenaSteerToCentre(void *self, float offset)
{
	double a = atan2((double)RBB_F(self, RBA_TO_CENTRE + 0), (double)RBB_F(self, RBA_TO_CENTRE + 4));

	rbb_SteerAngle(self, (float)(a + (double)offset + (double)RBB_F(self, RBA_JITTER)));
}

/* 0x0019b210  (file unknown)
Sumo/drop state 1: attack the target; beyond the safe margin go to state 2. */
void CRollerBallControlArena_Attack(void *self)
{
	if (!rbb_ArenaSpotVictim(self)) {
		if (RBB_F(self, RBA_RADIUS) - RBB_F(self, RBA_MARGIN) <= RBB_F(self, RBA_DIST)) {
			RBB_I(self, RBC_STATE) = 2;
			return;
		}
	}
	rbb_ArenaSteerAtTarget(self);
}

/* 0x0019b3a0  (file unknown)
Sumo/drop state 2: back towards the centre, passing a nearby opponent on one side (20
degrees), until within the safe margin (then state 1). */
void CRollerBallControlArena_Recover(void *self)
{
	char *other;
	float offset;

	if (!(RBB_F(self, RBA_RADIUS) - RBB_F(self, RBA_MARGIN) <= RBB_F(self, RBA_DIST))) {
		RBB_I(self, RBC_STATE) = 1;
		return;
	}
	other = CRollerBallControl_Nearest(self, RBB_V(RBC_E(self), RBE_POS));
	if (other != NULL) {
		char *me = RBC_E(self);
		float d = bmVectorDistance(RBB_V(me, RBE_POS), RBB_V(other, RBE_POS));
		if ((double)RBB_F(me, RBE_RADIUS) + (double)RBB_F(me, RBE_RADIUS) + 25.0 <= (double)d) {
			RBB_I(self, RBA_AVOIDING) = 0;
		} else if (RBB_I(self, RBA_AVOIDING) == 0) {
			float centre = (float)atan2(-(double)RBB_F(me, RBE_POS + 0), -(double)RBB_F(me, RBE_POS + 8));
			float to_other = (float)atan2((double)RBB_F(other, RBE_POS + 0) - (double)RBB_F(me, RBE_POS + 0),
				(double)RBB_F(other, RBE_POS + 8) - (double)RBB_F(me, RBE_POS + 8));
			RBB_I(self, RBA_DODGE_SIDE) = 0.0f <= CLMath_AngleDelta(centre, to_other) ? 1 : 0;
			RBB_I(self, RBA_AVOIDING) = 1;
		}
	}
	if (RBB_I(self, RBA_AVOIDING) == 0)
		offset = 0.0f;
	else if (RBB_I(self, RBA_DODGE_SIDE) == 0)
		offset = -0.34906584f;
	else
		offset = 0.34906584f;
	rbb_ArenaSteerToCentre(self, offset);
}

/* sumo/drop state 3 (dodge): run out of time or see a victim to go back to state 0, else
weave around the centre (wide inside the safe margin, narrow outside) */
static void rbb_ArenaDodge(void *self, float dt, float wide, float narrow)
{
	float offset;

	RBB_F(self, RBA_DODGE_TIME) = RBB_F(self, RBA_DODGE_TIME) - dt;
	if (RBB_F(self, RBA_DODGE_TIME) < 0.0f || rbb_ArenaSpotVictim(self)) {
		RBB_I(self, RBC_STATE) = 0;
		return;
	}
	if (RBB_I(self, RBA_DODGE_START) != 0) {
		char *me = RBC_E(self);
		char *other;
		RBB_I(self, RBA_DODGE_START) = 0;
		other = CRollerBallControl_Nearest(self, RBB_V(me, RBE_POS));
		if (other != NULL) {
			/* the original computes the side of the nearest opponent and drops it */
			float to_other = (float)atan2((double)RBB_F(other, RBE_POS + 0) - (double)RBB_F(me, RBE_POS + 0),
				(double)RBB_F(other, RBE_POS + 8) - (double)RBB_F(me, RBE_POS + 8));
			float centre = (float)atan2(-(double)RBB_F(me, RBE_POS + 0), -(double)RBB_F(me, RBE_POS + 8));
			CLMath_AngleDelta(centre, to_other);
		}
		RBB_I(self, RBA_DODGE_SIDE) = 1;
	}
	if (RBB_F(self, RBA_RADIUS) - RBB_F(self, RBA_MARGIN) > RBB_F(self, RBA_DIST))
		offset = RBB_I(self, RBA_DODGE_SIDE) == 0 ? -wide : wide;
	else
		offset = RBB_I(self, RBA_DODGE_SIDE) == 0 ? -narrow : narrow;
	rbb_ArenaSteerToCentre(self, offset);
}

/* 0x0019b570  (file unknown)
Sumo state 3: dodge (40 / 10 degrees). */
void CRollerBallControlSumo_Dodge(void *self, float dt)
{
	rbb_ArenaDodge(self, dt, 0.69813168f, 0.17453292f);
}

/* 0x0019b7a0  (file unknown)
Sumo/drop [7]: hit by `other`: with the aggressiveness as the chance, make it the target. */
void CRollerBallControlArena_Hit(void *self, void *other)
{
	if (other == RBB_P(RBC_E(self), 0x426c))
		return;
	if (RBB_RANDF() < RBB_F(self, RBC_P0)) {
		float r = RBB_RAND_HALF();
		RBB_P(self, RBC_TARGET) = other;
		RBB_F(self, RBA_RETARGET) = r * 200.0f;
	}
}

/* the sumo/drop timers (re-target, jitter, dodge) */
static void rbb_ArenaRollTimers(void *self)
{
	void *target;
	float r;

	RBB_F(self, RBA_MARGIN_BASE) = RBB_F(self, RBC_P2) * 300.0f;
	RBB_F(self, RBA_MARGIN_SPEED) = RBB_F(self, RBC_P2) * 30.0f;
	RBB_F(self, RBA_TIME_LIMIT) = RBB_F(self, RBC_P2) * 4.0f + 1.0f;
	RBB_F(self, RBA_RETARGET) = RBB_RAND_HALF() * 200.0f;
	target = CRollerBallControl_PickRandomTarget(self);
	RBB_P(self, RBC_TARGET) = target;
	RBB_F(self, RBA_JITTER_PERIOD) = (1.0f - RBB_F(self, RBC_P3)) * 150.0f;
	RBB_F(self, RBA_JITTER_TIMER) = RBB_RAND_HALF() * RBB_F(self, RBA_JITTER_PERIOD);
	RBB_F(self, RBA_JITTER_RANGE) = (1.0f - RBB_F(self, RBC_P3)) * 0.0174532924f;
	RBB_F(self, RBA_JITTER) = (RBB_F(self, RBA_JITTER_RANGE) + RBB_F(self, RBA_JITTER_RANGE)) * RBB_RANDF() -
		RBB_F(self, RBA_JITTER_RANGE);
	RBB_F(self, RBA_DODGE_PERIOD) = RBB_RANDF() * 400.0f + 400.0f;
	r = RBB_RAND_HALF();
	RBB_I(self, RBA_DODGE_START) = 0;
	RBB_I(self, RBA_AVOIDING) = 0;
	RBB_I(self, RBA_DODGE_SIDE) = 0;
	RBB_F(self, RBA_DODGE_TIMER) = r * RBB_F(self, RBA_DODGE_PERIOD);
}

/* sumo/drop reset with the arena radius `radius` */
static void rbb_ArenaReset(void *self, float radius)
{
	RBB_I(self, 0x2c) = 0;
	RBB_I(self, 0x30) = 0;
	RBB_I(self, 0x34) = 0;
	RBB_I(self, 0x38) = 0;
	RBB_I(self, RBC_STATE) = 0;
	rbb_LoadParams(self);
	RBB_F(self, RBA_CENTRE + 0) = 0.0f;
	RBB_F(self, RBA_CENTRE + 4) = 0.0f;
	RBB_F(self, RBA_CENTRE + 8) = 0.0f;
	RBB_F(self, RBA_RADIUS) = radius;
	rbb_ArenaRollTimers(self);
}

/* 0x0019b830  (file unknown)
Sumo reset (arena radius: the first entry at 0x002b4d08, 1180). */
void CRollerBallControlSumo_Reset(void *self)
{
	rbb_ArenaReset(self, RBD_F(0x002b4d08));
}

/* 0x0019baa0  (file unknown)
Sumo/drop: re-roll the timers. */
void CRollerBallControlArena_RollTimers(void *self)
{
	rbb_ArenaRollTimers(self);
}

/* 0x0019c000  (file unknown)
Drop controller scalar deleting destructor. */
void *CRollerBallControlDrop_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274de0;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x0019b1f0  (file unknown)
Sumo controller scalar deleting destructor. */
void *CRollerBallControlSumo_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274d74;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x0019c020  (file unknown)
Drop state 3: dodge (50 / 20 degrees). */
void CRollerBallControlDrop_Dodge(void *self, float dt)
{
	rbb_ArenaDodge(self, dt, 0.87266463f, 0.34906584f);
}

/* 0x0019c250  (file unknown)
Choose a target: keep a target near the edge that this player is rolling fast at;
else, unless clever (rand >= cleverness: a random opponent), the opponent farthest
from the centre (with the vindictiveness as the chance) or from this player. */
void CRollerBallControlArena_ChooseTarget(void *self)
{
	char *t = RBB_P(self, RBC_TARGET);
	float d = 0.0f;

	if (t != NULL) {
		float dx = RBB_F(t, RBE_POS + 0) - RBB_F(self, RBA_CENTRE + 0);
		float dz = RBB_F(t, RBE_POS + 8) - RBB_F(self, RBA_CENTRE + 8);
		d = sqrtf(dx * dx + dz * dz);
	}
	if (RBB_F(self, RBA_RADIUS) - 600.0f <= d) {
		char *me = RBC_E(self);
		double vz = (double)RBB_F(me, RBE_VEL + 8);
		double vx = (double)RBB_F(me, RBE_VEL + 0);
		double speed = sqrt(vx * vx + vz * vz);
		if ((double)RBB_F(me, 0x416c) * 10.0 <= speed) {
			float moving = (float)atan2(vx, vz);
			float to_t = (float)atan2((double)RBB_F(t, RBE_POS + 0) - (double)RBB_F(me, RBE_POS + 0),
				(double)RBB_F(t, RBE_POS + 8) - (double)RBB_F(me, RBE_POS + 8));
			if (fabsf(CLMath_AngleDelta(to_t, moving)) < 0.17453292f &&
				RBB_F(RBB_P(RBB_P(self, RBC_TARGET), RBE_CONTROLLER), RBA_DIST) < RBB_F(self, RBA_DIST))
				return;
		}
	}
	if (RBB_F(self, RBC_P2) <= RBB_RANDF()) {
		RBB_P(self, RBC_TARGET) = CRollerBallControl_PickRandomTarget(self);
		return;
	}
	if (RBB_RANDF() < RBB_F(self, RBC_P1)) {
		float origin[3];
		origin[0] = 0.0f;
		origin[1] = 0.0f;
		origin[2] = 0.0f;
		RBB_P(self, RBC_TARGET) = CRollerBallControl_Farthest(self, origin);
		return;
	}
	RBB_P(self, RBC_TARGET) = CRollerBallControl_Farthest(self, RBB_V(RBC_E(self), RBE_POS));
}

/* 0x0019c420  (file unknown)
Drop reset (arena radius: the first entry at 0x002b4da0, 885). */
void CRollerBallControlDrop_Reset(void *self)
{
	rbb_ArenaReset(self, RBD_F(0x002b4da0));
}

/* the per-frame part of sumo/drop: margin, distance from the centre, jitter, re-target,
dodge timer, and the arena radius from the variation object (virtual `fn_a`, `fn_b`,
`fn_t`: before the time limit the second radius, else the first) */
static void rbb_ArenaThink(void *self, float dt, int fn_a, int fn_b, int fn_t)
{
	char *me = RBC_E(self);
	void *var;
	float ra;
	float rb;
	float t;

	RBB_F(self, RBA_MARGIN) = sqrtf(RBB_F(me, RBE_VEL + 0) * RBB_F(me, RBE_VEL + 0) +
		RBB_F(me, RBE_VEL + 8) * RBB_F(me, RBE_VEL + 8)) * RBB_F(self, RBA_MARGIN_SPEED) +
		RBB_F(self, RBA_MARGIN_BASE);
	RBB_F(self, RBA_TO_CENTRE + 0) = RBB_F(self, RBA_CENTRE + 0) - RBB_F(me, RBE_POS + 0);
	RBB_F(self, RBA_TO_CENTRE + 4) = RBB_F(self, RBA_CENTRE + 8) - RBB_F(me, RBE_POS + 8);
	RBB_F(self, RBA_DIST) = sqrtf(RBB_F(self, RBA_TO_CENTRE + 4) * RBB_F(self, RBA_TO_CENTRE + 4) +
		RBB_F(self, RBA_TO_CENTRE + 0) * RBB_F(self, RBA_TO_CENTRE + 0));
	RBB_F(self, RBA_JITTER_TIMER) = RBB_F(self, RBA_JITTER_TIMER) - dt;
	if (RBB_F(self, RBA_JITTER_TIMER) < 0.0f) {
		RBB_F(self, RBA_JITTER_TIMER) = RBB_RAND_HALF() * RBB_F(self, RBA_JITTER_PERIOD);
		/* the original doubles the old angle, not the range */
		RBB_F(self, RBA_JITTER) = RBB_RANDF() * (RBB_F(self, RBA_JITTER) + RBB_F(self, RBA_JITTER)) -
			RBB_F(self, RBA_JITTER_RANGE);
	}
	RBB_F(self, RBA_RETARGET) = RBB_F(self, RBA_RETARGET) - dt;
	if (RBB_F(self, RBA_RETARGET) <= 0.0f || RBB_C(RBB_P(self, RBC_TARGET), RBE_ACTIVE) == 0) {
		RBB_F(self, RBA_RETARGET) = RBB_RAND_HALF() * 200.0f;
		CRollerBallControlArena_ChooseTarget(self);
	}
	if (RBB_I(self, RBC_STATE) != 3) {
		RBB_F(self, RBA_DODGE_TIMER) = RBB_F(self, RBA_DODGE_TIMER) - dt;
		if (RBB_F(self, RBA_DODGE_TIMER) < 0.0f) {
			float r = RBB_RAND_HALF();
			RBB_I(self, RBC_STATE) = 3;
			RBB_F(self, RBA_DODGE_TIMER) = r * RBB_F(self, RBA_DODGE_PERIOD);
			r = RBB_RANDF();
			RBB_I(self, RBA_DODGE_START) = 1;
			RBB_F(self, RBA_DODGE_TIME) = r * 200.0f + 100.0f;
		}
	}
	var = RBB_P(rbb_Game, 0x26f0);
	ra = RBB_VFN(var, fn_a, float (*)(void *))(var);
	var = RBB_P(rbb_Game, 0x26f0);
	rb = RBB_VFN(var, fn_b, float (*)(void *))(var);
	var = RBB_P(rbb_Game, 0x26f0);
	t = RBB_VFN(var, fn_t, float (*)(void *))(var);
	if (t < RBB_F(self, RBA_TIME_LIMIT)) {
		RBB_F(self, RBA_RADIUS) = rb;
		return;
	}
	RBB_F(self, RBA_RADIUS) = ra;
}

/* 0x0019c690  (file unknown)
Sumo per-frame thinking (the sumo variation's radii and time: virtual +0x24, +0x28,
+0x2c). */
void CRollerBallControlSumo_Think(void *self, float dt)
{
	rbb_ArenaThink(self, dt, 0x24, 0x28, 0x2c);
}

/* 0x0019c900  (file unknown)
Race: now and then change lane. */
void CRollerBallControlRace_Think(void *self, float dt)
{
	RBB_F(self, RBR_LANE_TIMER) = RBB_F(self, RBR_LANE_TIMER) - dt;
	if (RBB_F(self, RBR_LANE_TIMER) <= 0.0f) {
		RBB_F(self, RBR_LANE_TIMER) = RBB_RANDF() * 50.0f + 50.0f;
		CRollerBallControlRace_PickLane(self);
	}
}

/* 0x0019c960  (file unknown)
Race reset: state 1, a lane, the lane timer. */
void CRollerBallControlRace_Reset(void *self)
{
	RBB_I(self, 0x2c) = 0;
	RBB_I(self, 0x30) = 0;
	RBB_I(self, 0x34) = 0;
	RBB_I(self, 0x38) = 0;
	RBB_I(self, RBC_STATE) = 1;
	rbb_LoadParams(self);
	RBB_I(self, RBR_LANE) = -1;
	CRollerBallControlRace_PickLane(self);
	RBB_F(self, RBR_LANE_TIMER) = RBB_RANDF() * 50.0f + 50.0f;
}

/* 0x0019ca60  (file unknown)
Drop per-frame thinking (the drop variation's radii and time: virtual +0x38, +0x3c,
+0x40). */
void CRollerBallControlDrop_Think(void *self, float dt)
{
	rbb_ArenaThink(self, dt, 0x38, 0x3c, 0x40);
}

/* 0x0019cd20  (file unknown)
Sumo update (while the player is in the game). */
void CRollerBallControlSumo_Update(void *self, float dt)
{
	if (RBB_C(RBC_E(self), RBE_ACTIVE) == 0)
		return;
	CRollerBallControlSumo_Think(self, dt);
	switch (RBB_I(self, RBC_STATE)) {
	case 0:
		RBB_I(self, RBC_STATE) = 1;
		return;
	case 1:
		CRollerBallControlArena_Attack(self);
		return;
	case 2:
		CRollerBallControlArena_Recover(self);
		return;
	case 3:
		CRollerBallControlSumo_Dodge(self, dt);
		return;
	}
}

/* 0x0019cde0  (file unknown)
Drop update (while the player is in the game). */
void CRollerBallControlDrop_Update(void *self, float dt)
{
	if (RBB_C(RBC_E(self), RBE_ACTIVE) == 0)
		return;
	CRollerBallControlDrop_Think(self, dt);
	switch (RBB_I(self, RBC_STATE)) {
	case 0:
		RBB_I(self, RBC_STATE) = 1;
		return;
	case 1:
		CRollerBallControlArena_Attack(self);
		return;
	case 2:
		CRollerBallControlArena_Recover(self);
		return;
	case 3:
		CRollerBallControlDrop_Dodge(self, dt);
		return;
	}
}

/* ======================================================================== race2 and race */

/* 0x00274d98  race2 controller vtable */
const void *const rbb_vtable_00274d98[9] = {
	(const void *)FUN_0019a620, /* +0x00 init */
	(const void *)FUN_0019cd90, /* +0x04 update */
	(const void *)FUN_001d5950, /* +0x08 (empty) */
	(const void *)FUN_001d5950, /* +0x0c shutdown (empty) */
	(const void *)FUN_0019a660, /* +0x10 reset */
	(const void *)FUN_001d5950, /* +0x14 (empty) */
	(const void *)FUN_001ec260, /* +0x18 (empty) */
	(const void *)FUN_001ec260, /* +0x1c hit (empty) */
	(const void *)FUN_0019bc40, /* +0x20 scalar deleting destructor */
};

/* 0x00274dbc  race controller vtable */
const void *const rbb_vtable_00274dbc[9] = {
	(const void *)FUN_0019a620, /* +0x00 init */
	(const void *)FUN_0019cdb0, /* +0x04 update */
	(const void *)FUN_001d5950, /* +0x08 (empty) */
	(const void *)FUN_001d5950, /* +0x0c shutdown (empty) */
	(const void *)FUN_0019c960, /* +0x10 reset */
	(const void *)FUN_001d5950, /* +0x14 (empty) */
	(const void *)FUN_001ec260, /* +0x18 (empty) */
	(const void *)FUN_001ec260, /* +0x1c hit (empty) */
	(const void *)FUN_0019bf90, /* +0x20 scalar deleting destructor */
};

/* 0x0019bc40  (file unknown)
Race2 controller scalar deleting destructor. */
void *CRollerBallControlRace2_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274d98;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* the heading to the path point (+0x3c, +0x44), bent towards the current direction of
travel by the accuracy (no more than 90 degrees from the point); returns the straight
heading in *direct */
static float rbb_PathHeading(void *self, float *direct, float *dist)
{
	char *me = RBC_E(self);
	double dx = (double)RBB_F(self, 0x3c) - (double)RBB_F(me, RBE_POS + 0);
	double dz = (double)RBB_F(self, 0x44) - (double)RBB_F(me, RBE_POS + 8);
	double h;
	float a;

	if (dist != NULL)
		*dist = (float)sqrt(dx * dx + dz * dz);
	h = atan2(dx, dz);
	*direct = (float)h;
	if ((double)RBB_F(me, RBE_VEL + 0) != 0.0 || (double)RBB_F(me, RBE_VEL + 8) != 0.0) {
		double moving = atan2((double)RBB_F(me, RBE_VEL + 0), (double)RBB_F(me, RBE_VEL + 8));
		h = (double)CLMath_AngleDelta(*direct, (float)moving) * (double)RBB_F(self, RBC_P3) + (double)*direct;
	}
	a = (float)h;
	if (1.57079637f <= fabsf(CLMath_AngleDelta(*direct, a)))
		a = *direct;
	return a;
}

/* push along heading `a` with strength `force` (no push when it is 0 sideways) */
static void rbb_PathPush(void *self, float a, float force)
{
	float fwd[4];
	float dir[3];
	float v[4];

	fwd[0] = 0.0f;
	fwd[1] = 0.0f;
	fwd[2] = 1.0f;
	fwd[3] = 0.0f;
	bmVectorRotateY(dir, fwd, a);
	v[0] = dir[0] * force;
	v[1] = dir[1] * force;
	v[2] = dir[2] * force;
	v[3] = 0.0f;   /* not set in the original */
	if (v[0] != 0.0f || v[2] != 0.0f) {
		CRollerballPlayer_SetTargetHeading(RBC_E(self), (float)atan2((double)v[0], (double)v[2]));
		CRollerballEntity_SetPush(RBC_E(self), v);
	}
}

/* 0x0019bc60  (file unknown)
Race/race2 state 1: roll at the path point with the full roll force. */
void CRollerBallControlRace_Seek(void *self)
{
	float direct;
	float a = rbb_PathHeading(self, &direct, NULL);

	rbb_PathPush(self, a, rbb_RollForce(RBC_E(self)));
}

/* 0x0019bdd0  (file unknown; missing from the function table)
Race2 state 2: approach the path point: half the air roll force beyond 400, less closer
(none within 10). */
void FUN_0019bdd0(void *self)
{
	float direct;
	float dist;
	float a = rbb_PathHeading(self, &direct, &dist);
	float force;

	if (!(dist <= 400.0f)) {
		force = RBB_F(RBC_E(self), 0x416c) * 0.5f;
	} else {
		force = 0.0f;
		if (!(dist <= 10.0f))
			force = RBB_F(RBC_E(self), 0x416c) * 0.5f * dist * 0.00249999994f;
	}
	rbb_PathPush(self, a, force);
}

/* 0x0019bf90  (file unknown)
Race controller scalar deleting destructor. */
void *CRollerBallControlRace_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = rbb_vtable_00274dbc;
	if (flags & 1)
		RBB_DELETE(self);
	return self;
}

/* 0x0019bfb0  (file unknown)
Race: pick another lane (0-3). */
void CRollerBallControlRace_PickLane(void *self)
{
	unsigned int lane;

	do {
		DAT_00285630 = DAT_00285630 * 0x343fdu + 0x269ec3u;
		lane = (DAT_00285630 >> 16) & 3;
	} while (lane == RBB_U(self, RBR_LANE));
	RBB_U(self, RBR_LANE) = lane;
}

/* 0x0019cd90  (file unknown)
Race2 update (while the player is in the game). */
void CRollerBallControlRace2_Update(void *self, float dt)
{
	(void)dt;
	if (RBB_C(RBC_E(self), RBE_ACTIVE) == 0)
		return;
	if (RBB_I(self, RBC_STATE) == 1)
		CRollerBallControlRace_Seek(self);
	else if (RBB_I(self, RBC_STATE) == 2)
		FUN_0019bdd0(self);
}

/* 0x0019cdb0  (file unknown)
Race update (while the player is in the game). */
void CRollerBallControlRace_Update(void *self, float dt)
{
	if (RBB_C(RBC_E(self), RBE_ACTIVE) == 0)
		return;
	CRollerBallControlRace_Think(self, dt);
	if (RBB_I(self, RBC_STATE) == 1)
		CRollerBallControlRace_Seek(self);
}
