/*
MouseWhack/unplaced.c  (module mousewhack)

Gap functions of C:\BlitzParty\Game\Src\MouseWhack\ whose source file is not known: they
lie between CBbash_Game.cpp and CBbash_Player.cpp (0x00176100-0x0017d170), so they come
from another file of the directory.

  0x00176100-0x00176230  the frame timer of the arena (vtable 0x00272d64)
*/

#include "mousewhack.h"

/* bkernel: timestamp ticks per second */
extern longlong DAT_002cd7a0;

/* the frame timer (0xa0 bytes):
  +0x80 longlong ticks per second, +0x88 ulonglong now, +0x90 ulonglong the previous frame,
  +0x98 float milliseconds of the last frame, +0x9c float the same x 0.06 (frames at 60 Hz) */

/* 0x00272d64  the frame timer vtable */
const void *const vtable_00272d64[1] = {
	(const void *)FUN_00176230, /* [0] scalar deleting destructor */
};

/* 0x00176100  (gap): constructor */
void *MW_Timer_Ctor(void *self)
{
	*(const void *const **)self = vtable_00272d64;
	MW_AI(self, 0x80) = 0;
	MW_AI(self, 0x84) = 0;
	MW_AI(self, 0x88) = 0;
	MW_AI(self, 0x8c) = 0;
	MW_AI(self, 0x90) = 0;
	MW_AI(self, 0x94) = 0;
	MW_AI(self, 0x98) = 0;
	MW_AI(self, 0x9c) = 0;
	return self;
}

/* 0x00176140  (gap): destructor */
void MW_Timer_Dtor(void *self)
{
	*(const void *const **)self = vtable_00272d64;
}

/* 0x00176150  (gap): reset: reads the frequency and the time now */
int MW_Timer_Reset(void *self)
{
	ulonglong now;

	MW_AT(self, 0x80, longlong) = DAT_002cd7a0;
	now = bkTimerRead();
	MW_AT(self, 0x88, ulonglong) = now;
	MW_AT(self, 0x90, ulonglong) = now;
	return 1;
}

/* 0x001761a0  (gap): a frame: the time since the previous frame in milliseconds */
void MW_Timer_Update(void *self)
{
	ulonglong delta;
	double ms;

	/* _clearfp() (0x001e0a24): clears the x87 exception flags; nothing to do in the port */
	MW_AT(self, 0x90, ulonglong) = MW_AT(self, 0x88, ulonglong);
	MW_AT(self, 0x88, ulonglong) = bkTimerRead();
	delta = bkTimerDelta(MW_AT(self, 0x90, ulonglong), MW_AT(self, 0x88, ulonglong));
	ms = (double)(longlong)(delta * 1000u) / (double)MW_AT(self, 0x80, longlong);
	MW_AF(self, 0x98) = (float)ms;
	MW_AF(self, 0x9c) = (float)(ms * (double)0.06f);
}

/* 0x00176230  (gap): scalar deleting destructor */
void *MW_Timer_Destroy(void *self, unsigned char flags)
{
	*(const void *const **)self = vtable_00272d64;
	if ((flags & 1) != 0)
		MW_DELETE(self);
	return self;
}

/* ======================================================================== the bugs' behaviour

0x00176250-0x0017ce20: the states of each bug type (the bug object: CBbash_Bugs.c). Bug
types: 0 worm, 1 roach, 2 tremor, 3 fly, 4 spore, 5 bad worm, 6 bad roach, 7 tremor
rubble, 8 the train ("Tremors"), 10 a worm of "Tremors". The update (0x00181830) calls the
state handler of the type: +0x11ee is the state, +0x11f0 the frames in it.

Bug fields used here (byte offsets): +0x00 actor, +0x04 platform (train), +0x10 route,
+0x1008 the node the extra instance hangs from, +0x1070 the extra instance (spore cloud),
+0x11a4 its node, +0x11e0 its animation, +0x11ec type, +0x11ee state, +0x11f0 state time,
+0x11fa alive, +0x11fc splat now, +0x1200/+0x1202 moving/frames stuck, +0x1208 time,
+0x1214 short, +0x1218 float[4] target, +0x1228 bob phase, +0x1230 height above ground,
+0x1234 lean, +0x1238/+0x123c short kick counters, +0x1240 target point, +0x1242 slot,
+0x1250 time to the target, +0x126c speed, +0x1274 wanted heading, +0x1278 heading,
+0x127c float[4] velocity (saved).
*/

#define BG_S(off) MW_AS(self, off)
#define BG_I(off) MW_AI(self, off)
#define BG_F(off) MW_AF(self, off)
#define BG_V(off) MW_AV(self, off)
#define BG_ACTOR ((unsigned char *)MW_AP(self, 0x0))
#define BA_F(off) MW_AF(BG_ACTOR, off)
#define BA_I(off) MW_AI(BG_ACTOR, off)
#define BA_V(off) MW_AV(BG_ACTOR, off)

/* a random number 0..32767 as the game's inlined rand() */
#define BG_RAND() mw_rand()

/* the lean angle (+0x1234) halves (fly) or follows the turn (spore), clamped to 45
degrees and given to the actor as a roll around its heading */
static void bg_ApplyLean(void *self)
{
	float v[4];

	if (0.7853981f < BG_F(0x1234))
		BG_F(0x1234) = 0.7853981f;
	if (BG_F(0x1234) < -0.7853981f)
		BG_F(0x1234) = -0.7853981f;
	v[2] = BG_F(0x1234);
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[3] = 0.0f;
	bmVectorRotateY(v, v, BA_F(0x1e8));
	BA_F(0x1ec) = v[2];
	BA_F(0x1e4) = v[0];
}

/* the extra instance (+0x1070) follows the node +0x1008 of the actor */
static void bg_PlaceExtraInstance(void *self)
{
	float matrix[16];
	char *inst = (char *)self + 0x1070;

	MW_AI(inst, 0xc8) = 0;
	MW_AI(inst, 0xc4) = 0;
	MW_AI(inst, 0xc0) = 0;
	mw_MatrixMultiply(matrix, (float *)((char *)MW_AP(self, 0x1008) + 0x80), (float *)inst);
	mw_MatrixMultiply((float *)inst, (float *)(BG_ACTOR + 0x10), matrix);
}

/* 0x00176250  (gap): fly, dying: falls, slows down, shrinks after 30 frames, dies (and
splats) after 60; the lean dies away */
void Bug_Fly_Die(void *self)
{
	if (BG_F(0x11f0) == 0.0f)
		Actor_PlayAnim(BG_ACTOR, 5, 0, 0, 1.0f, 0.0f, 1);
	BA_F(0x198) = -(DAT_0028fda8 * 10.0f);
	if (20.0f <= BG_F(0x11f0)) {
		BA_F(0x194) = BA_F(0x194) * 0.8f;
		BA_F(0x19c) = BA_F(0x19c) * 0.8f;
	}
	if (30.0f <= BG_F(0x11f0))
		BA_F(0xe4) = BA_F(0xe4) - DAT_0028fda8 * 0.033333335f;
	if (60.0f <= BG_F(0x11f0)) {
		BG_S(0x11fa) = 0;
		BG_S(0x11fc) = 1;
	}
	BG_F(0x1234) = BG_F(0x1234) * 0.5f;
	bg_ApplyLean(self);
}

/* the rubble/landing points of a slot (+0x1242): "Brawl" 0x002b2d48, "Tremors" 0x002b2da8 */
static void bg_PlaceAtSlot(void *self, int markUsed)
{
	float *at;

	if (mw_Variation == 0)
		at = MW_FV(0x002b2d48 + BG_S(0x1242) * 0x10);
	else if (mw_Variation == 1)
		at = MW_FV(0x002b2da8 + BG_S(0x1242) * 0x10);
	else
		return;
	BA_F(0xd0) = at[0];
	BA_F(0xd4) = at[1];
	BA_F(0xd8) = at[2];
	if (markUsed)
		MW_S(0x0030ebf8 + BG_S(0x1242) * 2) = 1;
}

/* 0x001763d0  (gap): tremor rubble (type 7): stays on its slot and marks it used */
void Bug_Rubble_Update(void *self)
{
	BA_F(0xdc) = 1.0f;
	bg_PlaceAtSlot(self, 1);
	BA_F(0x198) = 0.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
	if (BG_F(0x11f0) == 0.0f) {
		Actor_PlayAnim(BG_ACTOR, 0, 0, 0, 1.0f, 0.0f, 1);
		Actor_PlayAnim(BG_ACTOR, 1, 0, 1, 1.0f, 0.1f, 1);
		BA_F(0x1e8) = 3.1415925f;
	}
	BG_F(0x11f0) = BG_F(0x11f0) + 1.0f;
}

/* 0x00176540  (gap): tremor, state 6: back in its hole (anims 3, 4, 1), held on its slot */
void Bug_Tremor_InHole(void *self)
{
	if (BG_F(0x11f0) == 0.0f) {
		Actor_PlayAnim(BG_ACTOR, 3, 0, 0, 1.0f, 0.1f, 1);
		Actor_PlayAnim(BG_ACTOR, 4, 0, 1, 1.0f, 0.1f, 1);
		Actor_PlayAnim(BG_ACTOR, 1, 1, 1, 1.0f, 0.1f, 1);
	}
	bg_PlaceAtSlot(self, 0);
	BA_F(0x198) = -1.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
}

/* 0x00176670  (gap): reachedPoint: roaches (patrol point +0x1240 of the arena's table,
within 100) and flies/spores (target +0x1218, within 100 in x/z); -1 for other types */
int Bug_ReachedPoint(void *self)
{
	float dx, dz;
	short type, point;

	type = BG_S(0x11ec);
	if (type == 1 || type == 6) {
		point = BG_S(0x1240);
		if (point < 1 || 0xe < point) {
			bkPrintf("bug heading to invalid point \n");
			return 0;
		}
		if (mw_Variation == 2)
			return bmVectorDistance(MW_FV(0x002b2b88 + point * 0x10), BA_V(0xd0)) < 100.0f;
		return bmVectorDistance(MW_FV(0x002b2a98 + point * 0x10), BA_V(0xd0)) < 100.0f;
	}
	if (type != 3 && type != 4) {
		if (mw_DbgMessages != 0)
			bkPrintf("Error in reachedpoint \n");
		return -1;
	}
	dx = BA_F(0xd0) - BG_F(0x1218);
	dz = BA_F(0xd8) - BG_F(0x1220);
	return dx * dx + dz * dz < 10000.0f;
}

/* 0x00176770  (gap): roach, state 3: dead, still; gone after 20 frames */
void Bug_Roach_Dead(void *self)
{
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_I(0x220) = 0;
	BA_F(0x198) = 0.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
	if (20.0f <= BG_F(0x11f0))
		BG_S(0x11fa) = 0;
}

/* 0x001767d0  (gap): worm, state 3: dead, still; gone after 20 frames */
void Bug_Worm_Dead(void *self)
{
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_F(0x198) = 0.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
	BA_I(0x220) = 0;
	if (20.0f <= BG_F(0x11f0))
		BG_S(0x11fa) = 0;
}

/* 0x00176830  (gap): spore, state 0: appears at a random point 2000 up with its cloud,
picks a target and a speed; returns the last random number / 20 */
unsigned int Bug_Spore_Start(void *self)
{
	int x, z;
	unsigned int r;

	BG_F(0x1278) = 3.1415925f;
	BG_F(0x1274) = 3.1415925f;
	x = BG_RAND() % 2000 - 1000;
	z = BG_RAND() % 2000 - 1000;
	BG_F(0x1230) = 2000.0f - mw_GroundProbe(BA_V(0xd0), 0.0f);
	Actor_PlayAnim(BG_ACTOR, 0, 1, 0, 1.0f, 0.0f, 1);
	baFlushAnimations((char *)self + 0x1070, (struct BActorNode *)MW_AP(self, 0x11a4), 1, 0);
	baQueueAnimation((struct BActorInstance *)((char *)self + 0x1070), (struct BActorNode *)MW_AP(self, 0x11a4),
		(struct BAnim *)MW_AP(self, 0x11e0), 0x400, -1, 1, NULL, NULL);
	BA_F(0xd0) = (float)x;
	BA_F(0xd4) = 2000.0f;
	BA_F(0xd8) = (float)z;
	BA_F(0xdc) = 1.0f;
	baPrepActorInstance((struct BActorInstance *)((char *)self + 0x1070), 1);
	bg_PlaceExtraInstance(self);
	BA_F(0x1e8) = BG_F(0x1278) + 1.5707963f;
	BG_S(0x11ee) = 1;
	BG_F(0x1218) = (float)(BG_RAND() % 0xaf0 - 0x578);
	r = (unsigned int)BG_RAND();
	BG_I(0x121c) = 0;
	BG_I(0x11f0) = 0;
	BG_S(0x1202) = 0;
	BG_I(0x1250) = 0;
	BG_F(0x1220) = (float)((int)r % 0xaf0 - 0x578);
	r = (unsigned int)BG_RAND();
	BG_F(0x126c) = (float)(r % 0x14) * 0.1f + 1.0f;
	return r / 0x14;
}

