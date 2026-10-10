/*
mousewhack module tests: one guest image (src/mousewhack/tests/run_tests.sh builds it from the
game build's objects of src/mousewhack and the generated weak stubs, so every other module's
function is a stub that returns 0).

The game image globs every .c under src/mousewhack, so this file compiles to nothing unless
OPENFRENZY_MOUSEWHACK_TEST is defined.

What the tests pin down, and why it matters to the game:
- splat pool: the 3000 mark records are linked into the free list; marks taken from it come
  back when they end, so the pool never leaks (a leak makes splats stop appearing after a
  while: "adding splat mark FAILED") and never links a record twice.
- pause: while the game is paused the marks do not move or end.
- ground clip ("Tremors"): a splat that straddles a ground edge is cut at the edge, and the
  texture's v is cut by the same fraction (else the splat hangs in the air or is squashed);
  a splat clear of the edges is whole.
- tremor holes: a rising tremor takes only a free hole (state 0 or 1) and marks it taken
  (2, or 3 when rubble is in it), so two tremors never share a hole.
- roach patrol points: in "Pied Pipers" a roach away from the far points goes to one of 6,
  8, 9, 10; the choice draws the shared random seed exactly as often as the original, for
  every kind of starting point (later random events depend on the sequence).
*/

#ifdef OPENFRENZY_MOUSEWHACK_TEST

#include "mousewhack.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* data of modules that the test image does not link; only their storage matters here */
float DAT_002855f0[16];
unsigned int DAT_00285630 = 1;
float DAT_0028fda8 = 1.0f;
float DAT_0028ffa8;
float DAT_0028ffac;
unsigned char DAT_00290718[0x60 * 8];
int DAT_00292c78;
float DAT_00292c98;
float DAT_00293920;
int DAT_00293928;
int DAT_00293e80;
void *DAT_00293ea0[4];
int DAT_00293f08;
float DAT_00294b60[4];
float DAT_00294b70[2];
float DAT_00294b78[12];
float DAT_00294ba8;
float DAT_00294bac;
float DAT_00294bb0;
float DAT_00294bb4;
float DAT_00294bb8;
unsigned char DAT_002ba750[0x100];
float DAT_002ba990 = 60.0f;
int DAT_002bae6c;
int DAT_002bae70;
int DAT_002bae94 = 4;
int DAT_002bae98;
int DAT_002baeb8;
int DAT_002baebc;
int DAT_002baed4[40];
int DAT_002baed8[40];
int DAT_002baee0[40];
int DAT_002baee8[40];
int DAT_002baef4[40];
unsigned int DAT_002bc490[256];
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];
float DAT_002be588;
float DAT_002be58c;
longlong DAT_002cd7a0;
void *DAT_002db808;
float DAT_002ea478[4];
unsigned char *DAT_002ea488;
int DAT_002ecd90;
int DAT_002ecdec;
void *DAT_002ecfac;
unsigned char DAT_002ed324[0x100];
float DAT_002f2890;
float DAT_002f2894;
void *DAT_002f289c[4];

static int checks;
static int failures;

#define CHECK(cond, what) do { \
	checks++; \
	if (!(cond)) { \
		failures++; \
		fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, what); \
	} \
} while (0)

#define NEAR(a, b) ((a) - (b) < 0.0001f && (b) - (a) < 0.0001f)

/* the splat pool: a static buffer in place of the heap block */
static unsigned char pool[0x203a00] __attribute__((aligned(16)));

static int count_list(unsigned char *head)
{
	int n = 0;

	while (head != NULL && n <= 3001) {
		n++;
		head = MW_AT(head, 0, unsigned char *);
	}
	return n;
}

#define SP_ACTIVE MW_G(0x0030eef4, unsigned char *)
#define SP_FREE MW_G(0x0030eef8, unsigned char *)

static void test_splat_pool(void)
{
	float pos[4] = {100.0f, 50.0f, -20.0f, 1.0f};

	MW_G(0x0030eec0, unsigned char *) = pool;
	MW_Splat_Reset();
	CHECK(SP_ACTIVE == NULL, "reset: no active marks");
	CHECK(count_list(SP_FREE) == 3000, "reset: 3000 free records");
	CHECK(MW_AT(pool + 0x203740, 0, unsigned char *) == NULL, "reset: the last record ends the free list");

	Splat_AddCrumbs(pos, 0);
	CHECK(count_list(SP_ACTIVE) == 8, "crumbs: 8 active records");
	CHECK(count_list(SP_FREE) == 2992, "crumbs: taken from the free list");

	/* paused: nothing moves or ends */
	DAT_002ecdec = 1;
	MW_Splat_Update();
	CHECK(count_list(SP_ACTIVE) == 8, "paused: the marks stay");
	CHECK(NEAR(MW_AF(SP_ACTIVE, 8), 50.0f), "paused: a crumb does not move");
	DAT_002ecdec = 0;

	/* the ground (a stub) is at 0 under them: the crumbs land and end */
	MW_Splat_Update();
	CHECK(SP_ACTIVE == NULL, "landed crumbs end");
	CHECK(count_list(SP_FREE) == 3000, "every record is back in the free list");
}

