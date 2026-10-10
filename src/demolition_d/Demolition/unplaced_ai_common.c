/*
Demolition/unplaced_ai_common.c  (module demolition_d)
<- C:\BlitzParty\Game\Src\Demolition\ (gap functions; the file is not known)

The methods shared by the four brain classes of the computer players (struct DemoAI): goal
picking (random point in the arena, near a player, around the target), steering towards the
goal, and target choice (random, nearest, leader, nemesis). Each brain drives one
CDemolition_CharLib player (demolition_b) through the CharLib AI's route and fake pad.
*/

#include "dd_ai.h"

/* 0x00105a10  Demolition (gap)
A threat is near: with skill >= 0.5 any seen bomb, else only a bomb about to go off. */
int DemoAI_ThreatNear(DemoAI *self)
{
	int i;

	if (self->params[2] < 0.5f) {
		for (i = 0; i < self->numThreats; i++) {
			if (self->threats[i].fuse < self->dangerFuse)
				return 1;
		}
	} else if (self->numThreats != 0) {
		return 1;
	}
	return 0;
}

/* 0x00105a70  Demolition (gap)
Slot 0 of every brain class: attach to the player and reset (virtual +0x10). */
int DemoAI_Init(DemoAI *self, char *owner)
{
	self->owner = owner;
	DD_VFN(self, 0x10, void (*)(DemoAI *))(self);
	return 1;
}

/* 0x00105b00  Demolition (gap)
A seen bomb is about to go off. */
int DemoAI_ThreatUrgent(DemoAI *self)
{
	int count;
	int i;

	count = self->numThreats;
	if (count < 1)
		return 0;
	for (i = 0; i < count; i++) {
		if (self->threats[i].fuse < self->dangerFuse)
			return 1;
	}
	return 0;
}

/* 0x00105b50  Demolition (gap)
Goes to the target player. */
void DemoAI_GoToTarget(DemoAI *self)
{
	char *target;

	target = self->target;
	if (target != NULL)
		ai_set_goal(self, DD_F32(target, DP_POS), DD_F32(target, DP_POS + 4),
		            DD_F32(target, DP_POS + 8), DD_F32(target, DP_POS + 12));
}

/* 0x00105b90  Demolition (gap)
Goes to a random point of the arena that is clear (radius `clearance`) and, if possible in
12 tries, not within `clearance` of a player. */
void DemoAI_GoRandom(DemoAI *self, float clearance)
{
	char *arena;
	char *player;
	float pos[4];
	float twice;
	float squared;
	float dx;
	float dz;
	int r1;
	int r2;
	int tries;

	squared = clearance * clearance;
	twice = clearance + clearance;
	/* y and w are not written by the original (uninitialised stack); 0 here */
	pos[1] = 0.0f;
	pos[3] = 0.0f;
	arena = AI_Arena();
	tries = 0;
	for (;;) {
		do {
			r1 = dd_rand();
			r2 = dd_rand();
			pos[0] = ((float)r1 * DD_RAND_SCALE * (DD_F32(arena, 0x34) + DD_F32(arena, 0x34)) -
			          DD_F32(arena, 0x34)) + DD_F32(arena, 0x24);
			pos[2] = ((float)r2 * DD_RAND_SCALE * (DD_F32(arena, 0x34) + DD_F32(arena, 0x34)) -
			          DD_F32(arena, 0x34)) + DD_F32(arena, 0x2c);
		} while (AI_RegionContains(arena, pos) == 0 || AI_RegionClear(arena, pos, clearance, 0) == 0);
		for (player = AI_FIRST_PLAYER(); player != NULL; player = AI_NEXT_PLAYER(player)) {
			dx = pos[0] - DD_F32(player, DP_POS);
			if (fabsf(dx) <= twice) {
				dz = pos[2] - DD_F32(player, DP_POS + 8);
				if (fabsf(dz) <= twice && dx * dx + dz * dz <= squared)
					break;
			}
		}
		if (player == NULL)
			break;
		if (!(tries++ <= 10))
			break;
	}
	ai_set_goal(self, pos[0], pos[1], pos[2], pos[3]);
}