/* turns the heading (+0x1278) towards `wanted` by at most +-`rate` a frame; returns the
turn */
static float bg_TurnTowards(void *self, float wanted, float rate)
{
	float turn;

	turn = wanted - BG_F(0x1278);
	if (turn < 0.0f)
		turn = turn + 6.2831855f;
	if (6.2831855f < turn)
		turn = turn - 6.2831855f;
	if (3.1415925f < turn)
		turn = turn - 6.2831855f;
	if (DAT_0028fda8 * rate < turn)
		turn = DAT_0028fda8 * rate;
	if (turn < DAT_0028fda8 * -rate)
		turn = DAT_0028fda8 * -rate;
	BG_F(0x1278) = CLMath_WrapAngle(turn + BG_F(0x1278));
	BA_F(0x1e8) = 3.1415925f - BG_F(0x1278);
	return turn;
}

/* 0x00176ac0  (gap): spore, state 1: drifts to random targets 200 above the ground with a
bob, turning slowly, its cloud attached */
void Bug_Spore_Fly(void *self)
{
	float delta[4];
	float ground, turn;

	BG_F(0x1208) = BG_F(0x1208) - DAT_0028fda8;
	BG_S(0x1214) = 2;
	BG_F(0x1250) = DAT_0028fda8 + BG_F(0x1250);
	if (Bug_ReachedPoint(self) != 0 || 0x14 < BG_S(0x1202) || 200.0f < BG_F(0x1250)) {
		if (0.0f <= BA_F(0xd0))
			BG_F(0x1218) = -(float)(BG_RAND() % 1000);
		else
			BG_F(0x1218) = (float)(BG_RAND() % 1000);
		if (0.0f <= BA_F(0xd8))
			BG_F(0x1220) = -(float)(BG_RAND() % 1000);
		else
			BG_F(0x1220) = (float)(BG_RAND() % 1000);
		BG_I(0x121c) = 0;
		BG_S(0x1202) = 0;
		BG_I(0x1250) = 0;
		BG_F(0x126c) = (float)(BG_RAND() % 10) * 0.1f + 1.0f;
	}
	baPrepActorInstance((struct BActorInstance *)((char *)self + 0x1070), 1);
	baUpdateAnimations((struct BActorInstance *)((char *)self + 0x1070), (struct BActorNode *)MW_AP(self, 0x11a4),
		DAT_0028fda8, NULL);
	bg_PlaceExtraInstance(self);
	ground = mw_GroundProbe(BA_V(0xd0), 30.0f);
	BG_F(0x1230) = BA_F(0xd4) - ground;
	if (ground < 0.0f)
		ground = 0.0f;
	BA_F(0x198) = (200.0f - ((float)sin((double)BG_F(0x1228)) * 50.0f + ground)) * 0.2f;
	if (BG_S(0x1238) != 0) {
		BA_F(0x198) = 10.0f;
		BG_S(0x1238) = (short)(BG_S(0x1238) - 1);
	}
	delta[0] = BA_F(0xd0) - BG_F(0x1218);
	delta[1] = 0.0f;
	delta[2] = BA_F(0xd8) - BG_F(0x1220);
	bmVectorSetLength(delta, delta, BG_F(0x126c) * 0.25f);
	BA_F(0x194) = BA_F(0x194) - DAT_0028fda8 * delta[0];
	BA_F(0x19c) = BA_F(0x19c) - DAT_0028fda8 * delta[2];
	BA_F(0x194) = BA_F(0x194) * 0.97f;
	BA_F(0x19c) = BA_F(0x19c) * 0.97f;
	if (BG_S(0x123c) != 0) {
		BG_S(0x123c) = (short)(BG_S(0x123c) - 1);
		BA_F(0x198) = BA_F(0x198) + 10.0f;
	}
	BG_F(0x1274) = mw_Heading(delta[0], -delta[2]);
	turn = bg_TurnTowards(self, BG_F(0x1274), 0.03f);
	BG_F(0x1234) = (BG_F(0x1234) * 1.9f + turn) * 0.5f;
	bg_ApplyLean(self);
	memcpy(BG_V(0x127c), BA_V(0x194), 0x10);
}

/* the player who hit the bug (+0x120c) scores its value (+0x11fe), shown as a floating
number in his colour at the bug */
static void bg_ScoreForHitter(void *self)
{
	char text[32];
	void *player;

	player = mw_Players[BG_S(0x120c)];
	MW_AI(player, 0x1400) = MW_AI(player, 0x1400) + (int)BG_S(0x11fe);
	sprintf(text, "%d", (int)BG_S(0x11fe));
	player = mw_Players[BG_S(0x120c)];
	baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)BA_F(0xd0), 340.0,
		(double)BA_F(0xd8), (int)MW_AS(player, 0x136e), (int)MW_AS(player, 0x1370), (int)MW_AS(player, 0x1372),
		text, 1.0);
}

/* 0x00177050  (gap): a bug was hit; `scored`: by a player (+0x120c), who scores. Worms
and roaches die (splat), tremors and flies lose a hit point (+0x11f4), spores burst. */
void Bug_Hit(void *self, int scored)
{
	switch (BG_S(0x11ec)) {
	case 0:
	case 5:
		if (BG_S(0x11ee) == 3)
			break;
		Actor_PlayAnim(BG_ACTOR, 1, 0, 0, 1.0f, 0.1f, 1);
		BG_S(0x11ee) = 3;
		BG_I(0x11f0) = 0;
		FUN_0017cf10(BG_RAND() % 6 + 9, 0, 0xff);
		FUN_001842b0(self, 2);
		if (scored != 0)
			bg_ScoreForHitter(self);
		break;
	case 1:
	case 6:
		if (BG_S(0x11ee) == 3)
			break;
		Actor_PlayAnim(BG_ACTOR, 1, 0, 0, 1.0f, 0.1f, 1);
		BG_S(0x11ee) = 3;
		BG_I(0x11f0) = 0;
		FUN_0017cf10(BG_RAND() % 6 + 9, 0, 0xff);
		FUN_001842b0(self, 4);
		FUN_0017d090((int)BG_S(0x11f6));
		if (scored != 0)
			bg_ScoreForHitter(self);
		break;
	case 2:
		if (BG_S(0x11ee) == 4 || BG_S(0x11ee) == 6)
			break;
		BG_S(0x11f4) = (short)(BG_S(0x11f4) - 1);
		BG_I(0x11f0) = 0;
		if (BG_S(0x11f4) == 0) {
			BG_S(0x11ee) = 4;
			FUN_001842b0(self, 8);
			if (scored != 0)
				bg_ScoreForHitter(self);
		} else {
			BG_S(0x11ee) = 6;
		}
		/* the slot can take a rubble again */
		if (mw_Variation == 0)
			MW_F(0x002b2d54 + BG_S(0x1242) * 0x10) = 1.0f;
		else if (mw_Variation == 1)
			MW_F(0x002b2db4 + BG_S(0x1242) * 0x10) = 1.0f;
		break;
	case 3:
		if (BG_S(0x11ee) == 4 || BG_S(0x11ee) == 3)
			break;
		if (BG_S(0x11f4) != 0)
			BG_S(0x11f4) = (short)(BG_S(0x11f4) - 1);
		BG_I(0x11f0) = 0;
		if (BG_S(0x11f4) < 1) {
			BG_S(0x11ee) = 4;
			FUN_001842b0(self, 4);
			if (scored == 0)
				return;
			bg_ScoreForHitter(self);
		} else {
			BG_S(0x11ee) = 3;
		}
		FUN_0017cf10((int)Core_Rand() % 3 + 0xf, 0, 0xff);
		FUN_0017d090((int)BG_S(0x11f6));
		break;
	case 4:
		if (BG_S(0x11ee) != 2) {
			BG_S(0x11ee) = 2;
			BG_I(0x11f0) = 0;
		}
		break;
	}
}

/* 0x00177520  (gap): the train, state 1: runs along x at 30 a frame with its headlight
(light 2), hoots once at +0x1248; gone past +-6000 */
void Bug_Train_Run(void *self)
{
	float front[4], beam[4];
	unsigned char *platform = (unsigned char *)MW_AP(self, 0x4);
	float x;

	front[0] = MW_AF(platform, 0xe0);
	front[1] = MW_AF(platform, 0xe4);
	front[2] = MW_AF(platform, 0xe8);
	front[3] = MW_AF(platform, 0xec);
	beam[0] = MW_AF(platform, 0xe0);
	beam[1] = MW_AF(platform, 0xe4) - 800.0f;
	beam[2] = MW_AF(platform, 0xe8);
	beam[3] = MW_AF(platform, 0xec);
	front[1] = front[1] + 500.0f;
	if (!(BG_F(0x11f0) < BG_F(0x1248)) && BG_S(0x123a) == 0) {
		BG_S(0x123a) = 1;
		FUN_0017cf10(BG_RAND() % 2 + 0x1a, 0, 0xff);
	}
	x = MW_AF(MW_AP(self, 0x4), 0xe0);
	if (BG_F(0x127c) <= 0.0f) {
		if (-6000.0f <= x) {
			BG_F(0x127c) = -30.0f;
			front[0] = front[0] - 2240.0f;
			beam[0] = front[0] - 1000.0f;
			goto light;
		}
	} else if (x <= 6000.0f) {
		BG_F(0x127c) = 30.0f;
		front[0] = front[0] + 2240.0f;
		beam[0] = front[0] + 1000.0f;
	light:
		baSetSpotLight((struct BLight *)mw_Light2, front, beam, 2.24e-05f, 1.0f);
		baSetLightColour((struct BLight *)mw_Light2, 0x7f, 0x7f, 0x7f);
		return;
	}
	BG_S(0x11fa) = 0;
	BG_S(0x11fc) = 0;
	MW_S(0x0030ce6a) = 0;
	baEnableLight((struct BLight *)mw_Light2, 0);
}

/* 0x001776e0  (gap): the train, state 0: comes in from one end (lane z 0 or 1200), the
headlight on, the engine sound; returns the last random number / 0x5c */
unsigned int Bug_Train_Start(void *self)
{
	float at[4], beam[4];
	unsigned char *platform;
	float x, z;
	int r;

	r = BG_RAND();
	MW_S(0x0030ce66) = 1;
	z = r % 3 == 2 ? 1200.0f : 0.0f;
	at[1] = 0.0f;
	beam[2] = z;
	beam[3] = 0.0f;
	at[3] = 1.0f;
	beam[1] = -800.0f;
	if (BG_RAND() % 10 < 5) {
		x = -9000.0f;
		BG_F(0x127c) = 30.0f;
		beam[0] = -8000.0f;
	} else {
		x = 9000.0f;
		BG_F(0x127c) = -30.0f;
		beam[0] = 8000.0f;
	}
	platform = (unsigned char *)MW_AP(self, 0x4);
	MW_AF(platform, 0xe0) = x;
	MW_AF(platform, 0xe4) = at[1];
	MW_AF(platform, 0xe8) = z;
	MW_AF(platform, 0xec) = at[3];
	if (0.0f <= x)
		at[0] = x - 2250.0f;
	else
		at[0] = x + 2250.0f;
	at[1] = 500.0f + 0.0f;
	at[2] = z;
	baSetSpotLight((struct BLight *)mw_Light2, beam, at, 2.24e-05f, 1.0f);
	baSetLightColour((struct BLight *)mw_Light2, 10, 0x7f, 10);
	baEnableLight((struct BLight *)mw_Light2, 1);
	BG_S(0x11f6) = (short)FUN_0017cf10(BG_RAND() % 2 + 0x18, 0, 0xff);
	BG_I(0x1280) = 0;
	BG_I(0x1284) = 0;
	BG_I(0x11f0) = 0;
	BG_S(0x11ee) = 1;
	r = BG_RAND();
	BG_S(0x123a) = 0;
	BG_F(0x1248) = (float)(r % 0x5c + 0x5b);
	return (unsigned int)r / 0x5c;
}

/* the lowest ground under a flying bug (its position and 4 points 40/90 around it);
+0x1230 gets its height above it. Returns the ground (not below 0: then the kick
counter +0x123c is set to 3 when `kick`). */
static float bg_GroundUnderFly(void *self, int kick)
{
	float probe[4][4];
	float ground, g;
	int i;

	for (i = 0; i < 4; i++)
		memcpy(probe[i], BA_V(0xd0), 0x10);
	probe[0][0] = probe[0][0] + 40.0f;
	probe[1][0] = probe[1][0] - 40.0f;
	probe[2][2] = probe[2][2] + 90.0f;
	probe[3][2] = probe[3][2] - 90.0f;
	ground = mw_GroundProbe(BA_V(0xd0), 30.0f);
	for (i = 0; i < 4; i++) {
		g = mw_GroundProbe(probe[i], 30.0f);
		if (g < ground)
			ground = g;
	}
	BG_F(0x1230) = BA_F(0xd4) - ground;
	if (ground < 0.0f) {
		ground = 0.0f;
		if (kick)
			BG_S(0x123c) = 3;
	}
	return ground;
}

