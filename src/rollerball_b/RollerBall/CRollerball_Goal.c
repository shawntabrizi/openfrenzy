/*
RollerBall/CRollerball_Goal.c  (module rollerball_b)

The rest of C:\BlitzParty\Game\Src\RollerBall\CRollerball_Goal.cpp (rollerball_a has the
start of the file, up to 0x00191530): the goal system of the "base" variation.

Goal system (at game + 0x1470, offsets RBGS_* in rollerball_b.h): 9 goals on the arena wall, of
which the system lights a random one now and then (state 1, "goalactivate"), and switches
the oldest lit one off again (state 2, "deactivate") so that the number of lit goals
follows the current pattern (+0x40, 0-2). A ball that rolls into a lit goal scores
(state 4): a flare, a particle burst in the player's colour and a push for the balls near
it. The functions here have no assert tag; their place in the file is inferred from the
address order (between 0x00191530, tagged CRollerball_Goal.cpp, and CRollerball_Entity.cpp).
*/

#include "rollerball_b.h"

/* rollerball_a 0x00190b70 (an animation callback that is not in the function table; a copy):
the goal's "goal scored" animation has ended */
static int rbb_GoalScoreAnimDone(void *inst, void *node, void *anim, int event, void *goal)
{
	(void)inst;
	(void)node;
	(void)anim;
	(void)event;
	RBB_B(goal, RBG_ANIM1_ON) = 0;
	return 1;
}

/* 0x001915c0  RollerBall/CRollerball_Goal.cpp (inferred)
Pick a random pattern, start the pattern timer (3-8 s) and reset every goal. */
void CRollerballGoalSystem_ResetGoals(void *self)
{
	char *goal;
	int i;

	RBB_I(self, RBGS_ACTIVE) = 1;
	RBB_I(self, RBGS_PATTERN) = rbb_rand() % 3;
	rbb_TimerSetDuration(RBB_ADDR(self, RBGS_TIMER), RBB_RANDF() * 5.0f + 3.0f);
	rbb_TimerStart(RBB_ADDR(self, RBGS_TIMER));
	RBB_F(self, RBGS_ACTIVATE_WAIT) = 0.0f;
	RBB_F(self, RBGS_DEACTIVATE_WAIT) = 0.0f;
	for (goal = RBB_P(self, RBGS_GOAL_LIST); goal != NULL; goal = RBB_P(goal, RBG_NEXT)) {
		RBB_S(goal, RBG_STATE) = RBG_STATE_IDLE;
		RBB_F(goal, RBG_RADIUS) = 80.0f;
		RBB_S(goal, RBG_ENABLED) = 1;
		RBB_I(goal, RBG_FADE) = 0;
		for (i = 0; i < 2; i++)
			RBB_B(goal, RBG_ANIM0_ON + i * 0x18) = 0;
	}
}

/* 0x00191690  RollerBall/CRollerball_Goal.cpp (inferred)
Start a flare at `pos` in colour (r, g, b): the first free one of the 3 flares, else the
one that has run longest. (If all are busy and none has run 0 s or more, the original
passes a null flare.) */
void CRollerballGoalSystem_StartFlare(void *self, float *pos, int r, int g, int b)
{
	char *flare = NULL;
	float oldest = 0.0f;
	int i;

	for (i = 0; i < 3; i++) {
		char *slot = (char *)self + RBGS_FLARES + i * RBGS_FLARE_SIZE;
		if (RBB_C(slot, 0x4e) == 0) {
			flare = slot;
			break;
		}
		if (oldest <= RBB_F(slot, 0x60)) {
			flare = slot;
			oldest = RBB_F(slot, 0x60);
		}
	}
	/* rollerball_a 0x00191110: the flare's start */
	RBB_FN(FUN_00191110, void, (void *, float *, int, int, int))(flare, pos, r, g, b);
}

