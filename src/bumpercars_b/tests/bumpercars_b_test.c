/*
bumpercars_b module tests: one guest image (src/bumpercars_b/tests/run_tests.sh builds it from
the game build's objects of src/bumpercars_b and the generated weak stubs).

The game image globs every .c under src/bumpercars_b, so this file compiles to nothing unless
OPENFRENZY_BUMPERCARS_B_TEST is defined. Only this module's objects are linked: functions of
other modules are the weak stubs (they do nothing and return 0).

What the tests pin down, and why it matters to the game:
- names in data files (0x000a4e00): the puck, powerup and world files give names in quotes;
  the parser copies at most `size` characters and then a NUL, and a quote at once gives an
  empty name. A wrong parser loads the wrong models or none.
- angles (0x000a5540): atan2 is moved into [0, 2pi), so a car tilted to the left gets an
  angle near 2pi, not a negative one. The tilt code wraps these angles; a sign change turns
  cars the wrong way.
- teams (0x000a5180): two cars are on one team only in a team game. Score code treats a hit
  from a team mate like a self kill (CBCar_Entity 0x0009c360), so this decides who scores.
  Pucks and other entities that are not cars are never on a team.
- the game count down (0x00096550): a game of N seconds shows N seconds remaining before
  it starts; the HUD clock and the end of the game read this value.
*/

#ifdef OPENFRENZY_BUMPERCARS_B_TEST

#include "bumpercars_b.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Data and functions of other modules that this test image does not link (the weak stubs
   cover only the functions of the function table). Core's .bss addresses are aliases into a
   copy of core's block at their real offsets (0x002ba4f0-0x002bb138), so the player table
   (DAT_002baed4, 10 ints per player, the team at +8 ints) keeps its layout; every other
   address gets its own zero 0x400-byte slot. The test uses only DAT_002baebc (team game) and
   DAT_002baed4; the rest only keeps the link complete. */
__attribute__((nocommon, aligned(16))) unsigned char of_core_bss[0xc48];
__attribute__((nocommon, aligned(16))) unsigned char of_bcb_test_foreign[25 * 0x400];
__asm__(
	".globl DAT_002baebc\n.set DAT_002baebc, of_core_bss + 0x9cc\n"
	".globl DAT_002baed4\n.set DAT_002baed4, of_core_bss + 0x9e4\n"
	".globl DAT_002ba928\n.set DAT_002ba928, of_core_bss + 0x438\n"
	".globl DAT_002ba92c\n.set DAT_002ba92c, of_core_bss + 0x43c\n"
	".globl DAT_002ba99c\n.set DAT_002ba99c, of_core_bss + 0x4ac\n"
	".globl DAT_002ba9a0\n.set DAT_002ba9a0, of_core_bss + 0x4b0\n"
	".globl DAT_002bae70\n.set DAT_002bae70, of_core_bss + 0x980\n"
	".globl DAT_002baeb8\n.set DAT_002baeb8, of_core_bss + 0x9c8\n"
	".globl DAT_002baed8\n.set DAT_002baed8, of_core_bss + 0x9e8\n"
	".globl DAT_002baee0\n.set DAT_002baee0, of_core_bss + 0x9f0\n"
	".globl DAT_00283dc0\n.set DAT_00283dc0, of_bcb_test_foreign + 0x0\n"
	".globl DAT_00283e58\n.set DAT_00283e58, of_bcb_test_foreign + 0x400\n"
	".globl DAT_00283ea4\n.set DAT_00283ea4, of_bcb_test_foreign + 0x800\n"
	".globl DAT_00284b10\n.set DAT_00284b10, of_bcb_test_foreign + 0xc00\n"
	".globl DAT_00284b14\n.set DAT_00284b14, of_bcb_test_foreign + 0x1000\n"
	".globl DAT_00284b18\n.set DAT_00284b18, of_bcb_test_foreign + 0x1400\n"
	".globl DAT_002855f0\n.set DAT_002855f0, of_bcb_test_foreign + 0x1800\n"
	".globl DAT_00285630\n.set DAT_00285630, of_bcb_test_foreign + 0x1c00\n"
	".globl DAT_00285644\n.set DAT_00285644, of_bcb_test_foreign + 0x2000\n"
	".globl DAT_00290718\n.set DAT_00290718, of_bcb_test_foreign + 0x2400\n"
	".globl DAT_00290e8c\n.set DAT_00290e8c, of_bcb_test_foreign + 0x2800\n"
	".globl DAT_00291734\n.set DAT_00291734, of_bcb_test_foreign + 0x2c00\n"
	".globl DAT_002b9968\n.set DAT_002b9968, of_bcb_test_foreign + 0x3000\n"
	".globl DAT_002b9c58\n.set DAT_002b9c58, of_bcb_test_foreign + 0x3400\n"
	".globl DAT_002b9c64\n.set DAT_002b9c64, of_bcb_test_foreign + 0x3800\n"
	".globl DAT_002bc490\n.set DAT_002bc490, of_bcb_test_foreign + 0x3c00\n"
	".globl DAT_002bc898\n.set DAT_002bc898, of_bcb_test_foreign + 0x4000\n"
	".globl DAT_002bce18\n.set DAT_002bce18, of_bcb_test_foreign + 0x4400\n"
	".globl DAT_002bd640\n.set DAT_002bd640, of_bcb_test_foreign + 0x4800\n"
	".globl DAT_002cd7a0\n.set DAT_002cd7a0, of_bcb_test_foreign + 0x4c00\n"
	".globl DAT_002db808\n.set DAT_002db808, of_bcb_test_foreign + 0x5000\n"
	".globl DAT_002ea478\n.set DAT_002ea478, of_bcb_test_foreign + 0x5400\n"
	".globl DAT_002ecfac\n.set DAT_002ecfac, of_bcb_test_foreign + 0x5800\n"
	".globl DAT_002ed324\n.set DAT_002ed324, of_bcb_test_foreign + 0x5c00\n"
	".globl DAT_002ed350\n.set DAT_002ed350, of_bcb_test_foreign + 0x6000\n"
);

