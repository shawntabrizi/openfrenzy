/*
clusteroids module tests: one guest image (src/clusteroids/tests/run_tests.sh builds it from
the game build's objects of src/clusteroids and the generated weak stubs).

The game image globs every .c under src/clusteroids, so this file compiles to nothing unless
OPENFRENZY_CLUSTEROIDS_TEST is defined. Only this module's objects are linked: functions of
other modules are the weak stubs (they do nothing and return 0), and other modules' data
are zero slots made by run_tests.sh.

What the tests pin down, and why it matters to the game:
- the player records: the code mixes the PL_* offsets (clusteroids_game.h) with Ghidra's
  absolute addresses (0x002fcfa4 is the character's animation node that ClustPlayer_Init
  fills and ClustPlayer_Update plays). If the two disagree, a player reads another
  player's actor or its own pod list from the wrong field.
- the data block keeps the XBE values: ClustPlayer_Init finds the 19 character animations
  by the names at 0x002972a8; wrong data gives a character without animations.
- headings (0x000e3c20): the AI and the pods steer by atan2(x, z) (0 is +z). A sign or
  axis change makes computer ships and pod chains turn the wrong way.
- scores (0x000dc9d0, 0x000dcab0, 0x000dcaf0): players that are out do not count, and a
  tie for first place has no winner (-1). The end of the round and the HUD use this.
- pod chains (0x000e5320, 0x000e53d0): only pods attached for less than
  tailsPodAttachTime (0x002f7d64) are loose; the score of the Tails variant counts them.
- the countdown (0x000de730-0x000de8e0): a countdown that has not started shows its full
  length and progress 0 (the HUD clock before the round).
- the arena floor (0x000e7a40): each Tails spawn point bends the floor by 500 / distance
  times its strength, clamped to [-400, 200]; ships and pods sit on this height.
- tokens (0x000e2b40): a token takes the first free slot of 50, power-ups (kind 0) are
  counted, and a full table refuses new tokens (returns 0) instead of overwriting.
*/

#ifdef OPENFRENZY_CLUSTEROIDS_TEST

#include "clusteroids.h"
#include "clusteroids_game.h"

static int failures;
static int checks;

#define CHECK(cond, what) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, what); \
		} \
	} while (0)

static int near(float a, float b)
{
	return fabsf(a - b) < 1e-5f;
}

/* module globals the tested functions use (the same addresses as in Clusteroids/ *.c) */
#define T_SPAWN_COUNT CL_VAR(short, 0x002f7f84)
#define T_SPAWN_POINTS CL_VAR(float *, 0x002f7f88)
#define T_FLOOR_SCALE CL_VAR(float, 0x002f7d58)
#define T_ATTACH_TIME CL_VAR(float, 0x002f7d64)
#define T_TOKENS CL_VAR(char *, 0x002fedf0)
#define T_TOKEN_COUNT CL_VAR(int, 0x002f9880)
#define T_CD_RUNNING CL_VAR(char, 0x002fa328)

/* ---------- layout */

static void test_layout(void)
{
	char *p0 = CL_PLAYER(0);

	CHECK(CL_PLAYER(1) - p0 == 0x850, "player records are 0x850 bytes apart");
	CHECK(p0 == (char *)CL_ADDR(0x002fcb20), "player 0 starts at 0x002fcb20");
	CHECK(p0 + PL_SHIP == (char *)CL_ADDR(0x002fcb90), "ship actor at 0x002fcb90");
	CHECK(p0 + PL_SHIP2 == (char *)CL_ADDR(0x002fcd00), "second ship actor at 0x002fcd00");
	CHECK(p0 + PL_CHAR == (char *)CL_ADDR(0x002fce70), "character actor at 0x002fce70");
	CHECK(p0 + PL_CHAR2 == (char *)CL_ADDR(0x002fcfe0), "second character actor at 0x002fcfe0");
	CHECK(p0 + PL_CHAR + 0x134 == (char *)CL_ADDR(0x002fcfa4), "character animation node 0x002fcfa4");
	CHECK(p0 + PL_PODS == (char *)CL_ADDR(0x002fd194), "pod chain head at 0x002fd194");
	CHECK(p0 + PL_TARGET_POD == (char *)CL_ADDR(0x002fd198), "AI target pod at 0x002fd198");
	CHECK(p0 + PL_ANIMS == (char *)CL_ADDR(0x002fd1a8), "animation table at 0x002fd1a8");
	CHECK(p0 + PL_IN_FIRE == (char *)CL_ADDR(0x002fd320), "Fire channel at 0x002fd320");
	CHECK(CL_PLAYER(3) + CL_PLAYER_SIZE <= (char *)CL_ADDR(0x002ff854) + 4,
		"four player records fit in the .bss block");
	CHECK(CL_ADDR(0x002f7a40) == NULL && CL_ADDR(0x002ff858) == NULL && CL_ADDR(0x00294bd4) == NULL,
		"addresses outside the blocks give NULL");
}

