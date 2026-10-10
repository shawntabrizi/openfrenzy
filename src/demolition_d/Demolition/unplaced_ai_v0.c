/*
Demolition/unplaced_ai_v0.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\ (gap functions; the file is not known)

The brain of a computer player in game variation 0 (vtable 0x0026bb58, 0x1d8 bytes; made by
demolition_b's 0x000f8dc0). It watches the bombs on the floor (both bombs of every player),
flees from them, chases its target player and throws (the action button) when the target is
in range, and picks a new target now and then (modes: random, nearest, revenge, nemesis,
leader).

Class fields after the shared part (struct DemoAI):
  0x16c wait timer (state 3)       0x188 too close    0x18c retreat end    0x190 too far
  0x194 retreating                 0x198 short: bombs ready (0..2)
  0x19c jump timer                 0x1a0 aim timer    0x1a4 wander timer   0x1a8 wander time
  0x1ac flee time                  0x1b0 flee timer   0x1b4 vec3: mean threat position
  0x1c4..0x1d4 the timers of modes 0..4
States: 0 start, 1 hunt, 2 flee, 3 wait, 4 attack, 5 wander.
*/

#include "dd_ai.h"

/* 0x0026bb58  the vtable (9 slots) */
const void *const vtable_0026bb58[9] = {
	(const void *)FUN_00105a70, /* [0] Init */
	(const void *)FUN_0010bbd0, /* [1] Process */
	(const void *)FUN_001d5950, /* [2] (empty) */
	(const void *)FUN_001d5950, /* [3] (empty) */
	(const void *)FUN_00106e80, /* [4] Reset */
	(const void *)FUN_001071f0, /* [5] Restart */
	(const void *)FUN_001ec260, /* [6] (empty, 1 argument) */
	(const void *)FUN_001068c0, /* [7] OnHit */
	(const void *)FUN_00106350, /* [8] scalar deleting destructor */
};

/* 0x001059e0  Demolition (gap)
State 3: wait until the timer runs out. */
void DemoAI0_StateWait(DemoAI *self, float dt)
{
	float t;

	t = AI_F(self, 0x16c) - dt;
	AI_F(self, 0x16c) = t;
	if (t <= 0.0f)
		self->state = 0;
}

/* 0x00106340  Demolition (gap)
Constructor. */
DemoAI *DemoAI0_Ctor(DemoAI *self)
{
	self->vtable = (const void *const *)vtable_0026bb58;
	return self;
}

