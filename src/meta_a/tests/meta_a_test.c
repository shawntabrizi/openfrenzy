/*
META_A_TEST.C

meta_a module tests (src/meta_a/tests/run_tests.sh builds this as the main() of a small
guest image: meta_a's Effects.c, unplaced_lightning.c, Meta .data block and globals, the
generated stubs and the platform layer). What they pin down, and why it matters:

- sliding-texture models (0x0015b690/0x0015b730/0x0015b760): u and v stay in [0, 1) while
  they scroll (they feed a texture matrix every frame; without the wrap they grow without
  limit and lose float precision), a model that is off does not move, and the puzzle switch
  changes only the models with its tag.
- glow sprites (0x0015c400, 0x0015b650): a new sprite keeps the level script's values in the
  slots the draw code reads, and the level's unload frees every sprite (a sprite left in
  the list would be drawn on the next board with a freed texture).
- goal effects (0x0015be40): the burst is scheduled 20 frames after the score, at the goal.
- lightning (0x0015ca40, 0x0015cb80, 0x0015cc80): the live-bolt counter counts a bolt once
  (meta_b's update decrements it when the bolt dies; a double count never returns to 0); a
  bolt starts at its target and ends 3000 units above; it hurts only players within 200
  units who are not already hit (state 10) or in state 13, and pushes them away at speed 4;
  the drawn bolt runs from start to end through points scattered only sideways (at most 75
  units), and its alpha is brightness + 100 (at most 255) so dim bolts stay visible. As in
  the original, the first vertex's colour is not written.

The game build compiles every .c under src/meta_a: without OPENFRENZY_META_A_TEST (set by
run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_META_A_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "meta_a.h"

/* ---------- data of other modules (the game links their owners) */

float DAT_0028fda8;                   /* classlib_ui: frame time factor */
int DAT_00284b10, DAT_00284b14, DAT_00284b18; /* bactor: ambient light */
float DAT_002855f0[16];               /* bdisplay: identity matrix */
unsigned int DAT_00285630;            /* the rand() seed */
unsigned char DAT_00290718[0x60 * 16]; /* classlib_ui: character colours */
int DAT_002baed4[40];                 /* core: players, 10 ints each */
unsigned int DAT_002bc490[256], DAT_002bc898[256], DAT_002bce18[256], DAT_002bd640[256];
unsigned char DAT_002ea478[16];       /* charlib: zero vector */
int *DAT_002ea488;                    /* charlib: the actor list */
unsigned char DAT_00308440[0xc];      /* core static constructors: the glow sprites */
unsigned char DAT_0030844c[0xc];      /* the sliding-texture models */
int DAT_003090b4;                     /* meta_b: water areas */
void *DAT_003090b8[6];

/* ---------- test doubles of engine services (the game uses bKernel's heap and ClassLib's
   lists; these copy their behaviour, as src/meta_b/tests/meta_b_test.c does) */

void *bkHeapAlloc(uint size, const char *file, int line, uint flags)
{
	(void)file;
	(void)line;
	(void)flags;
	return calloc(1, size);
}

static int frees;
void bkHeapFree(void *pointer)
{
	frees++;
	free(pointer);
}

/* classlib_ui ClassLib/unplaced.c 0x00071050-0x00071110 */
int *CLList_Init(int *list)
{
	list[1] = (int)list;
	list[0] = (int)list;
	list[2] = 0;
	return list;
}

void CLList_Insert(int *list, int *node, int at_tail)
{
	int *last;
	int first;

	if (at_tail != 0) {
		last = (int *)list[1];
		node[1] = (int)last;
		node[0] = *last;
		*(int **)(*last + 4) = node;
		*last = (int)node;
		list[2] = list[2] + 1;
		return;
	}
	first = list[0];
	node[0] = first;
	node[1] = *(int *)(first + 4);
	**(int ***)(first + 4) = node;
	*(int **)(first + 4) = node;
	list[2] = list[2] + 1;
}

int CLList_Remove(int *list, int *node)
{
	if (node[0] != 0) {
		*(int *)node[1] = node[0];
		*(int *)(node[0] + 4) = node[1];
		list[2] = list[2] - 1;
		node[0] = 0;
	}
	return node[1];
}

unsigned int CLList_Next(unsigned int *list, unsigned int *node)
{
	if (node == NULL)
		node = list;
	return *node & (((unsigned int *)*node == list) - 1);
}

/* bKernel: turn a vector about the y axis */
void bmVectorRotateY(float *out, const float *v, float angle)
{
	float x = v[0], z = v[2];

	out[0] = x * cosf(angle) + z * sinf(angle);
	out[1] = v[1];
	out[2] = z * cosf(angle) - x * sinf(angle);
	out[3] = v[3];
}

static int drawn_type, drawn_count;
static void *drawn_vertices;
void bdDrawPrimitive(int type, void *vertices, int count)
{
	drawn_type = type;
	drawn_vertices = vertices;
	drawn_count = count;
}

static void *hit[4];
static int hits;
void AI_SetState(void *ai, int state)
{
	if (state == 10 && hits < 4)
		hit[hits++] = ai;
}