/* ---------- XBE data */

static void test_data(void)
{
	CHECK(strcmp((char *)CL_ADDR(0x002972a8), "idle") == 0, "first animation name is idle");
	CHECK(strcmp((char *)CL_ADDR(0x002972a8 + 18 * 0x20), "victory_end") == 0,
		"19th animation name is victory_end");
	CHECK(strcmp((char *)CL_ADDR(0x00296ee8), "hit") == 0, "first sample name is hit");
}

/* ---------- headings */

static void test_heading(void)
{
	static const float dirs[][2] = {
		{ 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }, { 3, 0.5f }, { -0.25f, 7 }, { 2, -9 }, { -4, -0.1f },
	};
	unsigned int i;

	for (i = 0; i < sizeof dirs / sizeof dirs[0]; i++)
		CHECK(near(ClustPods_Heading(dirs[i][0], dirs[i][1]), atan2f(dirs[i][0], dirs[i][1])),
			"heading is atan2(x, z)");
	CHECK(ClustPods_Heading(0, 0) == 0.0f, "no direction gives heading 0");
	CHECK(near(ClustPods_Heading(0, 5), 0.0f), "+z is heading 0");
	CHECK(near(ClustPods_Heading(5, 0), 1.5707963f), "+x is heading pi/2");
	CHECK(near(ClustPods_Heading(-5, 0), -1.5707963f), "-x is heading -pi/2");
	CHECK(near(ClustPods_Heading(0, -5), 3.1415925f), "-z is heading pi");
}

/* ---------- scores */

static void set_scores(int s0, int s1, int s2, int s3)
{
	int scores[4] = { s0, s1, s2, s3 };
	int i;

	for (i = 0; i < 4; i++) {
		CL_I16(CL_PLAYER(i), PL_SCORE) = (short)scores[i];
		CL_I16(CL_PLAYER(i), PL_OUT) = 0;
	}
}

static void test_scores(void)
{
	set_scores(3, 7, 7, 1);
	CHECK(ClustGame_GetHighestScore() == 7, "highest score");
	CHECK(ClustGame_GetLowestScore() == 1, "lowest score");
	CHECK(ClustGame_GetWinner() == -1, "a tie for first place has no winner");
	CL_I16(CL_PLAYER(2), PL_OUT) = 1;
	CHECK(ClustGame_GetWinner() == 1, "a player that is out does not tie");
	CL_I16(CL_PLAYER(3), PL_OUT) = 1;
	CHECK(ClustGame_GetLowestScore() == 3, "the lowest score ignores players that are out");
	set_scores(0, 0, 0, 0);
	CL_I16(CL_PLAYER(0), PL_OUT) = 1;
	CL_I16(CL_PLAYER(1), PL_OUT) = 1;
	CL_I16(CL_PLAYER(2), PL_OUT) = 1;
	CL_I16(CL_PLAYER(3), PL_OUT) = 1;
	CHECK(ClustGame_GetHighestScore() == -1 && ClustGame_GetLowestScore() == 99,
		"nobody in: -1 and 99");
	set_scores(0, 0, 0, 0);
}

/* ---------- pod chains */

static void test_pods(void)
{
	static char pods[9][0x80];
	static const float times[9] = { 0, 10, 524, 1, 2, 3, 0, 5, 600 };
	int best;
	int i;

	T_ATTACH_TIME = 525.0f;
	memset(pods, 0, sizeof pods);
	for (i = 0; i < 9; i++) {
		CL_F32(pods[i], 0x4) = times[i];
		if (i % 3 != 2)
			CL_PTR(pods[i], 0) = pods[i + 1];
	}
	/* player 0: 3 loose, player 1: 3 loose, player 2: 2 loose + 1 permanent */
	CL_PTR(CL_PLAYER(0), PL_PODS) = pods[0];
	CL_PTR(CL_PLAYER(1), PL_PODS) = pods[3];
	CL_PTR(CL_PLAYER(2), PL_PODS) = pods[6];
	CL_PTR(CL_PLAYER(3), PL_PODS) = NULL;
	CHECK(ClustPods_CountLoose(2) == 2, "a permanent pod is not loose");
	CHECK(ClustPods_CountLooseOf(CL_PLAYER(0)) == 3, "loose pods of a player record");
	CHECK(ClustPods_CountLongest(0, &best) == 2 && best == 3, "two players share the longest chain");
	CL_PTR(CL_PLAYER(1), PL_PODS) = NULL;
	CHECK(ClustPods_CountLongest(0, &best) == 1 && best == 3, "one player has the longest chain");
	for (i = 0; i < 4; i++)
		CL_PTR(CL_PLAYER(i), PL_PODS) = NULL;
	CHECK(ClustPods_CountLongest(0, &best) == 0 && best == -1, "no chains");
}

