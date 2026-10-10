/*
ROLLERBALL_A_TEST.C

rollerball_a module tests (src/rollerball_a/tests/run_tests.sh builds this as the main() of a
small guest image: the rollerball_a objects, the generated stubs and the platform layer). They
check rules of the RollerBall minigame that the decompiled code must keep:

- a count-down timer counts game time only: a pause (DAT_002ecdec) does not use up the time,
  the remaining time never goes below 0 and the elapsed fraction never above 1;
- sumo and drop give 10, 5, 3, 1 points by rank, and players that share a rank share the
  points;
- the race gives 5, 3, 1, 0 points: players who finished keep their place, the others are
  placed by how far they got;
- a fallen player respawns at the nearest free respawn point, and two players never get the
  same point;
- the sub-system lists add at the head;
- a looping texture animation wraps to its first frame, a one-shot animation stops at its
  last frame, a ping-pong animation turns at both ends.

The game build compiles every .c under src/rollerball_a: without OPENFRENZY_ROLLERBALL_A_TEST
(set by run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_ROLLERBALL_A_TEST
#include "rollerball_a_ext.h"

static int checks, failures;
#define CHECK(cond, what) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (what)); \
		} \
	} while (0)

/* ---------- the other modules' globals that the rollerball_a objects use (their objects are
not in the test image) */

int DAT_00283e58, DAT_00283ea4, DAT_00283ef0;
unsigned int DAT_00285630;
float DAT_00285644;
float DAT_0028fda8 = 1.0f;
float DAT_0028ffa8, DAT_0028ffac;
float DAT_00290be4[4];
int DAT_00290e8c;
unsigned char DAT_00291734[0x40];
float DAT_00292c84;
int DAT_00293928;
unsigned char DAT_002b9968[0x100];
int DAT_002b9c48[4];
unsigned char DAT_002ba750[0x100];
int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c;
int DAT_002ba98c = 60;
void *DAT_002ba9a0;
int DAT_002bae70, DAT_002bae98, DAT_002baebc;
int DAT_002bc40c, DAT_002bc410;
void *DAT_002bc47c;
long long DAT_002cd7a0 = 1000;
float DAT_002ea478[4];
unsigned char DAT_002ecd90[0x100];
int DAT_002ecdec;
int DAT_002ecf90[4];
int DAT_002ecfac;
unsigned char DAT_002ed324[0x100];

/* core's player records: 4 of 0x28 bytes from 0x002baed8; the code names fields of them */
__attribute__((nocommon, aligned(16))) int rb_test_records[40];
__asm__(
	".globl DAT_002baed8\n.set DAT_002baed8, rb_test_records + 0x0\n"
	".globl DAT_002baee0\n.set DAT_002baee0, rb_test_records + 0x8\n"
	".globl DAT_002baee8\n.set DAT_002baee8, rb_test_records + 0x10\n"
	".globl DAT_002baf10\n.set DAT_002baf10, rb_test_records + 0x38\n"
	".globl DAT_002baf38\n.set DAT_002baf38, rb_test_records + 0x60\n"
	".globl DAT_002baf60\n.set DAT_002baf60, rb_test_records + 0x88\n");

/* rollerball_b's path record destructor (missing from the table, so it has no stub) */
void FUN_00198110(void *self)
{
	(void)self;
}

/* ---------- bKernel's heap (the C library's) and timer, driven by the test (1000 ticks a
second) */

void *bkHeapAlloc(uint size, const char *file, int line, uint flags)
{
	(void)file;
	(void)line;
	(void)flags;
	return calloc(1, size);
}

void bkHeapFree(void *pointer)
{
	free(pointer);
}

static unsigned long long test_ticks;

ulonglong bkTimerRead(void)
{
	return test_ticks;
}

ulonglong bkTimerDelta(ulonglong start, ulonglong end)
{
	return end - start;
}

/* ---------- a game object with four players */

static __attribute__((aligned(16))) unsigned char test_game[0x2ae0];
static __attribute__((aligned(16))) unsigned char test_players[4][0x4300];

static void setup_players(int count)
{
	int i;

	memset(test_game, 0, sizeof test_game);
	memset(test_players, 0, sizeof test_players);
	memset(rb_test_records, 0, sizeof rb_test_records);
	DAT_0030ef04 = test_game;
	RB_AT(test_game, RB_GO_PLAYERS, unsigned char *) = count > 0 ? test_players[0] : NULL;
	for (i = 0; i < count; i++) {
		RB_AT(test_players[i], 0x10, unsigned char *) = i + 1 < count ? test_players[i + 1] : NULL;
		RB_I32(test_players[i], RB_PL_INDEX) = i;
		RB_I32(test_players[i], RB_PL_SLOT) = i;
	}
}

