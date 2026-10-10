/*
classlib_fx module tests: one guest image (src/classlib_fx/tests/run_tests.sh builds it from
the game build's objects of src/classlib_fx and the generated weak stubs).

The game image globs every .c under src/classlib_fx, so this file compiles to nothing unless
OPENFRENZY_CLASSLIB_FX_TEST is defined. Only this module's objects are linked: functions of
other modules are the weak stubs (they do nothing and return 0), so the tests use the parts
that do not depend on them.

What the tests pin down, and why it matters to the game:
- rand: effects draw from the MSVC rand() sequence (seed 1: 41, 18467, ...), inlined in the
  game; another generator would change every particle burst and seagull flight.
- spring: a token's scale spring snaps the whole spring to its target as soon as one value
  settles (the original's behaviour: a token that starts at full size on one axis pops in).
- ripple: a water cell's speed relaxes towards its neighbours with adjacent cells weighted
  twice the diagonals, and fixed border cells never move; this shapes every splash.
- texture scroll: the water texture offsets wrap into [0, 1).
- trail: a projector trail keeps the newest point first, ignores points closer than 0.1, and
  without new points it loses a point and fades every tick.
- sparks: a spark that falls below the emitter height bounces (speed halved, vertical speed
  0) when the system bounces, and a system whose sparks have all expired turns itself off.
- emission: CParticleSystem emits at most maxEmit particles per tick and only while its emit
  time lasts.
- seagulls: a new flock target is always inside the flock's box.
*/

#ifdef OPENFRENZY_CLASSLIB_FX_TEST

#include "classlib_fx.h"

#include <stdlib.h>
#include <string.h>