static void test_ground_clip(void)
{
	unsigned char actor[0x200];
	unsigned char bug[0x1300];
	float out[4];
	int where;

	memset(actor, 0, sizeof actor);
	memset(bug, 0, sizeof bug);
	MW_AP(bug, 0) = actor;
	MW_AS(bug, 0x11ec) = 0;   /* a worm: splat size 75 */

	MW_AF(actor, 0xd8) = 0.0f;
	where = MW_Splat_ClipToGround(bug, out);
	CHECK(where == 3, "clear of the edges: a whole splat");

	/* straddles the edge at z 280, its middle before it */
	MW_AF(actor, 0xd8) = 250.0f;
	where = MW_Splat_ClipToGround(bug, out);
	CHECK(where == 2, "across an edge: cut");
	CHECK(NEAR(out[0], 280.0f) && NEAR(out[1], 175.0f), "cut at the edge, from its near side");
	CHECK(NEAR(out[2], 0.7f) && NEAR(out[3], 0.0f), "v cut by the same fraction (105 of 150)");

	/* its middle past the edge */
	MW_AF(actor, 0xd8) = 300.0f;
	where = MW_Splat_ClipToGround(bug, out);
	CHECK(where == 2, "past an edge: cut");
	CHECK(NEAR(out[0], 375.0f) && NEAR(out[1], 280.0f), "cut at the edge, to its far side");
	CHECK(NEAR(out[2], 1.0f) && NEAR(out[3], 55.0f / 150.0f), "v from the edge on (95 of 150)");
}

static void test_tremor_hole(void)
{
	static unsigned char bug[0x1300], actor[0x400];
	static unsigned char players[4][0x1410], pactors[4][0x400];
	int i, free_hole = 4;

	for (i = 0; i < 4; i++) {
		memset(players[i], 0, sizeof players[i]);
		memset(pactors[i], 0, sizeof pactors[i]);
		MW_AP(players[i], 0x30) = pactors[i];
		mw_Players[i] = players[i];
	}
	memset(bug, 0, sizeof bug);
	memset(actor, 0, sizeof actor);
	MW_AP(bug, 0) = actor;
	mw_Variation = 0;   /* "Brawl": holes at 0x002b2d48 */
	for (i = 0; i < 6; i++)
		MW_F(0x002b2d54 + i * 0x10) = 2.0f;   /* taken */
	MW_F(0x002b2d54 + free_hole * 0x10) = 1.0f;   /* free, with rubble */
	MW_AF(bug, 0x11f0) = 0.0f;
	Bug_Tremor_Rise(bug);
	CHECK(MW_AS(bug, 0x1242) == free_hole, "the tremor takes the only free hole");
	CHECK(MW_F(0x002b2d54 + free_hole * 0x10) == 3.0f, "the hole is taken, rubble remembered (1 -> 3)");
	CHECK(MW_AF(actor, 0xd0) == MW_F(0x002b2d48 + free_hole * 0x10) &&
		MW_AF(actor, 0xd8) == MW_F(0x002b2d50 + free_hole * 0x10), "the tremor stands in that hole");
	for (i = 0; i < 6; i++)
		if (i != free_hole)
			CHECK(MW_F(0x002b2d54 + i * 0x10) == 2.0f, "the other holes are untouched");
	for (i = 0; i < 6; i++)
		MW_F(0x002b2d54 + i * 0x10) = 0.0f;
}

/* the shared seed after `n` draws from `seed` */
static unsigned int seed_after(unsigned int seed, int n)
{
	while (n-- > 0)
		seed = seed * 0x343fdu + 0x269ec3u;
	return seed;
}

static void test_roach_points(void)
{
	static unsigned char bug[0x1300];
	static const struct { short from; int variation; int draws; } cases[] = {
		{1, 0, 1},     /* an edge point: (Core_Rand) then the point: 1 draw of the seed */
		{7, 0, 1},     /* (Core_Rand) then the point */
		{0x32, 0, 2},  /* a spawn tag: a rejected draw, then the point */
		{0x3c, 0, 2},
		{20, 0, 1},    /* any other: the point */
		{6, 3, 1},     /* "Pied Pipers" from a far point: as the others */
	};
	unsigned int seed;
	int i;

	memset(bug, 0, sizeof bug);
	for (i = 0; i < (int)(sizeof cases / sizeof cases[0]); i++) {
		mw_Variation = (short)cases[i].variation;
		MW_AS(bug, 0x1242) = cases[i].from;
		seed = DAT_00285630;
		Bug_Roach_ChoosePoint(bug);
		CHECK(DAT_00285630 == seed_after(seed, cases[i].draws), "the seed advances as in the original");
		CHECK(5 <= MW_AS(bug, 0x1240) && MW_AS(bug, 0x1240) <= 9, "the next point is one of 5..9");
	}
	/* "Pied Pipers" away from the far points: 6, 8, 9 or 10 (Core_Rand is a stub: 0 -> 6),
	no draw of the seed */
	mw_Variation = 3;
	MW_AS(bug, 0x1242) = 1;
	seed = DAT_00285630;
	Bug_Roach_ChoosePoint(bug);
	CHECK(MW_AS(bug, 0x1240) == 6, "Pied Pipers: a far point");
	CHECK(DAT_00285630 == seed, "Pied Pipers: the seed is not drawn");
	mw_Variation = 0;
}

int main(void)
{
	test_splat_pool();
	test_ground_clip();
	test_tremor_hole();
	test_roach_points();
	printf("mousewhack tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
