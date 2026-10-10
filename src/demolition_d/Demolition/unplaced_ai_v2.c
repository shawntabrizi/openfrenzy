/*
Demolition/unplaced_ai_v2.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\ (gap functions; the file is not known)

The brain of a computer player in game variation 2 (vtable 0x0026bba0, 0x1b4 bytes; made by
demolition_b's 0x000f8dc0). The players collect tokens (ClassLib CToken: bronze, silver,
gold) while bombs of the bomb system (game +0x13e8, 8 bombs 0x2b0 bytes apart) go off: the
brain walks to the best token (in the zone nearest to it when the variation has zones),
flees from bombs about to explode, and attacks a near enemy now and then.

Class fields after the shared part (struct DemoAI):
  0x170 retarget time, 0x174 timer      0x178 scan time, 0x17c timer
  0x188 turn rate (6 degrees)           0x18c the token it goes for
  0x190 re-goal time, 0x194 timer       0x198 reach time (150), 0x19c timer
  0x1a0 token reach (600..1200)         0x1a4 attack range (200..1000)
  0x1a8 attack time, 0x1ac timer        0x1b0 the variation has zones (byte +0x7e)
States: 0 start, 1 collect, 2 flee, 3 attack.
*/

#include "dd_ai.h"

/* 0x0026bba0  the vtable (9 slots) */
const void *const vtable_0026bba0[9] = {
	(const void *)FUN_00105a70, /* [0] Init */
	(const void *)FUN_0010bc50, /* [1] Process */
	(const void *)FUN_001d5950, /* [2] (empty) */
	(const void *)FUN_001d5950, /* [3] (empty) */
	(const void *)FUN_001085b0, /* [4] Reset */
	(const void *)FUN_00108900, /* [5] Restart */
	(const void *)FUN_00105aa0, /* [6] OnPlayerGone */
	(const void *)FUN_00108480, /* [7] OnHit */
	(const void *)FUN_00108110, /* [8] scalar deleting destructor */
};

/* the fields of a token (ClassLib CToken, classlib_fx) */
#define TOKEN_POS 0x180
#define TOKEN_STATE 0x1e8 /* 4: vanishing */
#define TOKEN_TYPE 0x1ec  /* 0 bronze, 1 silver, 2 gold */

/* a bomb of the bomb system: position at +0, live flag at +0x40, fuse timer at +0x60 */
#define AI2_BOMB(i) (DD_PTR(DD_GAME, DG_BOMBLIST) + 0x190 + (i) * 0x2b0)

/* 0x00105a90  Demolition (gap)
Constructor. */
DemoAI *DemoAI2_Ctor(DemoAI *self)
{
	self->vtable = (const void *const *)vtable_0026bba0;
	return self;
}

/* 0x00108110  Demolition (gap)
Scalar deleting destructor. */
DemoAI *DemoAI2_Delete(DemoAI *self, unsigned int flags)
{
	self->vtable = (const void *const *)vtable_0026bba0;
	if ((flags & 1) != 0)
		bkHeapFree(self);
	return self;
}

/* 0x00108130  Demolition (gap)
Now and then picks a new token reach; on its scan timer collects the live bombs within the
scan radius. */
void DemoAI2_Scan(DemoAI *self, float dt)
{
	char *bomb;
	float t;
	float r;
	float radiusSq;
	float dx;
	float dz;
	float distSq;
	float worst;
	int replace;
	int n;
	int i;
	int k;

	t = AI_F(self, 0x19c) - dt;
	AI_F(self, 0x19c) = t;
	if (t < 0.0f) {
		r = dd_randf();
		AI_F(self, 0x19c) = (r + 0.5f) * AI_F(self, 0x198);
		AI_F(self, 0x1a0) = (float)dd_rand() * DD_RAND_SCALE * 600.0f + 600.0f;
	}
	t = AI_F(self, 0x17c) - dt;
	AI_F(self, 0x17c) = t;
	if (!(t < 0.0f))
		return;
	r = dd_randf();
	AI_F(self, 0x17c) = (r + 0.5f) * AI_F(self, 0x178);
	AI_I(self, 0x1b0) = DD_U8(AI_VARIATION(), 0x7e);
	if (self->state == 2)
		self->scanRadius = self->f148;
	else
		self->scanRadius = self->f14c;
	radiusSq = self->scanRadius * self->scanRadius;
	self->numThreats = 0;
	/* the original's replacement index starts as garbage; it is only used after being set */
	replace = 0;
	for (k = 0; k < 8; k++) {
		bomb = AI2_BOMB(k);
		if (DD_U8(bomb, 0x40) == 0)
			continue;
		dx = DD_F32(AI_OWNER(self), DP_POS) - DD_F32(bomb, 0);
		dz = DD_F32(AI_OWNER(self), DP_POS + 8) - DD_F32(bomb, 8);
		distSq = dx * dx + dz * dz;
		if (!(distSq <= radiusSq))
			continue;
		n = self->numThreats;
		if (n < self->maxThreats) {
			replace = n;
			memcpy(self->threats[n].pos, bomb, 16);
			self->threats[n].dist = distSq;
			self->threats[n].fuse = DD_TimerValue(bomb + 0x60);
			self->numThreats = self->numThreats + 1;
		} else {
			worst = 0.0f;
			for (i = 0; i < self->maxThreats; i++) {
				if (worst < self->threats[i].dist) {
					worst = self->threats[i].dist;
					replace = i;
				}
			}
			if (distSq < worst) {
				memcpy(self->threats[replace].pos, bomb, 16);
				self->threats[replace].dist = distSq;
				self->threats[replace].fuse = DD_TimerValue(bomb + 0x60);
			}
		}
	}
}