/* 0x001916e0  RollerBall/CRollerball_Goal.cpp (inferred)
Start a particle burst at `pos` in colour (r, g, b): the first free one of the 8 bursts,
else the oldest (null in the same corner case as above). */
void CRollerballGoalSystem_StartBurst(void *self, float *pos, short r, short g, short b)
{
	char *burst = NULL;
	float oldest = 0.0f;
	int i;

	for (i = 0; i < 8; i++) {
		char *slot = (char *)self + RBGS_BURSTS + i * RBGS_BURST_SIZE;
		if (RBB_C(slot, 0x14) == 0) {
			burst = slot;
			break;
		}
		if (oldest <= RBB_F(slot, 0x18)) {
			burst = slot;
			oldest = RBB_F(slot, 0x18);
		}
	}
	RBB_F(burst, 0x04) = pos[0];
	RBB_F(burst, 0x08) = pos[1];
	RBB_F(burst, 0x0c) = pos[2];
	RBB_F(burst, 0x10) = pos[3];
	CParticleSystem_SetPosition((struct CPSHeader *)(burst + 0x1c), pos);
	CParticleSystem_Start((struct CParticleSystem *)(burst + 0x1c), r, g, b);
	RBB_B(burst, 0x6c) = 1;   /* the particle system is alive */
	RBB_B(burst, 0x14) = 1;
	RBB_B(burst, 0x15) = 1;
	RBB_F(burst, 0x18) = 0.0f;
}

/* 0x00191780  RollerBall/CRollerball_Goal.cpp (inferred)
Goal state 1: fade the light in; then the goal is lit (state 3). */
void CRollerballGoal_UpdateActivating(void *self)
{
	int fade = RBB_I(self, RBG_FADE) + 0x200;

	RBB_I(self, RBG_FADE) = fade;
	if (fade > 0x7fff)
		RBB_S(self, RBG_STATE) = RBG_STATE_LIT;
	/* rollerball_a 0x00191420: the goal's light colour */
	RBB_FN(FUN_00191420, void, (void *))(self);
}

/* 0x001917a0  RollerBall/CRollerball_Goal.cpp (inferred)
Goal state 2: fade the light out; then the goal is idle. */
void CRollerballGoal_UpdateDeactivating(void *self)
{
	int fade = RBB_I(self, RBG_FADE) - 0x200;

	RBB_I(self, RBG_FADE) = fade;
	if (fade < 1) {
		RBB_S(self, RBG_STATE) = RBG_STATE_IDLE;
		RBB_B(self, RBG_ANIM0_ON) = 0;
		return;
	}
	RBB_FN(FUN_00191420, void, (void *))(self);
}

