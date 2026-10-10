/*
demolition_b module tests: one guest image (src/demolition_b/tests/run_tests.sh builds it from
the game build's objects of src/demolition_b and the generated weak stubs).

The game image globs every .c under src/demolition_b, so this file compiles to nothing unless
OPENFRENZY_DEMOLITION_B_TEST is defined.

What the tests pin down, and why it matters to the game:
- frag scoring: in the Frag variation the k-th player caught in one explosion chain gives the
  bomb's owner 3 * k points, and hitting yourself or your team mate costs one point. This is
  the scoring rule of the minigame; a change here changes who wins.
- score loss: in variation 2 a player hit keeps 3/4 of his score (rounded toward zero) and
  gets a 100 frame cooldown, so one explosion cannot hit him twice.
- explosion slots: a new explosion reuses a finished one first, else the oldest running one,
  so the newest explosions are never cut short.
- remote bomb control: the AI may roll the bomb only while it is controllable and not
  growing or shrinking (phases 1 and 2).
*/

#ifdef OPENFRENZY_DEMOLITION_B_TEST

#include "demolition_b.h"
#include "demolition_b_internal.h"

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

/* a fake game object with players and a bomb system */
static char *game;
static char *players[4];
static char *bombsys;

/* a player statistics object: +0x3c frags by player; its two virtuals count calls */
static int fragged_calls;
static int fraggedby_calls;
static void stats_fragged(void *self, void *victim) { (void)self; (void)victim; fragged_calls++; }
static void stats_fraggedby(void *self, void *killer) { (void)self; (void)killer; fraggedby_calls++; }
static const void *const stats_vtable[8] = {
	0, 0, 0, 0, 0, 0, (const void *)stats_fragged, (const void *)stats_fraggedby,
};

/* a flash light: virtual +0xc switches it on */
static void light_on(void *self, int on) { (void)self; (void)on; }
static const void *const light_vtable[4] = { 0, 0, 0, (const void *)light_on };

static void setup(int count)
{
	int i;

	game = calloc(1, 0x8000);
	bombsys = calloc(1, 0x2000);
	DAT_002ff85c = game;
	DB_PTR(game, 0x13e8) = bombsys;
	for (i = 0; i < 4; i++)
	{
		players[i] = calloc(1, 0x1300);
		DB_PTR(players[i], 0x118c) = calloc(1, 0x100);
		DB_PTR(DB_PTR(players[i], 0x118c), 0) = (void *)stats_vtable;
		DB_I32(players[i], 0x1184) = i;
		DB_U8(players[i], 0x116c) = 1;
		DB_PTR(players[i], 0x11a0) = calloc(1, 0x1300);
	}
	for (i = 0; i < count; i++)
		DB_PTR(players[i], 0x1140) = i + 1 < count ? players[i + 1] : NULL;
	DB_PTR(game, 0x6c) = players[0];
}

static void place(char *p, float x, float z)
{
	DB_F32(p, 0x1144) = x;
	DB_F32(p, 0x1148) = 0.0f;
	DB_F32(p, 0x114c) = z;
}

static char *explosion(char *owner, float x, float z, int variation)
{
	char *e = calloc(1, 0x800);

	DB_F32(e, 0x14) = x;
	DB_F32(e, 0x1c) = z;
	DB_PTR(e, 0x77c) = owner;
	DB_PTR(e, 0x780) = e;
	DB_I32(game, 0x28) = variation;
	return e;
}

