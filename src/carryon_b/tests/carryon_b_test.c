/*
CARRYON_B_TEST.C

carryon_b module tests (src/carryon_b/tests/run_tests.sh builds this as the main() of a small
guest image: the carryon_b objects, the generated stubs and the platform layer). They check
the rules of the CarryOn minigame that the decompiled code must keep:

- a thrown rocket bounces four times, each bounce quieter, then lies still on the ground;
- a player picks up a lying rocket only when near it, at its height and below the carry
  limit (5 rockets in variant 0, else 1); the rocket then goes back to the free state;
- a rocket above a player's head drops to state 7 (a missed catch);
- smoke puffs come from the free list and go back to it when their life ends;
- the frame timer turns timer ticks into milliseconds and 60 Hz frames, and keeps at most
  50 marks.

The game build compiles every .c under src/carryon_b: without OPENFRENZY_CARRYON_B_TEST
(set by run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_CARRYON_B_TEST

#include "carryon_b.h"

/* ---------- globals of modules that this image does not link: zeroed 0x400-byte blocks,
   except the ones the tests set */

unsigned int DAT_00285630 = 1;
void *DAT_002f6d28[4];
void *DAT_002f6d5c;
int DAT_002bae94;
long long DAT_002cd7a0;

__attribute__((nocommon, aligned(16))) unsigned char of_cb_test_foreign[42 * 0x400];
__asm__(
	".globl DAT_002855f0\n.set DAT_002855f0, of_cb_test_foreign + 0x0\n"
	".globl DAT_00290718\n.set DAT_00290718, of_cb_test_foreign + 0x400\n"
	".globl DAT_00290719\n.set DAT_00290719, of_cb_test_foreign + 0x800\n"
	".globl DAT_0029071a\n.set DAT_0029071a, of_cb_test_foreign + 0xc00\n"
	".globl DAT_00292c60\n.set DAT_00292c60, of_cb_test_foreign + 0x1000\n"
	".globl DAT_00292c64\n.set DAT_00292c64, of_cb_test_foreign + 0x1400\n"
	".globl DAT_00292c68\n.set DAT_00292c68, of_cb_test_foreign + 0x1800\n"
	".globl DAT_00292c6c\n.set DAT_00292c6c, of_cb_test_foreign + 0x1c00\n"
	".globl DAT_00293928\n.set DAT_00293928, of_cb_test_foreign + 0x2000\n"
	".globl DAT_00293ad8\n.set DAT_00293ad8, of_cb_test_foreign + 0x2400\n"
	".globl DAT_00293ae8\n.set DAT_00293ae8, of_cb_test_foreign + 0x2800\n"
	".globl DAT_00293b44\n.set DAT_00293b44, of_cb_test_foreign + 0x2c00\n"
	".globl DAT_00294108\n.set DAT_00294108, of_cb_test_foreign + 0x3000\n"
	".globl DAT_0029410c\n.set DAT_0029410c, of_cb_test_foreign + 0x3400\n"
	".globl DAT_00294110\n.set DAT_00294110, of_cb_test_foreign + 0x3800\n"
	".globl DAT_00294114\n.set DAT_00294114, of_cb_test_foreign + 0x3c00\n"
	".globl DAT_00294118\n.set DAT_00294118, of_cb_test_foreign + 0x4000\n"
	".globl DAT_00294120\n.set DAT_00294120, of_cb_test_foreign + 0x4400\n"
	".globl DAT_00294124\n.set DAT_00294124, of_cb_test_foreign + 0x4800\n"
	".globl DAT_00294128\n.set DAT_00294128, of_cb_test_foreign + 0x4c00\n"
	".globl DAT_00294390\n.set DAT_00294390, of_cb_test_foreign + 0x5000\n"
	".globl DAT_00294394\n.set DAT_00294394, of_cb_test_foreign + 0x5400\n"
	".globl DAT_00294398\n.set DAT_00294398, of_cb_test_foreign + 0x5800\n"
	".globl DAT_002943b4\n.set DAT_002943b4, of_cb_test_foreign + 0x5c00\n"
	".globl DAT_002943b8\n.set DAT_002943b8, of_cb_test_foreign + 0x6000\n"
	".globl DAT_002943d0\n.set DAT_002943d0, of_cb_test_foreign + 0x6400\n"
	".globl DAT_002ba940\n.set DAT_002ba940, of_cb_test_foreign + 0x6800\n"
	".globl DAT_002ba944\n.set DAT_002ba944, of_cb_test_foreign + 0x6c00\n"
	".globl DAT_002ba948\n.set DAT_002ba948, of_cb_test_foreign + 0x7000\n"
	".globl DAT_002ba94c\n.set DAT_002ba94c, of_cb_test_foreign + 0x7400\n"
	".globl DAT_002baeac\n.set DAT_002baeac, of_cb_test_foreign + 0x7800\n"
	".globl DAT_002baed4\n.set DAT_002baed4, of_cb_test_foreign + 0x7c00\n"
	".globl DAT_002baed8\n.set DAT_002baed8, of_cb_test_foreign + 0x8000\n"
	".globl DAT_002bc490\n.set DAT_002bc490, of_cb_test_foreign + 0x8400\n"
	".globl DAT_002bc898\n.set DAT_002bc898, of_cb_test_foreign + 0x8800\n"
	".globl DAT_002bce18\n.set DAT_002bce18, of_cb_test_foreign + 0x8c00\n"
	".globl DAT_002bd640\n.set DAT_002bd640, of_cb_test_foreign + 0x9000\n"
	".globl DAT_002be330\n.set DAT_002be330, of_cb_test_foreign + 0x9400\n"
	".globl DAT_002db808\n.set DAT_002db808, of_cb_test_foreign + 0x9800\n"
	".globl DAT_002ecfac\n.set DAT_002ecfac, of_cb_test_foreign + 0x9c00\n"
	".globl DAT_002f6d60\n.set DAT_002f6d60, of_cb_test_foreign + 0xa000\n"
	".globl DAT_002f7960\n.set DAT_002f7960, of_cb_test_foreign + 0xa400\n"
);