/* 0x001917c0  RollerBall/CRollerball_Goal.cpp (inferred)
Goal state 4: the ball of player entity +0x08 rolled into the lit goal. Play the "scored"
animation, push the ball out, start a flare and a burst in the player's colour, add a
point (with the crowd cheer when the scorer takes or shares the lead) and push every
other ball within 1000 units away from the goal. */
void CRollerballGoal_Score(void *self)
{
	char *players = rbb_FirstPlayer;
	void *inst = RBB_P(self, RBG_ANIM1_INST);
	void *anim = RBB_P(self, RBG_SCORE_ANIM);
	char *scorer;
	char *p;
	int best;
	int score;
	float push[4];
	float flare_pos[4];
	float dir[3];
	float dist2;

	baFlushAnimations(inst, RBB_P(inst, 0x134), 1, 0);
	baQueueAnimation((struct BActorInstance *)inst, (struct BActorNode *)RBB_P(inst, 0x134),
		(struct BAnim *)anim, 0x400, -1, 0, (int (*)())rbb_GoalScoreAnimDone, self);
	scorer = RBB_P(self, RBG_PLAYER);
	RBB_B(self, RBG_ANIM1_ON) = 1;
	RBB_I(self, RBG_COLOUR + 0) = RBB_I(scorer, RBE_COLOUR + 0);
	RBB_I(self, RBG_COLOUR + 4) = RBB_I(scorer, RBE_COLOUR + 4);
	RBB_I(self, RBG_COLOUR + 8) = RBB_I(scorer, RBE_COLOUR + 8);

	push[0] = 0.0f;
	push[1] = 120.0f;
	push[2] = 0.0f;
	push[3] = 0.0f;   /* the original leaves w uninitialised (SetPush copies 4 floats) */
	CRollerballEntity_SetPush(scorer, push);

	flare_pos[0] = RBB_F(self, RBG_POS + 0);
	flare_pos[1] = RBB_F(self, RBG_POS + 4) + 80.0f;
	flare_pos[2] = RBB_F(self, RBG_POS + 8);
	flare_pos[3] = RBB_F(self, RBG_POS + 12);
	scorer = RBB_P(self, RBG_PLAYER);
	CRollerballGoalSystem_StartFlare(rbb_GoalSystem, flare_pos, RBB_U(scorer, RBE_COLOUR + 0) >> 1,
		RBB_U(scorer, RBE_COLOUR + 4) >> 1, RBB_U(scorer, RBE_COLOUR + 8) >> 1);
	scorer = RBB_P(self, RBG_PLAYER);
	CRollerballGoalSystem_StartBurst(rbb_GoalSystem, RBB_V(self, RBG_POS),
		(short)RBB_I(scorer, RBE_COLOUR + 0), (short)RBB_I(scorer, RBE_COLOUR + 4),
		(short)RBB_I(scorer, RBE_COLOUR + 8));
	rbb_PlaySound(RBB_SND_GOALSCORE, 0x7f, 0, 0, 0);

	best = 0;
	for (p = players; p != NULL; p = RBB_P(p, RBE_NEXT))
		if (best < RBB_I(p, RBE_SCORE))
			best = RBB_I(p, RBE_SCORE);
	score = RBB_I(RBB_P(self, RBG_PLAYER), RBE_SCORE);
	if (score <= best && (score + 1 == best || best == 0))
		rbb_PlaySound(RBB_SND_CROWDCHEER, 0xff, 0, 0, 0);

	scorer = RBB_P(self, RBG_PLAYER);
	CLSamples_PlayFromSet(RBB_C(scorer, RBE_CHARACTER), 4, RBB_V(scorer, RBE_POS), DAT_00293928,
		0, -1, 0);
	RBB_I(RBB_P(self, RBG_PLAYER), RBE_SCORE) += 1;

	for (p = players; p != NULL; p = RBB_P(p, RBE_NEXT)) {
		float d[3];
		if (p == RBB_P(self, RBG_PLAYER))
			continue;
		d[0] = RBB_F(p, RBE_POS + 0) - RBB_F(self, RBG_POS + 0);
		d[1] = RBB_F(p, RBE_POS + 4) - RBB_F(self, RBG_POS + 4);
		d[2] = RBB_F(p, RBE_POS + 8) - RBB_F(self, RBG_POS + 8);
		dist2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
		if (dist2 <= 1000000.0f) {
			float strength;
			bmVectorNormalise(dir, d);
			strength = 30.0f - sqrtf(dist2) * 30.0f * 0.001f;
			push[0] = dir[0] * strength;
			push[1] = dir[1] * strength;
			push[2] = dir[2] * strength;
			CRollerballEntity_SetPush(p, push);
		}
	}
	RBB_S(self, RBG_STATE) = RBG_STATE_IDLE;
	RBB_B(self, RBG_ANIM0_ON) = 0;
}

/* 0x00191a50  RollerBall/CRollerball_Goal.cpp (inferred)
Load the goal models (only in the "base" variation; others have no goals) and set up
the 3 flares, the 8 bursts and the goals. */
int CRollerballGoalSystem_Init(void *self, struct bkPackage *package)
{
	static const unsigned int model_table = 0x002b5084;   /* "goalsign.obe", "goalsfx.obe" */
	char name[256];
	int i;

	RBB_P(self, RBGS_GOAL_LIST) = NULL;
	RBB_I(self, RBGS_GOAL_COUNT) = 0;
	if (rbb_Variation != 0)
		return 1;
	rbb_TimerInit(RBB_ADDR(self, RBGS_TIMER));
	for (i = 0; i < 2; i++) {
		struct bkResource *res;
		sprintf(name, "models\\%s", RBD_SV(model_table)[i]);
		res = bLoadResource(package, name, 1, 0xdefa);
		RBB_P(self, RBGS_MODELS + i * 4) = res;
		if (res == NULL)
			return 0;
	}
	for (i = 0; i < 3; i++)   /* rollerball_a 0x00191060: a flare's init */
		RBB_FN(FUN_00191060, int, (void *))((char *)self + RBGS_FLARES + i * RBGS_FLARE_SIZE);
	for (i = 0; i < 8; i++)   /* rollerball_a 0x001911e0: a burst's init */
		RBB_FN(FUN_001911e0, int, (void *))((char *)self + RBGS_BURSTS + i * RBGS_BURST_SIZE);
	CRollerballGoalSystem_ResetGoals(self);
	return 1;
}

