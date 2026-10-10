/*
demolition_a module tests: one guest image (src/demolition_a/tests/run_tests.sh builds it from
the game build's objects of src/demolition_a and the generated weak stubs).

The game image globs every .c under src/demolition_a, so this file compiles to nothing unless
OPENFRENZY_DEMOLITION_A_TEST is defined. The test defines the few outside functions it needs
(the token list of demolition_d, the area shape test of demolition_c, the heap of bkernel and
the particle constructors); every other outside call is a weak stub that returns 0.

What the tests pin down, and why it matters to the game:
- cylinder contact (CDemo_InCylinder, also used by demolition_b's bombs and demolition_c's gas
  jets): contact needs the heights to overlap and the xz distance to be at most the sum of the
  radii, on both sides. The original's axis pre-test only rejects one side; the final distance
  test must still reject the other side, or players far "behind" a token would collect it.
- token scoring (variation 2): a token of type 0, 1, 2 gives 1, 3, 5 points to one player only,
  once; a stopped token system gives nothing (the round is over).
- explosion pool: Init builds 12 explosions and 2 flashes with lights in the light system.
  demolition_b reuses these slots and switches the flash lights on, so they must all exist.
*/

#ifdef OPENFRENZY_DEMOLITION_A_TEST

#include "demolition_a.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int checks;
static int failures;

#define CHECK(cond, what) do { \
	checks++; \
	if (!(cond)) { \
		failures++; \
		fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, what); \
	} \
} while (0)

/* ---------- other modules' globals (the test image links only demolition_a; weak, zero) */

#define W __attribute__((weak))
W int DAT_00283e58, DAT_00283ea4;
W unsigned int DAT_00285630;
W float DAT_0028fda8, DAT_0028ffa8, DAT_0028ffac;
W float DAT_00290be4[4];
W float DAT_00292c60, DAT_00292c64, DAT_00292c68, DAT_00292c6c;
W int DAT_00292c78;
W unsigned char DAT_002b9968[0x1000];
W int DAT_002b9c58[4];
W unsigned char DAT_002ba750[0x1000];
W int DAT_002ba940, DAT_002ba944, DAT_002ba948, DAT_002ba94c;
W void *DAT_002ba9a0;
W int DAT_002bae70, DAT_002bae98, DAT_002baebc;
W int DAT_002baed8[40];
W uint32_t DAT_002bce18[256], DAT_002bc490[256], DAT_002bd640[256], DAT_002bc898[256];
W uint32_t DAT_002cd7a0, DAT_002cd7a4;
W unsigned char DAT_002ecd90[0x1000];
W int DAT_002ecdec;
W void *DAT_002ecfac;
W unsigned char DAT_002ed324[0x1000];
#undef W

/* ---------- outside functions supplied by the test (strong; the image has only weak stubs) */

/* demolition_d's token list: a fixed array of fake tokens */
#define TOKENS 3
static char *tokens[TOKENS];
static int token_count;
static int collected[TOKENS];

void *CToken_Next(void *token)
{
	int i;

	if (token == NULL)
		return token_count > 0 ? tokens[0] : NULL;
	for (i = 0; i + 1 < token_count; i++)
		if (tokens[i] == token)
			return tokens[i + 1];
	return NULL;
}

void CToken_Collect(struct CToken *self)
{
	int i;

	for (i = 0; i < token_count; i++)
		if (tokens[i] == (char *)self)
		{
			collected[i]++;
			DA_FIELD(int, self, 0x1e8) = 4;
		}
}

/* demolition_c: every point is inside every area (so token placement ends) */
uint DcShape_Contains(struct DcShape *self, float *p)
{
	(void)self;
	(void)p;
	return 1;
}

/* bkernel's heap */
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

/* the particle systems' constructors give each system a vtable whose slots count calls */
static int ps_slot_calls;
static void ps_slot(void *self) { (void)self; ps_slot_calls++; }
static const void *const ps_vtable[8] = {
	(const void *)ps_slot, (const void *)ps_slot, (const void *)ps_slot, (const void *)ps_slot,
	(const void *)ps_slot, (const void *)ps_slot, (const void *)ps_slot, (const void *)ps_slot,
};
struct DcSparkSystem *DcSparkSystem_Construct(struct DcSparkSystem *self)
{
	*(const void *const **)self = ps_vtable;
	return self;
}
void CSmokeSystem_Construct(struct CSmokeSystem *self) { *(const void *const **)self = ps_vtable; }
void CGlowSystem_Construct(struct CGlowSystem *self) { *(const void *const **)self = ps_vtable; }
struct DcDebrisSystem *DcDebrisSystem_Construct(struct DcDebrisSystem *self)
{
	*(const void *const **)self = ps_vtable;
	return self;
}

/* ---------- a fake game object */

static char *game;
static char area[0x40];
static void *spawn_point(void *self, int player) { (void)self; (void)player; return area; }
static struct CDemolition_VariationVtbl variation_vtbl;
static const struct CDemolition_VariationVtbl *variation = &variation_vtbl;

static char *new_player(float x, float y, float z)
{
	char *p = calloc(1, 0x1300);

	DA_FIELD(float, p, DP_POS) = x;
	DA_FIELD(float, p, DP_POS + 4) = y;
	DA_FIELD(float, p, DP_POS + 8) = z;
	DA_FIELD(float, p, DP_RADIUS) = 30.0f;
	DA_FIELD(float, p, DP_HEIGHT) = 100.0f;
	return p;
}

static char *new_token(float x, float y, float z, int type)
{
	char *t = calloc(1, 0x290);

	DA_FIELD(float, t, 0x180) = x;
	DA_FIELD(float, t, 0x184) = y;
	DA_FIELD(float, t, 0x188) = z;
	DA_FIELD(int, t, 0x1ec) = type;
	return t;
}

