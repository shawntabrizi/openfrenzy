/*
Demolition/unplaced_ai_v1.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\ (gap functions; the file is not known)

The brain of a computer player in game variation 1 (vtable 0x0026bb7c, 0x1c8 bytes; made by
demolition_b's 0x000f8dc0). Each player steers a remote bomb (player +0x11a0, itself driven
like a CharLib player): the brain wanders, keeps away from the other armed remote bombs,
steers its own bomb at the target (or at the nearest threat) and sets it off (the action
button) when it is far enough from itself.

Class fields after the shared part (struct DemoAI):
  0x188 its remote bomb             0x18c turn rate (6 degrees)
  0x190 detonate timer, 0x194 its time      0x198 state 3 timer, 0x19c its time
  0x1a0 wander timer, 0x1a4 its time (200)  0x1a8 re-goal timer, 0x1ac its time
  0x1b0..0x1c0 the timers of modes 0..4     0x1c4 aim at the nearest threat
States: 0 start, 1 roam, 2 evade, 3 hold.
*/

#include "dd_ai.h"

/* 0x0026bb7c  the vtable (9 slots) */
const void *const vtable_0026bb7c[9] = {
	(const void *)FUN_00105a70, /* [0] Init */
	(const void *)FUN_0010bc20, /* [1] Process */
	(const void *)FUN_001d5950, /* [2] (empty) */
	(const void *)FUN_001d5950, /* [3] (empty) */
	(const void *)FUN_00107be0, /* [4] Reset */
	(const void *)FUN_00107f70, /* [5] Restart */
	(const void *)FUN_001ec260, /* [6] (empty, 1 argument) */
	(const void *)FUN_00107810, /* [7] OnHit */
	(const void *)FUN_00107370, /* [8] scalar deleting destructor */
};

#define AI1_BOMB(s) AI_P((s), 0x188)

/* 0x00105a60  Demolition (gap)
Constructor. */
DemoAI *DemoAI1_Ctor(DemoAI *self)
{
	self->vtable = (const void *const *)vtable_0026bb7c;
	return self;
}