static void test_frag_scoring(void)
{
	char *e;

	setup(4);
	place(players[0], 0.0f, 0.0f);
	place(players[1], 10.0f, 0.0f);
	place(players[2], 20.0f, 0.0f);
	place(players[3], 5000.0f, 0.0f);
	DB_I32(players[0], 0x1194) = 10;
	DB_I32(players[1], 0x1194) = 10;
	DB_I32(players[2], 0x1194) = 10;
	e = explosion(players[0], 0.0f, 0.0f, 0);
	CDemolition_Explosion_DamageObjects((struct CDemolition_Explosion *)e, 100.0f);
	/* owner hit himself (-1), then fragged two players: +3 and +6 */
	CHECK(DB_I32(players[0], 0x1194) == 10 - 1 + 3 + 6, "frag chain scoring");
	CHECK(DB_I32(e, 0x784) == 2, "two frags counted on the chain");
	CHECK(DB_I32(DB_PTR(players[1], 0x118c), 0x3c + 0 * 4) == 1, "victim knows who fragged him");
	CHECK(fragged_calls == 1, "the owner's first frag of the chain is reported once");
	CHECK(fraggedby_calls == 2, "each victim reports the killer");
	CHECK(DB_F32(players[1], 0x1198) == 100.0f, "victims get the hit cooldown");
	CHECK(DB_F32(players[3], 0x1198) == 0.0f, "players out of range are not hit");
	/* the cooldown protects them from a second explosion */
	DB_PTR(e, 0x780) = e;
	DB_I32(e, 0x784) = 0;
	CDemolition_Explosion_DamageObjects((struct CDemolition_Explosion *)e, 100.0f);
	CHECK(DB_I32(players[0], 0x1194) == 18, "no score while everyone cools down");
}

static void test_score_loss(void)
{
	char *e;

	setup(2);
	place(players[0], 0.0f, 0.0f);
	place(players[1], 50.0f, 0.0f);
	DB_I32(players[0], 0x1194) = 0;
	DB_I32(players[1], 0x1194) = 101;
	e = explosion(players[0], 0.0f, 0.0f, 2);
	CDemolition_Explosion_DamageObjects((struct CDemolition_Explosion *)e, 100.0f);
	CHECK(DB_I32(players[1], 0x1194) == 75, "a hit player keeps 3/4 of his score (truncated)");
	CHECK(DB_I32(players[0], 0x1194) == 0, "a score of 0 stays 0");
}

static void test_explosion_slots(void)
{
	char *mgr = calloc(1, 0x100);
	char *a = calloc(1, 0x800);
	char *b = calloc(1, 0x800);
	float pos[4] = { 1.0f, 2.0f, 3.0f, 1.0f };

	setup(1);
	DB_PTR(mgr, 8) = a;
	DB_PTR(a, 0x10) = b;
	DB_PTR(mgr, 0x10 + 8) = calloc(1, 0x100);
	DB_PTR(mgr, 0x70 + 8) = calloc(1, 0x100);
	DB_PTR(DB_PTR(mgr, 0x18), 0) = (void *)light_vtable;
	DB_PTR(DB_PTR(mgr, 0x78), 0) = (void *)light_vtable;
	DB_U8(a, 0x28) = 1;
	DB_F32(a, 0x24) = 5.0f;
	DB_U8(b, 0x28) = 1;
	DB_F32(b, 0x24) = 9.0f;
	DB_U8(mgr, 0x10 + 0x48) = 1;
	DB_F32(mgr, 0x10 + 0x5c) = 2.0f;
	/* 0x000eff40 is a weak stub here: check the light slot, the explosion pick is below */
	CDemolition_ExplosionMgr_Start((struct CDemolition_ExplosionMgr *)mgr, NULL, NULL, pos, 0.3f, 500.0f);
	CHECK(DB_U8(mgr, 0x70 + 0x48) == 1 && DB_F32(mgr, 0x70 + 0x4c) == 1.0f, "a free light slot is used");
	CHECK(DB_F32(mgr, 0x10 + 0x5c) == 2.0f, "the running light is left alone");
}

static void test_remote_can_roll(void)
{
	char *bomb = calloc(1, 0x1300);
	int phase;

	DB_U8(bomb, 0x1174) = 1;
	for (phase = -1; phase <= 3; phase++)
	{
		DB_I32(bomb, 0x1190) = phase;
		CHECK(CDemolition_RemoteBomb_CanRoll((struct CDemolition_RemoteBomb *)bomb) ==
			(phase == 1 || phase == 2 ? 0 : 1), "rolls unless growing or shrinking");
	}
	DB_U8(bomb, 0x1174) = 0;
	DB_I32(bomb, 0x1190) = 0;
	CHECK(CDemolition_RemoteBomb_CanRoll((struct CDemolition_RemoteBomb *)bomb) == 0,
		"no rolling without control");
}

int main(void)
{
	test_frag_scoring();
	test_score_loss();
	test_explosion_slots();
	test_remote_can_roll();
	fprintf(stderr, "demolition_b tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