/* ---------- test doubles of other modules' functions (replace the weak stubs) */

/* flat ground at height g_ground: the probe returns the drop from `pos` */
static float g_ground;
float FUN_000b3ff0(float *pos, float radius, void **tri_out, void **platform_out, int mode)
{
	(void)radius;
	(void)tri_out;
	(void)platform_out;
	(void)mode;
	return pos[1] - g_ground;
}

static int g_sound_kind[32], g_sound_volume[32], g_sounds;
/* (defined by address: carryon_a may still rename it) */
int FUN_000c77c0(struct CFireResource *self, int kind, int variant, int volume)
{
	(void)self;
	(void)variant;
	if (g_sounds < 32)
	{
		g_sound_kind[g_sounds] = kind;
		g_sound_volume[g_sounds] = volume;
	}
	g_sounds++;
	return 0;
}

static void *g_given_player;
static int g_given_type, g_given_count, g_gives;
int FUN_000c0af0(struct CCarryOnPlayer *self, int how, int type, int count)
{
	(void)how;
	g_given_player = self;
	g_given_type = type;
	g_given_count = count;
	g_gives++;
	return 1;
}

static unsigned long long g_ticks;
unsigned long long FUN_0004c5f0(void)
{
	return g_ticks;
}

unsigned long long FUN_0004c670(unsigned long long start, unsigned long long end)
{
	return end - start;
}

/* ---------- checks */

static int checks, failures;
#define CHECK(cond, what) \
	do \
	{ \
		checks++; \
		if (!(cond)) \
		{ \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, (what)); \
		} \
	} while (0)

/* the game object (carryon_a): +0x250 variant, +0x2b4 rocket animation, +0x374 catch radius */
static unsigned char g_game[0x400];
/* one player: +0x30 its body (position at +0xd0), +0x1130 rockets carried */
static unsigned char g_player[0x1200];
static unsigned char g_body[0x100];

static void setup_world(int variant, int players)
{
	memset(g_game, 0, sizeof g_game);
	memset(g_player, 0, sizeof g_player);
	memset(g_body, 0, sizeof g_body);
	CB_I32(g_game, 0x250) = variant;
	CB_F32(g_game, 0x374) = 50.0f;      /* below 100: the rocket uses 100 */
	CB_PTR(g_player, 0x30) = g_body;
	DAT_002f6d5c = g_game;
	DAT_002f6d28[0] = g_player;
	DAT_002bae94 = players;
	g_ground = 0.0f;
	g_sounds = 0;
	g_gives = 0;
}