static int score(int player)
{
	return rb_test_records[4 + player * 10];
}

/* ---------- tests */

static void test_timer(void)
{
	static __attribute__((aligned(16))) unsigned char t[0x40];

	memset(t, 0, sizeof t);
	CRollerBallTimer_Ctor(t);
	RB_AT(t, 0x18, long long) = 1000; /* CDemoCountdown_Init's frequency (demolition_a) */
	DAT_002ecdec = 0;
	test_ticks = 0;
	CRollerBallTimer_Set(t, 2.0f);
	CRollerBallTimer_Start(t);
	test_ticks = 500;
	CRollerBallTimer_Update(t);
	CHECK(fabsf(CRollerBallTimer_Remaining(t) - 1.5f) < 1e-4f, "0.5 s of 2 s: 1.5 s left");
	CHECK(fabsf(RB_F32(t, 0x34) - 0.25f) < 1e-4f, "the elapsed fraction is a quarter");
	DAT_002ecdec = 1; /* paused at 0.5 s */
	CRollerBallTimer_Update(t);
	test_ticks = 1500;
	CRollerBallTimer_Update(t);
	CHECK(fabsf(CRollerBallTimer_Remaining(t) - 1.5f) < 1e-4f, "a pause does not use up time");
	DAT_002ecdec = 0;
	CRollerBallTimer_Update(t);
	test_ticks = 2000;
	CRollerBallTimer_Update(t);
	CHECK(fabsf(CRollerBallTimer_Remaining(t) - 1.0f) < 1e-4f, "after the pause the time runs on");
	test_ticks = 9000;
	CRollerBallTimer_Update(t);
	CHECK(CRollerBallTimer_Remaining(t) == 0.0f, "the remaining time stops at 0");
	CHECK(RB_F32(t, 0x34) == 1.0f, "the fraction stops at 1");
	CRollerBallTimer_Stop(t);
	test_ticks = 0;
	CRollerBallTimer_Update(t);
	CHECK(CRollerBallTimer_Remaining(t) == 0.0f, "a stopped timer keeps its time");
}

static void test_rank_scores(void)
{
	setup_players(4);
	/* sumo ranks the players by +0x41f8: 1 is the best */
	RB_I32(test_players[0], RB_PL_RANK) = 2;
	RB_I32(test_players[1], RB_PL_RANK) = 1;
	RB_I32(test_players[2], RB_PL_RANK) = 1;
	RB_I32(test_players[3], RB_PL_RANK) = 3;
	CRollerBallSumo_SetScores(NULL);
	CHECK(score(1) == 10 && score(2) == 10, "shared first place: 10 each");
	CHECK(score(0) == 5, "the next rank gets 5 (ranks are not skipped)");
	CHECK(score(3) == 3, "then 3");
	/* drop ranks by +0x41f0 */
	setup_players(4);
	RB_I32(test_players[0], RB_PL_41F0) = 4;
	RB_I32(test_players[1], RB_PL_41F0) = 3;
	RB_I32(test_players[2], RB_PL_41F0) = 2;
	RB_I32(test_players[3], RB_PL_41F0) = 1;
	CRollerBallDrop_SetScores(NULL);
	CHECK(score(3) == 10 && score(2) == 5 && score(1) == 3 && score(0) == 1,
		"drop: 10, 5, 3, 1 by rank");
}

static void test_race_scores(void)
{
	setup_players(4);
	/* player 2 finished first (place 0), player 0 finished second (place 1) */
	RB_I32(test_players[2], RB_PL_41F0) = 3;
	RB_I32(test_players[2], RB_PL_RANK) = 0;
	RB_I32(test_players[0], RB_PL_41F0) = 3;
	RB_I32(test_players[0], RB_PL_RANK) = 1;
	/* the others did not finish: player 1 got further than player 3 */
	RB_I32(test_players[1], RB_PL_41F0) = 2;
	RB_F32(test_players[1], 0x41ec) = 0.8f;
	RB_I32(test_players[3], RB_PL_41F0) = 1;
	RB_F32(test_players[3], 0x41ec) = 0.4f;
	CRollerBallRace_SetScores(NULL);
	CHECK(score(2) == 5, "the winner gets 5");
	CHECK(score(0) == 3, "second place gets 3");
	CHECK(score(1) == 1, "the furthest player that did not finish is third: 1");
	CHECK(score(3) == 0, "the last gets nothing");
}