static void setup(void)
{
	game = calloc(1, DG_SIZE);
	DAT_002ff85c = game;
	variation_vtbl.spawnPoint = spawn_point;
	DA_FIELD(const void *, game, DG_VARIATIONOBJ) = &variation;
	DA_FIELD(float, area, 0x34) = 500.0f; /* half size */
	token_count = 0;
	memset(collected, 0, sizeof(collected));
}

/* ---------- tests */

static void test_cylinder(void)
{
	float point[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float base[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 1, "same place touches");
	base[0] = 129.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 1, "inside the sum of the radii (+x)");
	base[0] = 131.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 0, "outside the sum of the radii (+x)");
	base[0] = -129.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 1, "inside the sum of the radii (-x)");
	base[0] = -131.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 0,
		"outside on the side the pre-test does not reject (-x)");
	base[0] = 0.0f;
	base[2] = -131.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 0, "outside (-z)");
	base[2] = 0.0f;
	base[1] = 101.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 0, "base above the top");
	base[1] = -101.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 0, "base top below the point");
	base[1] = -99.0f;
	CHECK(CDemo_InCylinder(point, 100.0f, 100.0f, base, 30.0f, 100.0f) == 1, "heights overlap");
}

static void test_token_scoring(void)
{
	char *ts;
	char *a, *b;
	static const int points[3] = { 1, 3, 5 };
	int type;

	for (type = 0; type < 3; type++)
	{
		setup();
		ts = game + DG_TOKENS;
		CDemoTokenSystem_Ctor(ts);
		a = new_player(0.0f, 0.0f, 0.0f);
		b = new_player(10.0f, 0.0f, 0.0f); /* also touching the token */
		DA_FIELD(char *, a, DP_NEXT) = b;
		DA_FIELD(char *, game, DG_PLAYERS) = a;
		tokens[0] = new_token(5.0f, 0.0f, 0.0f, type);
		tokens[1] = new_token(5000.0f, 0.0f, 0.0f, 2); /* far away */
		token_count = 2;

		CDemoTokenSystem_UpdateTokens(ts, 0.02f);
		CHECK(DA_FIELD(int, a, DP_SCORE) == points[type], "a token gives 1, 3 or 5 points");
		CHECK(DA_FIELD(int, b, DP_SCORE) == 0, "only the first player collects a token");
		CHECK(collected[0] == 1 && collected[1] == 0, "only the touched token is collected");

		CDemoTokenSystem_UpdateTokens(ts, 0.02f);
		CHECK(DA_FIELD(int, a, DP_SCORE) == points[type], "a collected token scores once");
	}

	setup();
	ts = game + DG_TOKENS;
	a = new_player(0.0f, 0.0f, 0.0f);
	DA_FIELD(char *, game, DG_PLAYERS) = a;
	tokens[0] = new_token(5.0f, 0.0f, 0.0f, 2);
	token_count = 1;
	CDemoTokenSystem_Stop(ts);
	CDemoTokenSystem_UpdateTokens(ts, 0.02f);
	CHECK(DA_FIELD(int, a, DP_SCORE) == 0 && collected[0] == 0, "a stopped system scores nothing");

	CDemoTokenSystem_Init(ts, NULL);
	CHECK(DA_FIELD(char, ts, 4) == 0 && DA_FIELD(char, ts, 5) == 0, "Init: disabled, not stopped");
	CHECK(DA_FIELD(int, ts, 0x10) >= 0 && DA_FIELD(int, ts, 0x10) <= 3, "Init: one of the 4 areas");
	CHECK(DA_FIELD(float, ts, 0x14) >= 100.0f && DA_FIELD(float, ts, 0x14) <= 300.0f, "Init: area timer");
	CHECK(DA_FIELD(float, ts, 0x18) >= 50.0f && DA_FIELD(float, ts, 0x18) <= 100.0f, "Init: bonus timer");
}

static void test_explosion_pool(void)
{
	struct CDemolition_ExplosionSystem *es;
	struct CDemolition_Explosion *e, *f;
	struct CDemolition_LightSystem *ls;
	int n = 0, distinct = 1, inactive = 1;

	setup();
	es = (struct CDemolition_ExplosionSystem *)(game + DG_EXPLOSIONS);
	ls = (struct CDemolition_LightSystem *)(game + DG_LIGHTS);
	CDemoExplosionSystem_Ctor(es);
	ps_slot_calls = 0;
	CHECK(CDemoExplosionSystem_Init(es, NULL) == 1, "Init succeeds");
	for (e = es->first; e != NULL && n < 100; e = e->next)
	{
		n++;
		if (e->active || e->visible)
			inactive = 0;
		for (f = e->next; f != NULL; f = f->next)
			if (f == e)
				distinct = 0;
	}
	CHECK(n == 12, "12 explosions");
	CHECK(distinct, "the explosions are distinct");
	CHECK(inactive, "the explosions start inactive");
	CHECK(ps_slot_calls == 12 * 4, "each particle system is reset once");
	CHECK(es->flashes[0].light != NULL && es->flashes[1].light != NULL, "both flashes have a light");
	CHECK(es->flashes[0].active == 0 && es->flashes[1].active == 0, "the flashes start inactive");
	CHECK(ls->first == es->flashes[1].light && ls->first->next == es->flashes[0].light,
		"the flash lights are in the light system");
}

int main(void)
{
	test_cylinder();
	test_token_scoring();
	test_explosion_pool();
	fprintf(stderr, "demolition_a tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