/* 0x00105d60  Demolition (gap)
Goes to `pos`, stopping `offset` short of it on the line from the player. */
void DemoAI_GoNear(DemoAI *self, float *pos, float offset)
{
	float dx;
	float dz;
	float len;
	float inv;
	float nx;
	float nz;

	(void)AI_Arena();
	dx = pos[0] - DD_F32(AI_OWNER(self), DP_POS);
	dz = pos[2] - DD_F32(AI_OWNER(self), DP_POS + 8);
	len = sqrtf(dx * dx + dz * dz);
	/* the original leaves the direction uninitialised when the length is 0; 0 here */
	nx = 0.0f;
	nz = 0.0f;
	if (len != 0.0f) {
		inv = 1.0f / len;
		nx = inv * dx;
		nz = inv * dz;
	}
	/* w is not written by the original (uninitialised stack); 0 here */
	self->goal[3] = 0.0f;
	self->goal[2] = -nz * offset + pos[2];
	self->goal[0] = -nx * offset + pos[0];
	self->goal[1] = pos[1];
	self->hasGoal = 1;
	self->newGoal = 1;
}

/* 0x00105e10  Demolition (gap)
Goes to a random point on the circle of `radius` around the target player. */
void DemoAI_GoAroundTarget(DemoAI *self, float radius)
{
	char *arena;
	float pos[4];
	float angle;

	pos[1] = 0.0f; /* y and w: uninitialised in the original */
	pos[3] = 0.0f;
	arena = AI_Arena();
	do {
		do {
			angle = (float)dd_rand() * DD_RAND_SCALE * 6.28318548f;
			pos[0] = cosf(angle) * radius + DD_F32(self->target, DP_POS);
			pos[2] = sinf(angle) * radius + DD_F32(self->target, DP_POS + 8);
		} while (AI_RegionContains(arena, pos) == 0);
		/* this caller retries while the test is nonzero (DemoAI_GoRandom: while zero) */
	} while (AI_RegionClear(arena, pos, 200.0f, 0) != 0);
	ai_set_goal(self, pos[0], pos[1], pos[2], pos[3]);
}

/* 0x00105ef0  Demolition (gap)
The player faces its target within `maxAngle`. */
int DemoAI_FacingTarget(DemoAI *self, float maxAngle)
{
	char *target;
	char *owner;
	float angle;

	target = self->target;
	if (target != NULL) {
		owner = AI_OWNER(self);
		angle = atan2f(DD_F32(target, DP_POS) - DD_F32(owner, DP_POS),
		               DD_F32(target, DP_POS + 8) - DD_F32(owner, DP_POS + 8));
		angle = FUN_000703d0(DD_F32(DD_PTR(owner, DP_ACTOR), 0x1d8), angle);
		if (fabsf(angle) < maxAngle)
			return 1;
	}
	return 0;
}

/* 0x00105f70  Demolition (gap)
Distance (x, z) from `pos` to the target player (0 without a target). */
float DemoAI_TargetDistXZ(DemoAI *self, float *pos)
{
	char *target;
	float dx;
	float dz;

	target = self->target;
	if (target == NULL)
		return 0.0f;
	dx = DD_F32(target, DP_POS) - pos[0];
	dz = DD_F32(target, DP_POS + 8) - pos[2];
	return sqrtf(dx * dx + dz * dz);
}