/* steering of a flying bug towards (delta = position - target): accelerates at speed x
0.25 with `drag` (x/z, also y when `vertical`), the kick counter +0x123c lifts it when
`kick`, turns at most 0.1 a frame and leans 6 x the turn */
static void bg_FlySteer(void *self, float *delta, float drag, int vertical, int kick)
{
	float turn;

	bmVectorSetLength(delta, delta, BG_F(0x126c) * 0.25f);
	BA_F(0x194) = BA_F(0x194) - DAT_0028fda8 * delta[0];
	if (vertical)
		BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * delta[1];
	BA_F(0x19c) = BA_F(0x19c) - DAT_0028fda8 * delta[2];
	BA_F(0x194) = BA_F(0x194) * drag;
	if (vertical)
		BA_F(0x198) = BA_F(0x198) * drag;
	BA_F(0x19c) = BA_F(0x19c) * drag;
	if (kick && BG_S(0x123c) != 0) {
		BG_S(0x123c) = (short)(BG_S(0x123c) - 1);
		BA_F(0x198) = BA_F(0x198) + 10.0f;
	}
	BG_F(0x1274) = mw_Heading(delta[0], -delta[2]);
	if (vertical) {
		/* (the original draws the attack time here, between the heading and the turn) */
		int r = BG_RAND();

		BG_F(0x1208) = (float)((DAT_002bae70 == 2 ? r % 100 : r % 0x96) + 0x96) * DAT_0028fda8;
	}
	turn = bg_TurnTowards(self, BG_F(0x1274), 0.1f);
	BG_F(0x1234) = (turn * 6.0f + BG_F(0x1234)) * 0.5f;
	bg_ApplyLean(self);
	memcpy(BG_V(0x127c), BA_V(0x194), 0x10);
}

/* 0x001778f0  (gap): fly, state 5: attacks its target (+0x1204: a player, or in "Burgers"
his burger stand), flying low; on contact it bites the stand / the player and goes back
to wandering */
void Bug_Fly_Attack(void *self)
{
	float delta[4], dir[4], mid[4];
	float ground, dist, bite;
	float *target;
	int index, r;

	ground = bg_GroundUnderFly(self, 1);
	if ((BG_F(0x127c) != BG_F(0x128c) || BG_F(0x1284) != BG_F(0x1294)) && mw_Variation == 1)
		BG_S(0x123c) = 4;
	if (BG_S(0x123c) != 0)
		BG_S(0x123c) = (short)(BG_S(0x123c) - 1);
	if (BG_S(0x1006) == 0 && BG_S(0x123c) == 0) {
		BA_F(0x198) = (100.0f - ((float)sin((double)BG_F(0x1228)) * 25.0f + ground)) * 0.2f;
	} else {
		BA_F(0x198) = (200.0f - ((float)sin((double)BG_F(0x1228)) * 50.0f + ground)) * 0.2f;
		BG_S(0x1006) = 0;
	}
	if (BG_S(0x1238) != 0) {
		BA_F(0x198) = 10.0f;
		BG_S(0x1238) = (short)(BG_S(0x1238) - 1);
	}
	if (mw_Variation == 2)
		target = MW_FV(0x002b2718 + BG_S(0x1204) * 0x10);
	else
		target = MW_AV(MW_AP(mw_Players[BG_S(0x1204)], 0x30), 0xd0);
	delta[0] = BA_F(0xd0) - target[0];
	delta[2] = BA_F(0xd8) - target[2];
	delta[1] = 0.0f;
	dist = sqrtf(delta[0] * delta[0] + delta[2] * delta[2]);
	if (dist < 400.0f && BG_S(0x1214) != 6) {
		Actor_PlayAnim(BG_ACTOR, 6, 1, 0, 1.0f, 0.3f, 1);
		BG_S(0x1214) = 6;
	}
	index = BG_S(0x1204);
	if (dist < MW_F(0x0030ea00) * MW_F(0x0030e9f0 + index * 4) + MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x9c) + 10.0f) {
		if (mw_Variation == 2) {
			float *stand = MW_FV(0x002b2718 + index * 0x10);

			bite = DAT_002bae70 == 2 ? 0.15f : 0.1f;
			MW_F(0x0030e9f0 + index * 4) = MW_F(0x0030e9f0 + index * 4) - bite;
			if (MW_F(0x0030e9f0 + BG_S(0x1204) * 4) < 0.0f)
				MW_I(0x0030e9f0 + BG_S(0x1204) * 4) = 0;
			mid[0] = (stand[0] + BA_F(0xd0)) * 0.5f;
			mid[1] = (stand[1] + BA_F(0xd4)) * 0.5f;
			mid[2] = (stand[2] + BA_F(0xd8)) * 0.5f;
			if (0.0f < MW_F(0x0030e9f0 + BG_S(0x1204) * 4)) {
				FUN_00184760(mid, 0);
				FUN_0017cf10(0x22, 0, 0xff);
			}
			BG_S(0x11ee) = 2;
			BG_F(0x1208) = (float)(BG_RAND() % 0x96 + 0x96) * DAT_0028fda8;
			Actor_PlayAnim(BG_ACTOR, 2, 1, 0, 1.0f, 0.3f, 1);
			BG_S(0x1214) = 2;
		} else {
			bmVectorNormalise(dir, delta);
			BG_S(0x11ee) = 2;
			dir[0] = dir[0] * 30.0f;
			dir[1] = 90.0f;
			dir[2] = dir[2] * 30.0f;
			if (DAT_002bae70 == 2)
				r = BG_RAND() % 100 + 100;
			else
				r = BG_RAND() % 0x96 + 0x96;
			BG_F(0x1208) = (float)r * DAT_0028fda8;
			FUN_0017ed40(mw_Players[BG_S(0x1204)], BA_V(0xd0), 0);
		}
	}
	bg_FlySteer(self, delta, 0.97f, 0, 0);
}

/* the target of a wandering fly: a random point in the quarter away from where it is */
static void bg_NewWanderTarget(void *self)
{
	if (0.0f <= BA_F(0xd0))
		BG_F(0x1218) = -(float)(BG_RAND() % 1000);
	else
		BG_F(0x1218) = (float)(BG_RAND() % 1000);
	if (0.0f <= BA_F(0xd8))
		BG_F(0x1220) = -(float)(BG_RAND() % 1000);
	else
		BG_F(0x1220) = (float)(BG_RAND() % 1000);
	BG_I(0x121c) = 0;
}

/* 0x00177fd0  (gap): fly, state 0: appears (2000 up at a random point; in "Burgers" out of
the kitchen along 2 points, then a random target) and starts wandering. Returns the
last random number / 150 (or the actor's velocity address in "Burgers"). */
unsigned int Bug_Fly_Start(void *self)
{
	float delta[4];
	int x, z, r;
	short point;

	if (mw_Variation != 2) {
		BG_F(0x1278) = 3.1415925f;
		BG_F(0x1274) = 3.1415925f;
		x = BG_RAND() % 2000 - 1000;
		z = BG_RAND() % 2000 - 1000;
		BG_F(0x1230) = 2000.0f - mw_GroundProbe(BA_V(0xd0), 0.0f);
		Actor_PlayAnim(BG_ACTOR, 1, 0, 0, 1.0f, 0.0f, 1);
		Actor_PlayAnim(BG_ACTOR, 2, 1, 1, 1.0f, 0.3f, 1);
		BA_F(0xd0) = (float)x;
		BA_F(0xd4) = 2000.0f;
		BA_F(0xd8) = (float)z;
		BA_F(0xdc) = 1.0f;
		BA_F(0x1e8) = BG_F(0x1278) + 1.5707963f;
		BG_S(0x11ee) = 2;
		BG_S(0x1214) = 2;
		BG_F(0x1218) = (float)(BG_RAND() % 0xaf0 - 0x578);
		r = BG_RAND();
		BG_I(0x121c) = 0;
		BG_I(0x11f0) = 0;
		BG_S(0x1202) = 0;
		BG_I(0x1250) = 0;
		BG_F(0x1220) = (float)(r % 0xaf0 - 0x578);
		BG_F(0x126c) = (float)(BG_RAND() % 0x14) * 0.1f + 1.0f;
		r = BG_RAND();
		BG_F(0x1208) = (float)(r % 0x96 + 0x96) * DAT_0028fda8;
		return (unsigned int)r / 0x96;
	}
	if (BG_F(0x11f0) <= DAT_0028fda8 + DAT_0028fda8) {
		BA_F(0xd0) = 1720.0f;
		BA_F(0xd4) = 1200.0f;
		BA_F(0xd8) = 1100.0f;
		BA_F(0xdc) = 1.0f;
		BA_I(0x220) = 0x100;
		BG_F(0x126c) = 7.0f;
		Actor_PlayAnim(BG_ACTOR, 4, 0, 0, 1.0f, 0.3f, 1);
		Actor_PlayAnim(BG_ACTOR, 2, 1, 1, 1.0f, 0.3f, 1);
		BG_F(0x1278) = 3.1415925f;
		BG_F(0x1274) = 3.1415925f;
	}
	if (Bug_ReachedPoint(self) != 0)
		BG_S(0x1244) = (short)(BG_S(0x1244) + 1);
	point = BG_S(0x1244);
	if (point == 0) {
		BG_F(0x1218) = 1420.0f;
		BG_F(0x121c) = 800.0f;
		BG_F(0x1220) = 1400.0f;
	}
	if (point == 1) {
		BG_F(0x1218) = 820.0f;
		BG_F(0x121c) = 800.0f;
		BG_F(0x1220) = 800.0f;
	}
	if (point == 2) {
		BA_I(0x220) = 0x59;
		BG_S(0x11ee) = 2;
		BG_I(0x11f0) = 0;
		BG_S(0x1214) = 2;
		BG_F(0x1218) = (float)(BG_RAND() % 0xaf0 - 0x578);
		r = BG_RAND();
		BG_I(0x121c) = 0;
		BG_F(0x1220) = (float)(r % 0xaf0 - 0x578);
		BG_F(0x126c) = (float)(BG_RAND() % 0x14) * 0.1f + 3.0f;
	}
	BG_F(0x1224) = 1.0f;
	delta[0] = BA_F(0xd0) - BG_F(0x1218);
	delta[1] = BA_F(0xd4) - BG_F(0x121c);
	delta[2] = BA_F(0xd8) - BG_F(0x1220);
	bg_FlySteer(self, delta, 0.9f, 1, 0);
	return (unsigned int)(uintptr_t)BA_V(0x194);
}

/* 0x001786e0  (gap): fly, state 2: wanders (new targets when reached, stuck or after 200
frames) bobbing 200 above the ground; picks a player to attack when +0x1208 runs out */
void Bug_Fly_Wander(void *self)
{
	float delta[4];
	float ground;

	BG_F(0x1208) = BG_F(0x1208) - DAT_0028fda8;
	BG_S(0x1214) = 2;
	BG_F(0x1250) = DAT_0028fda8 + BG_F(0x1250);
	if (100.0f <= BG_F(0x11f0)) {
		BG_F(0x11f0) = (float)(BG_RAND() % 0x1e);
		Actor_PlayAnim(BG_ACTOR, 4, 0, 0, 1.0f, 0.3f, 1);
		Actor_PlayAnim(BG_ACTOR, 2, 1, 1, 1.0f, 0.3f, 1);
	}
	if (BG_F(0x1208) <= 0.0f) {
		BG_S(0x11ee) = 5;
		if (mw_Variation == 2)
			BG_S(0x1204) = CBbash_Arena_ChoosePlayerToAttack(mw_Arena);
		else
			BG_S(0x1204) = (short)(BG_RAND() & 3);
		BG_I(0x11f0) = 0;
	}
	if (Bug_ReachedPoint(self) != 0 || 0x14 < BG_S(0x1202) || 200.0f < BG_F(0x1250)) {
		bg_NewWanderTarget(self);
		BG_S(0x1202) = 0;
		BG_I(0x1250) = 0;
		BG_F(0x126c) = (float)(BG_RAND() % 0x14) * 0.1f + 1.0f;
	}
	ground = bg_GroundUnderFly(self, 0);
	BA_F(0x198) = (200.0f - ((float)sin((double)BG_F(0x1228)) * 50.0f + ground)) * 0.2f;
	if (BG_S(0x1238) != 0) {
		BA_F(0x198) = 10.0f;
		BG_S(0x1238) = (short)(BG_S(0x1238) - 1);
	}
	delta[0] = BA_F(0xd0) - BG_F(0x1218);
	delta[2] = BA_F(0xd8) - BG_F(0x1220);
	delta[1] = 0.0f;
	bg_FlySteer(self, delta, 0.97f, 0, 1);
}

/* 0x00178d30  (gap): fly, state 3: knocked up (hit but alive): bobs up from the ground
(+300 a frame while the kick counter +0x123c runs), back to wandering after 30 frames */
void Bug_Fly_Knocked(void *self)
{
	float bob, ground, lift;

	bob = (float)(sin((double)BG_F(0x1228)) * 100.0 + 50.0);
	ground = mw_GroundProbe(BA_V(0xd0), 0.0f);
	BG_F(0x1228) = BG_F(0x1228) + 0.1f;
	BG_F(0x1230) = ground;
	if (BG_S(0x123c) == 0) {
		lift = bob;
		BG_S(0x123e) = 0;
	} else {
		BG_S(0x123c) = (short)(BG_S(0x123c) - 1);
		lift = (float)(int)BG_S(0x123e) + bob;
		BG_S(0x123e) = (short)(BG_S(0x123e) + 300);
	}
	BA_F(0x198) = (ground - (lift + 100.0f)) * -0.2f;
	if (BA_F(0x198) < -90.0f)
		BA_F(0x198) = -90.0f;
	if (BG_F(0x11f0) == 0.0f) {
		Actor_PlayAnim(BG_ACTOR, 2, 0, 0, 1.0f, 0.0f, 1);
		BG_S(0x123c) = 7;
	}
	if (30.0f <= BG_F(0x11f0)) {
		BG_S(0x11ee) = 2;
		Actor_PlayAnim(BG_ACTOR, 4, 1, 1, 1.0f, 0.3f, 1);
		BG_I(0x11f0) = 0;
	}
}