/* 0x001083d0  Demolition (gap)
State 3: go for the target (not while it is knocked over, AI states 13..15) until the
attack time is up; throw when it is within 300 and in front. */
void DemoAI2_StateAttack(DemoAI *self, float dt)
{
	unsigned short s;
	float t;

	if (self->target != NULL) {
		s = DD_U16(self->target, DP_AISTATE);
		if (s < 0xd || s > 0xf) {
			t = AI_F(self, 0x1ac) - dt;
			AI_F(self, 0x1ac) = t;
			if (t >= 0.0f) {
				DemoAI_GoToTarget(self);
				DemoAI_Steer(self, dt, AI_F(self, 0x188));
				s = DD_U16(AI_OWNER(self), DP_AISTATE);
				if (s < 5 || s > 6) {
					if (DemoAI_TargetDistXZ(self, DD_FP(AI_OWNER(self), DP_POS)) < 300.0f &&
					    DemoAI_FacingTarget(self, 1.57079637f) != 0)
						AI_PressAction(self, 1);
				}
				return;
			}
		}
	}
	self->state = 0;
}

/* 0x00108480  Demolition (gap)
Slot 7: hit by `attacker` (not the team mate): maybe attack, the attacker or the leader. */
void DemoAI2_OnHit(DemoAI *self, char *attacker)
{
	float r;

	if (attacker == DD_PTR(AI_OWNER(self), DP_PARTNER))
		return;
	if (dd_randf() < self->params[0]) {
		r = dd_randf();
		AI_F(self, 0x174) = (r + 0.5f) * AI_F(self, 0x170);
		r = dd_randf();
		self->state = 3;
		AI_F(self, 0x1ac) = (r + 0.5f) * AI_F(self, 0x1a8);
		if (self->params[1] < dd_randf()) {
			self->target = attacker;
			return;
		}
		self->target = DemoAI_PickLeader(self);
	}
}

/* 0x001085b0  Demolition (gap)
Slot 4: reset. */
void DemoAI2_Reset(DemoAI *self)
{
	float r;

	ai_reset_base(self);
	AI_F(self, 0x188) = 0.104719758f;
	self->arriveDist = 100.0f;
	self->f160 = 0.785398185f;
	self->goal[0] = 0.0f;
	self->goal[2] = 0.0f;
	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	self->f148 = self->params[2] * 500.0f + 500.0f;
	self->f14c = self->params[2] * 400.0f + 300.0f;
	self->dangerFuse = self->params[2] * 2.5f;
	AI_F(self, 0x198) = 150.0f;
	AI_I(self, 0x194) = 0;
	AI_F(self, 0x190) = (1.0f - self->params[2]) * 100.0f;
	AI_F(self, 0x19c) = ai_vary(AI_F(self, 0x198));
	AI_F(self, 0x1a0) = (float)dd_rand() * DD_RAND_SCALE * 600.0f + 600.0f;
	AI_F(self, 0x170) = (1.0f - self->params[0]) * 500.0f + 200.0f;
	AI_F(self, 0x174) = ai_vary(AI_F(self, 0x170));
	AI_F(self, 0x1a4) = (float)dd_rand() * DD_RAND_SCALE * 800.0f + 200.0f;
	AI_F(self, 0x1a8) = self->params[0] * 500.0f + 200.0f;
	AI_F(self, 0x1ac) = ai_vary(AI_F(self, 0x1a8));
	AI_F(self, 0x178) = (1.0f - self->params[4]) * 60.0f + 10.0f;
	r = dd_randf();
	AI_I(self, 0x1b0) = 0;
	AI_I(self, 0x18c) = 0;
	AI_F(self, 0x17c) = (r + 0.5f) * AI_F(self, 0x178);
}

