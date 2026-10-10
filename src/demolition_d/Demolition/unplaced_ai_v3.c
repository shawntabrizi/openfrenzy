/*
Demolition/unplaced_ai_v3.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\ (gap functions; the file is not known)

The brain of a computer player in game variation 3 (vtable 0x0026bbc4; made by
demolition_b's 0x000f8dc0). Each player has a home base (game +0x7c0 + index * 0x20): the
brain fetches the bombs that land near its base, carries one round its base towards the
target's base and throws it (the action button, timed by the jump timer) at the target or
its base; it runs to a corner of its base when a bomb is about to go off.

Class fields after the shared part (struct DemoAI):
  0x178 scan time, 0x17c timer       0x188 turn rate (24 degrees)
  0x18c, 0x190 (1 - aggression) * 150 and a random share of it (not read here)
  0x194 the largest aim error, 0x198 the aim error
  0x19c aim-error time, 0x1a0 timer  0x1a4..0x1b4 the timers of modes 0..4
  0x1b8 vec4: where to throw
States: 0 start, 1 fetch, 2 carry, 3 evade, 5 throw.
*/

#include "dd_ai.h"

/* 0x0026bbc4  the vtable (9 slots) */
const void *const vtable_0026bbc4[9] = {
	(const void *)FUN_00105a70, /* [0] Init */
	(const void *)FUN_0010bce0, /* [1] Process */
	(const void *)FUN_001d5950, /* [2] (empty) */
	(const void *)FUN_001d5950, /* [3] (empty) */
	(const void *)FUN_00109980, /* [4] Reset */
	(const void *)FUN_00109d10, /* [5] Restart */
	(const void *)FUN_001ec260, /* [6] (empty, 1 argument) */
	(const void *)FUN_001098d0, /* [7] OnHit */
	(const void *)FUN_00108ec0, /* [8] scalar deleting destructor */
};

/* the home base of a player (vec4) */
#define AI3_HOME(player) DD_FP(DD_GAME, (DD_I32((player), DP_INDEX) + 0x3e) * 0x20)
#define AI3_CARRYING(s) DD_U8(AI_OWNER(s), DP_CARRYING)

/* 0x00105ac0  Demolition (gap)
Constructor. */
DemoAI *DemoAI3_Ctor(DemoAI *self)
{
	self->vtable = (const void *const *)vtable_0026bbc4;
	return self;
}

/* 0x00105ad0  Demolition (gap)
State 0: carry what the player holds, fetch a seen bomb, else wait. */
void DemoAI3_StateStart(DemoAI *self, float dt)
{
	(void)dt;
	if (AI3_CARRYING(self) != 0) {
		self->state = 2;
		return;
	}
	if (self->numThreats != 0)
		self->state = 1;
}