/* ======================================================================== the tremor (type 2)

The giant worm of "Brawl" and "Tremors": it rises out of one of 6 holes (slots: "Brawl"
0x002b2d48, "Tremors" 0x002b2da8, float[4] each; the w of a slot is its state: 0 free,
1 free with rubble, 2 taken, 3 taken with rubble), turns its 5 body segments towards the
nearest player, spits at him or bites him, and pushes everyone out of its way.

Tremor fields: +0x1008 BActorNode *segment[5] (the head is the last), +0x102c float
wiggle[5], +0x1040 float wiggle speed[5], +0x1054 float wanted body yaw, +0x1058 body yaw,
+0x105c pitch, +0x1060 wanted pitch, +0x11f4 hit points, +0x11f8 short, +0x1204 the
target player, +0x120e attacking, +0x1214 the animation, +0x1216 the shot is done.
Tweaks: 0x0030ec0c the wake-up distance, 0x0030ec10 the bite distance, 0x0030ec14 the
square root of the turn rate, 0x0030ec18 the bite animation speed.
*/

/* the slot table of the variation, or NULL ("Burgers", "Pied Pipers") */
static float *bg_TremorSlot(short slot)
{
	if (mw_Variation == 0)
		return MW_FV(0x002b2d48 + slot * 0x10);
	if (mw_Variation == 1)
		return MW_FV(0x002b2da8 + slot * 0x10);
	return NULL;
}

/* the index of the player nearest to the bug's actor (all 4), and his distance */
static short bg_NearestPlayer(void *self, float *outDist)
{
	float best = 9999999.0f, dist;
	short i, found = 0;

	for (i = 0; i < 4; i++) {
		dist = bmVectorDistance(MW_AV(MW_AP(mw_Players[i], 0x30), 0xd0), BA_V(0xd0));
		if (dist < best) {
			best = dist;
			found = i;
		}
	}
	*outDist = best;
	return found;
}

/* wraps an angle difference into -pi..pi (the original's inline sequence) */
static float bg_WrapDelta(float delta)
{
	if (delta < 0.0f)
		delta = delta + 6.2831855f;
	if (6.2831855f < delta)
		delta = delta - 6.2831855f;
	if (3.1415925f < delta)
		delta = delta - 6.2831855f;
	return delta;
}

/* moves a body segment's position out to 230 from `centre` */
static void bg_PushOut(float *pos, const float *centre)
{
	float d[4];

	d[0] = pos[0] - centre[0];
	d[1] = pos[1] - centre[1];
	d[2] = pos[2] - centre[2];
	if (sqrtf(d[1] * d[1] + d[0] * d[0] + d[2] * d[2]) < 230.0f) {
		bmVectorSetLength(d, d, 230.0f);
		pos[0] = d[0] + centre[0];
		pos[1] = centre[1] + d[1];
		pos[2] = d[2] + centre[2];
	}
}

/* 0x00178e80  (gap): the tremor looks at `target`: turns the body (yaw +0x1278, at most
rate^2 a frame) and the head (pitch +0x105c), bends the 5 segments, wiggles them, and
pushes the players and the live small bugs out of 230 around each segment */
void Bug_Tremor_Aim(void *self, float *target)
{
	float d[4], node[4];
	float pitch[5], bend[5];
	float quat[4], matrix[16];
	float segment[5][4];
	float turn, rate, yaw, x;
	double delta;
	unsigned char *node8;
	short i;
	int k;

	d[0] = BA_F(0xd0) - target[0];
	d[1] = BA_F(0xd4) - target[1];
	d[2] = BA_F(0xd8) - target[2];
	BG_F(0x1274) = mw_Heading(-d[0], d[2]);
	baGetNodePosition((struct BActorInstance *)(BG_ACTOR + 0x10), (struct BActorNode *)MW_AP(self, 0x1018), node);
	BG_F(0x1060) = mw_Heading(node[1] - target[1], sqrtf(d[2] * d[2] + d[0] * d[0] + d[1] * d[1]));
	if (BG_F(0x1060) < 0.0f)
		BG_F(0x1060) = 0.0f;
	if (1.5707963f < BG_F(0x1060))
		BG_F(0x1060) = 1.5707963f;

	/* the pitch of the head, then each segment gets a fifth of it */
	turn = bg_WrapDelta(BG_F(0x1060) - BG_F(0x105c));
	rate = MW_F(0x0030ec14) * MW_F(0x0030ec14);
	if (rate < turn)
		turn = rate;
	if (turn < -rate)
		turn = -rate;
	BG_F(0x105c) = turn + BG_F(0x105c);
	BG_F(0x105c) = CLMath_WrapAngle(BG_F(0x105c));
	pitch[0] = 0.0f;
	pitch[1] = BG_F(0x105c) * 0.2f;

	/* the yaw of the whole worm */
	turn = bg_WrapDelta(BG_F(0x1274) - BG_F(0x1278));
	rate = MW_F(0x0030ec14) * MW_F(0x0030ec14);
	if (rate < turn)
		turn = rate;
	if (turn < -rate)
		turn = -rate;
	BG_F(0x1278) = turn + BG_F(0x1278);
	pitch[2] = pitch[1];
	pitch[3] = pitch[1];
	pitch[4] = pitch[1];
	BG_F(0x1278) = CLMath_WrapAngle(BG_F(0x1278));
	BG_F(0x1054) = (3.1415925f - BG_F(0x1278)) + 3.1415925f;

	/* the base turns towards it by at most 45 degrees behind, then 1/30 of the rest a frame */
	delta = (double)CLMath_AngleDelta(BG_F(0x1054), BG_F(0x1058));
	if (delta < 0.0)
		delta = delta + (double)6.2831855f;
	if ((double)6.2831855f < delta)
		delta = delta - (double)6.2831855f;
	if ((double)3.1415925f < delta)
		delta = delta - (double)6.2831855f;
	if ((double)0.7853981f < delta)
		BG_F(0x1058) = (float)((double)BG_F(0x1058) - ((double)0.7853981f - delta));
	else if (delta < (double)-0.7853981f)
		BG_F(0x1058) = (float)(delta + (double)BG_F(0x1058) + (double)0.7853981f);
	BG_F(0x1058) = (float)(delta * (double)0.033333335f + (double)BG_F(0x1058));
	BG_F(0x1058) = CLMath_WrapAngle(BG_F(0x1058));
	BG_F(0x1054) = CLMath_WrapAngle(BG_F(0x1054));
	x = bg_WrapDelta(BG_F(0x1054) - BG_F(0x1058));
	for (k = 1; k < 5; k++)
		bend[k] = (float)k * x * 0.2f;
	bend[0] = 0.0f;

	for (i = 0; i < 5; i++) {
		float wiggle = BG_F(0x102c + i * 4);

		BG_F(0x102c + i * 4) = wiggle + BG_F(0x1040 + i * 4);
		if (0.0f < BG_F(0x1040 + i * 4) && 0.19634953f < wiggle + BG_F(0x1040 + i * 4))
			BG_F(0x1040 + i * 4) = -BG_F(0x1040 + i * 4);
		if (BG_F(0x1040 + i * 4) < 0.0f && BG_F(0x102c + i * 4) < -0.19634953f)
			BG_F(0x1040 + i * 4) = -BG_F(0x1040 + i * 4);
		if (i == 0) {
			yaw = BG_F(0x102c) + BG_F(0x1058);
			x = 0.0f;
		} else {
			yaw = bend[i] + BG_F(0x102c + i * 4);
			x = -pitch[i];
		}
		bmMatrixRotationYX(matrix, x, yaw);
		bmMatrixToQuaternion(quat, matrix);
		baSetNodeRotation((struct BActorNode *)MW_AP(self, 0x1008 + i * 4), quat);
		baGetNodePosition((struct BActorInstance *)(BG_ACTOR + 0x10), (struct BActorNode *)MW_AP(self, 0x1008 + i * 4),
			segment[i]);
	}

	for (i = 0; i < 5; i++)
		for (k = 0; k < 4; k++)
			bg_PushOut(MW_AV(MW_AP(mw_Players[k], 0x30), 0xd0), segment[i]);
	for (node8 = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node8 != NULL;
	     node8 = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node8)) {
		/* node = bug + 8: +0x11f2 alive, +0x11e4 type */
		if (MW_AS(node8, 0x11f2) != 0 && MW_AS(node8, 0x11e4) < 7 && MW_AS(node8, 0x11e4) != 2) {
			for (i = 0; i < 5; i++)
				bg_PushOut(MW_AV(MW_AP(node8, -8), 0xd0), segment[i]);
		}
	}
}

/* 0x00179540  (gap): tremor, state 4: dying: looks at the nearest player, sinks (anim 9),
frees its slot after 30 frames */
void Bug_Tremor_Die(void *self)
{
	float dist;
	float *slot;
	short nearest;

	nearest = bg_NearestPlayer(self, &dist);
	Bug_Tremor_Aim(self, MW_AV(MW_AP(mw_Players[nearest], 0x30), 0xd0));
	if (BG_F(0x11f0) == 0.0f)
		Actor_PlayAnim(BG_ACTOR, 9, 0, 0, 3.0f, 0.1f, 1);
	slot = bg_TremorSlot(BG_S(0x1242));
	if (slot != NULL) {
		BA_F(0xd0) = slot[0];
		BA_F(0xd8) = slot[2];
	}
	BA_F(0x198) = DAT_0028fda8 * -1.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
	if (10.0f <= BG_F(0x11f0))
		BA_I(0x220) = 0;
	if (30.0f <= BG_F(0x11f0)) {
		if (mw_Variation == 0)
			MW_F(0x002b2d54 + BG_S(0x1242) * 0x10) = 1.0f;
		else if (mw_Variation == 1)
			MW_F(0x002b2db4 + BG_S(0x1242) * 0x10) = 1.0f;
		BG_S(0x11fa) = 0;
		BG_S(0x11fc) = 1;
	}
}

/* 0x001796f0  (gap): tremor, state 2: attacks the nearest standing player: within the
bite distance a bite (anim 5) that throws him back, else a spit ball (anim 7) */
void Bug_Tremor_Attack(void *self)
{
	float node[4], push[4];
	float best, dist, heading;
	float *slot;
	unsigned char *target;
	short i, found, state;

	found = 0;
	best = 9999999.0f;
	slot = bg_TremorSlot(BG_S(0x1242));
	if (slot != NULL) {
		BA_F(0xd0) = slot[0];
		BA_F(0xd4) = slot[1];
		BA_F(0xd8) = slot[2];
		BA_F(0xdc) = slot[3];
	}
	for (i = 0; i < 4; i++) {
		state = MW_AS(mw_Players[i], 0x36);
		if (state == 6 || state == 7)
			continue;
		dist = bmVectorDistance(MW_AV(MW_AP(mw_Players[i], 0x30), 0xd0), BA_V(0xd0));
		if (dist < best) {
			best = dist;
			found = i;
		}
	}
	Bug_Tremor_Aim(self, MW_AV(MW_AP(mw_Players[found], 0x30), 0xd0));
	if (BG_S(0x120e) == 0) {
		heading = CLMath_HeadingTo(MW_AV(MW_AP(mw_Players[found], 0x30), 0xd0), BA_V(0xd0));
		if ((double)CLMath_AngleDistance(heading, BG_F(0x1054)) < 0.1) {
			BG_S(0x1204) = found;
			if (best <= MW_F(0x0030ec10)) {
				Actor_PlayAnim(BG_ACTOR, 5, 0, 0, MW_F(0x0030ec18), 0.5f, 1);
				BG_S(0x1214) = 5;
			} else {
				Actor_PlayAnim(BG_ACTOR, 7, 0, 0, 1.0f, 0.5f, 1);
				BG_S(0x1214) = 7;
			}
			BG_S(0x1216) = 0;
			BG_F(0x11f0) = -DAT_0028fda8;
			BG_S(0x120e) = 1;
		}
	}
	if (MW_AS(BG_ACTOR, 0x234) != 0 && BG_S(0x120e) == 1) {
		/* the animation ended: back to watching */
		BG_S(0x11ee) = 1;
		BG_S(0x1216) = 0;
		BG_S(0x120e) = 0;
		BG_F(0x11f0) = -1.0f;
		return;
	}
	if (BG_S(0x1214) == 7 && 1.0f < BG_F(0x11f0) && BG_S(0x1216) == 0 &&
	    (double)Actor_AnimProgress(BG_ACTOR + 0x10) > 0.54) {
		/* the spit leaves the head */
		BG_S(0x1216) = 1;
		baGetNodePosition((struct BActorInstance *)(BG_ACTOR + 0x10), (struct BActorNode *)MW_AP(self, 0x1018), node);
		FUN_00180de0(mw_Bullets, node, MW_AV(MW_AP(mw_Players[BG_S(0x1204)], 0x30), 0xd0));
	}
	if (BG_S(0x1214) == 5 && 1.0f < BG_F(0x11f0) && BG_S(0x1216) == 0 &&
	    (double)Actor_AnimProgress(BG_ACTOR + 0x10) > 0.54) {
		/* the bite lands when the player is still in front and in reach */
		BG_S(0x1216) = 1;
		target = (unsigned char *)mw_Players[BG_S(0x1204)];
		dist = bmVectorDistance(MW_AV(MW_AP(target, 0x30), 0xd0), BA_V(0xd0));
		push[0] = MW_AF(MW_AP(target, 0x30), 0xd0) - BA_F(0xd0);
		push[2] = MW_AF(MW_AP(target, 0x30), 0xd8) - BA_F(0xd8);
		push[1] = 0.0f;
		heading = mw_Heading(push[0], -push[2]);
		if (fabsf(heading - BG_F(0x1278)) < 1.0f && dist < MW_F(0x0030ec10) && MW_AS(target, 0x36) != 7 &&
		    MW_AS(target, 0x36) != 6) {
			bmVectorNormalise(push, push);
			push[0] = push[0] * 15.0f;
			push[1] = push[1] * 15.0f;
			push[2] = push[2] * 15.0f;
			MW_AF(MW_AP(target, 0x30), 0xd4) = MW_AF(MW_AP(target, 0x30), 0xd4) + 50.0f;
			MW_AI(MW_AP(target, 0x30), 0x214) = 0;
			cmb_vec3_add(MW_AV(MW_AP(target, 0x30), 0x194), MW_AV(MW_AP(target, 0x30), 0x194), push);
			cmb_vec3_add(MW_AV(target, 0x7c), MW_AV(target, 0x7c), push);
			FUN_0017ed40(mw_Players[BG_S(0x1204)], BA_V(0xd0), 1);
			if (MW_AI(mw_Players[BG_S(0x1204)], 0x1400) < 0)
				MW_AI(mw_Players[BG_S(0x1204)], 0x1400) = 0;
		}
	}
}