static void test_respawn(void)
{
	static __attribute__((aligned(16))) unsigned char rs[0x300];
	float near_pos[4] = { 100.0f, 0.0f, 100.0f, 1.0f };
	float far_pos[4] = { 1000.0f, 0.0f, 1000.0f, 1.0f };
	float dir[4] = { 0.0f, 0.0f, 1.0f, 0.0f };
	float fall[4] = { 90.0f, -500.0f, 90.0f, 1.0f };
	unsigned char *e1, *e2, *p1, *p2;

	setup_players(2);
	memset(rs, 0, sizeof rs);
	CRollerBallRespawn_Ctor(rs);
	CRollerBallRespawn_Init(rs, NULL);
	CRollerBallRespawn_AddPoint(rs, 0, far_pos, dir);
	CRollerBallRespawn_AddPoint(rs, 1, near_pos, dir);
	CHECK(RB_I32(rs, 0xc) == 2, "two respawn points");
	e1 = CRollerBallRespawn_RespawnAt(rs, test_players[0], fall);
	CHECK(e1 != NULL, "a free respawn entry is used");
	p1 = e1 ? RB_PTR(e1, 0x4c) : NULL;
	CHECK(p1 != NULL && RB_F32(p1, 8) == 100.0f, "the nearest point is chosen");
	CHECK(p1 != NULL && RB_U8(p1, 0x2c) == 1, "the point is marked used");
	CHECK(RB_U8(test_players[0], RB_PL_ACTIVE) == 0, "the player is out while respawning");
	e2 = CRollerBallRespawn_RespawnAt(rs, test_players[1], fall);
	p2 = e2 ? RB_PTR(e2, 0x4c) : NULL;
	CHECK(e2 != NULL && e2 != e1, "a second respawn uses another entry");
	CHECK(p2 != NULL && p2 != p1 && RB_F32(p2, 8) == 1000.0f,
		"the second player gets the other point");
	CRollerBallRespawn_Shutdown(rs);
}

static void test_lists(void)
{
	static unsigned char list[8], a[8], b[8];

	memset(list, 0, sizeof list);
	CRollerBallList_Add(list, a);
	CRollerBallList_Add(list, b);
	CHECK(RB_PTR(list, 4) == b && RB_PTR(b, 4) == a && RB_PTR(a, 4) == NULL,
		"items are added at the head");
}

static void test_texanim(void)
{
	static __attribute__((aligned(16))) unsigned char e[0x1c];

	memset(e, 0, sizeof e);
	RB_I16(e, 8) = 2;      /* first frame */
	RB_I16(e, 0xa) = 4;    /* last frame */
	RB_I16(e, 0xe) = 3;
	RB_F32(e, 0x18) = 1.0f;
	RB_U8(e, 0x12) = 1;
	RB_U16(e, 0x10) = 1;   /* loop */
	CRollerBallTexAnim_Update(e, 1.5f);
	CHECK(RB_I16(e, 0xe) == 2, "a loop wraps to the first frame at the last");
	CHECK(fabsf(RB_F32(e, 0x14) - 0.5f) < 1e-5f, "the spare time is kept");
	RB_U16(e, 0x10) = 0;   /* once */
	RB_I16(e, 0xe) = 3;
	RB_F32(e, 0x14) = 0.0f;
	CRollerBallTexAnim_Update(e, 1.5f);
	CHECK(RB_U8(e, 0x12) == 0, "a one-shot animation stops at the last frame");
	RB_U8(e, 0x12) = 1;
	RB_U16(e, 0x10) = 3;   /* ping-pong */
	RB_U8(e, 0x13) = 0;
	RB_I16(e, 0xe) = 3;
	RB_F32(e, 0x14) = 0.0f;
	CRollerBallTexAnim_Update(e, 1.5f);
	CHECK(RB_I16(e, 0xe) == 3 && RB_U8(e, 0x13) == 1, "ping-pong turns at the last frame");
	RB_I16(e, 0xe) = 2;
	CRollerBallTexAnim_Update(e, 1.0f);
	CHECK(RB_I16(e, 0xe) == 2 && RB_U8(e, 0x13) == 0, "and at the first frame");
}

int main(void)
{
	test_timer();
	test_rank_scores();
	test_race_scores();
	test_respawn();
	test_lists();
	test_texanim();
	fprintf(stderr, "rollerball_a tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