static void place_player(float x, float y, float z, int carried)
{
	CB_F32(g_body, 0xd0) = x;
	CB_F32(g_body, 0xd4) = y;
	CB_F32(g_body, 0xd8) = z;
	CB_I32(g_player, 0x1130) = carried;
}

static void lying_rocket(struct CFire_Rocket *rocket, int type)
{
	CFire_Rocket_ctor(rocket, 0);
	rocket->state = 6;
	rocket->type = type;
	rocket->pos[3] = 1.0f;
}

static void test_rocket_bounces(void)
{
	struct CFire_Rocket rocket;
	float start[4] = { 0.0f, 100.0f, 0.0f, 1.0f };
	int i, bounces = 0;

	setup_world(1, 0);
	CFire_Rocket_ctor(&rocket, 0);
	CHECK(rocket.state == 0 && rocket.actor == NULL, "a new rocket is free and has no model");
	CFire_Rocket_Throw(&rocket, start, 0.0f, 0.0f, 0.0f, 3, NULL);
	CHECK(rocket.state == 2 && rocket.type == 3, "a thrown rocket flies (state 2)");
	CHECK(g_sounds == 1 && g_sound_kind[0] == 0xb, "throwing plays the throw sound");
	for (i = 0; i < 1000 && rocket.state != 6; i++)
		CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 6, "after four bounces the rocket lies on the ground (state 6)");
	CHECK(rocket.pos[1] == 0.0f, "it lies exactly on the ground");
	for (i = 1; i < g_sounds && i < 32; i++)
		if (g_sound_kind[i] == 0xd)
			bounces++;
	CHECK(bounces == 4, "one bounce sound per bounce");
	CHECK(g_sound_volume[1] == 0x80 && g_sound_volume[2] == 0x60 && g_sound_volume[3] == 0x40 &&
		g_sound_volume[4] == 0x20, "each bounce is quieter");
	for (i = 0; i < 100; i++)
		CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 6 && rocket.pos[1] == 0.0f, "a lying rocket stays put");
	CHECK(rocket.phase >= 0.0f && rocket.phase < 1.0f, "its spin phase wraps within 0-1");
}

static void test_rocket_pickup(void)
{
	struct CFire_Rocket rocket;

	/* variant 1: a player carries one rocket at most */
	setup_world(1, 1);
	place_player(150.0f, 0.0f, 0.0f, 0);
	lying_rocket(&rocket, 4);
	CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 6 && g_gives == 0, "a player 150 away does not pick the rocket up");

	place_player(60.0f, 0.0f, 60.0f, 0);
	CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 0, "a near player picks the rocket up; it is free again");
	CHECK(g_gives == 1 && g_given_player == g_player && g_given_type == 4 && g_given_count == 1,
		"the player gets one rocket of the rocket's type");

	lying_rocket(&rocket, 4);
	place_player(10.0f, 0.0f, 0.0f, 1);
	g_gives = 0;
	CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 6 && g_gives == 0, "variant 1: a player with one rocket cannot take more");

	/* variant 0: up to 5 */
	setup_world(0, 1);
	place_player(10.0f, 0.0f, 0.0f, 4);
	lying_rocket(&rocket, 1);
	CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 0 && g_gives == 1, "variant 0: a player with 4 rockets takes a fifth");
	place_player(10.0f, 0.0f, 0.0f, 5);
	lying_rocket(&rocket, 1);
	g_gives = 0;
	CFire_Rocket_Update(&rocket, 1.0f);
	CHECK(rocket.state == 6 && g_gives == 0, "variant 0: but not a sixth");
}