/* 0x00179ba0  (gap): tremor, state 1: watches from its hole (anim 8), sometimes roars
(anim 2 and a sound); attacks when the nearest player comes within the wake-up distance.
(The original returns a leftover value that its caller ignores.) */
void Bug_Tremor_Watch(void *self)
{
	float best;
	float *slot;
	short nearest;

	slot = bg_TremorSlot(BG_S(0x1242));
	if (slot != NULL) {
		BA_F(0xd0) = slot[0];
		BA_F(0xd4) = slot[1];
		BA_F(0xd8) = slot[2];
	}
	BA_F(0x198) = -1.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
	if (BG_S(0x1214) != 8) {
		Actor_PlayAnim(BG_ACTOR, 8, 0, 0, 1.0f, 0.5f, 1);
		BG_S(0x1214) = 8;
		BG_I(0x11f0) = 0;
	}
	if (MW_AS(BG_ACTOR, 0x234) != 0) {
		if (BG_RAND() % 3 != 1) {
			BG_S(0x1204) = -1;
			goto attack;
		}
		Actor_PlayAnim(BG_ACTOR, 2, 0, 0, 1.0f, 0.5f, 1);
		Actor_PlayAnim(BG_ACTOR, 8, 0, 1, 1.0f, 0.5f, 1);
		BG_S(0x1214) = 8;
		FUN_0017cf10(BG_RAND() % 3 + 6, 0, 0xff);
		BG_I(0x11f0) = 0;
	}
	nearest = bg_NearestPlayer(self, &best);
	Bug_Tremor_Aim(self, MW_AV(MW_AP(mw_Players[nearest], 0x30), 0xd0));
	if (!(best < MW_F(0x0030ec0c)))
		return;
	BG_S(0x120e) = 0;
	BG_S(0x1204) = nearest;
	BG_S(0x1216) = 0;
attack:
	BG_S(0x11ee) = 2;
	BG_F(0x11f0) = -1.0f;
}

/* 0x00179e10  (gap): tremor, state 0: rises: on the first frame it takes a random free
hole, kills the bullets within 500 and hits the small bugs within 400; when the rise
(anim 0) starts it shakes the camera, puts rubble collision on a fresh hole and roars */
void Bug_Tremor_Rise(void *self)
{
	float centre[4], at[4];
	float best, heading, angle;
	float *slot;
	unsigned char *node;
	short nearest, tries, pick;
	unsigned int base;
	void *platform;

	nearest = bg_NearestPlayer(self, &best);
	BA_F(0xdc) = 1.0f;
	Bug_Tremor_Aim(self, MW_AV(MW_AP(mw_Players[nearest], 0x30), 0xd0));
	if (BG_F(0x11f0) == 0.0f) {
		if (mw_Variation == 0 || mw_Variation == 1) {
			base = mw_Variation == 0 ? 0x002b2d48u : 0x002b2da8u;
			tries = 0;
			do {
				pick = (short)(BG_RAND() % 6);
				if (mw_Variation == 0 && mw_DbgMessages != 0)
					bkPrintf("tremor spawn %d\n", (int)pick);
				tries = (short)(tries + 1);
				if (tries == 10000)
					bkPrintf("Error with spawn tremor!!\n");
				slot = MW_FV(base + pick * 0x10);
			} while (slot[3] != 0.0f && (double)slot[3] != 1.0);
			BA_F(0xd0) = slot[0];
			BA_F(0xd4) = slot[1];
			BA_F(0xd8) = slot[2];
			BA_F(0xdc) = 1.0f;
			if (MW_AI(slot, 0xc) == 0x3f800000)
				slot[3] = 3.0f;
			else
				slot[3] = 2.0f;
			BG_S(0x11f8) = 0;
			BG_S(0x1242) = pick;
			heading = mw_Heading(-BA_F(0xd0), BA_F(0xd8));
			BG_F(0x1274) = heading;
			BG_F(0x1278) = heading;
			BG_S(0x1204) = -1;
			angle = (3.1415925f - heading) + 3.1415925f;
			BG_F(0x1054) = angle;
			BG_F(0x1058) = angle;
			BA_F(0x1e8) = 0.0f;
		}
		BG_I(0x1270) = 0;
		BG_I(0x126c) = 0;
		for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, NULL); node != NULL;
		     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, (unsigned int *)node)) {
			/* node = bullet + 0x19c; the bullet's instance (+0) has its position at +0xe0 */
			unsigned char *inst = (unsigned char *)MW_AP(node, -0x19c);
			float dx, dy, dz;

			if (inst == NULL)
				continue;
			dx = BA_F(0xd0) - MW_AF(inst, 0xe0);
			dy = BA_F(0xd4) - MW_AF(inst, 0xe4);
			dz = BA_F(0xd8) - MW_AF(inst, 0xe8);
			if (sqrtf(dx * dx + dy * dy + dz * dz) < 500.0f) {
				MW_AS(node, 8) = 1;
				MW_AS(node, 10) = 1;
			}
		}
		centre[0] = BA_F(0xd0);
		centre[2] = BA_F(0xd8);
		centre[3] = BA_F(0xdc);
		centre[1] = 0.0f;
		for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
		     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
			short type = MW_AS(node, 0x11e4);

			if (type == 2 || type == 8 || type == 7 || type == 4)
				continue;
			at[0] = MW_AF(MW_AP(node, -8), 0xd0);
			at[2] = MW_AF(MW_AP(node, -8), 0xd8);
			at[3] = MW_AF(MW_AP(node, -8), 0xdc);
			at[1] = 0.0f;
			if (bmVectorDistance(centre, at) <= 400.0f) {
				MW_AS(node, 0x1204) = (short)(BG_RAND() & 3);
				Bug_Hit(node - 8, 0);
			}
		}
		return;
	}
	if (BG_F(0x11f0) <= 0.0f)
		return;
	if (BG_S(0x11f8) != 0)
		return;
	slot = bg_TremorSlot(BG_S(0x1242));
	if (slot != NULL) {
		BA_F(0xd0) = slot[0];
		BA_F(0xd4) = slot[1];
		BA_F(0xd8) = slot[2];
		BA_F(0xdc) = 1.0f;
	}
	BA_F(0x198) = -1.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 0.0f;
	if (BG_S(0x1214) != 0 && BG_S(0x1214) != 8) {
		Actor_PlayAnim(BG_ACTOR, 0, 0, 0, 1.0f, 0.5f, 1);
		CCamera_Shake(mw_Camera, 0x19, 0x32);
		BG_S(0x1214) = 0;
		if (slot != NULL && slot[3] == 2.0f) {
			at[0] = BA_F(0xd0);
			at[2] = BA_F(0xd8);
			at[1] = BA_F(0xd4) - 200.0f;
			at[3] = BA_F(0xdc);
			platform = Platform_Create("models\\rublcoll.obe", at, 0);
			MW_P(0x0030a324) = platform;
			MW_AS(platform, 0x3fa) = 0;
			MW_AS(platform, 0x3da) = 0;
			MW_AS(platform, 0x3f8) = 1;
			MW_AS(platform, 0x3d8) = 1;
			MW_AU(platform, 0x230) = MW_AU(platform, 0x230) | 0x500000;
		}
		FUN_0017cf10(5, 0, 0xff);
	}
	if (BG_S(0x1214) == 0 && MW_AS(BG_ACTOR, 0x234) != 0)
		BG_S(0x11ee) = 1;
}

/* ======================================================================== roaches (type 1, 6)

A roach patrols between the numbered points of the arena ("Burgers" 0x002b2b88, others
0x002b2a98, float[4] each; +0x1240 the point it runs to, +0x1242 the point it came from
or its spawn tag 0x32/0x3c). In "Burgers" it attacks a burger stand (state 4); in "Pied
Pipers" it runs home to the player who hit it (state 5).

Roach fields: +0x1248 time on patrol, +0x124c short frames left of a dodge, +0x124e short
frames left before it may dodge again, +0x1250 time on the way to the point, +0x1264 the
model's yaw offset, +0x126c/+0x1270 speed, +0x127c float[4] the velocity it asked for,
+0x128c float[4] the velocity after the collision (set by the update).
*/

/* the roach turns its heading (+0x1278) towards the wanted one (+0x1274) by 0.1 a frame */
static float bg_RoachTurn(void *self)
{
	float turn;

	turn = bg_WrapDelta(BG_F(0x1274) - BG_F(0x1278));
	if (DAT_0028fda8 * 0.1f < turn)
		turn = DAT_0028fda8 * 0.1f;
	if (turn < DAT_0028fda8 * -0.1f)
		turn = DAT_0028fda8 * -0.1f;
	BG_F(0x1278) = turn + BG_F(0x1278);
	BG_F(0x1278) = CLMath_WrapAngle(BG_F(0x1278));
	return BG_F(0x1278);
}

/* the roach ran into something: its velocity changed since it asked for it */
static int bg_RoachBlocked(void *self)
{
	return BG_F(0x127c) != BG_F(0x128c) || (BG_F(0x1284) != BG_F(0x1294) && BG_F(0x1278) == BG_F(0x1274));
}

/* the velocity it asked for this frame */
static void bg_SaveVelocity(void *self)
{
	BG_I(0x127c) = BA_I(0x194);
	BG_I(0x1280) = BA_I(0x198);
	BG_I(0x1284) = BA_I(0x19c);
	BG_I(0x1288) = BA_I(0x1a0);
}

/* 0x0017ac20  (gap): the roach picks its next patrol point (+0x1240). In "Pied Pipers"
(away from points 6, 8, 9, 10) one of 6, 8, 9, 10. Otherwise a random point 5..9; the
original first draws other candidates for some points (7, 8, 50, 60 and the edge points)
that its range check always rejects, so only their random draws remain. */
void Bug_Roach_ChoosePoint(void *self)
{
	short from = BG_S(0x1242);
	int r;

	if (mw_Variation == 3 && from != 6 && from != 8 && from != 9 && from != 10) {
		r = ((int)Core_Rand() % 4) * 2 + 6;
		if (r == 0xc)
			r = 9;
		BG_S(0x1240) = (short)r;
		return;
	}
	switch (from) {
	case 1: case 2: case 3: case 4: case 5: case 6:
	case 9: case 10: case 11: case 12: case 13: case 14:
		(void)((int)Core_Rand() % 3);
		break;
	case 7:
	case 8:
		(void)Core_Rand();
		break;
	case 0x32:
	case 0x3c:
		(void)BG_RAND();
		break;
	default:
		break;
	}
	BG_S(0x1240) = (short)(BG_RAND() % 5 + 5);
}

/* 0x0017b870  (gap): a spawn point of "Brawl": 2 doors in the back wall (running in -z
... the heading -pi/2) or 3 on the left (running +x, heading 0), speed 7 */
void Bug_SpawnBrawl(void *self)
{
	int r = BG_RAND();

	BA_F(0xdc) = 1.0f;
	switch ((short)(r % 5)) {
	case 0:
	case 1:
		BA_F(0xd0) = (short)(r % 5) == 0 ? 1240.0f : -720.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = 1100.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 0.0f;
		BA_F(0x19c) = 7.0f;
		BG_F(0x1278) = -1.5707963f;
		BG_F(0x1274) = -1.5707963f;
		return;
	case 2:
		BA_F(0xd0) = -1050.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = 440.0f;
		break;
	case 3:
		BA_F(0xd0) = -1050.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = 160.0f;
		break;
	case 4:
		BA_F(0xd0) = -1050.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = -720.0f;
		break;
	default:
		return;
	}
	BA_F(0x19c) = 0.0f;
	BA_F(0x198) = 0.0f;
	BA_F(0x194) = 7.0f;
	BG_F(0x1278) = 0.0f;
	BG_F(0x1274) = 0.0f;
}