static float pushed[4][3];
static int pushes;
void Platform_AddVelocity(float *vel, float *add)
{
	(void)vel;
	if (pushes < 4) {
		pushed[pushes][0] = add[0];
		pushed[pushes][1] = add[1];
		pushed[pushes][2] = add[2];
		pushes++;
	}
}

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
	return fabsf(a - b) < 1e-4f;
}

/* ---------- sliding-texture models */

static char *new_slider(int tag, float su, float sv, float u, float v)
{
	char *s = (char *)calloc(1, 0x34);

	MA_F32(s, 8) = su;
	MA_F32(s, 0xc) = sv;
	MA_F32(s, 0x10) = u;
	MA_F32(s, 0x14) = v;
	MA_I32(s, 0x18) = tag;
	MA_I32(s, 0x1c) = 1;
	CLList_Insert((int *)DAT_0030844c, (int *)s, 0);
	return s;
}

static void test_sliders(void)
{
	char *a, *b;

	CLList_Init((int *)DAT_0030844c);
	DAT_0028fda8 = 1.0f;
	a = new_slider(3, 0.25f, -0.25f, 0.875f, 0.125f);
	b = new_slider(4, 0.5f, 0.0f, 0.0f, 0.0f);
	FUN_0015b730();
	CHECK(near(MA_F32(a, 0x10), 0.125f), "u past 1 wraps back into [0, 1)");
	CHECK(near(MA_F32(a, 0x14), 0.875f), "v below 0 wraps up into [0, 1)");
	CHECK(near(MA_F32(b, 0x10), 0.5f), "u scrolls by speed x frame factor");
	FUN_0015b760(3, 0);
	CHECK(MA_I32(a, 0x1c) == 0 && MA_I32(b, 0x1c) == 1, "the switch changes only its tag");
	FUN_0015b730();
	CHECK(near(MA_F32(a, 0x10), 0.125f), "a model that is off does not scroll");
	CHECK(near(MA_F32(b, 0x10), 0.0f), "u reaching exactly 1 wraps to 0");
	CLList_Remove((int *)DAT_0030844c, (int *)a);
	CLList_Remove((int *)DAT_0030844c, (int *)b);
	free(a);
	free(b);
}

/* ---------- glow sprites */

static void test_glow_sprites(void)
{
	float pos[4] = { 1.0f, 2.0f, 3.0f, 9.0f };
	char *s;

	CLList_Init((int *)DAT_00308440);
	s = (char *)FUN_0015c400(pos, 80.0f, 40.0f, "glow.bmp", 10, 20, 30, 40, 5.0f);
	CHECK(s != NULL && MA_I32(DAT_00308440, 8) == 1, "a sprite joins the glow list");
	if (s == NULL)
		return;
	CHECK(MA_F32(s, 8) == 1.0f && MA_F32(s, 0xc) == 2.0f && MA_F32(s, 0x10) == 3.0f
		&& MA_F32(s, 0x14) == 1.0f, "position kept, w forced to 1");
	CHECK(MA_F32(s, 0x28) == 80.0f && MA_F32(s, 0x2c) == 40.0f && MA_F32(s, 0x30) == 5.0f,
		"width, height and pull toward the camera");
	CHECK(MA_I32(s, 0x40) == 10 && MA_I32(s, 0x44) == 20 && MA_I32(s, 0x48) == 30
		&& MA_I32(s, 0x4c) == 40, "colour in the draw call's order r, g, b, a");
	CHECK(MA_I32(s, 0x54) == 0 && MA_I32(s, 0x58) == 0, "not attached to an actor node");
	FUN_0015c400(pos, 1.0f, 1.0f, "glow.bmp", 0, 0, 0, 0, 0.0f);
	frees = 0;
	FUN_0015b650();
	CHECK(MA_I32(DAT_00308440, 8) == 0 && frees == 2, "unloading frees every sprite");
	CHECK(CLList_Next((unsigned int *)DAT_00308440, NULL) == 0, "the glow list is empty");
}

/* ---------- goal effects */

static void test_goal_schedule(void)
{
	float at[4] = { 5.0f, 6.0f, 7.0f, 0.0f };

	FUN_0015be40(2, (undefined4 *)at);
	CHECK(DAT_003083d8[2][0] == 5.0f && DAT_003083d8[2][2] == 7.0f && DAT_003083d8[2][3] == 1.0f,
		"the burst is placed at the goal (w = 1)");
	CHECK(DAT_00308424[2] == 20.0f, "the burst starts 20 frames later");
}

/* ---------- lightning */