/* 0x00191a7b  RollerBall/CRollerball_Goal.cpp (inferred)
Not a function: Ghidra split the tail of 0x00191a50 (from the CDemoCountdown_Init call)
into its own entry. Nothing calls this address; the code is in CRollerballGoalSystem_Init
above. Defined so that the table entry has a body: it returns 0 ("failed"). */
int FUN_00191a7b(void)
{
	return 0;
}

/* 0x00191b30  RollerBall/CRollerball_Goal.cpp (inferred)
Per-frame update of one goal: its state, then its two animated actors. */
void CRollerballGoal_Update(void *self, float dt)
{
	int i;

	RBB_F(self, RBG_STATE_TIME) = dt + RBB_F(self, RBG_STATE_TIME);
	if (RBB_S(self, RBG_ENABLED) != 0) {
		switch (RBB_S(self, RBG_STATE)) {
		case RBG_STATE_ACTIVATING:
			CRollerballGoal_UpdateActivating(self);
			break;
		case RBG_STATE_DEACTIVATING:
			CRollerballGoal_UpdateDeactivating(self);
			break;
		case RBG_STATE_LIT:
			RBB_FN(FUN_00191420, void, (void *))(self);
			break;
		case RBG_STATE_SCORED:
			CRollerballGoal_Score(self);
			break;
		}
	}
	for (i = 0; i < 2; i++) {
		void *inst = RBB_P(self, RBG_ANIM0_INST + i * 0x18);
		if (RBB_C(self, RBG_ANIM0_ON + i * 0x18) != 0) {
			baUpdateAnimations((struct BActorInstance *)inst, (struct BActorNode *)RBB_P(inst, 0x134),
				dt, NULL);
			baPrepActorInstance((struct BActorInstance *)inst, 1);
		}
	}
}

/* the goal pointers +0x14..+0x34 */
#define RBGS_GOAL(self, i) ((char *)RBB_P(self, RBGS_GOALS + (i) * 4))