/* 0x0017b9c0  (gap): a spawn point of "Burgers": 4 openings 800 up at the sides */
void Bug_SpawnBurgers(void *self)
{
	int r = BG_RAND();

	BA_F(0xdc) = 1.0f;
	switch (r & 3) {
	case 0:
	case 1:
		BA_F(0xd0) = -1490.0f;
		BA_F(0xd4) = 800.0f;
		BA_F(0xd8) = (r & 3) == 0 ? 200.0f : -400.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 0.0f;
		BA_F(0x19c) = 7.0f;
		BG_F(0x1278) = 0.0f;
		BG_F(0x1274) = 0.0f;
		return;
	case 2:
	case 3:
		BA_F(0xd0) = 1510.0f;
		BA_F(0xd4) = 800.0f;
		BA_F(0xd8) = (r & 3) == 2 ? 200.0f : -400.0f;
		break;
	}
	BA_F(0x198) = 0.0f;
	BA_F(0x194) = 0.0f;
	BA_F(0x19c) = 7.0f;
	BG_F(0x1278) = 3.1415925f;
	BG_F(0x1274) = 3.1415925f;
}

/* 0x0017bb40  (gap): a spawn point of "Pied Pipers": the 5 of "Brawl", 3 in the front
wall (heading pi/2) and 2 on the right (heading pi). Returns 0. */
int Bug_SpawnPiedPipers(void *self)
{
	int r = BG_RAND();
	short pick = (short)(r % 10);

	BA_F(0xdc) = 1.0f;
	switch (pick) {
	case 0:
	case 1:
		BA_F(0xd0) = pick == 0 ? 1240.0f : -720.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = 1100.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 0.0f;
		BA_F(0x19c) = 7.0f;
		BG_F(0x1278) = -1.5707963f;
		BG_F(0x1274) = -1.5707963f;
		return 0;
	case 2:
	case 3:
	case 4:
		BA_F(0xd0) = -1050.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = pick == 2 ? 440.0f : pick == 3 ? 160.0f : -720.0f;
		BA_F(0x19c) = 0.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 7.0f;
		BG_F(0x1278) = 0.0f;
		BG_F(0x1274) = 0.0f;
		return 0;
	case 5:
	case 6:
	case 7:
		BA_F(0xd0) = pick == 5 ? -380.0f : pick == 6 ? 10.0f : 420.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = -1500.0f;
		BA_F(0x19c) = 0.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 7.0f;
		BG_F(0x1278) = 1.5707963f;
		BG_F(0x1274) = 1.5707963f;
		return 0;
	case 8:
	case 9:
		BA_F(0xd0) = 1650.0f;
		BA_F(0xd4) = 0.0f;
		BA_F(0xd8) = pick == 8 ? -430.0f : 40.0f;
		BA_F(0x19c) = 0.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 7.0f;
		BG_F(0x1278) = 3.1415925f;
		BG_F(0x1274) = 3.1415925f;
		return 0;
	}
	return 0;
}

