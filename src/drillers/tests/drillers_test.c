/*
DRILLERS_TEST.C

drillers module tests (src/drillers/tests/run_tests.sh builds this as the main() of a small
guest image: the drillers objects, the generated stubs and the platform layer). They check
rules of the Drillers minigame that the decompiled code must keep:

- the AI turns by the shortest signed angle, so a CPU bike never turns the long way round;
- a point-in-polygon test decides what a closed snare loop catches;
- a CPU driller treats the edge of the map, a drilled cell, another player in play on the
  next cell (or the one after it) and another player's target cell as blocked, but never
  itself;
- the target cell of a CPU driller is the next cell on its heading;
- a harder CPU gets more skill and less caution;
- a sharp turn of a jet bike eases off by one degree a frame and ends at +-2;
- closing a snare loop collects only the tokens inside it that have bounced more than
  twice, and (as in the original) adds their points to the player's score twice.

The game build compiles every .c under src/drillers: without OPENFRENZY_DRILLERS_TEST (set
by run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_DRILLERS_TEST
#include "drillers.h"

static int checks, failures;
#define CHECK(cond, what) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (what)); \
		} \
	} while (0)

/* the other modules' globals that the drillers objects use (their objects are not in the
test image); types as drillers declares them */
int DAT_00283ea4;
unsigned int DAT_00285630;
float DAT_00285644;
unsigned char DAT_002ba750[0x100];
int DAT_002ba99c;
unsigned int DAT_002bc490[256];
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];
void *DAT_002db808;
int DAT_002ecfac;
unsigned char DAT_002ed324[0x100];
void *DAT_002f26c4;
int DAT_00283ef0;
float DAT_0028fda8;
float DAT_0028ffa8;
float DAT_0028ffac;
int DAT_002ba56c;
void *DAT_002ba9a0;
int DAT_002bae8c;
int DAT_002bae94;
int DAT_002baeb8;
int DAT_002baebc;
int DAT_002ecdec;
unsigned char DAT_002ecd90[0x100];
int DAT_002bae70;
int DAT_00290e8c;
unsigned char DAT_00291734[0x100];
float DAT_002ea478[4];
int DAT_002bae98;
int DAT_002baea8;
long long DAT_002cd7a0;
void *DAT_002ecc74;
/* foxhunt defines this vtable of the snare polygon object */
const void *const vtable_0026cc44[1] = {0};

/* the character colour table (0x00290700; 0x00290718 is its first colour) */
__attribute__((nocommon, aligned(16))) unsigned char dr_test_colours[0x18 + 0x60 * 16];
__asm__(".globl DAT_00290700\n.set DAT_00290700, dr_test_colours + 0x0\n"
	".globl DAT_00290718\n.set DAT_00290718, dr_test_colours + 0x18\n");

/* core's player records: 4 of 0x28 bytes from 0x002baed4 */
__attribute__((nocommon, aligned(16))) int dr_test_records[44];
__asm__(".globl DAT_002baed4\n.set DAT_002baed4, dr_test_records + 0x0\n"
	".globl DAT_002baed8\n.set DAT_002baed8, dr_test_records + 0x4\n"
	".globl DAT_002baee8\n.set DAT_002baee8, dr_test_records + 0x14\n"
	".globl DAT_002baef4\n.set DAT_002baef4, dr_test_records + 0x20\n"
	".globl DAT_002baf00\n.set DAT_002baf00, dr_test_records + 0x2c\n"
	".globl DAT_002baf10\n.set DAT_002baf10, dr_test_records + 0x3c\n"
	".globl DAT_002baf1c\n.set DAT_002baf1c, dr_test_records + 0x48\n"
	".globl DAT_002baf38\n.set DAT_002baf38, dr_test_records + 0x64\n"
	".globl DAT_002baf44\n.set DAT_002baf44, dr_test_records + 0x70\n"
	".globl DAT_002baf60\n.set DAT_002baf60, dr_test_records + 0x8c\n"
	".globl DAT_002baf6c\n.set DAT_002baf6c, dr_test_records + 0x98\n"
	".globl DAT_002baf28\n.set DAT_002baf28, dr_test_records + 0x54\n"
	".globl DAT_002baf50\n.set DAT_002baf50, dr_test_records + 0x7c\n");

