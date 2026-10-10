/*
RollerBall/CRollerball_Goal.c  <-  C:\BlitzParty\Game\Src\RollerBall\CRollerball_Goal.cpp

The goal system (game +0x1470, vtable 0x00274778), first part (rollerball_b has the rest,
0x001915c0..): +8 and +0xc two resources, +0x10 the goals (linked by +4), +0x14 the goals
by id, +0x38 their count, +0x48 a timer, +0x80 three flares (0x68 bytes, vtable
0x00274768), +0x1b8 eight particle emitters (0xc4 bytes, vtable 0x00274770).

A goal (0x78 bytes, vtable 0x00274774): +4 next, +8 the player that scored, +0xc position,
+0x1c direction, +0x2c radius, +0x30, +0x32 state (4: scored), +0x34 id, +0x38 / +0x3c two
actor instances, +0x40 / +0x58 their draw flags, +0x44 / +0x5c the instances again with
+0x48 / +0x60 the "active" and "flare" sub-objects, +0x64 the flare's ambient colour,
+0x74 the glow.

A flare: +8 a point light, +0x10 a timer, +0x48 colour (3 shorts), +0x4e burning,
+0x50 position, +0x60 age. An emitter: +0x14 active, +0x15 visible, +0x18 age, +0x1c a
CParticleSystem (classlib_fx).

Functions at 0x00190b80-0x00191040 are gap functions before the first tag of this file;
they belong to these classes and are placed here (inferred).
*/

#include "rollerball_a_ext.h"

/* the particle system parameters of classlib_fx's CParticleSystem_Init (0x40 bytes) */
struct RB_PSParams {
	unsigned char active, unknown51;
	short pad52;
	float angleSpeed, riseAccel, gravity, riseSpeed, speed, drag, radiusSpeed, radius;
	int numRings, ringLength, maxEmit, lifeMin, lifeRange;
	float pointSize, emitTime;
};
OF_ASSERT_SIZE(struct RB_PSParams, 0x40);

/* 0x00274768  vtable of a flare */
const void *const vtable_00274768[1] = { (const void *)CRollerBallFlare_DeletingDtor };
/* 0x00274770  vtable of a goal particle emitter */
const void *const vtable_00274770[1] = { (const void *)CRollerBallGoalFx_DeletingDtor };
/* 0x00274774  vtable of a goal */
const void *const vtable_00274774[1] = { (const void *)CRollerBallGoal_DeletingDtor };
/* 0x00274778  vtable of the goal system */
const void *const vtable_00274778[1] = { (const void *)CRollerBallGoals_DeletingDtor };

/* 0x00190b80  RollerBall (gap): flare constructor */
void *CRollerBallFlare_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274768;
	CRollerBallTimer_Ctor(RB_P(self, 0x10));
	return self;
}

/* 0x00190ba0  RollerBall (gap): flare destructor */
void CRollerBallFlare_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274768;
	CRollerBallTimer_Dtor(RB_P(self, 0x10));
}

/* 0x00190bb0  RollerBall (gap)
Update of a burning flare: it ages; its light shrinks (range 6.67e-5 down by 1.1e-5) and
dims with the timer's fraction; at the end the flare stops and its light goes off. */
void CRollerBallFlare_Update(void *self, float dt)
{
	unsigned char *s = self;
	void *light;
	float f, fade;
	int r, g, b;

	if (RB_U8(s, 0x4e) == 0)
		return;
	RB_F32(s, 0x60) = dt + RB_F32(s, 0x60);
	CRollerBallTimer_Update(s + 0x10);
	f = RB_F32(s + 0x10, 0x34); /* 0x000965f0 (folded): the timer's fraction */
	CRollerBallLight_SetRange(RB_PTR(s, 8),
		f * rb_bitsf(0xb73a69dcu) /* 0x0027476c */ + 6.66666674e-05f);
	fade = 1.0f - f;
	b = RB_FTOL((float)RB_I16(s, 0x4c) * fade);
	g = RB_FTOL((float)RB_I16(s, 0x4a) * fade);
	r = RB_FTOL((float)RB_I16(s, 0x48) * fade);
	CRollerBallLight_SetColour(RB_PTR(s, 8), r, g, b);
	if (f >= 1.0f) {
		RB_U8(s, 0x4e) = 0;
		light = RB_PTR(s, 8);
		RB_VFN(light, 0x0c, void (*)(void *, int))(light, 0);
	}
}