/* bumpercars_a's CBCar_Game mode handlers, missing from the function table (no weak stub);
   CGameMode_BumperCars (bcb_misc.c) calls them. The tests do not. */
void FUN_00086c90(void *self) { (void)self; }
void FUN_00086ea0(void *self) { (void)self; }
void FUN_00086ed0(void *self) { (void)self; }
void FUN_00086f80(void *self, int top) { (void)self; (void)top; }
void FUN_000874c0(void *self, int top) { (void)self; (void)top; }

static int checks;
static int failures;

#define CHECK(cond, what) do { \
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

/* ---------- names in data files */

static void test_text_name(void)
{
	char line[64];
	char out[16];

	strcpy(line, "MODEL \"puck01\" 3");
	memset(out, 'x', sizeof out);
	CHECK(BCarText_GetName(line, out, 15) == 1, "a quoted name is found");
	CHECK(strcmp(out, "puck01") == 0, "the text between the quotes is copied");

	memset(out, 'x', sizeof out);
	CHECK(BCarText_GetName(line, out, 3) == 1, "a long name is found");
	CHECK(strcmp(out, "puc") == 0, "at most `size` characters, then a NUL");

	strcpy(line, "\"\" rest");
	memset(out, 'x', sizeof out);
	CHECK(BCarText_GetName(line, out, 15) == 1 && out[0] == '\0', "empty quotes give an empty name");

	memset(out, 'x', sizeof out);
	CHECK(BCarText_GetName(NULL, out, 15) == 0 && out[0] == '\0', "no line: 0 and an empty name");
}

/* ---------- angles */

static void test_atan2(void)
{
	const float pi = 3.14159265f;
	float a;

	CHECK(near(BCarMath_Atan2(0.0f, 1.0f), 0.0f), "straight ahead is 0");
	CHECK(near(BCarMath_Atan2(1.0f, 0.0f), pi / 2.0f), "quarter turn is pi/2");
	CHECK(near(BCarMath_Atan2(0.0f, -1.0f), pi), "half turn is pi");
	CHECK(near(BCarMath_Atan2(-1.0f, 0.0f), 3.0f * pi / 2.0f), "negative angles move up by 2pi");
	a = BCarMath_Atan2(-0.001f, 1.0f);
	CHECK(a > 6.28f && a < 6.2831855f, "a small negative angle becomes nearly 2pi");
}

/* ---------- teams */

static void test_same_team(void)
{
	static struct BCarEntity cars[4];
	static struct BCarEntity puck;
	int i;
	int ia;
	int ib;

	for (i = 0; i < 4; i++)
	{
		g_BCarPlayers[i].entity = &cars[i];
		DAT_002baed4[i * 10 + 8] = i & 1;      /* teams: 0, 1, 0, 1 */
	}

	DAT_002baebc = 0;
	ia = ib = 99;
	CHECK(BCarEntity_SameTeam(&cars[0], &cars[2], &ia, &ib) == 0, "no teams outside a team game");
	CHECK(ia == 0 && ib == 2, "the player index of each car");

	DAT_002baebc = 1;
	CHECK(BCarEntity_SameTeam(&cars[0], &cars[2], NULL, NULL) == 1, "players 1 and 3 are one team");
	CHECK(BCarEntity_SameTeam(&cars[3], &cars[1], NULL, NULL) == 1, "players 2 and 4 are one team");
	CHECK(BCarEntity_SameTeam(&cars[0], &cars[1], NULL, NULL) == 0, "players 1 and 2 are opponents");

	ia = ib = 99;
	CHECK(BCarEntity_SameTeam(&cars[1], &puck, &ia, &ib) == 0, "a puck is on no team");
	CHECK(ia == 1 && ib == -1, "an entity that is not a car has index -1");

	DAT_002baebc = 0;
	for (i = 0; i < 4; i++)
		g_BCarPlayers[i].entity = NULL;
}

/* ---------- the game count down */

static void test_game_timer(void)
{
	static struct BCarGameTimer t;

	CHECK(BCarGameTimer_Init(&t, 90) == 1, "init returns 1");
	CHECK(t.seconds == 90, "the length is kept");
	BCarTimer_Update(&t.timer);
	CHECK(near(BCarTimer_GetRemaining(&t.timer), 90.0f), "a stopped 90 s game shows 90 s left");
	CHECK(near(BCarTimer_GetElapsed(&t.timer), 0.0f), "nothing elapsed before the start");
	CHECK(near(BCarTimer_GetFraction(&t.timer), 0.0f), "a stopped timer has fraction 0");
}

int main(void)
{
	test_text_name();
	test_atan2();
	test_same_team();
	test_game_timer();
	fprintf(stderr, "bumpercars_b tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