/* 0x00107370  Demolition (gap)
Scalar deleting destructor. */
DemoAI *DemoAI1_Delete(DemoAI *self, unsigned int flags)
{
	self->vtable = (const void *const *)vtable_0026bb7c;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00107390  Demolition (gap)
Collects the other players' armed remote bombs within the scan radius. */
void DemoAI1_Scan(DemoAI *self, float dt)
{
	char *player;
	char *bomb;
	float radiusSq;
	float distSq;
	float worst;
	int replace;
	int n;
	int i;

	(void)dt;
	AI1_BOMB(self) = DD_PTR(AI_OWNER(self), DP_BOMB);
	if (self->state == 2)
		self->scanRadius = self->f148;
	else
		self->scanRadius = self->f14c;
	self->numThreats = 0;
	radiusSq = self->scanRadius * self->scanRadius;
	/* the original's replacement index starts as garbage; it is only used after being set */
	replace = 0;
	for (player = AI_FIRST_PLAYER(); player != NULL; player = AI_NEXT_PLAYER(player)) {
		bomb = DD_PTR(player, DP_BOMB);
		if (DD_U8(bomb, DP_ACTIVE) == 0 || bomb == AI1_BOMB(self))
			continue;
		distSq = DD_FN(FUN_00051540, float, (float *, float *))(DD_FP(AI_OWNER(self), DP_POS),
		                                                        DD_FP(bomb, DP_POS));
		if (!(distSq <= radiusSq))
			continue;
		n = self->numThreats;
		if (n < self->maxThreats) {
			memcpy(self->threats[n].pos, DD_FP(bomb, DP_POS), 16);
			self->threats[n].dist = distSq;
			self->numThreats = self->numThreats + 1;
			replace = n;
		} else {
			worst = 0.0f;
			for (i = 0; i < self->maxThreats; i++) {
				if (worst < self->threats[i].dist) {
					worst = self->threats[i].dist;
					replace = i;
				}
			}
			if (distSq < worst) {
				memcpy(self->threats[replace].pos, DD_FP(bomb, DP_POS), 16);
				self->threats[replace].dist = distSq;
			}
		}
	}
}

/* 0x00107500  Demolition (gap)
State 0: roam (aggressive) or evade. */
void DemoAI1_StateStart(DemoAI *self, float dt)
{
	(void)dt;
	if (dd_randf() < self->params[0]) {
		self->state = 1;
		return;
	}
	self->state = 2;
}

/* 0x00107550  Demolition (gap)
Steers its remote bomb at the target, or at the nearest threat when 0x1c4 is set. */
void DemoAI1_SteerBomb(DemoAI *self, float dt)
{
	char *target;
	float dx;
	float dz;
	float dir[4];
	float stick[4];

	(void)dt;
	if (self->numThreats == 0 || AI_I(self, 0x1c4) == 0) {
		target = self->target;
		if (target == NULL)
			return;
		dx = DD_F32(target, DP_POS) - DD_F32(AI1_BOMB(self), DP_POS);
		dz = DD_F32(target, DP_POS + 8) - DD_F32(AI1_BOMB(self), DP_POS + 8);
	} else {
		dx = self->threats[0].pos[0] - DD_F32(AI1_BOMB(self), DP_POS);
		dz = self->threats[0].pos[2] - DD_F32(AI1_BOMB(self), DP_POS + 8);
	}
	dir[0] = 0.0f;
	dir[1] = 0.0f;
	dir[2] = 1.0f;
	dir[3] = 0.0f;
	FUN_00051170(stick, dir, atan2f(dx, dz));
	AI_SetStick(AI1_BOMB(self), stick, 1.0f);
}

/* 0x00107600  Demolition (gap)
Sets the target mode: 0 random, 1 nearest, 2 the last attacker, 3 nemesis, 4 leader. */
void DemoAI1_SetMode(DemoAI *self, int mode)
{
	float r;

	switch (mode) {
	case 0:
		self->target = DemoAI_PickRandom(self);
		r = dd_randf();
		self->mode = mode;
		AI_F(self, 0x1b0) = (r * 0.5f + 0.75f) * 150.0f;
		return;
	case 1:
		self->target = DemoAI_PickNearest(self, DD_FP(AI_OWNER(self), DP_POS));
		r = dd_randf();
		self->mode = mode;
		AI_F(self, 0x1b4) = (r * 0.5f + 0.75f) * 150.0f;
		return;
	case 2:
		self->target = self->lastTarget;
		r = dd_randf();
		self->mode = mode;
		AI_F(self, 0x1b8) = (r * 0.5f + 0.75f) * 300.0f;
		return;
	case 3:
		self->target = DemoAI_PickNemesis(self);
		r = dd_randf();
		self->mode = mode;
		AI_F(self, 0x1bc) = (r * 0.5f + 0.75f) * 300.0f;
		return;
	case 4:
		self->target = DemoAI_PickLeader(self);
		r = dd_randf();
		AI_F(self, 0x1c0) = (r * 0.5f + 0.75f) * 200.0f;
		break;
	}
	self->mode = mode;
}

/* 0x00107810  Demolition (gap)
Slot 7: hit by `attacker` (not the team mate): maybe take revenge. */
void DemoAI1_OnHit(DemoAI *self, char *attacker)
{
	float r;

	if (attacker == DD_PTR(AI_OWNER(self), DP_PARTNER))
		return;
	if (dd_randf() < self->params[0]) {
		self->lastTarget = attacker;
		self->target = attacker;
		r = dd_randf();
		self->mode = 2;
		AI_F(self, 0x1b8) = (r * 0.5f + 0.75f) * 300.0f;
	}
}

/* 0x001078c0  Demolition (gap)
Evades the seen bombs: in a clear spot runs 500 units away from their mean position, else
circles the arena centre away from the nearest one; then steers there. */
void DemoAI1_Evade(DemoAI *self, float dt)
{
	char *arena;
	char *owner;
	float sx;
	float sz;
	float inv;
	float angle;
	float dist;
	float dx;
	float dz;
	float x;
	float z;
	int i;

	if (self->numThreats <= 0)
		return;
	arena = AI_Arena();
	owner = AI_OWNER(self);
	if (AI_RegionClear(arena, DD_FP(owner, DP_POS), 300.0f, 0) == 0) {
		sx = 0.0f;
		sz = 0.0f;
		for (i = 0; i < self->numThreats; i++) {
			sx = (DD_F32(AI_OWNER(self), DP_POS) - self->threats[i].pos[0]) + sx;
			sz = (DD_F32(AI_OWNER(self), DP_POS + 8) - self->threats[i].pos[2]) + sz;
		}
		inv = 1.0f / (float)self->numThreats;
		angle = atan2f(inv * sx, inv * sz);
		x = sinf(angle) * 500.0f + DD_F32(AI_OWNER(self), DP_POS);
		z = cosf(angle) * 500.0f + DD_F32(AI_OWNER(self), DP_POS + 8);
	} else {
		angle = atan2f(DD_F32(owner, DP_POS) - DD_F32(arena, 0x24),
		               DD_F32(owner, DP_POS + 8) - DD_F32(arena, 0x2c));
		dist = FUN_000703d0(angle, atan2f(self->threats[0].pos[0] - DD_F32(arena, 0x24),
		                                  self->threats[0].pos[2] - DD_F32(arena, 0x2c)));
		dx = DD_F32(arena, 0x24) - DD_F32(AI_OWNER(self), DP_POS);
		dz = DD_F32(arena, 0x2c) - DD_F32(AI_OWNER(self), DP_POS + 8);
		if (dist <= 0.0f)
			angle = angle - 0.52359879f;
		else
			angle = angle + 0.52359879f;
		dist = sqrtf(dx * dx + dz * dz);
		x = sinf(angle) * dist + DD_F32(arena, 0x24);
		z = cosf(angle) * dist + DD_F32(arena, 0x2c);
	}
	/* w: uninitialised in the original */
	ai_set_goal(self, x, 0.0f, z, 0.0f);
	DemoAI_Steer(self, dt, 0.209439516f);
}

/* 0x00107aa0  Demolition (gap)
Wanders: a new random goal now and then, steering at the class's turn rate; when the wander
time is up, usually wander again, else hold (state 3). */
void DemoAI1_Wander(DemoAI *self, float dt)
{
	float t;
	float r;

	t = AI_F(self, 0x1a0) - dt;
	AI_F(self, 0x1a0) = t;
	if (t >= 0.0f) {
		t = AI_F(self, 0x1a8) - dt;
		AI_F(self, 0x1a8) = t;
		if (t < 0.0f) {
			r = dd_randf();
			AI_F(self, 0x1a8) = (r + 0.5f) * AI_F(self, 0x1ac);
			DemoAI_GoRandom(self, 400.0f);
		}
		DemoAI_Steer(self, dt, AI_F(self, 0x18c));
		return;
	}
	r = dd_randf();
	AI_F(self, 0x1a0) = (r + 0.5f) * AI_F(self, 0x1a4);
	if (!(dd_randf() < 0.899999976f))
		self->state = 3;
}

/* 0x00107be0  Demolition (gap)
Slot 4: reset. */
void DemoAI1_Reset(DemoAI *self)
{
	float r;

	ai_reset_base(self);
	self->lastTarget = NULL;
	AI_F(self, 0x18c) = 0.104719758f;
	self->arriveDist = 10.0f;
	self->f160 = 0.785398185f;
	self->goal[0] = 0.0f;
	self->goal[2] = 0.0f;
	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	self->f148 = self->params[2] * 1000.0f + 400.0f;
	self->f14c = self->params[2] * 400.0f + 100.0f;
	AI_F(self, 0x194) = (1.0f - self->params[0]) * 150.0f;
	AI_F(self, 0x190) = ai_vary(AI_F(self, 0x194));
	AI_F(self, 0x19c) = (1.0f - self->params[2]) * 50.0f;
	r = dd_randf();
	AI_F(self, 0x1a4) = 200.0f;
	AI_F(self, 0x198) = (r + 0.5f) * AI_F(self, 0x19c);
	AI_F(self, 0x1a0) = ai_vary(AI_F(self, 0x1a4));
	AI_F(self, 0x1ac) = (1.0f - self->params[2]) * 80.0f;
	r = dd_randf();
	AI_I(self, 0x1c4) = 0;
	AI_F(self, 0x1a8) = (r + 0.5f) * AI_F(self, 0x1ac);
	if (dd_randf() < 0.5f) {
		self->target = DemoAI_PickRandom(self);
		r = dd_randf();
		self->mode = 0;
		AI_F(self, 0x1b0) = (r * 0.5f + 0.75f) * 150.0f;
		return;
	}
	self->target = DemoAI_PickLeader(self);
	r = dd_randf();
	self->mode = 4;
	AI_F(self, 0x1c0) = (r * 0.5f + 0.75f) * 200.0f;
}

/* 0x00107f70  Demolition (gap)
Slot 5: restart (the skill-dependent values and timers again). */
void DemoAI1_Restart(DemoAI *self)
{
	float r;

	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	self->f148 = self->params[2] * 1000.0f + 400.0f;
	self->f14c = self->params[2] * 400.0f + 100.0f;
	AI_F(self, 0x194) = (1.0f - self->params[0]) * 150.0f;
	AI_F(self, 0x190) = ai_vary(AI_F(self, 0x194));
	AI_F(self, 0x19c) = (1.0f - self->params[2]) * 50.0f;
	r = dd_randf();
	AI_F(self, 0x1a4) = 200.0f;
	AI_F(self, 0x198) = (r + 0.5f) * AI_F(self, 0x19c);
	AI_F(self, 0x1a0) = ai_vary(AI_F(self, 0x1a4));
	AI_F(self, 0x1ac) = (1.0f - self->params[2]) * 80.0f;
	AI_F(self, 0x1a8) = ai_vary(AI_F(self, 0x1ac));
}

/* 0x0010a560  Demolition (gap)
State 1: roam; with the bomb armed, set it off when the target is nearer to the player than
the far bomb is (and the bomb is over 800 away), and steer it; else detonate on the timer
and hold. A seen threat switches to evading. */
void DemoAI1_StateRoam(DemoAI *self, float dt)
{
	float d;
	float t;
	float r;

	if (self->numThreats > 0) {
		self->state = 2;
		return;
	}
	DemoAI1_Wander(self, dt);
	if (DD_U8(AI1_BOMB(self), DP_ACTIVE) != 0) {
		d = DemoAI_TargetDistXZ(self, DD_FP(AI1_BOMB(self), DP_POS));
		if (d > 800.0f && DemoAI_TargetDistXZ(self, DD_FP(AI_OWNER(self), DP_POS)) < d) {
			AI_PressAction(self, 1);
			r = dd_randf();
			AI_F(self, 0x190) = (r + 0.5f) * AI_F(self, 0x194);
		}
		DemoAI1_SteerBomb(self, dt);
		return;
	}
	t = AI_F(self, 0x190) - dt;
	AI_F(self, 0x190) = t;
	if (t <= 0.0f) {
		AI_PressAction(self, 1);
		r = dd_randf();
		self->state = 3;
		AI_F(self, 0x198) = (r + 0.5f) * AI_F(self, 0x19c);
		r = dd_randf();
		AI_F(self, 0x190) = (r + 0.5f) * AI_F(self, 0x194);
	}
}

/* 0x0010a6f0  Demolition (gap)
State 2: evade the seen bombs; skilled brains sometimes aim the bomb at the nearest threat
and set it off once it is 1000 away. */
void DemoAI1_StateEvade(DemoAI *self, float dt)
{
	float t;
	float r;

	if (self->numThreats < 1) {
		self->state = 1;
		AI_I(self, 0x1c4) = 0;
		return;
	}
	t = AI_F(self, 0x190) - dt;
	AI_F(self, 0x190) = t;
	if (t < 0.0f)
		AI_F(self, 0x190) = 0.0f;
	if (self->params[2] >= 0.5f && AI_I(self, 0x1c4) == 0) {
		if (0.899999976f < dd_randf())
			AI_I(self, 0x1c4) = 1;
	}
	if (DD_U8(AI1_BOMB(self), DP_ACTIVE) != 0) {
		if (AI_I(self, 0x1c4) != 0 && AI_F(self, 0x190) <= 0.0f) {
			if (FUN_00051500(DD_FP(AI1_BOMB(self), DP_POS), DD_FP(AI_OWNER(self), DP_POS)) >= 1000.0f) {
				AI_PressAction(self, 1);
				r = AI_CoreRandF();
				AI_F(self, 0x190) = (r + 0.5f) * AI_F(self, 0x194);
			}
		}
		DemoAI1_SteerBomb(self, dt);
		DemoAI1_Evade(self, dt);
		return;
	}
	if (AI_I(self, 0x1c4) != 0 && AI_F(self, 0x190) <= 0.0f) {
		AI_PressAction(self, 1);
		r = dd_randf();
		AI_F(self, 0x190) = (r + 0.5f) * AI_F(self, 0x194);
	}
	DemoAI1_Evade(self, dt);
}

/* 0x0010a8b0  Demolition (gap)
State 3: hold still for a while, steering the bomb if it is armed. */
void DemoAI1_StateHold(DemoAI *self, float dt)
{
	float t;
	float r;

	t = AI_F(self, 0x198) - dt;
	AI_F(self, 0x198) = t;
	if (t >= 0.0f) {
		if (DD_U8(AI1_BOMB(self), DP_ACTIVE) != 0)
			DemoAI1_SteerBomb(self, dt);
		return;
	}
	r = dd_randf();
	self->state = 0;
	AI_F(self, 0x198) = (r + 0.5f) * AI_F(self, 0x19c);
}

/* the target choices of the modes */
static void ai1_to_leader(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickLeader(self);
	r = dd_randf();
	self->mode = 4;
	AI_F(self, 0x1c0) = (r * 0.5f + 0.75f) * 200.0f;
}

static void ai1_to_nearest(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickNearest(self, DD_FP(AI_OWNER(self), DP_POS));
	r = dd_randf();
	self->mode = 1;
	AI_F(self, 0x1b4) = (r * 0.5f + 0.75f) * 150.0f;
}

/* a mode's timer ran out and the target is over `range` from the bomb (x, z) */
static int ai1_mode_due(DemoAI *self, int timer, float dt, float range)
{
	char *target;
	float dx;
	float dz;
	float t;

	t = AI_F(self, timer) - dt;
	AI_F(self, timer) = t;
	if (!(t <= 0.0f))
		return 0;
	target = self->target;
	AI_F(self, timer) = 0.0f;
	if (target == NULL)
		return 0;
	dx = DD_F32(target, DP_POS) - DD_F32(AI1_BOMB(self), DP_POS);
	dz = DD_F32(target, DP_POS + 8) - DD_F32(AI1_BOMB(self), DP_POS + 8);
	return range < sqrtf(dx * dx + dz * dz);
}

/* 0x0010a930  Demolition (gap)
Mode 0 (random): next the leader or the nearest enemy. */
void DemoAI1_ModeRandom(DemoAI *self, float dt)
{
	if (ai1_mode_due(self, 0x1b0, dt, 800.0f)) {
		if (dd_randf() < self->params[1]) {
			ai1_to_leader(self);
			return;
		}
		ai1_to_nearest(self);
	}
}

/* 0x0010aab0  Demolition (gap)
Mode 1 (nearest): next the leader or a random enemy. */
void DemoAI1_ModeNearest(DemoAI *self, float dt)
{
	if (ai1_mode_due(self, 0x1b4, dt, 500.0f)) {
		if (dd_randf() < self->params[1]) {
			ai1_to_leader(self);
			return;
		}
		DemoAI1_SetMode(self, 0);
	}
}

/* 0x0010abd0  Demolition (gap)
Mode 2 (revenge): next the nemesis or the leader. */
void DemoAI1_ModeRevenge(DemoAI *self, float dt)
{
	float r;

	if (ai1_mode_due(self, 0x1b8, dt, 500.0f)) {
		if (dd_randf() < self->params[0]) {
			self->target = DemoAI_PickNemesis(self);
			r = dd_randf();
			self->mode = 3;
			AI_F(self, 0x1bc) = (r * 0.5f + 0.75f) * 300.0f;
			return;
		}
		ai1_to_leader(self);
	}
}

/* 0x0010ad40  Demolition (gap)
Mode 3 (nemesis): next the leader or the nearest enemy. */
void DemoAI1_ModeNemesis(DemoAI *self, float dt)
{
	if (ai1_mode_due(self, 0x1bc, dt, 600.0f)) {
		if (dd_randf() < self->params[1]) {
			ai1_to_leader(self);
			return;
		}
		ai1_to_nearest(self);
	}
}

/* 0x0010aec0  Demolition (gap)
Mode 4 (leader): next the leader again or a random enemy. */
void DemoAI1_ModeLeader(DemoAI *self, float dt)
{
	if (ai1_mode_due(self, 0x1c0, dt, 500.0f)) {
		if (dd_randf() < self->params[1]) {
			ai1_to_leader(self);
			return;
		}
		DemoAI1_SetMode(self, 0);
	}
}

/* 0x0010ba40  Demolition (gap)
The state machine, then the target mode. */
void DemoAI1_Think(DemoAI *self, float dt)
{
	switch (self->state) {
	case 0:
		DemoAI1_StateStart(self, dt);
		break;
	case 1:
		DemoAI1_StateRoam(self, dt);
		break;
	case 2:
		DemoAI1_StateEvade(self, dt);
		break;
	case 3:
		DemoAI1_StateHold(self, dt);
		break;
	}
	switch (self->mode) {
	case 0:
		DemoAI1_ModeRandom(self, dt);
		return;
	case 1:
		DemoAI1_ModeNearest(self, dt);
		return;
	case 2:
		DemoAI1_ModeRevenge(self, dt);
		return;
	case 3:
		DemoAI1_ModeNemesis(self, dt);
		return;
	case 4:
		DemoAI1_ModeLeader(self, dt);
		return;
	}
}

/* 0x0010bc20  Demolition (gap)
Slot 1: per frame (not in the CharLib AI states 13..17): scan, think. */
void DemoAI1_Process(DemoAI *self, float dt)
{
	unsigned short aiState;

	aiState = DD_U16(AI_OWNER(self), DP_AISTATE);
	if (aiState < 0xd || aiState > 0x11) {
		DemoAI1_Scan(self, dt);
		DemoAI1_Think(self, dt);
	}
}