#define HALF_Z DR_VAR(int, 0x002ff9e4u)
#define HALF_X DR_VAR(int, 0x002ff9e8u)
#define CELL_SIZE DR_VAR(int, 0x002ff9e0u)
#define HEADING(i) DR_PF(i, 0x0c, float)
#define COL(i) DR_PF(i, 0x1c, signed char)
#define ROW(i) DR_PF(i, 0x1d, signed char)
#define PLAYING(i) DR_PF(i, 0x20, signed char)
#define POINTS(i) DR_PF(i, 0x60, int)
#define TURN(i) DR_PF(i, 0x66c, int)
#define TARGET_COL(i) DR_PF(i, 0x730, signed char)
#define TARGET_ROW(i) DR_PF(i, 0x731, signed char)

/* a 4 x 4 map of whole cells */
static DrCell dr_test_cells[4][4];

static void reset_world(void)
{
	int i, x, z;

	memset(DR_PLR(0), 0, 4 * 0x730);
	memset(&dr_test_cells, 0, sizeof dr_test_cells);
	for (x = 0; x < 4; x++) {
		g_DrMap->columns[x] = dr_test_cells[x];
		for (z = 0; z < 4; z++)
			dr_test_cells[x][z].height = 0x400;
	}
	HALF_X = 2;
	HALF_Z = 2;
	CELL_SIZE = 1024;
	for (i = 0; i < 4; i++) {
		TARGET_COL(i) = -10;
		TARGET_ROW(i) = -10;
	}
	memset(g_DrFx->tokens, 0, sizeof g_DrFx->tokens);
}

static void test_angle_diff(void)
{
	CHECK(DrAi_AngleDiff(NULL, 350.0f, 10.0f) == -20.0f, "350 - 10 is a turn of -20");
	CHECK(DrAi_AngleDiff(NULL, 10.0f, 350.0f) == 20.0f, "10 - 350 is a turn of +20");
	CHECK(DrAi_AngleDiff(NULL, -170.0f, 170.0f) == 20.0f, "-170 - 170 is a turn of +20");
	CHECK(DrAi_AngleDiff(NULL, 180.0f, 0.0f) == 180.0f, "a half turn stays +180");
	CHECK(DrAi_AngleDiff(NULL, 30.0f, 10.0f) == 20.0f, "a small turn is unchanged");
}

static void test_in_polygon(void)
{
	static const float square[10] = {0, 0, 10, 0, 10, 10, 0, 10, 0, 0};

	CHECK(DrAi_InPolygon(NULL, 5.0f, 5.0f, 5, square) == 1, "the centre is inside");
	CHECK(DrAi_InPolygon(NULL, 15.0f, 5.0f, 5, square) == 0, "a point right of it is out");
	CHECK(DrAi_InPolygon(NULL, 5.0f, -3.0f, 5, square) == 0, "a point below it is out");
}

static void test_blocked(void)
{
	reset_world();
	COL(0) = 1;
	ROW(0) = 1;
	PLAYING(0) = 1;
	CHECK(FUN_00121590(90.0f, 0) == 0, "a whole cell ahead is free");
	CHECK(FUN_00121590(0.0f, 0) == 0, "a whole cell above is free");

	dr_test_cells[2][1].height = 0;
	CHECK(FUN_00121590(90.0f, 0) == 1, "a drilled cell ahead blocks");
	dr_test_cells[2][1].height = 0x400;

	COL(0) = 3;
	CHECK(FUN_00121590(90.0f, 0) == 1, "the last column blocks a move to +x");
	COL(0) = 0;
	CHECK(FUN_00121590(270.0f, 0) == 1, "the first column blocks a move to -x");
	COL(0) = 1;

	COL(1) = 3;
	ROW(1) = 1;
	PLAYING(1) = 1;
	CHECK(FUN_00121590(90.0f, 0) == 1, "a player two cells ahead blocks");
	PLAYING(1) = 0;
	CHECK(FUN_00121590(90.0f, 0) == 0, "a player out of play does not block");

	PLAYING(1) = 1;
	COL(1) = 0;
	ROW(1) = 3;
	TARGET_COL(1) = 2;
	TARGET_ROW(1) = 1;
	CHECK(FUN_00121590(90.0f, 0) == 1, "another player's target cell blocks");

	TARGET_COL(0) = 2;
	TARGET_ROW(0) = 1;
	PLAYING(1) = 0;
	CHECK(FUN_00121590(90.0f, 0) == 0, "the player's own target does not block it");
}