/* 0x0017c490  (gap): roach, state 0: appears at a spawn point of the variation and runs
in for 50 frames, then starts its patrol */
void Bug_Roach_Start(void *self)
{
	short pick;

	if (BG_F(0x11f0) == 0.0f) {
		BG_F(0x1270) = 5.0f;
		BG_F(0x126c) = 5.0f;
		Actor_PlayAnim(BG_ACTOR, 0, 1, 0, 1.0f, 0.1f, 1);
		BG_I(0x1248) = 0;
		if (mw_Variation == 0) {
			Bug_SpawnBrawl(self);
		} else if (mw_Variation == 3) {
			Bug_SpawnPiedPipers(self);
		} else if (mw_Variation == 2) {
			Bug_SpawnBurgers(self);
		} else {
			/* "Tremors": 2 holes on the right (heading pi) or one on the left */
			BA_F(0xd4) = 0.0f;
			pick = (short)(BG_RAND() % 3);
			if (pick == 0 || pick == 1) {
				BA_F(0xd0) = 1700.0f;
				BA_F(0xd8) = pick == 0 ? 400.0f : -600.0f;
				BG_S(0x11f8) = 0;
				BG_F(0x1278) = 3.1415925f;
				BG_F(0x1274) = 3.1415925f;
			} else {
				BA_F(0xd0) = mw_Variation == 0 ? -1000.0f : -1245.0f;
				BA_F(0xd8) = pick == 2 ? -151.0f : -703.0f;
				BG_S(0x11f8) = 0;
				BG_F(0x1278) = 0.0f;
				BG_F(0x1274) = 0.0f;
				BG_S(0x1242) = 0x32;
				goto face;
			}
		}
		BG_S(0x1242) = 0x3c;
	} else if (0.0f < BG_F(0x11f0)) {
		if (mw_Variation == 2) {
			BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * 0.5f;
			BA_F(0x194) = (float)(cos((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 5.0);
			BA_F(0x19c) = (float)(sin((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 5.0);
		} else {
			BA_F(0x198) = 0.0f;
			BA_F(0x194) = (float)(cos((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 2.5);
			BA_F(0x19c) = (float)(sin((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 2.5);
		}
		if (50.0f < BG_F(0x11f0)) {
			BG_S(0x11ee) = 2;
			BG_F(0x11f0) = -1.0f;
			Bug_Roach_ChoosePoint(self);
			BG_F(0x1270) = (float)BG_RAND() * 5.0f * 3.0518509e-05f + 5.0f;
			BG_F(0x1278) = BG_F(0x1278) + 1.5707963f;
			return;
		}
	}
face:
	BA_F(0x1e8) = ((1.5707963f - BG_F(0x1278)) - BG_F(0x1264)) + 3.1415925f;
}

/* 0x0017c7b0  (gap): roach, state 2: patrols: runs to its point (+0x1240), picks the
next one there or after 300 frames; dodges by 90 degrees when it runs into something.
In "Burgers" it goes for a burger stand after (5 - difficulty) x 40 frames. */
void Bug_Roach_Patrol(void *self)
{
	float dir[4];
	float *point;

	BG_F(0x1248) = DAT_0028fda8 + BG_F(0x1248);
	BG_F(0x1250) = DAT_0028fda8 + BG_F(0x1250);
	if (mw_Variation == 2)
		point = MW_FV(0x002b2b88 + BG_S(0x1240) * 0x10);
	else
		point = MW_FV(0x002b2a98 + BG_S(0x1240) * 0x10);
	dir[0] = point[0] - BA_F(0xd0);
	dir[1] = point[1] - BA_F(0xd4);
	dir[2] = point[2] - BA_F(0xd8);
	bmVectorNormalise(dir, dir);
	if (mw_Variation == 2 && (float)((5 - DAT_002bae70) * 0x28) <= BG_F(0x1248)) {
		BG_S(0x1204) = CBbash_Arena_ChoosePlayerToAttack(mw_Arena);
		BG_I(0x11f0) = 0;
		BG_S(0x11ee) = 4;
		BG_S(0x124e) = 200;
	}
	if (BG_S(0x124c) == 0) {
		BG_F(0x1274) = mw_Heading(dir[0], -dir[2]);
		if (BG_S(0x124e) != 0)
			BG_S(0x124e) = (short)(BG_S(0x124e) - 1);
	} else {
		BG_S(0x124c) = (short)(BG_S(0x124c) - 1);
	}
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * 0.5f;
	BA_F(0x194) = (float)(cos((double)BG_F(0x1278) - (double)1.5707963f) * (double)DAT_0028fda8 *
		(double)BG_F(0x1270));
	BA_F(0x19c) = (float)(sin((double)BG_F(0x1278) - (double)1.5707963f) * (double)DAT_0028fda8 *
		(double)BG_F(0x1270));
	BA_F(0x1e8) = (3.1415925f - bg_RoachTurn(self)) + 3.1415925f;
	if (Bug_ReachedPoint(self) != 0 || 300.0f < BG_F(0x1250)) {
		if (300.0f < BG_F(0x1250))
			bkPrintf("Roach giving up on patrol point\n");
		BG_S(0x1242) = BG_S(0x1240);
		Bug_Roach_ChoosePoint(self);
		BG_I(0x1250) = 0;
	}
	if (bg_RoachBlocked(self) && BG_S(0x124c) == 0 && BG_S(0x124e) == 0) {
		(void)BG_RAND();
		BG_F(0x1274) = BG_F(0x1274) - 1.5707963f;
		BG_S(0x124c) = (short)(BG_RAND() % 5 + 5);
		BG_S(0x124e) = (short)(BG_RAND() % 5 + 5);
	}
	bg_SaveVelocity(self);
}

/* 0x0017a530  (gap): roach, state 5 ("Pied Pipers"): runs home to the base of the player
who hit it (+0x120c, bases at 0x002b2d08); there it is gone and he scores the roach value
(0x0030ee1c), shown as a floating number, with a sound */
void Bug_Roach_GoHome(void *self)
{
	char text[32];
	float *home;
	void *player;
	float turn;

	if (BG_F(0x11f0) == 0.0f)
		BG_S(0x1240) = BG_S(0x120c);
	home = MW_FV(0x002b2d08 + BG_S(0x1240) * 0x10);
	BG_F(0x1274) = mw_Heading(BA_F(0xd0) - home[0], BA_F(0xd8) - home[2]);
	BG_F(0x1274) = CLMath_WrapAngle(-BG_F(0x1274));
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_F(0x198) = BA_F(0x198) - 10.0f;
	BA_F(0x194) = (float)(cos((double)BG_F(0x1278) - (double)1.5707963f) * (double)BG_F(0x1270) *
		(double)DAT_0028fda8);
	BA_F(0x19c) = (float)(sin((double)BG_F(0x1278) - (double)1.5707963f) * (double)BG_F(0x1270) *
		(double)DAT_0028fda8);
	turn = bg_WrapDelta(BG_F(0x1274) - BG_F(0x1278));
	if (DAT_0028fda8 * 0.1f < turn)
		turn = DAT_0028fda8 * 0.1f;
	if (turn < DAT_0028fda8 * -0.1f)
		turn = DAT_0028fda8 * -0.1f;
	BG_F(0x1278) = turn + BG_F(0x1278);
	BG_F(0x1278) = CLMath_WrapAngle(BG_F(0x1278));
	bg_SaveVelocity(self);
	BA_F(0x1e8) = (3.1415925f - BG_F(0x1278)) + 3.1415925f;
	if (bmVectorDistance(BA_V(0xd0), MW_FV(0x002b2d08 + BG_S(0x1240) * 0x10)) < 150.0f) {
		BG_S(0x11fa) = 0;
		BG_S(0x11fc) = 1;
		MW_AI(mw_Players[BG_S(0x120c)], 0x1400) = MW_AI(mw_Players[BG_S(0x120c)], 0x1400) + MW_I(0x0030ee1c);
		sprintf(text, "%d", MW_I(0x0030ee1c));
		player = mw_Players[BG_S(0x120c)];
		baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)BA_F(0xd0), 340.0,
			(double)BA_F(0xd8), (int)MW_AS(player, 0x136e), (int)MW_AS(player, 0x1370), (int)MW_AS(player, 0x1372),
			text, 1.0);
		FUN_0017cf10(BG_RAND() % 6 + 9, 0, 0xff);
	}
}

/* 0x0017a830  (gap): roach, state 4 ("Burgers"): runs to a burger stand (+0x1204) and
bites it (0.11/0.16/0.21 of its size by difficulty, with a crumb splat and a sound),
then patrols again; gives up when it runs into something */
void Bug_Roach_AttackStand(void *self)
{
	float dir[4], mid[4];
	float distSq, size;
	float *stand;
	int index;

	BG_F(0x1250) = DAT_0028fda8 + BG_F(0x1250);
	stand = MW_FV(0x002b2718 + BG_S(0x1204) * 0x10);
	dir[0] = stand[0] - BA_F(0xd0);
	dir[2] = stand[2] - BA_F(0xd8);
	dir[1] = 0.0f;
	if (BG_S(0x124e) != 0)
		BG_S(0x124e) = (short)(BG_S(0x124e) - 1);
	distSq = dir[2] * dir[2] + dir[0] * dir[0];
	bmVectorNormalise(dir, dir);
	BG_F(0x1274) = mw_Heading(dir[0], -dir[2]);
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_F(0x198) = BA_F(0x198) - 10.0f;
	BA_F(0x194) = (float)(cos((double)BG_F(0x1278) - (double)1.5707963f) * (double)DAT_0028fda8 *
		(double)BG_F(0x1270));
	BA_F(0x19c) = (float)(sin((double)BG_F(0x1278) - (double)1.5707963f) * (double)DAT_0028fda8 *
		(double)BG_F(0x1270));
	index = BG_S(0x1204);
	if (sqrtf(distSq) < MW_F(0x0030ea00) * MW_F(0x0030e9f0 + index * 4) + MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x9c) + 10.0f) {
		stand = MW_FV(0x002b2718 + index * 0x10);
		size = MW_F(0x0030e9f0 + index * 4);
		if (DAT_002bae70 == 2)
			size = size - 0.21f;
		else if (DAT_002bae70 == 1)
			size = size - 0.16f;
		else
			size = size - 0.11f;
		MW_F(0x0030e9f0 + index * 4) = size;
		if (MW_F(0x0030e9f0 + BG_S(0x1204) * 4) < 0.0f)
			MW_I(0x0030e9f0 + BG_S(0x1204) * 4) = 0;
		mid[0] = (stand[0] + BA_F(0xd0)) * 0.5f;
		mid[1] = (BA_F(0xd4) + stand[1]) * 0.5f;
		mid[2] = (BA_F(0xd8) + stand[2]) * 0.5f;
		if (0.0f < MW_F(0x0030e9f0 + BG_S(0x1204) * 4)) {
			FUN_00184760(mid, 0);
			FUN_0017cf10(0x22, 0, 0xff);
		}
		if (MW_F(0x0030e9f0 + BG_S(0x1204) * 4) < 0.0f)
			MW_I(0x0030e9f0 + BG_S(0x1204) * 4) = 0;
		BG_S(0x11ee) = 2;
		BG_I(0x1248) = 0;
	}
	BA_F(0x1e8) = (3.1415925f - bg_RoachTurn(self)) + 3.1415925f;
	if (bg_RoachBlocked(self) && BG_S(0x124e) == 0) {
		BG_S(0x11ee) = 2;
		if (mw_DbgMessages != 0)
			bkPrintf("roach problems reaching attack zone going back to patrol\n");
		BG_I(0x1248) = 0;
		BG_I(0x1250) = 0;
		BG_I(0x11f0) = 0;
		(void)BG_RAND();
		BG_F(0x1274) = BG_F(0x1274) - 1.5707963f;
	}
	bg_SaveVelocity(self);
}

/* 0x0017cd90  (gap): the roach's state machine */
void Bug_Roach_Update(void *self)
{
	switch (BG_S(0x11ee)) {
	case 0:
		Bug_Roach_Start(self);
		break;
	default:
		if (mw_DbgMessages != 0)
			bkPrintf("Error confused roach\n");
		break;
	case 2:
		Bug_Roach_Patrol(self);
		break;
	case 3:
		Bug_Roach_Dead(self);
		break;
	case 4:
		Bug_Roach_AttackStand(self);
		break;
	case 5:
		Bug_Roach_GoHome(self);
		break;
	}
	BG_F(0x11f0) = DAT_0028fda8 + BG_F(0x11f0);
}

/* ======================================================================== worms (type 0, 5, 10)

Worms wander with a wiggle: +0x1264 the wiggle angle (+-45 degrees), +0x1268 short its
direction (+1/-1), +0x1004 short frames to the next random turn, +0x126c speed,
+0x1270 the speed it wants, +0x1248 time since the last bite. */

/* the worm's heading (+0x1278) turns towards the wanted one by at most 0.1 a frame; its
model faces the heading less the wiggle; the velocity is saved */
static void bg_WormTurn(void *self)
{
	float turn;

	turn = bg_WrapDelta(BG_F(0x1274) - BG_F(0x1278));
	if (DAT_0028fda8 * 0.1f < turn)
		turn = DAT_0028fda8 * 0.1f;
	if (turn < DAT_0028fda8 * -0.1f)
		turn = DAT_0028fda8 * -0.1f;
	BG_F(0x1278) = turn + BG_F(0x1278);
	BG_F(0x1278) = CLMath_WrapAngle(BG_F(0x1278));
	BA_F(0x1e8) = (float)((((double)1.5707963f - (double)BG_F(0x1278)) - (double)BG_F(0x1264)) +
		(double)3.1415925f);
	bg_SaveVelocity(self);
}

/* a new random wander: a heading within 90 degrees of straight back, a speed 7..14 and
100 frames until the next one */
static float bg_WormNewWander(void *self)
{
	float heading, r;

	heading = (float)BG_RAND() * 1.5707963f * 3.0518509e-05f + 3.1415925f + BG_F(0x1278);
	BG_F(0x1274) = heading;
	if (6.2831855f < heading)
		BG_F(0x1274) = heading - 6.2831855f;
	r = (float)BG_RAND() * 7.0f * 3.0518509e-05f;
	BG_S(0x1004) = 100;
	BG_F(0x1270) = r + 7.0f;
	return r;
}

/* 0x0017adb0  (gap): worm, state 5: drops in at the far end (x 3200..3350, 530 up,
z 2250..2840) running +x at 7, a random heading, then runs for 50 frames and wanders */
void Bug_Worm_DropIn(void *self)
{
	float heading;

	if (BG_F(0x11f0) == 0.0f) {
		BA_F(0xd0) = (float)(BG_RAND() % 0x96) + 3200.0f;
		BA_F(0xd4) = 530.0f;
		BA_F(0xd8) = (float)(BG_RAND() % 0x24e) + 2250.0f;
		BA_F(0x19c) = 0.0f;
		BA_F(0x198) = 0.0f;
		BA_F(0x194) = 7.0f;
		heading = (float)BG_RAND() * 9.587672e-05f;
		BG_F(0x1270) = 7.0f;
		BG_F(0x126c) = 7.0f;
		BG_F(0x1278) = heading;
		BG_F(0x1274) = heading;
		Actor_PlayAnim(BG_ACTOR, 0, 1, 0, 1.0f, 0.1f, 1);
		BG_I(0x1248) = 0;
	} else if (0.0f < BG_F(0x11f0)) {
		if (mw_Variation == 2)
			BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * 0.5f;
		else
			BA_F(0x198) = 0.0f;
		BA_F(0x194) = (float)(cos((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 7.0);
		BA_F(0x19c) = (float)(sin((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 7.0);
		if (50.0f < BG_F(0x11f0)) {
			BG_S(0x11ee) = 2;
			BG_F(0x11f0) = -1.0f;
		}
	}
	BA_F(0x1e8) = BG_F(0x1278) + 1.5707963f;
}

/* 0x0017afb0  (gap): worm, state 4 ("Pied Pipers"): runs home to the base of the player
who hit it (+0x120c); there it is gone and he scores the worm value (0x0030ee18) */
void Bug_Worm_GoHome(void *self)
{
	char text[32];
	float *home;
	void *player;

	if (BG_F(0x11f0) == 0.0f)
		BG_S(0x1240) = BG_S(0x120c);
	home = MW_FV(0x002b2d08 + BG_S(0x1240) * 0x10);
	BG_F(0x1274) = mw_Heading(BA_F(0xd0) - home[0], BA_F(0xd8) - home[2]);
	BG_F(0x1274) = CLMath_WrapAngle(4.712389f - BG_F(0x1274));
	BG_I(0x1264) = 0;
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * 10.0f;
	BA_F(0x194) = (float)(cos((double)BG_F(0x1278) + (double)BG_F(0x1264)) * (double)DAT_0028fda8 * 7.0);
	BA_F(0x19c) = (float)(sin((double)BG_F(0x1278) + (double)BG_F(0x1264)) * (double)DAT_0028fda8 * 7.0);
	bg_WormTurn(self);
	if (bmVectorDistance(BA_V(0xd0), MW_FV(0x002b2d08 + BG_S(0x1240) * 0x10)) < 150.0f) {
		BG_S(0x11fa) = 0;
		BG_S(0x11fc) = 1;
		MW_AI(mw_Players[BG_S(0x120c)], 0x1400) = MW_AI(mw_Players[BG_S(0x120c)], 0x1400) + MW_I(0x0030ee18);
		sprintf(text, "%d", MW_I(0x0030ee18));
		player = mw_Players[BG_S(0x120c)];
		baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318), (double)BA_F(0xd0), 340.0,
			(double)BA_F(0xd8), (int)MW_AS(player, 0x136e), (int)MW_AS(player, 0x1370), (int)MW_AS(player, 0x1372),
			text, 1.0);
		FUN_0017cf10(BG_RAND() % 6 + 9, 0, 0xff);
	}
}

/* 0x0017b2d0  (gap): worm, state 2: wanders with a wiggle, turns away when it runs into
something or every 100 frames, eases its speed by at most 0.5 a frame. In "Burgers" it
bites a burger stand it touches (0.02, hard 0.04; a crumb splat and a sound) and turns
back, at most once in 30 frames. */
void Bug_Worm_Wander(void *self)
{
	float delta[4], mid[4];
	float reach, step;
	float *stand, *size;
	int k;

	if (BG_S(0x1268) == 1) {
		BG_F(0x1264) = DAT_0028fda8 * 0.03f + BG_F(0x1264);
		if (0.7853981f < BG_F(0x1264))
			BG_S(0x1268) = -1;
	} else if (BG_S(0x1268) == -1) {
		BG_F(0x1264) = BG_F(0x1264) - DAT_0028fda8 * 0.03f;
		if (BG_F(0x1264) < -0.7853981f)
			BG_S(0x1268) = 1;
	}
	if (0.0f < BA_F(0x198))
		BA_F(0x198) = 0.0f;
	BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * 0.5f;
	BA_F(0x194) = (float)(cos((double)BG_F(0x1278) + (double)BG_F(0x1264)) * (double)DAT_0028fda8 * 7.0);
	BA_F(0x19c) = (float)(sin((double)BG_F(0x1278) + (double)BG_F(0x1264)) * (double)DAT_0028fda8 * 7.0);
	if (bg_RoachBlocked(self))
		bg_WormNewWander(self);
	BG_S(0x1004) = (short)(BG_S(0x1004) - 1);
	BG_F(0x1248) = BG_F(0x1248) + 1.0f;
	/* the speed step; when the wander timer runs out the original uses the new random
	part of the wanted speed instead (a reused temporary) */
	step = BG_F(0x1270) - BG_F(0x126c);
	if (BG_S(0x1004) == 0)
		step = bg_WormNewWander(self);
	if (mw_Variation == 2 && 30.0f < BG_F(0x1248)) {
		for (k = 0; k < 4; k++) {
			stand = MW_FV(0x002b2718 + k * 0x10);
			size = MW_FV(0x0030e9f0 + k * 4);
			delta[0] = stand[0] - BA_F(0xd0);
			delta[1] = stand[1] - BA_F(0xd4);
			delta[2] = stand[2] - BA_F(0xd8);
			reach = MW_F(0x0030ea00) * *size + MW_AF(MW_AP(BG_ACTOR, 0x18c), 0x9c) + 10.0f;
			if (sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]) < reach) {
				*size = *size - (DAT_002bae70 == 2 ? 0.04f : 0.02f);
				if (*size < 0.0f)
					*size = 0.0f;
				mid[0] = (stand[0] + BA_F(0xd0)) * 0.5f;
				mid[1] = (BA_F(0xd4) + stand[1]) * 0.5f;
				mid[2] = (BA_F(0xd8) + stand[2]) * 0.5f;
				if (0.0f < *size) {
					FUN_00184760(mid, 0);
					FUN_0017cf10(0x22, 0, 0xff);
				}
				bmVectorSetLength(delta, delta, reach);
				if (*size < 0.0f)
					*size = 0.0f;
				BG_I(0x1248) = 0;
				BG_F(0x1274) = CLMath_WrapAngle(BG_F(0x1274) + 3.1415925f);
			}
		}
	}
	if (0.5f < step)
		step = 0.5f;
	else if (step < -0.5f)
		step = -0.5f;
	BG_F(0x126c) = step + BG_F(0x126c);
	bg_WormTurn(self);
}

/* 0x0017cbd0  (gap): worm, state 0: appears at a spawn point of "Brawl", "Burgers" or
"Pied Pipers", runs in for 50 frames at 7, then wanders */
void Bug_Worm_Start(void *self)
{
	if (BG_F(0x11f0) == 0.0f) {
		BG_F(0x1270) = 7.0f;
		BG_F(0x126c) = 7.0f;
		Actor_PlayAnim(BG_ACTOR, 0, 1, 0, 1.0f, 0.1f, 1);
		BG_I(0x1248) = 0;
		if (mw_Variation == 0) {
			Bug_SpawnBrawl(self);
			BG_S(0x1242) = 0;
		} else if (mw_Variation == 3) {
			Bug_SpawnPiedPipers(self);
			BG_S(0x1242) = 0;
		} else if (mw_Variation == 2) {
			Bug_SpawnBurgers(self);
			BG_S(0x1242) = 0;
		}
	} else if (0.0f < BG_F(0x11f0)) {
		if (mw_Variation == 2)
			BA_F(0x198) = BA_F(0x198) - DAT_0028fda8 * 0.5f;
		else
			BA_F(0x198) = 0.0f;
		BA_F(0x194) = (float)(cos((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 7.0);
		BA_F(0x19c) = (float)(sin((double)BG_F(0x1278)) * (double)DAT_0028fda8 * 7.0);
		if (50.0f < BG_F(0x11f0)) {
			BG_S(0x11ee) = 2;
			BG_F(0x11f0) = -1.0f;
		}
	}
	BA_F(0x1e8) = BG_F(0x1278) + 1.5707963f;
}

/* 0x0017ce20  (gap): the worm's state machine */
void Bug_Worm_Update(void *self)
{
	switch (BG_S(0x11ee)) {
	case 0:
		Bug_Worm_Start(self);
		break;
	case 2:
		Bug_Worm_Wander(self);
		break;
	case 3:
		Bug_Worm_Dead(self);
		break;
	case 4:
		Bug_Worm_GoHome(self);
		break;
	case 5:
		Bug_Worm_DropIn(self);
		break;
	}
	BG_F(0x11f0) = DAT_0028fda8 + BG_F(0x11f0);
}

/* 0x0017c360  (gap): the train's state machine */
void Bug_Train_Update(void *self)
{
	if (BG_S(0x11ee) == 0)
		Bug_Train_Start(self);
	else if (BG_S(0x11ee) == 1)
		Bug_Train_Run(self);
	BG_F(0x11f0) = DAT_0028fda8 + BG_F(0x11f0);
}

/* 0x0017c3a0  (gap): the fly's state machine */
void Bug_Fly_Update(void *self)
{
	switch (BG_S(0x11ee)) {
	case 0:
		Bug_Fly_Start(self);
		break;
	case 2:
		Bug_Fly_Wander(self);
		break;
	case 3:
		Bug_Fly_Knocked(self);
		break;
	case 4:
		Bug_Fly_Die(self);
		break;
	case 5:
		Bug_Fly_Attack(self);
		break;
	}
	BG_F(0x11f0) = DAT_0028fda8 + BG_F(0x11f0);
}

/* 0x0017c410  (gap): the tremor's state machine */
void Bug_Tremor_Update(void *self)
{
	switch (BG_S(0x11ee)) {
	case 0:
		Bug_Tremor_Rise(self);
		break;
	case 1:
		Bug_Tremor_Watch(self);
		break;
	case 2:
		Bug_Tremor_Attack(self);
		break;
	case 4:
		Bug_Tremor_Die(self);
		break;
	case 6:
		Bug_Tremor_InHole(self);
		break;
	}
	BG_F(0x11f0) = DAT_0028fda8 + BG_F(0x11f0);
}

/* 0x0017bd90  (gap): spore, state 2: hit: turns to its cloud animation (facing the hitter
or not), pulses in size, and after 65 frames bursts: an explosion, a sound, a camera
shake, a splat, the blast rings; the players within 700 are knocked down and lose points
(the hitter 0x0030ee28, the others 0x0030ee2c); the other live bugs within 700 are hit
(they score for the hitter unless he was caught in the blast himself) */
void Bug_Spore_Burst(void *self)
{
	char text[32];
	float heading;
	unsigned char *node;
	void **helper;
	void *player;
	int scored, lost, k;
	short i;

	text[0] = 0;
	if (BG_F(0x11f0) == 0.0f) {
		heading = CLMath_HeadingTo(MW_AV(MW_AP(mw_Players[BG_S(0x120c)], 0x30), 0xd0), BA_V(0xd0));
		heading = CLMath_AngleDistance(heading, BA_F(0x1e8));
		BG_F(0x1248) = 1.0f;
		BG_F(0x1264) = 0.3f;
		if (1.5707963f < heading) {
			Actor_PlayAnim(BG_ACTOR, 1, 0, 0, 1.0f, 0.0f, 1);
			baFlushAnimations((char *)self + 0x1070, (struct BActorNode *)MW_AP(self, 0x11a4), 1, 0);
			baQueueAnimation((struct BActorInstance *)((char *)self + 0x1070), (struct BActorNode *)MW_AP(self, 0x11a4),
				(struct BAnim *)MW_AP(self, 0x11e4), 0x400, -1, 1, NULL, NULL);
		} else {
			Actor_PlayAnim(BG_ACTOR, 2, 0, 0, 1.0f, 0.0f, 1);
			baFlushAnimations((char *)self + 0x1070, (struct BActorNode *)MW_AP(self, 0x11a4), 1, 0);
			baQueueAnimation((struct BActorInstance *)((char *)self + 0x1070), (struct BActorNode *)MW_AP(self, 0x11a4),
				(struct BAnim *)MW_AP(self, 0x11e8), 0x400, -1, 1, NULL, NULL);
		}
	}
	baPrepActorInstance((struct BActorInstance *)((char *)self + 0x1070), 1);
	baUpdateAnimations((struct BActorInstance *)((char *)self + 0x1070), (struct BActorNode *)MW_AP(self, 0x11a4),
		DAT_0028fda8, NULL);
	bg_PlaceExtraInstance(self);
	baPrepActorInstance((struct BActorInstance *)((char *)self + 0x1070), 1);
	BA_F(0x198) = -(DAT_0028fda8 * 10.0f);
	if (5.0f <= BG_F(0x11f0)) {
		/* the pulse: scale 1.5..2.5 */
		BA_F(0x194) = BA_F(0x194) * 0.3f;
		BA_F(0x19c) = BA_F(0x19c) * 0.3f;
		BG_F(0x1248) = DAT_0028fda8 * BG_F(0x1264) + BG_F(0x1248);
		if (1.5f < BG_F(0x1248)) {
			BG_F(0x1264) = -0.2f;
			BG_F(0x1248) = 1.5f;
		}
		if (BG_F(0x1248) < 0.5f) {
			BG_F(0x1264) = 0.2f;
			BG_F(0x1248) = 0.5f;
		}
		BA_F(0xe8) = BG_F(0x1248) + 1.0f;
		BA_F(0xe4) = BA_F(0xe8);
		BA_F(0xe0) = BA_F(0xe4);
	}
	if (65.0f <= BG_F(0x11f0)) {
		FUN_00185d70(BA_V(0xd0));
		FUN_0017cf10(5, 5000, 0xff);
		CCamera_Shake(mw_Camera, 0x19, 0x32);
		FUN_001842b0(self, 7);
		/* every free blast ring starts here */
		helper = mw_Helpers;
		for (k = 10; k != 0; k--, helper++) {
			if (MW_AS(*helper, 0x10) == 0) {
				MW_AS(*helper, 0x10) = 1;
				MW_AI(*helper, 0x18) = 0;
				memcpy(*helper, BA_V(0xd0), 0x10);
				MW_AF(*helper, 0x14) = 700.0f;
			}
		}
		scored = 1;
		BG_S(0x11fa) = 0;
		BG_S(0x11fc) = 0;
		for (i = 0; i < 4; i++) {
			player = mw_Players[i];
			if (!(bmVectorDistance(BA_V(0xd0), MW_AV(MW_AP(player, 0x30), 0xd0)) < 700.0f))
				continue;
			if (i == BG_S(0x120c))
				scored = 0;
			FUN_0017ed40(player, BA_V(0xd0), 1);
			if (i == BG_S(0x120c)) {
				MW_AI(player, 0x1400) = MW_AI(player, 0x1400) - MW_I(0x0030ee28);
				lost = MW_I(0x0030ee28);
			} else {
				MW_AI(player, 0x1400) = MW_AI(player, 0x1400) - MW_I(0x0030ee2c);
				lost = MW_I(0x0030ee2c);
			}
			if (MW_AI(player, 0x1400) < 0)
				MW_AI(player, 0x1400) = 0;
			sprintf(text, "%d", -lost);
			baAddExtraItem((struct BExtraSystem *)MW_P(0x0030ea70), MW_P(0x0030a318),
				(double)MW_AF(MW_AP(player, 0x30), 0xd0), 340.0, (double)MW_AF(MW_AP(player, 0x30), 0xd8),
				(int)MW_AS(player, 0x136e), (int)MW_AS(player, 0x1370), (int)MW_AS(player, 0x1372), text, 1.0);
		}
		for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, NULL); node != NULL;
		     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ee30, (unsigned int *)node)) {
			unsigned char *bug = node - 8;

			if (MW_AS(bug, 0x11fa) != 0 && MW_AS(bug, 0x11ec) != 2 && bug != (unsigned char *)self &&
			    bmVectorDistance(BA_V(0xd0), MW_AV(MW_AP(bug, 0), 0xd0)) < 700.0f) {
				MW_AS(bug, 0x11fc) = 1;
				MW_AS(bug, 0x120c) = BG_S(0x120c);
				Bug_Hit(bug, scored);
			}
		}
	}
	/* the lean dies away */
	BG_F(0x1234) = BG_F(0x1234) * 0.5f;
	bg_ApplyLean(self);
}

/* 0x0017cd40  (gap): the spore's state machine */
void Bug_Spore_Update(void *self)
{
	short state = BG_S(0x11ee);

	if (state == 0)
		Bug_Spore_Start(self);
	else if (state == 1)
		Bug_Spore_Fly(self);
	else if (state == 2)
		Bug_Spore_Burst(self);
	BG_F(0x11f0) = DAT_0028fda8 + BG_F(0x11f0);
}

/* ======================================================================== sounds

The game's 49 samples: a table at 0x002b2e08 of {char name[32]; struct BSample *sample}
(0x24 bytes each), loaded from "sounds\<name>.wav". */

#define MW_SAMPLE(i) MW_G(0x002b2e28 + (i) * 0x24, struct BSample *)

/* 0x0017ce90  (gap): loads the 49 samples from the package */
void MW_LoadSounds(struct bkPackage *package)
{
	char path[256];
	struct bkResource *res;
	int i;

	for (i = 0; i < 0x31; i++) {
		sprintf(path, "sounds\\%s.wav", MW_A(0x002b2e08 + i * 0x24));
		res = bLoadResource(package, path, 2, 0xdefa);
		MW_G(0x002b2e28 + i * 0x24, struct bkResource *) = res;
		bkPrintf(res == NULL ? "error loading sample %s\n" : "sample %s loaded ok\n", path);
	}
}

/* 0x0017cf10  (gap): plays sample `index` at `volume`, its frequency shifted by
`freqChange` (0: its own); when no channel is free it takes channel index % 64. Returns
the channel or -1. */
int MW_PlaySound(unsigned int index, int freqChange, int volume)
{
	int channel, frequency = 0;

	if (MW_SAMPLE(index) == NULL) {
		bkPrintf("tried to play sound index %d sample not there \n", index);
		return -1;
	}
	if (mw_DbgMessages != 0)
		bkPrintf("playing sample %s freqChange %d\n", MW_A(0x002b2e08 + index * 0x24), freqChange);
	if (freqChange != 0)
		frequency = MW_AI(MW_SAMPLE(index), 0x20) + freqChange;
	channel = bsPlaySample(MW_SAMPLE(index), volume, 0, frequency, 0);
	if (channel == -1) {
		channel = (int)index % 64;
		bsStopChannel(channel);
		channel = bsPlaySampleOnChannel(channel, MW_SAMPLE(index), volume, 0, frequency, 1);
	}
	return channel;
}

/* 0x0017cfc0  (gap): plays sample `index` at the moving position `pos` (heard 300..5000) */
int MW_PlaySoundAt(unsigned int index, int freqChange, float *pos)
{
	int channel, frequency = 0;

	if (MW_SAMPLE(index) == NULL) {
		bkPrintf("tried to play sound index %d sample not there \n", index);
		return -1;
	}
	if (mw_DbgMessages != 0)
		bkPrintf("playing sample %s freqChange %d\n", MW_A(0x002b2e08 + index * 0x24), freqChange);
	if (freqChange != 0)
		frequency = MW_AI(MW_SAMPLE(index), 0x20) + freqChange;
	channel = bsPlaySampleMoving(MW_SAMPLE(index), pos, 0xff, frequency, 0, NULL, NULL, 300.0f, 5000.0f);
	if (channel == -1) {
		channel = (int)index % 64;
		bsStopChannel(channel);
		channel = bsPlaySampleMovingOnChannel(channel, MW_SAMPLE(index), pos, 0xff, frequency, 0, NULL, NULL, 300.0f,
			5000.0f);
	}
	return channel;
}

/* 0x0017d090  (gap): "stops" a sound channel: the original prints a warning and stops
channel -1 whatever `channel` is (it overwrites its own argument) */
void MW_StopSound(int channel)
{
	(void)channel;
	bkPrintf("asked to stop none existing sound channel!\n");
	bsStopChannel(-1);
}

/* 0x0017d0b0  (gap): a voice line of the player's character (thiscall: the player) */
void MW_Player_PlayVoice(void *player, int kind, int volume, float cooldown, unsigned int flags, int choice,
	int allowRepeat)
{
	CLSamples_PlayVoice((char)DAT_002baed4[MW_AT(MW_AP(player, 0x30), 0x230, unsigned short) * 10], kind, NULL, volume,
		cooldown, flags, choice, allowRepeat);
}

/* 0x0017d100  (gap): removes the bullet that just hit the player (fastcall: the player;
+0x1138 his character, whose collision (+0x18c) remembers the hitting instance at
+0x1bc while +0x1d0 is set): the bullet's node gets its dead flags (+8, +10) */
void MW_Player_KillHittingBullet(void *player)
{
	unsigned char *coll = (unsigned char *)MW_AP(MW_AP(player, 0x1138), 0x18c);
	unsigned char *node;

	if (MW_AI(coll, 0x1d0) == 0 || MW_AI(coll, 0x1bc) == 0)
		return;
	for (node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, NULL); node != NULL;
	     node = (unsigned char *)(uintptr_t)CLList_Next((unsigned int *)DAT_0030ed00, (unsigned int *)node)) {
		if (MW_AI(node, -0x19c) == MW_AI(MW_AP(MW_AP(player, 0x1138), 0x18c), 0x1bc)) {
			MW_AS(node, 8) = 1;
			MW_AS(node, 10) = 1;
		}
	}
}