/* globals of other modules that this test image does not link */
float DAT_0028fda8 = 1.0f;
float DAT_0028fdb8[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
float DAT_0028fdc8[3] = { 0.0f, 1.0f, 0.0f };
float DAT_0028fdd8[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
unsigned int DAT_00285630 = 1;
int DAT_00290cb8 = 1;
int DAT_00293928;
byte *DAT_002ba96c;
int DAT_002bae94;
int DAT_002baebc;
int DAT_002baed4[40];
int DAT_002baed8[40];
int DAT_002baef4[40];
byte *DAT_002ea488;
float DAT_002f2890;
float DAT_002f2894;

static int failures;
static int checks;

#define CHECK(condition, ...) \
	do { \
		checks++; \
		if (!(condition)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #condition); \
			fprintf(stderr, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
		} \
	} while (0)

static int near(float a, float b)
{
	return fabsf(a - b) < 1e-4f;
}

/* ---------- rand */

static void test_rand(void)
{
	static const int msvc[5] = { 41, 18467, 6334, 26500, 19169 };
	int i;

	DAT_00285630 = 1;
	for (i = 0; i < 5; i++)
		CHECK(CLF_Rand() == msvc[i], "rand number %d", i);
}

/* ---------- spring */

static void test_spring(void)
{
	CSpring s;
	int frames;
	int done;

	DAT_0028fda8 = 1.0f;
	memset(&s, 0, sizeof(s));
	s.target[0] = 10.0f;
	s.target[1] = 10.0f;
	s.target[2] = 10.0f;
	s.stiffness = 0.17f;
	s.damping = 0.92f;
	s.count = 3;
	done = 0;
	for (frames = 0; frames < 1000 && !done; frames++)
		done = CSpring_Update(&s);
	CHECK(done, "the spring settles");
	CHECK(s.cur[0] == 10.0f && s.cur[1] == 10.0f && s.cur[2] == 10.0f, "it ends exactly on its target");
	CHECK(s.vel[0] == 0.0f && s.vel[3] == 1.0f, "its speed is reset");

	memset(&s, 0, sizeof(s));
	s.cur[0] = 10.0f;
	s.target[0] = 10.0f;
	s.target[1] = 10.0f;
	s.target[2] = 10.0f;
	s.stiffness = 0.17f;
	s.damping = 0.92f;
	s.count = 3;
	CHECK(CSpring_Update(&s) == 1, "one value at its target settles the spring at once");
	CHECK(s.cur[1] == 10.0f && s.cur[2] == 10.0f, "the other values snap to their targets");
}

/* ---------- water */

#define WN 6

static float water_cells[WN * WN][2];
static byte water_flags[WN * WN];

static void water_setup(CWaterField *w)
{
	int x;
	int z;

	memset(w, 0, sizeof(*w));
	memset(water_cells, 0, sizeof(water_cells));
	for (z = 0; z < WN; z++)
		for (x = 0; x < WN; x++)
			water_flags[z * WN + x] = (x == 0 || z == 0 || x == WN - 1 || z == WN - 1) ? 1 : 0;
	w->p.nx = WN;
	w->p.nz = WN;
	w->p.damping = 1.0f;
	w->p.tension = 1.0f;
	w->numCells = WN * WN;
	w->cells = water_cells;
	w->flags = water_flags;
}

static void test_ripple(void)
{
	static CWaterField w;
	int c;

	water_setup(&w);
	c = 2 * WN + 2;
	water_cells[c][0] = 1.0f;
	water_cells[0][0] = 5.0f; /* a fixed corner */
	CWaterField_Ripple(&w);
	CHECK(near(water_cells[c][0], 0.0f), "the raised cell falls back (%f)", water_cells[c][0]);
	CHECK(near(water_cells[c + 1][0], 0.13333334f), "an adjacent cell rises by 2/15 (%f)",
	      water_cells[c + 1][0]);
	CHECK(near(water_cells[c + WN][0], 0.13333334f), "the cell below too");
	CHECK(near(water_cells[c + WN + 1][0], 0.0666666701f), "a diagonal cell rises by 1/15 (%f)",
	      water_cells[c + WN + 1][0]);
	CHECK(water_cells[0][0] == 0.0f && water_cells[0][1] == 0.0f, "a fixed cell is held at 0");
}

static void test_scroll_uv(void)
{
	static CWaterField w;

	memset(&w, 0, sizeof(w));
	CWaterField_ScrollUV(&w, 1.25f, -0.25f);
	CHECK(near(w.uOffset, 0.25f), "u wraps down into [0, 1) (%f)", w.uOffset);
	CHECK(near(w.vOffset, 0.75f), "v wraps up into [0, 1) (%f)", w.vOffset);
}

/* ---------- projector trail */

static void test_trail(void)
{
	static CProjTrail t;
	static int colours[5][4];
	static float widths[5];
	static float extra[5];
	static float points[4][4];
	float p[4];

	memset(&t, 0, sizeof(t));
	t.colours = colours;
	t.widths = widths;
	t.extra = extra;
	t.points = points;
	t.maxPoints = 4;
	t.headColour[0] = 0x7f;
	t.headColour[3] = 0x7f;
	t.tailColour[2] = 0x7f;
	t.tailColour[3] = 0x7f;
	t.headWidth = 4.0f;
	t.tailWidth = 0.0f;
	t.fadeRate = 127.0f / 4.0f;
	DAT_0028fda8 = 1.0f;

	p[0] = 0.0f;
	p[1] = 0.0f;
	p[2] = 0.0f;
	p[3] = 1.0f;
	CProjTrail_AddPoint(&t, p);
	p[0] = 0.05f;
	CProjTrail_AddPoint(&t, p);
	CHECK(t.numPoints == 1, "a point closer than 0.1 is ignored (%d points)", t.numPoints);
	p[0] = 5.0f;
	CProjTrail_AddPoint(&t, p);
	CHECK(t.numPoints == 2 && points[0][0] == 5.0f && points[1][0] == 0.0f,
	      "the newest point comes first");
	CHECK(t.dirty == 1 && t.fade == 127.0f, "a new point marks the trail and resets the fade");

	CProjTrail_Update(&t);
	CHECK(t.numPoints == 2 && t.dirty == 0, "a fresh trail keeps its points");
	CHECK(colours[0][0] == 126 && colours[0][2] == 0, "the head has the head colour (%d %d)",
	      colours[0][0], colours[0][2]);
	CHECK(colours[1][2] == 0, "the tail colour is limited by the head colour (%d)", colours[1][2]);
	CHECK(near(widths[0], 4.0f) && near(widths[1], 2.0f), "widths blend head to tail (%f %f)",
	      widths[0], widths[1]);

	CProjTrail_Update(&t);
	CHECK(t.numPoints == 1, "without new points the trail shrinks (%d)", t.numPoints);
	CHECK(near(t.fade, 127.0f - 127.0f / 4.0f), "and fades (%f)", t.fade);
}

/* ---------- sparks */

static void test_sparks(void)
{
	static CSparkSystem s;
	static CParticle parts[1];
	CParticle *p;

	memset(&s, 0, sizeof(s));
	memset(parts, 0, sizeof(parts));
	s.h.particles = parts;
	s.h.maxParticles = 1;
	s.p.active = 1;
	s.p.bounce = 1;
	s.p.drag = 1.0f;
	s.p.gravity = 0.0f;
	p = &parts[0];
	p->alive = 1;
	p->life = 2;
	p->pos[1] = 1.0f;
	p->vel[0] = 4.0f;
	p->vel[1] = -10.0f;
	CSparkSystem_Update(&s, 1.0f);
	CHECK(s.h.numAlive == 1, "the spark is alive");
	CHECK(p->pos[1] == 0.0f && p->vel[1] == 0.0f && p->vel[0] == 2.0f,
	      "it bounces at the emitter height (y %f, vel %f %f)", p->pos[1], p->vel[0], p->vel[1]);
	CSparkSystem_Update(&s, 1.0f);
	CSparkSystem_Update(&s, 1.0f);
	CHECK(p->alive == 0 && s.h.numAlive == 0, "it dies after its life");
	CHECK(s.p.active == 0, "a system without sparks turns itself off");
}

/* ---------- emission */

static void test_emission(void)
{
	static CParticleSystem s;
	static CParticle parts[3];
	static CPSRing ring;
	static float ringPoints[2][4];
	int i;

	memset(&s, 0, sizeof(s));
	memset(parts, 0, sizeof(parts));
	memset(&ring, 0, sizeof(ring));
	ring.points = ringPoints;
	s.h.particles = parts;
	s.h.maxParticles = 3;
	s.rings = &ring;
	s.h.vtable = vtable_00266cd8;
	s.p.active = 1;
	s.p.numRings = 1;
	s.p.ringLength = 2;
	s.p.maxEmit = 2;
	s.p.lifeMin = 10;
	s.p.lifeRange = 1;
	s.p.drag = 1.0f;
	s.emitTimer = 1.5f;
	for (i = 0; i < 3; i++)
		parts[i].alive = 0;
	CParticleSystem_Update(&s, 1.0f);
	CHECK(s.h.numAlive == 2, "at most maxEmit particles per tick (%d)", s.h.numAlive);
	CParticleSystem_Update(&s, 1.0f);
	CHECK(s.emitTimer == 0.0f, "the emit time runs out");
	CHECK(s.h.numAlive == 2 && parts[2].alive == 0, "no emission after it (%d)", s.h.numAlive);
}

/* ---------- seagulls */

static void test_seagulls(void)
{
	static CSeagulls f;
	int i;
	int inside;

	memset(&f, 0, sizeof(f));
	f.p.boxMin[0] = -10.0f;
	f.p.boxMin[1] = 0.0f;
	f.p.boxMin[2] = -10.0f;
	f.p.boxMax[0] = 10.0f;
	f.p.boxMax[1] = 5.0f;
	f.p.boxMax[2] = 10.0f;
	inside = 1;
	for (i = 0; i < 200; i++) {
		CSeagulls_PickTarget(&f);
		inside = inside && f.target[0] >= -10.0f && f.target[0] <= 10.0f && f.target[1] >= 0.0f &&
		         f.target[1] <= 5.0f && f.target[2] >= -10.0f && f.target[2] <= 10.0f;
	}
	CHECK(inside, "every target is inside the box");
}

int main(void)
{
	test_rand();
	test_spring();
	test_ripple();
	test_scroll_uv();
	test_trail();
	test_sparks();
	test_emission();
	test_seagulls();
	fprintf(stderr, "classlib_fx tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