/* 0x00191bd0  RollerBall/CRollerball_Goal.cpp (inferred)
Per-frame update of the goal system ("base" variation only): flares, bursts, the
pattern (how many goals should be lit), lighting and switching off goals, and the
goals. When the game clock has run out every goal goes dark. */
void CRollerballGoalSystem_Update(void *self, float dt)
{
	int i;
	int wanted;
	unsigned int lit;

	if (rbb_Variation != 0)
		return;
	if (RBB_I(self, RBGS_GOAL_COUNT) < 1)
		return;

	for (i = 0; i < 3; i++)   /* rollerball_a 0x00190bb0: a flare's update */
		RBB_FN(FUN_00190bb0, void, (void *, float))((char *)self + RBGS_FLARES + i * RBGS_FLARE_SIZE, dt);
	for (i = 0; i < 8; i++) {
		char *slot = (char *)self + RBGS_BURSTS + i * RBGS_BURST_SIZE;
		if (RBB_C(slot, 0x14) != 0) {
			void *ps = slot + 0x1c;
			RBB_F(slot, 0x18) = dt + RBB_F(slot, 0x18);
			RBB_VFN(ps, 0x04, void (*)(void *, float))(ps, dt);
			if (RBB_C(slot, 0x6c) == 0) {
				RBB_B(slot, 0x14) = 0;
				RBB_B(slot, 0x15) = 0;
			}
		}
	}

	RBB_F(self, RBGS_ACTIVATE_WAIT) = RBB_F(self, RBGS_ACTIVATE_WAIT) - dt;
	if (RBB_F(self, RBGS_ACTIVATE_WAIT) < 0.0f)
		RBB_F(self, RBGS_ACTIVATE_WAIT) = 0.0f;
	RBB_F(self, RBGS_DEACTIVATE_WAIT) = RBB_F(self, RBGS_DEACTIVATE_WAIT) - dt;
	if (RBB_F(self, RBGS_DEACTIVATE_WAIT) < 0.0f)
		RBB_F(self, RBGS_DEACTIVATE_WAIT) = 0.0f;

	if (RBB_I(self, RBGS_ACTIVE) == 0)
		goto update_goals;

	/* game + 0x26b0: the game clock (rollerball_a) */
	if (rbb_TimerRemaining((char *)rbb_Game + 0x26b0) <= 0.0f) {
		/* time is up: every goal goes dark */
		RBB_I(self, RBGS_ACTIVE) = 0;
		for (i = 0; i < 9; i++) {
			char *goal = RBGS_GOAL(self, i);
			if (RBB_S(goal, RBG_STATE) != RBG_STATE_IDLE) {
				RBB_S(goal, RBG_STATE) = RBG_STATE_IDLE;
				RBB_B(goal, RBG_ANIM0_ON) = 0;
			}
			RBB_S(goal, RBG_ENABLED) = 0;
		}
		return;
	}

	wanted = RBB_I(self, RBGS_PATTERN) + 1;
	lit = 0;
	for (i = 0; i < 9; i++)
		if (RBB_S(RBGS_GOAL(self, i), RBG_STATE) != RBG_STATE_IDLE)
			lit++;
	if (lit == 0)
		RBB_F(self, RBGS_ACTIVATE_WAIT) = 0.0f;

	if (wanted < (int)lit) {
		/* too many: switch off the goal that has been lit longest */
		if (RBB_F(self, RBGS_DEACTIVATE_WAIT) <= 0.0f) {
			int pick = 0;
			float longest = 0.0f;
			char *goal;
			for (i = 0; i < 9; i++) {
				goal = RBGS_GOAL(self, i);
				if (RBB_S(goal, RBG_STATE) == RBG_STATE_LIT && longest < RBB_F(goal, RBG_STATE_TIME)) {
					pick = i;
					longest = RBB_F(goal, RBG_STATE_TIME);
				}
			}
			goal = RBGS_GOAL(self, pick);
			RBB_S(goal, RBG_STATE) = RBG_STATE_DEACTIVATING;
			RBB_F(goal, RBG_STATE_TIME) = 0.0f;
			RBB_F(self, RBGS_DEACTIVATE_WAIT) = 120.0f;
			rbb_PlaySound(RBB_SND_DEACTIVATE, 0xff, 0, 0, 0);
		}
	} else if ((int)lit < wanted && RBB_F(self, RBGS_ACTIVATE_WAIT) <= 0.0f) {
		/* too few: light a random goal that has been idle for more than 500 frames (else
		the last such goal in order, else any random idle goal) */
		int pick = -1;
		char *goal;
		for (i = 0; i < 9; i++) {
			int r;
			goal = RBGS_GOAL(self, i);
			if (RBB_S(goal, RBG_STATE) == RBG_STATE_IDLE && 500.0f < RBB_F(goal, RBG_STATE_TIME))
				pick = i;
			r = rbb_rand() % 9;
			goal = RBGS_GOAL(self, r);
			if (RBB_S(goal, RBG_STATE) == RBG_STATE_IDLE && 500.0f < RBB_F(goal, RBG_STATE_TIME)) {
				pick = r;
				break;
			}
		}
		if (pick == -1) {
			short state;
			do {
				pick = rbb_rand() % 9;
				state = RBB_S(RBGS_GOAL(self, pick), RBG_STATE);
			} while (state == RBG_STATE_LIT || state == RBG_STATE_ACTIVATING ||
				state == RBG_STATE_DEACTIVATING);
		}
		goal = RBGS_GOAL(self, pick);
		RBB_B(goal, RBG_ANIM0_ON) = 1;
		RBB_S(goal, RBG_STATE) = RBG_STATE_ACTIVATING;
		RBB_F(goal, RBG_STATE_TIME) = 0.0f;
		RBB_F(self, RBGS_ACTIVATE_WAIT) = 120.0f;
		rbb_PlaySound(RBB_SND_GOALACTIVATE, 0x7f, 0, 0, 0);
	}

	/* a new random pattern every 3-8 seconds */
	rbb_TimerUpdate(RBB_ADDR(self, RBGS_TIMER));
	if (1.0f <= rbb_TimerProgress(RBB_ADDR(self, RBGS_TIMER))) {
		int pattern;
		do
			pattern = rbb_rand() % 3;
		while (pattern == RBB_I(self, RBGS_PATTERN));
		RBB_I(self, RBGS_PATTERN) = pattern;
		rbb_TimerSetDuration(RBB_ADDR(self, RBGS_TIMER), RBB_RANDF() * 5.0f + 3.0f);
		rbb_TimerStart(RBB_ADDR(self, RBGS_TIMER));
	}

update_goals:
	{
		char *goal;
		for (goal = RBB_P(self, RBGS_GOAL_LIST); goal != NULL; goal = RBB_P(goal, RBG_NEXT))
			CRollerballGoal_Update(goal, dt);
	}
}