static void test_strike(void)
{
	float bolt[0x48 / 4];

	memset(bolt, 0, sizeof bolt);
	bolt[2] = 10.0f; /* pos */
	bolt[3] = 1.0f;
	bolt[4] = -20.0f;
	bolt[5] = 1.0f;
	DAT_00308458 = 0;
	FUN_0015ca40((int)bolt);
	CHECK(DAT_00308458 == 1, "a new bolt is counted");
	CHECK(bolt[0x44 / 4] == 22.0f, "a bolt lives 22 frames");
	CHECK(bolt[7] == 10.0f && bolt[8] == 1.0f && bolt[9] == -20.0f,
		"without scatter it starts at its target");
	CHECK(bolt[11] == 10.0f && bolt[12] == 3001.0f && bolt[13] == -20.0f,
		"it ends 3000 units above its start");
	FUN_0015ca40((int)bolt);
	CHECK(DAT_00308458 == 1, "striking a live bolt again does not count it twice");

	bolt[0x18 / 4] = 50.0f; /* scatter */
	bolt[0x44 / 4] = 0.0f;
	FUN_0015ca40((int)bolt);
	CHECK(near(hypotf(bolt[7] - 10.0f, bolt[9] + 20.0f), 50.0f) && bolt[8] == 1.0f,
		"with scatter it starts that far away at the same height");
	DAT_00308458 = 0;
}

static void test_bolt_hits_players(void)
{
	static const float at[4][3] = {
		{ 100.0f, 0.0f, 0.0f },   /* 100 units away: hit, pushed along +x */
		{ 0.0f, 0.0f, 250.0f },   /* 250 units away: missed */
		{ 50.0f, 0.0f, 0.0f },    /* already hit (state 10): missed */
		{ 0.0f, 0.0f, 0.0f },     /* at the strike point: hit, not pushed */
	};
	float bolt[0x48 / 4];
	char *player[4];
	char *character[4];
	int i;

	memset(bolt, 0, sizeof bolt);
	for (i = 0; i < 4; i++) {
		player[i] = (char *)calloc(1, 0x1200);
		character[i] = (char *)calloc(1, 0x200);
		MA_PTR(player[i], 0x30) = character[i];
		MA_F32(character[i], 0xd0) = at[i][0];
		MA_F32(character[i], 0xd4) = at[i][1];
		MA_F32(character[i], 0xd8) = at[i][2];
		DAT_003075bc[i] = (int)player[i];
	}
	MA_I16(player[2], 0x36) = 10;
	hits = 0;
	pushes = 0;
	FUN_0015cb80((int)bolt);
	CHECK(hits == 2 && hit[0] == player[0] && hit[1] == player[3],
		"only players within 200 units and not already hit are struck");
	CHECK(pushes == 1 && near(pushed[0][0], 4.0f) && pushed[0][1] == 0.0f && pushed[0][2] == 0.0f,
		"a struck player is pushed away from the strike at speed 4");
	for (i = 0; i < 4; i++) {
		DAT_003075bc[i] = 0;
		free(character[i]);
		free(player[i]);
	}
}

static void test_bolt_draw(void)
{
	float from[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float to[4] = { 0.0f, 120.0f, 0.0f, 1.0f };
	float *v;
	int i;
	int sideways_ok = 1;
	int heights_ok = 1;

	for (i = 0; i < 256; i++) {
		DAT_002bc490[i] = (unsigned int)i;
		DAT_002bc898[i] = (unsigned int)i << 8;
		DAT_002bce18[i] = (unsigned int)i << 16;
		DAT_002bd640[i] = (unsigned int)i << 24;
	}
	DAT_003084e8[6] = 0x12345678u;
	FUN_0015cc80(from, to, 12, 0.5f);
	CHECK(drawn_type == 2 && drawn_vertices == DAT_003084e8 && drawn_count == 13,
		"12 segments draw 13 vertices");
	v = (float *)DAT_003084e8;
	CHECK(v[0] == 0.0f && v[1] == 0.0f && v[2] == 0.0f, "the bolt starts at `from`");
	CHECK(v[12 * 9 + 0] == 0.0f && v[12 * 9 + 1] == 120.0f && v[12 * 9 + 2] == 0.0f,
		"the bolt ends at `to`");
	for (i = 1; i < 12; i++) {
		if (!near(v[i * 9 + 1], 10.0f * (float)i))
			heights_ok = 0;
		if (hypotf(v[i * 9 + 0], v[i * 9 + 2]) > 75.0f + 1e-3f)
			sideways_ok = 0;
	}
	CHECK(heights_ok, "inner points are evenly spaced along the bolt");
	CHECK(sideways_ok, "inner points are scattered at most 75 units sideways");
	CHECK(DAT_003084e8[1 * 9 + 6] == 0xe37f7f7fu && DAT_003084e8[12 * 9 + 6] == 0xe37f7f7fu,
		"brightness 0.5: colour 127, alpha 127 + 100");
	CHECK(DAT_003084e8[6] == 0x12345678u, "vertex 0's colour is not written (as the original)");
	FUN_0015cc80(from, to, 12, 1.0f);
	CHECK(DAT_003084e8[5 * 9 + 6] == 0xffffffffu, "alpha is capped at 255");
}

int main(void)
{
	DAT_002ea488 = (int *)calloc(1, 0xc);
	CLList_Init(DAT_002ea488);
	test_sliders();
	test_glow_sprites();
	test_goal_schedule();
	test_strike();
	test_bolt_hits_players();
	test_bolt_draw();
	fprintf(stderr, "meta_a tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