/* ---------- countdown */

static void test_countdown(void)
{
	DAT_002ecdec = 0;
	CHECK(ClustCountdown_Init() == 1, "init returns 1");
	ClustCountdown_SetDuration(60.0f);
	CHECK(T_CD_RUNNING == 0, "setting the length stops the countdown");
	ClustCountdown_Update();
	CHECK(near(ClustCountdown_GetRemaining(), 60.0f), "a countdown not started shows its length");
	CHECK(CL_VAR(float, 0x002fa35c) == 0.0f, "a countdown not started has progress 0");
}

/* ---------- arena floor */

static void test_floor(void)
{
	static float points[2][13];

	T_SPAWN_COUNT = 0;
	CHECK(ClustArena_GetHeight(10, 10) == 0.0f, "no spawn points: flat floor");
	memset(points, 0, sizeof points);
	T_FLOOR_SCALE = 1.0f;
	T_SPAWN_POINTS = &points[0][0];
	T_SPAWN_COUNT = 1;
	points[0][8] = -1.0f;
	CHECK(near(ClustArena_GetHeight(10, 0), 50.0f), "raised by 500 / distance");
	CHECK(near(ClustArena_GetHeight(1, 0), 200.0f), "raised at most 200");
	CHECK(near(ClustArena_GetHeight(0, 0), 200.0f), "distance 0 counts as 1");
	points[0][8] = 1.0f;
	CHECK(near(ClustArena_GetHeight(0, 2), -250.0f), "sunk by 500 / distance");
	CHECK(near(ClustArena_GetHeight(0, 1), -400.0f), "sunk at most 400");
	T_FLOOR_SCALE = 2.0f;
	CHECK(near(ClustArena_GetHeight(0, 20), -50.0f), "the scale divides the distance");
	T_FLOOR_SCALE = 1.0f;
	points[1][0] = 100.0f;
	points[1][8] = -1.0f;
	T_SPAWN_COUNT = 2;
	CHECK(near(ClustArena_GetHeight(50, 0), 0.0f), "spawn points add up (stride 13 floats)");
	T_SPAWN_COUNT = 0;
	T_SPAWN_POINTS = NULL;
}

/* ---------- tokens */

static void test_tokens(void)
{
	static char tokens[50][0x2c];
	int i;
	int ok = 1;

	memset(tokens, 0, sizeof tokens);
	T_TOKENS = &tokens[0][0];
	T_TOKEN_COUNT = 0;
	CHECK(ClustTokens_Add(5.0f, -7.0f, 0) == 1, "a token is added");
	CHECK(CL_I16(tokens[0], 0x1c) == 1 && CL_F32(tokens[0], 0x8) == 5.0f &&
		CL_F32(tokens[0], 0x10) == -7.0f && CL_I16(tokens[0], 0x4) == 0,
		"the first slot holds the token at (x, z)");
	CHECK(T_TOKEN_COUNT == 1, "a power-up (kind 0) is counted");
	CHECK(ClustTokens_Add(1.0f, 1.0f, 2) == 1 && T_TOKEN_COUNT == 1, "other kinds are not counted");
	CHECK(CL_I16(tokens[1], 0x4) == 2, "the next token takes the next slot");
	for (i = 2; i < 50; i++)
		ok &= ClustTokens_Add(0.0f, 0.0f, 1) == 1;
	CHECK(ok, "50 tokens fit");
	CHECK(ClustTokens_Add(9.0f, 9.0f, 0) == 0 && CL_F32(tokens[0], 0x8) == 5.0f,
		"a full table refuses a token and keeps the old ones");
	T_TOKENS = NULL;
	T_TOKEN_COUNT = 0;
}

int main(void)
{
	test_layout();
	test_data();
	test_heading();
	test_scores();
	test_pods();
	test_countdown();
	test_floor();
	test_tokens();
	fprintf(stderr, "clusteroids tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