/* 0x00105fb0  Demolition (gap)
Distance from `pos` to the target player (0 without a target). */
float DemoAI_TargetDist(DemoAI *self, float *pos)
{
	char *target;
	float dx;
	float dy;
	float dz;

	target = self->target;
	if (target == NULL)
		return 0.0f;
	dx = DD_F32(target, DP_POS) - pos[0];
	dy = DD_F32(target, DP_POS + 4) - pos[1];
	dz = DD_F32(target, DP_POS + 8) - pos[2];
	return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* 0x00106000  Demolition (gap)
Turns the player towards the goal (at most dt * turnRate) and pushes the stick forwards.
Returns 1 (without pushing) once the goal is within arriveDist. */
int DemoAI_Steer(DemoAI *self, float dt, float turnRate)
{
	char *owner;
	float dx;
	float dz;
	float dir[4];
	float stick[4];

	owner = AI_OWNER(self);
	self->hasGoal = 0;
	dx = self->goal[0] - DD_F32(owner, DP_POS);
	dz = self->goal[2] - DD_F32(owner, DP_POS + 8);
	AI_HEADING(self) = AI_TurnTowards(DD_F32(DD_PTR(owner, DP_ACTOR), 0x1d8), atan2f(dx, dz), dt * turnRate);
	if (sqrtf(dz * dz + dx * dx) < self->arriveDist)
		return 1;
	dir[0] = 0.0f;
	dir[1] = 0.0f;
	dir[2] = 1.0f;
	dir[3] = 0.0f;
	FUN_00051170(stick, dir, AI_HEADING(self));
	AI_SetStick(AI_OWNER(self), stick, 1.0f);
	return 0;
}

/* a player the brain may choose: alive, not itself, not its team mate */
static int ai_is_enemy(DemoAI *self, char *player)
{
	return player != AI_OWNER(self) && DD_U8(player, DP_ALIVE) != 0 &&
	       DD_PTR(AI_OWNER(self), DP_PARTNER) != player;
}

/* 0x001060e0  Demolition (gap)
Chooses a random enemy (0 when there is none). */
char *DemoAI_PickRandom(DemoAI *self)
{
	char *player;
	int count;
	int pick;
	int i;

	count = 0;
	player = AI_FIRST_PLAYER();
	if (player == NULL)
		return NULL;
	for (; player != NULL; player = AI_NEXT_PLAYER(player)) {
		if (ai_is_enemy(self, player))
			count++;
	}
	if (count == 0)
		return NULL;
	pick = dd_rand() % count;
	i = 0;
	for (player = AI_FIRST_PLAYER(); player != NULL; player = AI_NEXT_PLAYER(player)) {
		if (ai_is_enemy(self, player)) {
			if (pick <= i)
				return player;
			i++;
		}
	}
	return self->target;
}

/* 0x00106180  Demolition (gap)
Chooses the enemy with the best score (the last of equals; a tie draws a rand() that the
original compares with 0.5 and then ignores). Without an enemy: the first player. */
char *DemoAI_PickLeader(DemoAI *self)
{
	char *player;
	char *best;
	int bestScore;
	int score;

	bestScore = 0;
	player = AI_FIRST_PLAYER();
	best = player;
	for (; player != NULL; player = AI_NEXT_PLAYER(player)) {
		if (!ai_is_enemy(self, player))
			continue;
		score = DD_I32(player, DP_SCORE);
		if (score < bestScore)
			continue;
		if (score == bestScore)
			(void)dd_rand();
		bestScore = score;
		best = player;
	}
	return best;
}

/* 0x00106210  Demolition (gap)
Chooses the enemy nearest to `pos` (the first player when there is none). */
char *DemoAI_PickNearest(DemoAI *self, float *pos)
{
	char *player;
	char *best;
	float bestDist;
	float dx;
	float dy;
	float dz;
	float dist;

	player = AI_FIRST_PLAYER();
	bestDist = 99999.0f;
	best = player;
	for (; player != NULL; player = AI_NEXT_PLAYER(player)) {
		if (!ai_is_enemy(self, player))
			continue;
		dx = pos[0] - DD_F32(player, DP_POS);
		dy = pos[1] - DD_F32(player, DP_POS + 4);
		dz = pos[2] - DD_F32(player, DP_POS + 8);
		dist = sqrtf(dx * dx + dy * dy + dz * dz);
		if (dist < bestDist) {
			best = player;
			bestDist = dist;
		}
	}
	return best;
}

/* 0x001062a0  Demolition (gap)
Chooses the enemy that fragged the player most (statistics +0x3c by index; ties as in
DemoAI_PickLeader). */
char *DemoAI_PickNemesis(DemoAI *self)
{
	char *player;
	char *best;
	int bestCount;
	int count;

	bestCount = 0;
	player = AI_FIRST_PLAYER();
	best = player;
	for (; player != NULL; player = AI_NEXT_PLAYER(player)) {
		if (!ai_is_enemy(self, player))
			continue;
		count = DD_I32(DD_PTR(AI_OWNER(self), DP_STATS), 0x3c + DD_I32(player, DP_INDEX) * 4);
		if (count < bestCount)
			continue;
		if (count == bestCount)
			(void)dd_rand();
		bestCount = count;
		best = player;
	}
	return best;
}

/* 0x00105aa0  Demolition (gap)
Slot 6 of the variation 2 brain: a player went away; stop attacking it. */
void DemoAI2_OnPlayerGone(DemoAI *self, char *player)
{
	if (self->state == 3 && player == self->target)
		self->state = 0;
}