/* 0x00108900  Demolition (gap)
Slot 5: restart (the skill-dependent values and timers again). */
void DemoAI2_Restart(DemoAI *self)
{
	self->maxThreats = (int)(longlong)(self->params[2] * 4.0f);
	self->numThreats = 0;
	AI_I(self, 0x194) = 0;
	AI_F(self, 0x198) = 150.0f;
	self->f148 = self->params[2] * 500.0f + 500.0f;
	self->f14c = self->params[2] * 400.0f + 300.0f;
	self->dangerFuse = self->params[2] * 2.5f;
	AI_F(self, 0x190) = (1.0f - self->params[2]) * 100.0f;
	AI_F(self, 0x19c) = ai_vary(AI_F(self, 0x198));
	AI_F(self, 0x1a0) = (float)dd_rand() * DD_RAND_SCALE * 600.0f + 600.0f;
	AI_F(self, 0x170) = (1.0f - self->params[0]) * 500.0f + 200.0f;
	AI_F(self, 0x174) = ai_vary(AI_F(self, 0x170));
	AI_F(self, 0x1a4) = (float)dd_rand() * DD_RAND_SCALE * 800.0f + 200.0f;
	AI_F(self, 0x1a8) = self->params[0] * 500.0f + 200.0f;
	AI_F(self, 0x1ac) = ai_vary(AI_F(self, 0x1a8));
	AI_F(self, 0x178) = (1.0f - self->params[4]) * 60.0f + 10.0f;
	AI_F(self, 0x17c) = ai_vary(AI_F(self, 0x178));
}

/* 0x00108b60  Demolition (gap)
Pushes the stick away from the mean position of the seen bombs. */
void DemoAI2_Flee(DemoAI *self, float dt)
{
	char *owner;
	float sum[4];
	float d[4];
	float stick[4];
	float inv;
	int n;
	int i;

	(void)dt;
	n = self->numThreats;
	if (n <= 0)
		return;
	owner = AI_OWNER(self);
	sum[0] = 0.0f;
	sum[1] = 0.0f;
	sum[2] = 0.0f;
	sum[3] = 0.0f; /* w: uninitialised in the original */
	d[3] = 0.0f;
	for (i = 0; i < n; i++) {
		d[0] = DD_F32(owner, DP_POS) - self->threats[i].pos[0];
		d[1] = DD_F32(owner, DP_POS + 4) - self->threats[i].pos[1];
		d[2] = DD_F32(owner, DP_POS + 8) - self->threats[i].pos[2];
		sum[0] = d[0] + sum[0];
		sum[1] = d[1] + sum[1];
		sum[2] = d[2] + sum[2];
	}
	inv = 1.0f / (float)n;
	sum[0] = sum[0] * inv;
	sum[1] = sum[1] * inv;
	sum[2] = inv * sum[2];
	FUN_000514a0(d, sum);
	stick[0] = d[0];
	stick[1] = 0.0f;
	stick[2] = d[2];
	stick[3] = 0.0f; /* uninitialised in the original */
	AI_SetStick(AI_OWNER(self), stick, 1.0f);
}

/* 0x00108c60  Demolition (gap)
Chooses the token to go for (in the zone nearest to the player when the variation has
zones; far tokens only by a skill roll): gold, unless silver is under half or bronze under
0.3 of its distance; else silver, unless bronze is under half its distance; else bronze. */
char *DemoAI2_PickToken(DemoAI *self)
{
	char *zone;
	char *z;
	char *tok;
	char *bronze;
	char *silver;
	char *gold;
	float best;
	float dBronze;
	float dSilver;
	float dGold;
	float dx;
	float dz;
	float d;
	int i;

	zone = NULL;
	if (AI_I(self, 0x1b0) != 0) {
		best = 99999.0f;
		for (i = 0; i < 4; i++) {
			z = AI_Zone(i);
			dx = DD_F32(z, 0x24) - DD_F32(AI_OWNER(self), DP_POS);
			dz = DD_F32(z, 0x2c) - DD_F32(AI_OWNER(self), DP_POS + 8);
			d = sqrtf(dx * dx + dz * dz);
			if (d < best) {
				zone = z;
				best = d;
			}
		}
	}
	bronze = NULL;
	silver = NULL;
	gold = NULL;
	dGold = 99999.0f;
	dSilver = 99999.0f;
	dBronze = 99999.0f;
	tok = (char *)CToken_Next(NULL);
	if (tok == NULL)
		return NULL;
	do {
		if (DD_I32(tok, TOKEN_STATE) != 4 &&
		    (zone == NULL || AI_RegionContains(zone, DD_FP(tok, TOKEN_POS)) != 0)) {
			dx = DD_F32(tok, TOKEN_POS) - DD_F32(AI_OWNER(self), DP_POS);
			dz = DD_F32(tok, TOKEN_POS + 8) - DD_F32(AI_OWNER(self), DP_POS + 8);
			d = sqrtf(dx * dx + dz * dz);
			if (d <= AI_F(self, 0x1a0) || dd_randf() <= self->params[2]) {
				switch (DD_I32(tok, TOKEN_TYPE)) {
				case 0:
					if (d < dBronze) {
						bronze = tok;
						dBronze = d;
					}
					break;
				case 1:
					if (d < dSilver) {
						dSilver = d;
						silver = tok;
					}
					break;
				case 2:
					if (d < dGold) {
						dGold = d;
						gold = tok;
					}
					break;
				}
			}
		}
		tok = (char *)CToken_Next(tok);
	} while (tok != NULL);
	if (gold != NULL) {
		if (silver == NULL) {
			if (bronze != NULL && dBronze < dGold * 0.300000012f)
				return bronze;
		} else if (dSilver < dGold * 0.5f) {
			return silver;
		}
		return gold;
	}
	if (silver != NULL) {
		if (bronze != NULL && dBronze < dSilver * 0.5f)
			return bronze;
		return silver;
	}
	if (bronze != NULL)
		return bronze;
	return NULL;
}