static void test_rocket_catch(void)
{
	struct CFire_Rocket rocket;

	setup_world(1, 1);
	lying_rocket(&rocket, 2);
	place_player(0.0f, 200.0f, 0.0f, 0);
	CHECK(CFire_Rocket_CheckCatch(&rocket, 0) == 0, "a rocket far below the player is not caught");
	CHECK(rocket.state == 7, "and is marked as missed (state 7)");
	place_player(0.0f, 50.0f, 0.0f, 0);
	CHECK(CFire_Rocket_CheckCatch(&rocket, 0) == 1 && rocket.state == 0 && g_gives == 1,
		"a rocket at the player's height is caught");
	rocket.state = 0;
	CHECK(CFire_Rocket_CheckCatch(&rocket, 0) == 0, "a free rocket cannot be caught");
}

static int count_puffs(struct CFire_Puff *puff)
{
	int n = 0;

	for (; puff != NULL; puff = puff->next)
		n++;
	return n;
}

static void test_smoke(void)
{
	struct CFire_Puff puffs[3];
	struct CFire_Smoke smoke;
	float pos[4] = { 10.0f, 20.0f, 30.0f, 0.0f };
	int i;

	memset(puffs, 0, sizeof puffs);
	memset(&smoke, 0, sizeof smoke);
	puffs[0].next = &puffs[1];
	puffs[1].next = &puffs[2];
	smoke.puffs = puffs;
	smoke.free = puffs;
	CFire_Smoke_Add(&smoke, pos, 30.0f, 2.0f, 6.0f);
	CFire_Smoke_Add(&smoke, pos, 30.0f, 2.0f, 6.0f);
	CHECK(count_puffs(smoke.used) == 2 && count_puffs(smoke.free) == 1,
		"added puffs move from the free list to the used list");
	CHECK(smoke.used->life >= 2.0f && smoke.used->life <= 4.0f, "a puff lives 1-2 times `life`");
	CHECK(smoke.used->pos[1] == 20.0f && smoke.used->pos[3] == 1.0f,
		"a puff starts at the height given (spread only in x and z)");
	CHECK(smoke.used->vel[1] >= 4.0f && smoke.used->vel[1] <= 8.0f, "and rises at 4-8 per frame");
	CHECK(smoke.used->alpha == 255.0f, "opaque");
	CFire_Smoke_Add(&smoke, pos, 30.0f, 2.0f, 6.0f);
	CFire_Smoke_Add(&smoke, pos, 30.0f, 2.0f, 6.0f);
	CHECK(count_puffs(smoke.used) == 3 && smoke.free == NULL, "with no free puff, Add does nothing");
	for (i = 0; i < 5; i++)
		CFire_Smoke_Update(&smoke, 0.0f);
	CHECK(smoke.used == NULL && count_puffs(smoke.free) == 3,
		"puffs whose life ends go back to the free list");
}

static void test_timer(void)
{
	struct CFire_Timer timer;
	int i;

	memset(&timer, 0, sizeof timer);
	CFire_Timer_ctor(&timer);
	DAT_002cd7a0 = 1000;
	g_ticks = 5000;
	CHECK(CFire_Timer_Init(&timer) == 1 && timer.ticksPerSecond == 1000, "Init reads the frequency");
	g_ticks += 20;
	CFire_Timer_Frame(&timer);
	CHECK(timer.ms == 20.0f, "20 ticks at 1000 per second are 20 ms");
	CHECK(timer.frames > 1.19f && timer.frames < 1.21f, "and 1.2 frames at 60 Hz");
	for (i = 0; i < 60; i++)
	{
		g_ticks++;
		CFire_Timer_Mark(&timer, 0xff00ff00u);
	}
	CHECK(timer.marks == 50, "at most 50 marks");
	CHECK(timer.markTimes[0] == 1.0f && timer.markTimes[49] == 50.0f,
		"marks are ms since the frame start");
	timer.sections[2].start = g_ticks;
	g_ticks += 7;
	CFire_Timer_End(&timer, 2);
	CHECK(timer.sections[2].ms == 7.0f, "a section measures from its start");
	CFire_Timer_End(&timer, 5);    /* out of range: ignored */
	CFire_Timer_Frame(&timer);
	CHECK(timer.marks == 0, "a new frame clears the marks");
}

int main(void)
{
	test_rocket_bounces();
	test_rocket_pickup();
	test_rocket_catch();
	test_smoke();
	test_timer();
	fprintf(stderr, "carryon_b tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