/* 0x00190c80  RollerBall (gap): goal emitter constructor */
void *CRollerBallGoalFx_Ctor(void *self)
{
	*(const void *const **)self = vtable_00274770;
	CParticleSystem_Construct((struct CParticleSystem *)RB_P(self, 0x1c));
	return self;
}

/* 0x00190ca0  RollerBall (gap): goal emitter destructor */
void CRollerBallGoalFx_Dtor(void *self)
{
	*(const void *const **)self = vtable_00274770;
	CParticleSystem_Destruct((struct CParticleSystem *)RB_P(self, 0x1c));
}

/* the variation's ambient light */
static void rb_RestoreAmbient(void)
{
	int v = RB_I32(RB_GAME, RB_GO_VARIATION);

	baSetAmbientLight(RB_VS(int, v, 0x002b4bd0u), RB_VS(int, v, 0x002b4bd4u),
		RB_VS(int, v, 0x002b4bd8u));
}

/* 0x00190cb0  RollerBall (gap)
Draw of a goal: its "active" instance glowing (ambient = glow / 128, texture factor 127)
and its "flare" instance in the flare's colour, each when its flag is set. */
void CRollerBallGoal_Draw(void *self)
{
	unsigned char *s = self;
	int i, a;

	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetRenderState(10, 1, 0);
	for (i = 0; i < 2; i++) {
		if (RB_U8(s, 0x40 + i * 0x18) == 0)
			continue;
		if (i == 0) {
			a = RB_I32(s, 0x74) >> 7;
			baSetAmbientLight(a, a, a);
			bdPushRenderState(3, 1, 0);
			CRollerBall_SetFactorAlphaStage(0x7f);
			baDrawActorInstance(RB_PTR(s, 0x44), 7);
			FUN_001bea60(); /* the Direct3D helper's stage reset (ECX = game +0x26f4) */
			bdPopRenderState(3);
			rb_RestoreAmbient();
		} else if (i == 1) {
			baSetAmbientLight(RB_I32(s, 0x64), RB_I32(s, 0x68), RB_I32(s, 0x6c));
			bdPushRenderState(3, 1, 0);
			baDrawActorInstance(RB_PTR(s, 0x5c), 7);
			bdPopRenderState(3);
			rb_RestoreAmbient();
		} else {
			baDrawActorInstance(RB_PTR(s, 0x44 + i * 0x18), 7);
		}
	}
	bdPopRenderState(5);
	bdPopRenderState(6);
}

/* 0x00190e10  RollerBall (gap): goal system constructor */
void *CRollerBallGoals_Ctor(void *self)
{
	unsigned char *s = self;
	int i;

	*(const void *const **)s = vtable_00274778;
	CRollerBallTimer_Ctor(s + 0x48);
	for (i = 0; i < 3; i++)
		CRollerBallFlare_Ctor(s + 0x80 + i * 0x68);
	for (i = 0; i < 8; i++)
		CRollerBallGoalFx_Ctor(s + 0x1b8 + i * 0xc4);
	return s;
}

/* 0x00190ea0  RollerBall (gap): goal system destructor */
void CRollerBallGoals_Dtor(void *self)
{
	unsigned char *s = self;
	int i;

	*(const void *const **)s = vtable_00274778;
	for (i = 7; i >= 0; i--)
		CRollerBallGoalFx_Dtor(s + 0x1b8 + i * 0xc4);
	for (i = 2; i >= 0; i--)
		CRollerBallFlare_Dtor(s + 0x80 + i * 0x68);
	CRollerBallTimer_Dtor(s + 0x48);
}