/* 0x0010afe0  Demolition (gap)
State 1: collect tokens; flee from a bomb about to go off; now and then attack the nearest
enemy when it is within the attack range. */
void DemoAI2_StateCollect(DemoAI *self, float dt)
{
	char *tok;
	float *pos;
	float t;
	float r;
	float dx;
	float dz;
	int i;

	for (i = 0; i < self->numThreats; i++) {
		if (self->threats[i].fuse < self->dangerFuse) {
			self->state = 2;
			return;
		}
	}
	t = AI_F(self, 0x174) - dt;
	AI_F(self, 0x174) = t;
	if (t < 0.0f) {
		pos = DD_FP(AI_OWNER(self), DP_POS);
		AI_F(self, 0x174) = 0.0f;
		self->target = DemoAI_PickNearest(self, pos);
		if (self->target != NULL && DemoAI_TargetDistXZ(self, pos) < AI_F(self, 0x1a4)) {
			r = dd_randf();
			AI_F(self, 0x174) = (r + 0.5f) * AI_F(self, 0x170);
			r = dd_randf();
			self->state = 3;
			AI_F(self, 0x1ac) = (r + 0.5f) * AI_F(self, 0x1a8);
			return;
		}
	}
	t = AI_F(self, 0x194) - dt;
	AI_F(self, 0x194) = t;
	dx = 0.0f;
	dz = 0.0f;
	if (!(t < 0.0f)) {
		dx = self->goal[0] - DD_F32(AI_OWNER(self), DP_POS);
		dz = self->goal[2] - DD_F32(AI_OWNER(self), DP_POS + 8);
	}
	if (t < 0.0f || sqrtf(dx * dx + dz * dz) < self->arriveDist) {
		r = dd_randf();
		AI_F(self, 0x194) = (r * 0.5f + 0.75f) * AI_F(self, 0x190);
		tok = DemoAI2_PickToken(self);
		AI_P(self, 0x18c) = tok;
		if (tok != NULL) {
			memcpy(self->goal, DD_FP(tok, TOKEN_POS), 16);
			self->hasGoal = 1;
			self->newGoal = 1;
		}
	}
	DemoAI_Steer(self, dt, AI_F(self, 0x188));
}

/* 0x0010b210  Demolition (gap)
State 2: flee while a bomb is about to go off. */
void DemoAI2_StateFlee(DemoAI *self, float dt)
{
	int i;

	for (i = 0; i < self->numThreats; i++) {
		if (self->threats[i].fuse < self->dangerFuse) {
			DemoAI2_Flee(self, dt);
			return;
		}
	}
	self->state = 0;
}

/* 0x0010bc50  Demolition (gap)
Slot 1: per frame (not in the CharLib AI states 13..17): follow the route, scan, and run the
state. */
void DemoAI2_Process(DemoAI *self, float dt)
{
	unsigned short aiState;

	aiState = DD_U16(AI_OWNER(self), DP_AISTATE);
	if (aiState < 0xd || aiState > 0x11) {
		DD_I16(AI_OWNER(self), DP_ROUTE_ACTIVE) = 0;
		if (self->hasGoal != 0)
			AI_FollowRoute(AI_OWNER(self));
		DemoAI2_Scan(self, dt);
		switch (self->state) {
		case 0:
			self->state = 1;
			return;
		case 1:
			DemoAI2_StateCollect(self, dt);
			return;
		case 2:
			DemoAI2_StateFlee(self, dt);
			return;
		case 3:
			DemoAI2_StateAttack(self, dt);
			return;
		}
	}
}