static void test_target(void)
{
	reset_world();
	COL(2) = 2;
	ROW(2) = 1;
	HEADING(2) = 270.0f;
	FUN_00121880(2);
	CHECK(TARGET_COL(2) == 1 && TARGET_ROW(2) == 1, "heading 270 targets the cell at -x");
	HEADING(2) = 0.0f;
	FUN_00121880(2);
	CHECK(TARGET_COL(2) == 2 && TARGET_ROW(2) == 2, "heading 0 targets the cell at +z");
}

static void test_ai_init(void)
{
	float easy_skill, hard_skill, easy_caution, hard_caution;
	void *ai = DR_PTR(void, 0x002aadf8u);
	extern int DAT_002bae70;

	g_DrVariant = 1;
	DAT_002bae70 = 0;
	DrAi_Init(ai);
	easy_skill = DR_F32(ai, 4);
	easy_caution = DR_F32(ai, 8);
	DAT_002bae70 = 2;
	DrAi_Init(ai);
	hard_skill = DR_F32(ai, 4);
	hard_caution = DR_F32(ai, 8);
	CHECK(easy_skill < hard_skill, "a hard CPU has more skill");
	CHECK(hard_caution < easy_caution, "a hard CPU is less cautious");
	DAT_002bae70 = 0;
}

static void test_turn_eases(void)
{
	void *ai = DR_PTR(void, 0x002aadf8u);

	reset_world();
	/* the bike at the map centre: no homing; no player in play: nothing ahead */
	DR_PF(0, 0x04, float) = 2.0f * 1024.0f;
	DR_PF(0, 0x08, float) = 2.0f * 1024.0f;
	HEADING(0) = 10.0f;
	TURN(0) = 5;
	DAT_00285630 = 0; /* the next rand() is 0x26: no new random drift */
	FUN_00122c10(ai, 0);
	CHECK(TURN(0) == 4, "a sharp turn loses one degree a frame");
	CHECK(HEADING(0) == 14.0f, "the heading moves by the eased turn");

	TURN(0) = -2;
	HEADING(0) = 10.0f;
	DAT_00285630 = 0;
	FUN_00122c10(ai, 0);
	CHECK(TURN(0) == 0 && HEADING(0) == 10.0f, "a turn of -2 ends without moving");
}

static void test_snare_close(void)
{
	void *loop = DR_PTR(void, 0x002aae60u);
	DrTokenSlot *t = g_DrFx->tokens;
	int i;

	reset_world();
	/* player 0's trail: a square of 4096 from the origin */
	DR_TRAIL_X(0, 0) = 0.0f;
	DR_TRAIL_Z(0, 0) = 0.0f;
	DR_TRAIL_X(0, 1) = 4096.0f;
	DR_TRAIL_Z(0, 1) = 0.0f;
	DR_TRAIL_X(0, 2) = 4096.0f;
	DR_TRAIL_Z(0, 2) = 4096.0f;
	DR_TRAIL_X(0, 3) = 0.0f;
	DR_TRAIL_Z(0, 3) = 4096.0f;
	for (i = 1; i < 4; i++) {
		DR_TRAIL_X(i, 0) = 100000.0f;
		DR_TRAIL_Z(i, 0) = 100000.0f;
	}
	/* token 0: gold, inside, bounced 3 times; token 1: inside, bounced once; token 2:
	   outside */
	t[0].active = 1;
	t[0].type = 2;
	t[0].bounces = 3.0f;
	t[0].x = 1.0f;
	t[0].z = 1.0f;
	t[1].active = 1;
	t[1].type = 0;
	t[1].bounces = 1.0f;
	t[1].x = 2.0f;
	t[1].z = 2.0f;
	t[2].active = 1;
	t[2].type = 1;
	t[2].bounces = 3.0f;
	t[2].x = 10.0f;
	t[2].z = 1.0f;
	POINTS(0) = 0;
	DrSnare_Close(loop, 0, 4);
	CHECK(t[0].active == 0, "a bounced token inside the loop is collected");
	CHECK(t[1].active == 1, "a token that bounced only once stays");
	CHECK(t[2].active == 1, "a token outside the loop stays");
	CHECK(POINTS(0) == 10, "a gold token's 5 points are added twice, as in the original");
}

int main(void)
{
	test_angle_diff();
	test_in_polygon();
	test_blocked();
	test_target();
	test_ai_init();
	test_turn_eases();
	test_snare_close();
	fprintf(stderr, "drillers tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}
#endif