/* 0x00190f20  RollerBall (gap)
Shutdown (variation 0 only): the goals (their actor instances, then the goal), the two
resources, and the emitters' particle systems (virtual +0). */
void CRollerBallGoals_Shutdown(void *self)
{
	unsigned char *s = self;
	unsigned char *g, *next, *ps;
	int i;

	if (RB_I32(RB_GAME, RB_GO_VARIATION) != 0)
		return;
	g = RB_PTR(s, 0x10);
	while (g != NULL) {
		next = RB_PTR(g, 4);
		if (RB_I32(RB_GAME, RB_GO_VARIATION) == 0) {
			for (i = 0; i < 2; i++) {
				if (RB_PTR(g, 0x38 + i * 4) != NULL) {
					baDeleteActorInstance(RB_PTR(g, 0x38 + i * 4));
					RB_PTR(g, 0x38 + i * 4) = NULL;
				}
			}
		}
		RB_VFN(g, 0x00, void *(*)(void *, unsigned int))(g, 1);
		g = next;
	}
	for (i = 0; i < 2; i++) {
		if (RB_PTR(s, 8 + i * 4) != NULL) {
			bkDeleteResource(RB_PTR(s, 8 + i * 4));
			RB_PTR(s, 8 + i * 4) = NULL;
		}
	}
	for (i = 0; i < 8; i++) {
		ps = s + 0x1d4 + i * 0xc4;
		RB_VFN(ps, 0x00, void (*)(void *))(ps);
	}
}

/* 0x00190fe0  RollerBall (gap)
Draw (variation 0 only): the visible emitters' particle systems (virtual +8), then the
goals. */
void CRollerBallGoals_Draw(void *self)
{
	unsigned char *s = self;
	unsigned char *ps, *g;
	int i;

	if (RB_I32(RB_GAME, RB_GO_VARIATION) != 0)
		return;
	for (i = 0; i < 8; i++) {
		ps = s + 0x1d4 + i * 0xc4;
		if (RB_U8(ps, -7) != 0)
			RB_VFN(ps, 0x08, void (*)(void *))(ps);
	}
	for (g = RB_PTR(s, 0x10); g != NULL; g = RB_PTR(g, 4))
		CRollerBallGoal_Draw(g);
}

/* 0x00191040  RollerBall (gap): flare deleting destructor (flags bit 0: free) */
void *CRollerBallFlare_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallFlare_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00191060  RollerBall/CRollerball_Goal.cpp
Initialise of a flare: a point light in the flare's colour, added to the light system;
the timer's frequency; not burning. Returns 1. */
int CRollerBallFlare_Init(void *self)
{
	unsigned char *s = self;
	void *light;

	light = RB_NEW(0x34, RB_FILE_GOAL, 0x57);
	if (light != NULL)
		CRollerBallPointLight_Ctor(light);
	RB_PTR(s, 8) = light;
	RB_VFN(light, 0x04, int (*)(void *))(light);
	CRollerBallLight_SetColour(RB_PTR(s, 8), RB_I16(s, 0x48), RB_I16(s, 0x4a), RB_I16(s, 0x4c));
	CRollerBallLights_Add(RB_GAME + RB_GO_LIGHTS, RB_PTR(s, 8));
	CDemoCountdown_Init((int)(uintptr_t)(s + 0x10));
	RB_U8(s, 0x4e) = 0;
	RB_F32(s, 0x60) = 0.0f;
	return 1;
}