/* 0x00106350  Demolition (gap)
Scalar deleting destructor. */
DemoAI *DemoAI0_Delete(DemoAI *self, unsigned int flags)
{
	self->vtable = (const void *const *)vtable_0026bb58;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* remember a bomb: append it, or replace the farthest one when the list is full */
static void ai0_add_threat(DemoAI *self, char *bomb, float distSq, int *replace)
{
	int i;
	int n;
	float worst;

	n = self->numThreats;
	if (n < self->maxThreats) {
		*replace = n;
		memcpy(self->threats[n].pos, DD_FP(bomb, PB_POS), 16);
		self->threats[n].dist = distSq;
		self->threats[n].fuse = DD_TimerValue(bomb + PB_TIMER);
		self->numThreats = self->numThreats + 1;
		return;
	}
	worst = 0.0f;
	for (i = 0; i < self->maxThreats; i++) {
		if (worst < self->threats[i].dist) {
			*replace = i;
			worst = self->threats[i].dist;
		}
	}
	if (distSq < worst) {
		memcpy(self->threats[*replace].pos, DD_FP(bomb, PB_POS), 16);
		self->threats[*replace].dist = distSq;
		self->threats[*replace].fuse = DD_TimerValue(bomb + PB_TIMER);
	}
}

/* 0x00106370  Demolition (gap)
Counts its own ready bombs and collects the live bombs of every player within the scan
radius (not far below the player). */
void DemoAI0_Scan(DemoAI *self, float dt)
{
	char *bomb;
	char *player;
	char *owner;
	float radiusSq;
	float dx;
	float dz;
	float distSq;
	int replace;
	int i;

	(void)dt;
	owner = AI_OWNER(self);
	DD_I16(self, 0x198) = 0;
	bomb = DD_PTR(owner, DP_BOMB);
	if (DD_U8(bomb, PB_ACTIVE) == 0 || DD_I32(bomb, PB_KIND) == 1)
		DD_I16(self, 0x198) = 1;
	bomb = DD_PTR(owner, DP_BOMB2);
	if (DD_U8(bomb, PB_ACTIVE) == 0 || DD_I32(bomb, PB_KIND) == 1)
		DD_I16(self, 0x198) = DD_I16(self, 0x198) + 1;
	if (self->state == 2)
		self->scanRadius = self->f148;
	else
		self->scanRadius = self->f14c;
	self->numThreats = 0;
	radiusSq = self->scanRadius * self->scanRadius;
	/* the original's replacement index starts as garbage; it is only used after being set */
	replace = 0;
	for (player = AI_FIRST_PLAYER(); player != NULL; player = AI_NEXT_PLAYER(player)) {
		for (i = 0; i < 2; i++) {
			bomb = DD_PTR(player, DP_BOMB + i * 4);
			if (DD_U8(bomb, PB_ACTIVE) == 0 || DD_I32(bomb, PB_KIND) == 1)
				continue;
			owner = AI_OWNER(self);
			if (!(DD_F32(owner, DP_POS + 4) <= DD_F32(bomb, PB_POS + 4) + 300.0f))
				continue;
			dx = DD_F32(owner, DP_POS) - DD_F32(bomb, PB_POS);
			dz = DD_F32(owner, DP_POS + 8) - DD_F32(bomb, PB_POS + 8);
			distSq = dx * dx + dz * dz;
			if (distSq <= radiusSq)
				ai0_add_threat(self, bomb, distSq, &replace);
		}
	}
}

/* 0x001065c0  Demolition (gap)
State 4: face the target and throw when the jump timer says the distance is right (higher or
lower targets shift the timing); then wander for a while. */
void DemoAI0_StateAttack(DemoAI *self, float dt)
{
	char *owner;
	char *target;
	float v;

	(void)dt;
	target = self->target;
	if (target == NULL || DD_I16(self, 0x198) <= 0) {
		self->state = 0;
		return;
	}
	owner = AI_OWNER(self);
	DD_F32(DD_PTR(owner, DP_ACTOR), 0x1d8) =
		atan2f(DD_F32(target, DP_POS) - DD_F32(owner, DP_POS),
		       DD_F32(target, DP_POS + 8) - DD_F32(owner, DP_POS + 8));
	owner = AI_OWNER(self);
	v = DemoAI_TargetDistXZ(self, DD_FP(owner, DP_POS)) * 0.000624999986f;
	target = self->target;
	if (DD_F32(owner, DP_HEIGHT) + DD_F32(owner, DP_POS + 4) < DD_F32(target, DP_POS + 4))
		v = (float)((double)v + 0.25);
	if (DD_F32(target, DP_HEIGHT) + DD_F32(target, DP_POS + 4) < DD_F32(owner, DP_POS + 4))
		v = (float)((double)v - 0.25);
	if (v > 1.0f)
		v = 1.0f;
	else if (v < 0.0f)
		v = 0.0f;
	if (DD_U8(owner, DP_CANJUMP) == 0) {
		DD_I32(DD_PTR(owner, DP_BUTTON_ACTION), 0x14) = 1;
		return;
	}
	if (AI_TimeLeft(owner + DP_JUMPTIMER) < v) {
		AI_PressAction(self, 1);
		return;
	}
	owner = AI_OWNER(self);
	if (DD_F32(owner, DP_HEIGHT) + DD_F32(owner, DP_POS + 4) < DD_F32(self->target, DP_POS + 4)) {
		/* the target is above: jump at it */
		if (DD_I32(DD_PTR(owner, DP_ACTOR), 0x214) != 0) {
			DD_I32(DD_PTR(owner, DP_BUTTON_JUMP), 0x14) = 1;
			return;
		}
	}
	DD_I32(DD_PTR(owner, DP_BUTTON_ACTION), 0x14) = 0;
	if (!(AI_CoreRandF() < 0.25f)) {
		self->state = 0;
		return;
	}
	DemoAI_GoAroundTarget(self, AI_CoreRandF() * 500.0f + 500.0f);
	v = AI_CoreRandF();
	self->state = 5;
	AI_F(self, 0x1a4) = (v + 0.5f) * AI_F(self, 0x1a8);
}

/* 0x00106830  Demolition (gap)
State 5: wander to the goal until the timer runs out or the route ends; flee from threats. */
void DemoAI0_StateWander(DemoAI *self, float dt)
{
	float t;

	if (DemoAI_ThreatNear(self) != 0) {
		t = dd_randf();
		self->state = 2;
		AI_F(self, 0x1b0) = (t + 0.5f) * AI_F(self, 0x1ac);
		return;
	}
	if (self->target != NULL) {
		t = AI_F(self, 0x1a4) - dt;
		AI_F(self, 0x1a4) = t;
		if (t > 0.0f && self->routing != 0 && self->arrived == 0)
			return;
	}
	self->state = 0;
}

/* 0x001068c0  Demolition (gap)
Slot 7: hit by `attacker` (not the team mate): take revenge, or run off somewhere. */
void DemoAI0_OnHit(DemoAI *self, char *attacker)
{
	float r;

	if (attacker == DD_PTR(AI_OWNER(self), DP_PARTNER))
		return;
	if (dd_randf() < self->params[0]) {
		self->lastTarget = attacker;
		self->target = attacker;
		r = dd_randf();
		self->mode = 2;
		AI_F(self, 0x1cc) = (r * 0.5f + 0.75f) * 300.0f;
		return;
	}
	DemoAI_GoRandom(self, 500.0f);
	r = dd_randf();
	self->state = 5;
	AI_F(self, 0x1a4) = (r + 0.5f) * AI_F(self, 0x1a8);
}

/* 0x001069d0  Demolition (gap)
Flees from the seen bombs: sometimes jumps when one is very close, and runs 500 units away
from their mean position, turned by up to 30 degrees (skill) towards the arena centre. */
void DemoAI0_Flee(DemoAI *self, float dt)
{
	char *arena;
	char *owner;
	float sx;
	float sy;
	float sz;
	float inv;
	float toCentre;
	float away;
	float angle;
	int n;
	int i;

	(void)dt;
	if (dd_randf() < 0.5f) {
		for (i = 0; i < self->numThreats; i++) {
			if (self->threats[i].dist < 200.0f) {
				AI_PressJump(self, 1);
				break;
			}
		}
	}
	n = self->numThreats;
	if (n != 0) {
		sx = 0.0f;
		sy = 0.0f;
		sz = 0.0f;
		for (i = 0; i < n; i++) {
			sx = sx + self->threats[i].pos[0];
			sy = sy + self->threats[i].pos[1];
			sz = sz + self->threats[i].pos[2];
		}
		inv = 1.0f / (float)n;
		AI_F(self, 0x1b4) = inv * sx;
		AI_F(self, 0x1b8) = inv * sy;
		AI_F(self, 0x1bc) = inv * sz;
	}
	if (self->hasGoal == 0) {
		arena = AI_Arena();
		owner = AI_OWNER(self);
		toCentre = atan2f(DD_F32(owner, DP_POS) - DD_F32(arena, 0x24),
		                  DD_F32(owner, DP_POS + 8) - DD_F32(arena, 0x2c));
		away = atan2f(DD_F32(owner, DP_POS) - AI_F(self, 0x1b4),
		              DD_F32(owner, DP_POS + 8) - AI_F(self, 0x1bc));
		if (FUN_000703d0(toCentre, away) <= 0.0f)
			angle = self->params[2] * 0.52359879f + away;
		else
			angle = away - self->params[2] * 0.52359879f;
		owner = AI_OWNER(self);
		/* y and w: uninitialised in the original */
		ai_set_goal(self, sinf(angle) * 500.0f + DD_F32(owner, DP_POS), 0.0f,
		            cosf(angle) * 500.0f + DD_F32(owner, DP_POS + 8), 0.0f);
	}
}

/* 0x00106bb0  Demolition (gap)
Chases the target: without ready bombs, or too close, wander around it; keep the distance
between "too close" and "too far" (retreat to the retreat distance), aim, and attack when
the aim timer runs out. */
void DemoAI0_Chase(DemoAI *self, float dt)
{
	char *owner;
	char *target;
	float t;
	float dist;
	float r;

	t = AI_F(self, 0x1a0) - dt;
	AI_F(self, 0x1a0) = t;
	if (t < 0.0f)
		AI_F(self, 0x1a0) = 0.0f;
	if (DD_I16(self, 0x198) < 1) {
		DemoAI_GoAroundTarget(self, (float)dd_rand() * DD_RAND_SCALE * 500.0f + 500.0f);
		r = dd_randf();
		self->state = 5;
		AI_F(self, 0x1a4) = (r + 0.5f) * AI_F(self, 0x1a8);
		return;
	}
	dist = DemoAI_TargetDist(self, DD_FP(AI_OWNER(self), DP_POS));
	if (dist < AI_F(self, 0x188)) {
		DemoAI_GoAroundTarget(self, (float)dd_rand() * DD_RAND_SCALE * AI_F(self, 0x188) + AI_F(self, 0x188));
		r = dd_randf();
		self->state = 5;
		AI_F(self, 0x1a4) = (r + 0.5f) * AI_F(self, 0x1a8);
	}
	if (AI_I(self, 0x194) != 0) {
		if (dist < AI_F(self, 0x18c)) {
			owner = AI_OWNER(self);
			self->hasGoal = 0;
			memcpy(self->goal, DD_FP(owner, DP_POS), 16);
			AI_I(self, 0x194) = 0;
			return;
		}
		DemoAI_GoNear(self, DD_FP(self->target, DP_POS), 300.0f);
		return;
	}
	if (!(dist <= AI_F(self, 0x190))) {
		AI_I(self, 0x194) = 1;
		return;
	}
	owner = AI_OWNER(self);
	target = self->target;
	DD_F32(DD_PTR(owner, DP_ACTOR), 0x1d8) =
		AI_TurnTowards(DD_F32(DD_PTR(owner, DP_ACTOR), 0x1d8),
		               atan2f(DD_F32(target, DP_POS) - DD_F32(owner, DP_POS),
		                      DD_F32(target, DP_POS + 8) - DD_F32(owner, DP_POS + 8)),
		               dt * 0.052359879f);
	if (AI_F(self, 0x1a0) <= 0.0f) {
		r = dd_randf();
		self->state = 4;
		AI_F(self, 0x19c) = (r + 0.5f) * AI_F(self, 0x1a0);
	}
}

/* 0x00106e80  Demolition (gap)
Slot 4: reset (parameters of the variation and difficulty, timers, a first target). */
void DemoAI0_Reset(DemoAI *self)
{
	float f;
	float r;

	ai_reset_base(self);
	self->arriveDist = 100.0f;
	self->f160 = 0.785398185f;
	self->goal[0] = 0.0f;
	self->goal[2] = 0.0f;
	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	self->f148 = self->params[2] * 700.0f + 400.0f;
	f = self->params[2] * 500.0f;
	self->f14c = 200.0f + f;
	self->dangerFuse = self->params[2] * 2.5f;
	AI_F(self, 0x188) = f + 400.0f;
	AI_I(self, 0x194) = 0;
	AI_F(self, 0x18c) = self->params[2] * 800.0f + 400.0f;
	AI_F(self, 0x190) = self->params[2] * 1200.0f + 800.0f;
	AI_F(self, 0x1a0) = (1.0f - self->params[0]) * 150.0f;
	r = dd_randf();
	AI_F(self, 0x1a8) = 80.0f;
	AI_F(self, 0x19c) = (r + 0.5f) * AI_F(self, 0x1a0);
	r = dd_randf();
	AI_F(self, 0x1ac) = 100.0f;
	AI_F(self, 0x1a4) = (r + 0.5f) * AI_F(self, 0x1a8);
	AI_F(self, 0x1b0) = ai_vary(AI_F(self, 0x1ac));
	if (dd_randf() < 0.5f) {
		self->target = DemoAI_PickRandom(self);
		r = dd_randf();
		self->mode = 0;
		AI_I(self, 0x180) = 0;
		AI_F(self, 0x1c4) = (r * 0.5f + 0.75f) * 150.0f;
		return;
	}
	self->target = DemoAI_PickLeader(self);
	r = dd_randf();
	AI_I(self, 0x180) = 0;
	self->mode = 4;
	AI_F(self, 0x1d4) = (r * 0.5f + 0.75f) * 200.0f;
}

/* 0x001071f0  Demolition (gap)
Slot 5: restart (the skill-dependent values and timers again). */
void DemoAI0_Restart(DemoAI *self)
{
	float r;

	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	AI_I(self, 0x194) = 0;
	self->f148 = self->params[2] * 700.0f + 400.0f;
	self->f14c = self->params[2] * 500.0f + 200.0f;
	self->dangerFuse = self->params[2] * 2.5f;
	AI_F(self, 0x18c) = self->params[2] * 800.0f + 400.0f;
	AI_F(self, 0x190) = self->params[2] * 1200.0f + 800.0f;
	AI_F(self, 0x1a0) = (1.0f - self->params[0]) * 150.0f;
	r = dd_randf();
	AI_F(self, 0x1a8) = 80.0f;
	AI_F(self, 0x19c) = (r + 0.5f) * AI_F(self, 0x1a0);
	r = dd_randf();
	AI_F(self, 0x1ac) = 100.0f;
	AI_F(self, 0x1a4) = (r + 0.5f) * AI_F(self, 0x1a8);
	AI_F(self, 0x1b0) = ai_vary(AI_F(self, 0x1ac));
}

/* 0x00109ec0  Demolition (gap)
State 1: hunt the target, or flee from a near threat. */
void DemoAI0_StateHunt(DemoAI *self, float dt)
{
	float r;

	if (DemoAI_ThreatNear(self) != 0) {
		r = dd_randf();
		self->state = 2;
		AI_I(self, 0x194) = 0;
		AI_F(self, 0x1b0) = (r + 0.5f) * AI_F(self, 0x1ac);
		return;
	}
	DemoAI0_Chase(self, dt);
}

/* 0x00109f30  Demolition (gap)
State 2: flee while a threat is near. */
void DemoAI0_StateFlee(DemoAI *self, float dt)
{
	float t;

	if (DemoAI_ThreatNear(self) == 0) {
		self->state = 1;
		return;
	}
	t = AI_F(self, 0x19c) - dt;
	AI_F(self, 0x19c) = t;
	if (t < 0.0f)
		AI_F(self, 0x19c) = 0.0f;
	DemoAI0_Flee(self, dt);
}

/* the target choices when a mode's timer runs out */
static void ai0_to_leader(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickLeader(self);
	r = dd_randf();
	self->mode = 4;
	AI_F(self, 0x1d4) = (r * 0.5f + 0.75f) * 200.0f;
}

static void ai0_to_random(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickRandom(self);
	r = dd_randf();
	self->mode = 0;
	AI_F(self, 0x1c4) = (r * 0.5f + 0.75f) * 150.0f;
}

static void ai0_to_nearest(DemoAI *self)
{
	float r;

	self->target = DemoAI_PickNearest(self, DD_FP(AI_OWNER(self), DP_POS));
	r = dd_randf();
	self->mode = 1;
	AI_F(self, 0x1c8) = (r * 0.5f + 0.75f) * 150.0f;
}

/* 0x00109f70  Demolition (gap)
Mode 0 (random target): when its time is up, the leader or the nearest enemy. */
void DemoAI0_ModeRandom(DemoAI *self, float dt)
{
	float t;

	t = AI_F(self, 0x1c4) - dt;
	AI_F(self, 0x1c4) = t;
	if (t <= 0.0f) {
		AI_F(self, 0x1c4) = 0.0f;
		if (dd_randf() < self->params[1]) {
			ai0_to_leader(self);
			return;
		}
		ai0_to_nearest(self);
	}
}

/* 0x0010a0a0  Demolition (gap)
Mode 1 (nearest enemy): when its time is up, the leader or a random enemy. */
void DemoAI0_ModeNearest(DemoAI *self, float dt)
{
	float t;

	t = AI_F(self, 0x1c8) - dt;
	AI_F(self, 0x1c8) = t;
	if (t <= 0.0f) {
		AI_F(self, 0x1c8) = 0.0f;
		if (dd_randf() < self->params[1]) {
			ai0_to_leader(self);
			return;
		}
		ai0_to_random(self);
	}
}

/* 0x0010a1d0  Demolition (gap)
Mode 2 (revenge on the last attacker): when its time is up, the nemesis or the leader. */
void DemoAI0_ModeRevenge(DemoAI *self, float dt)
{
	float t;
	float r;

	t = AI_F(self, 0x1cc) - dt;
	AI_F(self, 0x1cc) = t;
	if (t <= 0.0f) {
		AI_F(self, 0x1cc) = 0.0f;
		if (dd_randf() < self->params[0]) {
			self->target = DemoAI_PickNemesis(self);
			r = dd_randf();
			self->mode = 3;
			AI_F(self, 0x1d0) = (r * 0.5f + 0.75f) * 300.0f;
			return;
		}
		ai0_to_leader(self);
	}
}

/* 0x0010a300  Demolition (gap)
Mode 3 (nemesis): when its time is up, the leader or the nearest enemy. */
void DemoAI0_ModeNemesis(DemoAI *self, float dt)
{
	float t;

	t = AI_F(self, 0x1d0) - dt;
	AI_F(self, 0x1d0) = t;
	if (t <= 0.0f) {
		AI_F(self, 0x1d0) = 0.0f;
		if (dd_randf() < self->params[1]) {
			ai0_to_leader(self);
			return;
		}
		ai0_to_nearest(self);
	}
}

/* 0x0010a430  Demolition (gap)
Mode 4 (leader): when its time is up, the leader again or a random enemy. */
void DemoAI0_ModeLeader(DemoAI *self, float dt)
{
	float t;

	t = AI_F(self, 0x1d4) - dt;
	AI_F(self, 0x1d4) = t;
	if (t <= 0.0f) {
		AI_F(self, 0x1d4) = 0.0f;
		if (dd_randf() < self->params[1]) {
			ai0_to_leader(self);
			return;
		}
		ai0_to_random(self);
	}
}

/* the first part of DemoAI0_Think: plan the route to the goal (now when asked, else every
   (1 - skill) * 60 + 30 frames) and note whether the route still runs */
static void ai0_update_route(DemoAI *self, float dt)
{
	char *owner;
	float t;

	self->arrived = 0;
	if (self->hasGoal == 0)
		return;
	owner = AI_OWNER(self);
	if (self->newGoal != 0) {
		self->newGoal = 0;
		FUN_000adb70((struct CLRoute *)(owner + DP_ROUTE), DD_FP(owner, DP_POS), self->goal,
		             DD_F32(owner, DP_POS + 4));
	} else {
		t = self->routeTimer - dt;
		self->routeTimer = t;
		if (t <= 0.0f) {
			self->routeTimer = (1.0f - self->params[2]) * 60.0f + 30.0f;
			FUN_000adb70((struct CLRoute *)(owner + DP_ROUTE), DD_FP(owner, DP_POS), self->goal,
			             DD_F32(owner, DP_POS + 4));
		}
	}
	owner = AI_OWNER(self);
	if (DD_I16(owner, DP_ROUTE_NODE) == -1)
		self->routing = 0;
	else
		self->routing = 1;
	if (DD_I16(owner, DP_ROUTE_ACTIVE) == 0 && self->routing != 0)
		return;
	self->hasGoal = 0;
	if (self->routing != 0)
		self->arrived = 1;
}

/* 0x0010b8b0  Demolition (gap)
The route, the state machine, and in state 4 the target mode. */
void DemoAI0_Think(DemoAI *self, float dt)
{
	ai0_update_route(self, dt);
	switch (self->state) {
	case 0:
		self->state = 1;
		break;
	case 1:
		DemoAI0_StateHunt(self, dt);
		break;
	case 2:
		DemoAI0_StateFlee(self, dt);
		break;
	case 3:
		DemoAI0_StateWait(self, dt);
		break;
	case 4:
		DemoAI0_StateAttack(self, dt);
		break;
	case 5:
		DemoAI0_StateWander(self, dt);
		break;
	}
	if (self->state == 4) {
		switch (self->mode) {
		case 0:
			DemoAI0_ModeRandom(self, dt);
			return;
		case 1:
			DemoAI0_ModeNearest(self, dt);
			return;
		case 2:
			DemoAI0_ModeRevenge(self, dt);
			return;
		case 3:
			DemoAI0_ModeNemesis(self, dt);
			return;
		case 4:
			DemoAI0_ModeLeader(self, dt);
			return;
		}
	}
}

/* 0x0010bbd0  Demolition (gap)
Slot 1: per frame (not in the CharLib AI states 13..17): follow the route, scan, think. */
void DemoAI0_Process(DemoAI *self, float dt)
{
	unsigned short aiState;

	aiState = DD_U16(AI_OWNER(self), DP_AISTATE);
	if (aiState < 0xd || aiState > 0x11) {
		DD_I16(AI_OWNER(self), DP_ROUTE_ACTIVE) = 0;
		if (self->hasGoal != 0)
			AI_FollowRoute(AI_OWNER(self));
		DemoAI0_Scan(self, dt);
		DemoAI0_Think(self, dt);
	}
}