/* 0x00108ec0  Demolition (gap)
Scalar deleting destructor. */
DemoAI *DemoAI3_Delete(DemoAI *self, unsigned int flags)
{
	self->vtable = (const void *const *)vtable_0026bbc4;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00108ee0  Demolition (gap)
Now and then a new aim error; on its scan timer collects the live bombs of the bomb list
within 400 of its base (x and z) that are not far above it. */
void DemoAI3_Scan(DemoAI *self, float dt)
{
	char *bomb;
	char *owner;
	float *home;
	float homeX;
	float homeZ;
	float t;
	float r;
	float dx;
	float dz;
	float dist;
	float worst;
	int replace;
	int n;
	int i;

	t = AI_F(self, 0x1a0) - dt;
	AI_F(self, 0x1a0) = t;
	if (t < 0.0f) {
		r = dd_randf();
		AI_F(self, 0x1a0) = (r + 0.5f) * AI_F(self, 0x19c);
		/* the original doubles the old error (0x198), not the largest one (0x194) */
		AI_F(self, 0x198) = (AI_F(self, 0x198) + AI_F(self, 0x198)) * ((float)dd_rand() * DD_RAND_SCALE) -
		                    AI_F(self, 0x194);
	}
	t = AI_F(self, 0x17c) - dt;
	AI_F(self, 0x17c) = t;
	if (!(t < 0.0f))
		return;
	r = dd_randf();
	AI_F(self, 0x17c) = (r + 0.5f) * AI_F(self, 0x178);
	bomb = DD_PTR(DD_PTR(DD_GAME, DG_BOMBLIST), 4);
	home = AI3_HOME(AI_OWNER(self));
	homeX = home[0];
	homeZ = home[2];
	self->numThreats = 0;
	/* the original's replacement index starts as garbage; it is only used after being set */
	replace = 0;
	for (; bomb != NULL; bomb = DD_PTR(bomb, 0x10)) {
		if (DD_U8(bomb, PB_ACTIVE) == 0)
			continue;
		if (!(homeX - 400.0f <= DD_F32(bomb, PB_POS)) || !(DD_F32(bomb, PB_POS) <= homeX + 400.0f))
			continue;
		if (!(homeZ - 400.0f <= DD_F32(bomb, PB_POS + 8)) || !(DD_F32(bomb, PB_POS + 8) <= homeZ + 400.0f))
			continue;
		owner = AI_OWNER(self);
		if (!(DD_F32(bomb, PB_POS + 4) - DD_F32(bomb, PB_HEIGHT) <= DD_F32(owner, DP_POS + 4) + 500.0f))
			continue;
		dx = DD_F32(bomb, PB_POS) - DD_F32(owner, DP_POS);
		dz = DD_F32(bomb, PB_POS + 8) - DD_F32(owner, DP_POS + 8);
		dist = sqrtf(dx * dx + dz * dz);
		n = self->numThreats;
		if (n < self->maxThreats) {
			replace = n;
			FUN_000127e0(self->threats[n].pos, DD_FP(bomb, PB_POS));
			self->threats[n].dist = dist;
			self->threats[n].fuse = DD_TimerValue(bomb + PB_FUSE_TIMER);
			self->numThreats = self->numThreats + 1;
		} else {
			worst = 0.0f;
			for (i = 0; i < self->maxThreats; i++) {
				if (worst < self->threats[i].dist) {
					worst = self->threats[i].dist;
					replace = i;
				}
			}
			if (dist < worst) {
				memcpy(self->threats[replace].pos, DD_FP(bomb, PB_POS), 16);
				self->threats[replace].dist = dist;
				self->threats[replace].fuse = DD_TimerValue(bomb + PB_FUSE_TIMER);
			}
		}
	}
}

/* the threat with the lowest negative fuse (the first one when none is below 0) */
static int ai3_most_urgent(DemoAI *self)
{
	float best;
	int idx;
	int i;

	idx = 0;
	best = 0.0f;
	for (i = 0; i < self->numThreats; i++) {
		if (self->threats[i].fuse < best) {
			best = self->threats[i].fuse;
			idx = i;
		}
	}
	return idx;
}

/* 0x001091e0  Demolition (gap)
State 1: walk to a seen bomb; evade when one is about to go off. */
void DemoAI3_StateFetch(DemoAI *self, float dt)
{
	int idx;

	if (DemoAI_ThreatUrgent(self) != 0) {
		self->state = 3;
		return;
	}
	if (AI3_CARRYING(self) != 0) {
		self->state = 2;
		return;
	}
	if (self->numThreats > 0) {
		idx = ai3_most_urgent(self);
		memcpy(self->goal, self->threats[idx].pos, 16);
		self->hasGoal = 1;
		self->newGoal = 1;
		DemoAI_Steer(self, dt, AI_F(self, 0x188));
		return;
	}
	self->state = 0;
}

/* 0x00109290  Demolition (gap)
State 2: carrying a bomb. Throw it near the arena centre when its fuse is short; else walk
round the base (400 out) towards the target's base and throw at the target when it is in
line, or at a point near its base when the walk arrived. */
void DemoAI3_StateCarry(DemoAI *self, float dt)
{
	char *bomb;
	char *owner;
	float *home;
	float *theirs;
	float hx;
	float hz;
	float tx;
	float tz;
	float a;
	float ang;
	float rad;
	float d;
	float px;
	float pz;
	float x;
	float z;
	float r1;
	float r2;
	int arrived;
	int s1;
	int s2;

	owner = AI_OWNER(self);
	if (DD_U8(owner, DP_CARRYING) == 0) {
		self->state = 0;
		return;
	}
	bomb = DD_PTR(owner, DP_BOMB);
	if (bomb != NULL && DD_TimerValue(bomb + PB_FUSE_TIMER) < self->dangerFuse) {
		s1 = dd_rand();
		s2 = dd_rand();
		r1 = (float)s1 * DD_RAND_SCALE * 600.0f;
		self->state = 5;
		r1 = r1 - 300.0f;
		r2 = (float)s2 * DD_RAND_SCALE * 600.0f - 300.0f;
		/* y and w: uninitialised in the original */
		AI_F(self, 0x1bc) = 0.0f;
		AI_F(self, 0x1c4) = 0.0f;
		AI_F(self, 0x1b8) = r1;
		AI_F(self, 0x1c0) = r2;
		return;
	}
	if (self->target == NULL) {
		self->state = 0;
		return;
	}
	home = AI3_HOME(AI_OWNER(self));
	hx = home[0];
	hz = home[2];
	a = atan2f(hx, hz) + 3.14159274f;
	theirs = AI3_HOME(self->target);
	tx = theirs[0];
	tz = theirs[2];
	d = FUN_000703d0(a, atan2f(tx, tz));
	if (0.52359879f <= fabsf(d)) {
		if (d < 0.0f)
			ang = a - 0.785398185f;
		else
			ang = a + 0.785398185f;
		rad = 400.0f - DD_F32(AI_OWNER(self), DP_RADIUS);
		/* y and w: uninitialised in the original */
		ai_set_goal(self, sinf(ang) * rad + hx, 0.0f, cosf(ang) * rad + hz, 0.0f);
		arrived = DemoAI_Steer(self, dt, AI_F(self, 0x188));
		px = sinf(a) * 600.0f + hx;
		pz = cosf(a) * 600.0f + hz;
		owner = AI_OWNER(self);
		d = FUN_000703d0(atan2f(DD_F32(owner, DP_POS) - px, DD_F32(owner, DP_POS + 8) - pz),
		                 atan2f(px - DD_F32(self->target, DP_POS), pz - DD_F32(self->target, DP_POS + 8)));
		if (0.785398185f <= fabsf(d)) {
			if (arrived != 0) {
				if (d < 0.0f)
					ang = a - 0.52359879f;
				else
					ang = a + 0.52359879f;
				x = sinf(ang) * 400.0f + tx;
				z = cosf(ang) * 400.0f + tz;
				self->state = 5;
				AI_F(self, 0x1bc) = 0.0f; /* y and w: uninitialised in the original */
				AI_F(self, 0x1c4) = 0.0f;
				AI_F(self, 0x1b8) = x;
				AI_F(self, 0x1c0) = z;
			}
			return;
		}
	}
	self->state = 5;
	memcpy(DD_FP(self, 0x1b8), DD_FP(self->target, DP_POS), 16);
}

/* 0x00109640  Demolition (gap)
State 3: a bomb is about to go off: run 800 out to the corner of the base away from it. */
void DemoAI3_StateEvade(DemoAI *self, float dt)
{
	float *home;
	float hx;
	float hz;
	float bz;
	float x;
	float z;
	int idx;
	int corner;

	if (DemoAI_ThreatUrgent(self) == 0) {
		self->state = 0;
		return;
	}
	if (AI3_CARRYING(self) != 0) {
		self->state = 2;
		return;
	}
	idx = ai3_most_urgent(self);
	home = AI3_HOME(AI_OWNER(self));
	hx = home[0];
	hz = home[2];
	bz = self->threats[idx].pos[2];
	if (self->threats[idx].pos[0] <= hx) {
		corner = 1;
		if (bz <= hz)
			corner = 3;
	} else if (bz <= hz) {
		corner = 4;
	} else {
		corner = 2;
	}
	x = 0.0f;
	z = 0.0f;
	switch (corner) {
	case 1:
		x = hx + 800.0f;
		z = hz - 800.0f;
		break;
	case 2:
		x = hx - 800.0f;
		z = hz - 800.0f;
		break;
	case 3:
		x = hx + 800.0f;
		z = hz + 800.0f;
		break;
	case 4:
		x = hx - 800.0f;
		z = hz + 800.0f;
		break;
	}
	/* w: uninitialised in the original */
	ai_set_goal(self, x, 0.0f, z, 0.0f);
	DemoAI_Steer(self, dt, AI_F(self, 0x188));
}

/* 0x001097d0  Demolition (gap)
State 5: face the throw point (plus the aim error) and press the action button until the
jump timer is below the distance share. */
void DemoAI3_StateThrow(DemoAI *self, float dt)
{
	char *owner;
	float dx;
	float dz;
	float v;

	(void)dt;
	owner = AI_OWNER(self);
	if (self->target == NULL || DD_U8(owner, DP_CARRYING) == 0) {
		self->state = 0;
		return;
	}
	dx = AI_F(self, 0x1b8) - DD_F32(owner, DP_POS);
	dz = AI_F(self, 0x1c0) - DD_F32(owner, DP_POS + 8);
	DD_F32(DD_PTR(owner, DP_ACTOR), 0x1d8) = atan2f(dx, dz) + AI_F(self, 0x198);
	v = sqrtf(dx * dx + dz * dz) * 0.000833333354f;
	if (v > 1.0f)
		v = 1.0f;
	else if (v < 0.0f)
		v = 0.0f;
	if (DD_U8(AI_OWNER(self), DP_CANJUMP) != 0) {
		if (v <= AI_TimeLeft(AI_OWNER(self) + DP_JUMPTIMER)) {
			AI_PressAction(self, 0);
			return;
		}
		AI_PressAction(self, 1);
		return;
	}
	AI_PressAction(self, 1);
}

/* 0x001098d0  Demolition (gap)
Slot 7: hit by `attacker` (not the team mate): maybe take revenge. */
void DemoAI3_OnHit(DemoAI *self, char *attacker)
{
	float r;

	if (attacker == DD_PTR(AI_OWNER(self), DP_PARTNER))
		return;
	if (dd_randf() < self->params[0]) {
		self->lastTarget = attacker;
		self->target = attacker;
		r = dd_randf();
		self->mode = 2;
		AI_F(self, 0x1ac) = (r * 0.5f + 0.75f) * 300.0f;
	}
}

/* 0x00109980  Demolition (gap)
Slot 4: reset. */
void DemoAI3_Reset(DemoAI *self)
{
	float r;

	ai_reset_base(self);
	AI_F(self, 0x188) = 0.418879032f;
	self->arriveDist = 20.0f;
	self->f160 = 0.785398185f;
	self->goal[0] = 0.0f;
	self->goal[2] = 0.0f;
	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	self->scanRadius = self->params[2] * 300.0f + 150.0f;
	self->dangerFuse = (1.0f - self->params[3]) * 2.5f;
	AI_F(self, 0x190) = (1.0f - self->params[0]) * 150.0f;
	AI_F(self, 0x18c) = ai_vary(AI_F(self, 0x190));
	AI_F(self, 0x178) = (1.0f - self->params[4]) * 60.0f;
	AI_F(self, 0x17c) = ai_vary(AI_F(self, 0x178));
	AI_F(self, 0x19c) = (1.0f - self->params[3]) * 150.0f;
	AI_F(self, 0x1a0) = ai_vary(AI_F(self, 0x19c));
	AI_F(self, 0x194) = (1.0f - self->params[3]) * 0.0174532924f;
	AI_F(self, 0x198) = (AI_F(self, 0x194) + AI_F(self, 0x194)) * ((float)dd_rand() * DD_RAND_SCALE) -
	                    AI_F(self, 0x194);
	if (dd_randf() < 0.5f) {
		self->target = DemoAI_PickRandom(self);
		r = dd_randf();
		self->mode = 0;
		AI_F(self, 0x1a4) = (r * 0.5f + 0.75f) * 150.0f;
		return;
	}
	self->target = DemoAI_PickLeader(self);
	r = dd_randf();
	self->mode = 4;
	AI_F(self, 0x1b4) = (r * 0.5f + 0.75f) * 200.0f;
}

/* 0x00109d10  Demolition (gap)
Slot 5: restart (the skill-dependent values and timers again). */
void DemoAI3_Restart(DemoAI *self)
{
	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	self->scanRadius = self->params[2] * 300.0f + 150.0f;
	self->dangerFuse = (1.0f - self->params[3]) * 2.5f;
	AI_F(self, 0x190) = (1.0f - self->params[0]) * 150.0f;
	AI_F(self, 0x18c) = ai_vary(AI_F(self, 0x190));
	AI_F(self, 0x178) = (1.0f - self->params[4]) * 60.0f;
	AI_F(self, 0x17c) = ai_vary(AI_F(self, 0x178));
	AI_F(self, 0x19c) = (1.0f - self->params[3]) * 150.0f;
	AI_F(self, 0x1a0) = ai_vary(AI_F(self, 0x19c));
	AI_F(self, 0x194) = (1.0f - self->params[3]) * 0.0174532924f;
	AI_F(self, 0x198) = (AI_F(self, 0x194) + AI_F(self, 0x194)) * ((float)dd_rand() * DD_RAND_SCALE) -
	                    AI_F(self, 0x194);
}

/* the target choices of the modes */
static void ai3_to_leader(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickLeader(self);
	r = dd_randf();
	self->mode = 4;
	AI_F(self, 0x1b4) = (r * 0.5f + 0.75f) * 200.0f;
}

static void ai3_to_random(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickRandom(self);
	r = dd_randf();
	self->mode = 0;
	AI_F(self, 0x1a4) = (r * 0.5f + 0.75f) * 150.0f;
}

static void ai3_to_nearest(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickNearest(self, DD_FP(AI_OWNER(self), DP_POS));
	r = dd_randf();
	self->mode = 1;
	AI_F(self, 0x1a8) = (r * 0.5f + 0.75f) * 150.0f;
}

/* a mode's timer ran out, or its target died */
static int ai3_mode_due(DemoAI *self, int timer, float dt)
{
	float t;

	t = AI_F(self, timer) - dt;
	AI_F(self, timer) = t;
	if (t <= 0.0f || (self->target != NULL && DD_U8(self->target, DP_ALIVE) == 0)) {
		AI_F(self, timer) = 0.0f;
		return 1;
	}
	return 0;
}

/* 0x0010b250  Demolition (gap)
Mode 0 (random): next the leader or the nearest enemy. */
void DemoAI3_ModeRandom(DemoAI *self, float dt)
{
	if (ai3_mode_due(self, 0x1a4, dt)) {
		if (dd_randf() < self->params[1]) {
			ai3_to_leader(self);
			return;
		}
		ai3_to_nearest(self);
	}
}

/* 0x0010b3a0  Demolition (gap)
Mode 1 (nearest): next the leader or a random enemy. */
void DemoAI3_ModeNearest(DemoAI *self, float dt)
{
	if (ai3_mode_due(self, 0x1a8, dt)) {
		if (dd_randf() < self->params[1]) {
			ai3_to_leader(self);
			return;
		}
		ai3_to_random(self);
	}
}

/* 0x0010b4e0  Demolition (gap)
Mode 2 (revenge): next the nemesis or the leader. */
void DemoAI3_ModeRevenge(DemoAI *self, float dt)
{
	float r;

	if (ai3_mode_due(self, 0x1ac, dt)) {
		if (dd_randf() < self->params[0]) {
			self->target = DemoAI_PickNemesis(self);
			r = dd_randf();
			self->mode = 3;
			AI_F(self, 0x1b0) = (r * 0.5f + 0.75f) * 300.0f;
			return;
		}
		ai3_to_leader(self);
	}
}

/* 0x0010b620  Demolition (gap)
Mode 3 (nemesis): next the leader or the nearest enemy. */
void DemoAI3_ModeNemesis(DemoAI *self, float dt)
{
	if (ai3_mode_due(self, 0x1b0, dt)) {
		if (dd_randf() < self->params[1]) {
			ai3_to_leader(self);
			return;
		}
		ai3_to_nearest(self);
	}
}

/* 0x0010b770  Demolition (gap)
Mode 4 (leader): next the leader again or a random enemy. */
void DemoAI3_ModeLeader(DemoAI *self, float dt)
{
	if (ai3_mode_due(self, 0x1b4, dt)) {
		if (dd_randf() < self->params[1]) {
			ai3_to_leader(self);
			return;
		}
		ai3_to_random(self);
	}
}

/* 0x0010bb00  Demolition (gap)
The state machine, and in state 5 the target mode. */
void DemoAI3_Think(DemoAI *self, float dt)
{
	switch (self->state) {
	case 0:
		DemoAI3_StateStart(self, dt);
		break;
	case 1:
		DemoAI3_StateFetch(self, dt);
		break;
	case 2:
		DemoAI3_StateCarry(self, dt);
		break;
	case 3:
		DemoAI3_StateEvade(self, dt);
		break;
	case 5:
		DemoAI3_StateThrow(self, dt);
		break;
	}
	if (self->state == 5) {
		switch (self->mode) {
		case 0:
			DemoAI3_ModeRandom(self, dt);
			return;
		case 1:
			DemoAI3_ModeNearest(self, dt);
			return;
		case 2:
			DemoAI3_ModeRevenge(self, dt);
			return;
		case 3:
			DemoAI3_ModeNemesis(self, dt);
			return;
		case 4:
			DemoAI3_ModeLeader(self, dt);
			return;
		}
	}
}

/* 0x0010bce0  Demolition (gap)
Slot 1: per frame (not in the CharLib AI states 13..17): scan, think. */
void DemoAI3_Process(DemoAI *self, float dt)
{
	unsigned short aiState;

	aiState = DD_U16(AI_OWNER(self), DP_AISTATE);
	if (aiState < 0xd || aiState > 0x11) {
		DemoAI3_Scan(self, dt);
		DemoAI3_Think(self, dt);
	}
}