/* 0x00191110  RollerBall/CRollerball_Goal.cpp
Lights a flare at POS in half the colour R, G, B for one second. */
void CRollerBallFlare_Start(void *self, const float *pos, int r, int g, int b)
{
	unsigned char *s = self;
	void *light;

	RB_I16(s, 0x48) = (short)(r / 2);
	RB_I16(s, 0x4a) = (short)(g / 2);
	RB_U8(s, 0x4e) = 1;
	RB_F32(s, 0x60) = 0.0f;
	RB_I16(s, 0x4c) = (short)(b / 2);
	RB_F32(s, 0x50) = pos[0];
	RB_F32(s, 0x54) = pos[1];
	RB_F32(s, 0x58) = pos[2];
	RB_F32(s, 0x5c) = pos[3];
	CRollerBallLight_SetPosition(RB_PTR(s, 8), pos);
	CRollerBallLight_SetRange(RB_PTR(s, 8), rb_bitsf(0x38690453u));
	CRollerBallLight_SetColour(RB_PTR(s, 8), RB_I16(s, 0x48), RB_I16(s, 0x4a), RB_I16(s, 0x4c));
	light = RB_PTR(s, 8);
	RB_VFN(light, 0x0c, void (*)(void *, int))(light, 1);
	CRollerBallTimer_Set(s + 0x10, 1.0f);
	CRollerBallTimer_Start(s + 0x10);
}

/* 0x001911c0  RollerBall/CRollerball_Goal.cpp: goal emitter deleting destructor */
void *CRollerBallGoalFx_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallGoalFx_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x001911e0  RollerBall/CRollerball_Goal.cpp
Initialise of a goal emitter: 25 particles, two rings of 10 (radius 200), the textures
game +0x1eec / +0x1ee8; not active. Returns 1. */
int CRollerBallGoalFx_Init(void *self)
{
	struct RB_PSParams params;
	struct CParticleSystem *ps = (struct CParticleSystem *)RB_P(self, 0x1c);

	memset(&params, 0, sizeof params);
	params.active = 0;
	params.unknown51 = 0;
	params.angleSpeed = rb_bitsf(0x3e20d97cu);
	params.riseAccel = 0.3f;
	params.gravity = -0.01f;
	params.riseSpeed = 20.0f;
	params.speed = 5.0f;
	params.drag = rb_bitsf(0x3f7d70a4u);
	params.radiusSpeed = 2.0f;
	params.radius = 200.0f;
	params.numRings = 2;
	params.ringLength = 10;
	params.maxEmit = 1;
	params.lifeMin = 0x1e;
	params.lifeRange = 0x1e;
	params.pointSize = 100.0f;
	params.emitTime = 50.0f;
	CParticleSystem_Init(ps, (struct CPSParams *)&params, 0x19);
	CParticleSystem_SetTexture((struct CPSHeader *)ps, RB_I32(RB_GAME, 0x1eec));
	CParticleSystem_SetTexture2(ps, RB_I32(RB_GAME, 0x1ee8));
	RB_U8(self, 0x14) = 0;
	RB_U8(self, 0x15) = 0;
	RB_F32(self, 0x18) = 0.0f;
	return 1;
}

/* 0x001912c0  RollerBall/CRollerball_Goal.cpp: goal deleting destructor */
void *CRollerBallGoal_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_00274774;
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x001912e0  RollerBall/CRollerball_Goal.cpp
Initialise of a goal: two actor instances of the game's goal actors (+0x1440, +0x1444) at
the goal's position and direction, their "active" and "flare" sub-objects, the "active"
animation looping; radius 80, not scored. Returns 1. */
int CRollerBallGoal_Init(void *self)
{
	unsigned char *s = self;
	struct BActorInstance *inst;
	const char *const *name = (const char *const *)RB_ADDR(0x002b508cu);
	int i;

	for (i = 0; i < 2; i++) {
		inst = baInitActorInstance(NULL, RB_AT(RB_GAME, 0x1440 + i * 4, struct BActor *), 1,
			RB_FILE_GOAL, 0x1ba);
		RB_PTR(s, 0x38 + i * 4) = inst;
		baPrepActorInstance(inst, 1);
		inst = RB_PTR(s, 0x38 + i * 4);
		RB_F32(inst, 0xc0) = RB_F32(s, 0xc);
		RB_F32(inst, 0xc4) = RB_F32(s, 0x10);
		RB_F32(inst, 0xc8) = RB_F32(s, 0x14);
		RB_F32(inst, 0xcc) = RB_F32(s, 0x18);
		inst = RB_PTR(s, 0x38 + i * 4);
		RB_F32(inst, 0xe0) = RB_F32(s, 0x1c);
		RB_F32(inst, 0xe4) = RB_F32(s, 0x20);
		RB_F32(inst, 0xe8) = RB_F32(s, 0x24);
		RB_F32(inst, 0xec) = RB_F32(s, 0x28);
	}
	RB_PTR(s, 0x44) = RB_PTR(s, 0x38);
	RB_PTR(s, 0x5c) = RB_PTR(s, 0x3c);
	for (i = 0; i < 2; i++, name++) {
		RB_U8(s, 0x40 + i * 0x18) = 0;
		RB_PTR(s, 0x48 + i * 0x18) = baFindSubObject(RB_PTR(s, 0x44 + i * 0x18), *name);
	}
	inst = RB_PTR(s, 0x44);
	baFlushAnimations(inst, RB_PTR(inst, 0x134), 1, 0);
	baQueueAnimation(inst, RB_PTR(inst, 0x134), RB_PTR(s, 0x48), 0x400, -1, 1, NULL, NULL);
	RB_I16(s, 0x32) = 0;
	RB_F32(s, 0x2c) = 80.0f;
	RB_I16(s, 0x30) = 1;
	RB_I32(s, 0x74) = 0;
	RB_U8(s, 0x40) = 0;
	RB_U8(s, 0x58) = 0;
	return 1;
}

/* 0x00191420  RollerBall/CRollerball_Goal.cpp
Scoring: the first player inside the goal (within the radius in x and z, and its bottom
at most the radius above the goal) is kept (+8) and the goal is scored (state 4). */
void CRollerBallGoal_Check(void *self)
{
	unsigned char *s = self;
	unsigned char *p;
	float dx, dz, r;

	for (p = RB_FIRST_PLAYER(); p != NULL; p = RB_NEXT(p)) {
		r = RB_F32(s, 0x2c);
		dx = RB_F32(p, RB_PL_POS) - RB_F32(s, 0xc);
		if (!(fabsf(dx) <= r))
			continue;
		dz = RB_F32(p, RB_PL_POS + 8) - RB_F32(s, 0x14);
		if (!(fabsf(dz) <= r))
			continue;
		if (!((RB_F32(p, RB_PL_POS + 4) - RB_F32(p, 0x413c)) - RB_F32(s, 0x10) <= r))
			continue;
		if (!(dx * dx + dz * dz <= r * r))
			continue;
		RB_PTR(s, 8) = p;
		RB_I16(s, 0x32) = 4;
		return;
	}
}

/* 0x00191510  RollerBall/CRollerball_Goal.cpp: goal system deleting destructor */
void *CRollerBallGoals_DeletingDtor(void *self, unsigned int flags)
{
	CRollerBallGoals_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00191530  RollerBall/CRollerball_Goal.cpp
Adds goal ID at POS facing DIR (at the head of the list and in the table by id) and
initialises it. */
void CRollerBallGoals_AddGoal(void *self, int id, const float *pos, const float *dir)
{
	unsigned char *s = self;
	unsigned char *g;

	g = RB_NEW(0x78, RB_FILE_GOAL, 0x41a);
	if (g != NULL)
		*(const void *const **)g = vtable_00274774;
	RB_F32(g, 0xc) = pos[0];
	RB_F32(g, 0x10) = pos[1];
	RB_F32(g, 0x14) = pos[2];
	RB_F32(g, 0x18) = pos[3];
	RB_F32(g, 0x1c) = dir[0];
	RB_F32(g, 0x20) = dir[1];
	RB_F32(g, 0x24) = dir[2];
	RB_F32(g, 0x28) = dir[3];
	RB_I32(g, 0x34) = id;
	if (RB_PTR(s, 0x10) == NULL)
		RB_PTR(g, 4) = NULL;
	else
		RB_PTR(g, 4) = RB_PTR(s, 0x10);
	RB_PTR(s, 0x10) = g;
	RB_PTR(s, 0x14 + id * 4) = g;
	RB_I32(s, 0x38) = RB_I32(s, 0x38) + 1;
	CRollerBallGoal_Init(g);
}
