/*
rowing_b module tests: one guest image (src/rowing_b/tests/run_tests.sh builds it from the
game build's objects of src/rowing_b and the generated weak stubs).

The game image globs every .c under src/rowing_b, so this file compiles to nothing unless
OPENFRENZY_ROWING_B_TEST is defined. Only this module's objects are linked: functions of
other modules are the weak stubs (they do nothing and return 0), and other modules' data
are zero slots made by run_tests.sh.

What the tests pin down, and why it matters to the game:
- the shared data blocks: rowing_a names the Rowing globals by their Ghidra names
  (extern DAT_0030f0b0 ...) while rowing_b reaches them through RW_VAR(original address).
  Both must be the same storage, or the two halves of the game see different state.
- the data block keeps the XBE values: the model type table decides which class an object
  file gets; "overhead crane" must be tested before "crane" and "bouy xref" / "race bouy"
  before "bouy" (strstr matches the first entry), or the level's cranes and gate buoys
  become plain scenery.
- gates (0x001b99e0, 0x001b9ac0): crossing a gate from its front to 20 behind passes it,
  the other way is the wrong way, and the gates before it that were skipped become missed.
  The race placing and the "wrong gate" rumble use this.
- checkpoints (0x001b9490): the progress between two parallel checkpoints is the distance
  along the first one's normal, 0 to 1; a boat past the next checkpoint is not between
  them. The race order uses this.
- segments (0x001b93a0, 0x001b9c30): crossing lines and the nearest point on a segment
  (clamped to its ends) are what the AI steers by.
- the helix trail (0x001b2990): at most 18 points, newest first; the shared polyline arrays
  hold 18 entries, so a longer trail would write past them.
- particles (0x001b1f80, 0x001b1e90): an expired steam puff goes back to the free list (a
  leak would stop the steam after the pool runs dry).
- the sound queue (0x001b6c30, 0x001b6cc0): at most 50 queued sounds, played first in first
  out, one per frame.
- lights (0x001b5510, 0x001b56a0): a light the game switched off (flag 2) cannot be switched
  on by its group.
- floating object pull (0x001b8df0): an object on the boat's left pushes the look-ahead to
  the right and one on its right to the left; the AI swerves around obstacles with this.
*/

#ifdef OPENFRENZY_ROWING_B_TEST

#include "rowing_b.h"

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

/* Ghidra names that rowing_a uses (aliases into the blocks) */
extern unsigned int DAT_0030f0b0;
extern int DAT_002b5a10;
extern unsigned char DAT_0030fc80[];

static void test_data(void)
{
	struct { const char *name; int type; } *types = RW_ADDR(0x002b5998);
	int overhead = -1, crane = -1, xref = -1, race = -1, bouy = -1;
	int i;

	CHECK((void *)&DAT_0030f0b0 == RW_ADDR(0x0030f0b0), "DAT_0030f0b0 is the block's word");
	CHECK((void *)&DAT_002b5a10 == RW_ADDR(0x002b5a10), "DAT_002b5a10 is the block's word");
	CHECK((void *)DAT_0030fc80 == RW_ADDR(0x0030fc80), "DAT_0030fc80 is the block's word");
	CHECK(RW_VAR(int, 0x002b5a10) == 15, "15 model types (XBE value)");
	for (i = 0; i < RW_VAR(int, 0x002b5a10); i++) {
		if (strcmp(types[i].name, "overhead crane") == 0)
			overhead = i;
		else if (strcmp(types[i].name, "crane") == 0)
			crane = i;
		else if (strcmp(types[i].name, "bouy xref") == 0)
			xref = i;
		else if (strcmp(types[i].name, "race bouy") == 0)
			race = i;
		else if (strcmp(types[i].name, "bouy") == 0)
			bouy = i;
	}
	CHECK(overhead >= 0 && crane > overhead && types[overhead].type == 2 && types[crane].type == 1,
		"overhead crane (2) is matched before crane (1)");
	CHECK(xref >= 0 && race >= 0 && bouy > xref && bouy > race && types[xref].type == 14
		&& types[race].type == 15 && types[bouy].type == 8,
		"buoy xref (14) and race buoy (15) are matched before buoy (8)");
	CHECK(strcmp(*(const char **)RW_ADDR(0x002b5a18), "jetboats_01a") == 0,
		"the first sound is jetboats_01a");
}

static void make_gate(struct RowPath *gate, float *points, float x0, float z0, float x1, float z1)
{
	points[0] = x0;
	points[1] = z0;
	points[2] = x1;
	points[3] = z1;
	gate->count = 2;
	gate->points = points;
	gate->closed = 0;
}

static void test_gates(void)
{
	struct RowPath gates[3];
	float pts[3][4];
	char done[3];
	float side;
	int state = 0;
	int i;

	for (i = 0; i < 3; i++)
		make_gate(&gates[i], pts[i], -100.0f, (float)i * 1000.0f, 100.0f, (float)i * 1000.0f);
	CHECK(RowGate_Side(&side, 0.0f, -50.0f, &gates[0]) == 1 && near(side, -50.0f),
		"a point 50 before the gate is on its front side");
	CHECK(RowGate_Side(&side, 300.0f, -50.0f, &gates[0]) == 0 && side == 1000.0f,
		"a point beside the gate's end is not level with it");

	memset(done, 0, sizeof done);
	CHECK(RowGates_Check(0.0f, 1950.0f, &state, done, 3, gates) == 0 && state == 3,
		"approaching the third gate from the front selects it (+3)");
	CHECK(RowGates_Check(0.0f, 2030.0f, &state, done, 3, gates) == 1,
		"crossing to 20 behind it passes it");
	CHECK(done[2] == 1 && done[0] == 2 && done[1] == 2 && state == 0,
		"the passed gate is done, the skipped ones are missed");

	memset(done, 0, sizeof done);
	state = 0;
	CHECK(RowGates_Check(0.0f, 50.0f, &state, done, 3, gates) == 0 && state == -1,
		"approaching the first gate from behind selects it (-1)");
	CHECK(RowGates_Check(0.0f, -30.0f, &state, done, 3, gates) == 2 && done[0] == 2,
		"crossing it backwards is the wrong way");
	state = 1;
	CHECK(RowGates_Check(500.0f, 0.0f, &state, done, 3, gates) == 0 && state == 0,
		"leaving the selected gate's side forgets it");
}

static void test_checkpoints(void)
{
	static unsigned char world[0x380];
	struct RowPath cps[2];
	float pts[2][4];
	float frac = -1.0f;
	float heading = 99.0f;
	unsigned char *saved = g_RowWorld;

	make_gate(&cps[0], pts[0], -100.0f, 0.0f, 100.0f, 0.0f);
	make_gate(&cps[1], pts[1], -100.0f, 100.0f, 100.0f, 100.0f);
	RB_I32(world, 0x144) = 2;
	RB_PTR(world, 0x148) = cps;
	g_RowWorld = world;
	CHECK(RowCheckpoint_Progress(0, 0.0f, 50.0f, &frac, &heading) == 1 && near(frac, 0.5f),
		"half way between two parallel checkpoints is progress 0.5");
	CHECK(near(heading, 0.0f), "the heading is atan2(-nz, nx) of the first checkpoint");
	CHECK(RowCheckpoint_Progress(0, 0.0f, 150.0f, &frac, NULL) == 0,
		"a boat past the next checkpoint is not between them");
	g_RowWorld = saved;
}

static void test_segments(void)
{
	float a[4] = { -10.0f, 0.0f, 10.0f, 0.0f };
	float b[4] = { 3.0f, -5.0f, 3.0f, 5.0f };
	float c[4] = { -10.0f, 4.0f, 10.0f, 4.0f };
	float seg[4] = { 0.0f, 0.0f, 10.0f, 0.0f };
	float p[2] = { 15.0f, 3.0f };
	float out[2], closest[2], foot[2];
	float perp, along, d;

	CHECK(RowSegment_Intersect(out, a, b) == 1 && near(out[0], 3.0f) && near(out[1], 0.0f),
		"crossing lines meet at (3, 0)");
	CHECK(RowSegment_Intersect(out, a, c) == 0, "parallel lines do not meet");
	d = RowSegment_Distance(seg, p, closest, foot, &perp, &along);
	CHECK(near(closest[0], 10.0f) && near(closest[1], 0.0f) && near(d, sqrtf(34.0f)),
		"past the end, the nearest point is the end");
	CHECK(near(foot[0], 15.0f) && near(perp, 3.0f) && near(along, 15.0f),
		"the foot and the perpendicular distance are on the line");
}

static void test_helix(void)
{
	static struct RowHelixPoint points[18], vels[18];
	static int alphas[18];
	struct RowHelix helix;
	float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float vel[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	int i;

	memset(&helix, 0, sizeof helix);
	helix.points = points;
	helix.vels = vels;
	helix.alphas = alphas;
	for (i = 0; i < 20; i++) {
		pos[0] = (float)i;
		RowHelix_Push(&helix, pos, vel, i);
	}
	CHECK(helix.count == 18, "the trail keeps at most 18 points");
	CHECK(points[0].pos[0] == 19.0f && alphas[0] == 19 && points[17].pos[0] == 2.0f,
		"the newest point is first, the oldest ones are dropped");
}

static void test_particles(void)
{
	static struct RowSteam pool[2];
	struct RowParticles parts;
	float pos[4] = { 1.0f, 2.0f, 3.0f, 1.0f };
	struct RowSteam *puff;

	memset(&parts, 0, sizeof parts);
	memset(pool, 0, sizeof pool);
	pool[0].next = &pool[1];
	parts.steamFree = &pool[0];
	puff = RowParticles_AddSteam(&parts, pos, 10.0f, 2.0f);
	CHECK(puff == &pool[0] && parts.steam == puff && parts.steamFree == &pool[1],
		"a puff moves from the free list to the active list");
	CHECK(puff->pos[1] == 2.0f && puff->life == 2.0f && puff->alpha == 48.0f,
		"it starts at pos with its life and alpha 48");
	RowParticles_UpdateSteam(&parts);
	CHECK(parts.steam == puff, "it lives while its life lasts");
	RowParticles_UpdateSteam(&parts);
	CHECK(parts.steam == NULL && parts.steamFree == puff, "an expired puff goes back to the free list");
}

static void test_sound_queue(void)
{
	static unsigned char queue[0x5c4];
	static unsigned char sample[0x40];
	unsigned char *saved = RW_VAR(unsigned char *, 0x0030fc80);
	void *savedSample = RW_VAR(void *, 0x0030fc08);
	float pos[4] = { 7.0f, 8.0f, 9.0f, 1.0f };
	int i;

	memset(queue, 0, sizeof queue);
	RW_VAR(unsigned char *, 0x0030fc80) = queue;
	RW_VAR(void *, 0x0030fc08) = sample;
	for (i = 0; i < 51; i++)
		RowSound_Queue(0, i);
	CHECK(RB_I32(queue, 0x578) == 50, "at most 50 sounds are queued");
	RowSound_Queue(-1, 0);
	RowSound_Queue(0x1e, 0);
	CHECK(RB_I32(queue, 0x578) == 50, "unknown samples are not queued");
	RowSound_Update(queue);
	CHECK(RB_I32(queue, 0x578) == 49 && RB_I32(queue, 0x18) == 1,
		"one sound plays a frame and the next moves to the front");
	RB_I32(queue, 0x578) = 0;
	RowSound_Queue3D(0, pos, 0x80);
	CHECK(RB_I32(queue, 0) == 1 && RB_F32(queue, 0x0c) == 8.0f && RB_I32(queue, 0x18) == 0x80,
		"a positional sound keeps its position");
	RW_VAR(unsigned char *, 0x0030fc80) = saved;
	RW_VAR(void *, 0x0030fc08) = savedSample;
}

static void test_lights(void)
{
	struct RowLights lights;
	struct RowLight a, b;

	memset(&lights, 0, sizeof lights);
	memset(&a, 0, sizeof a);
	memset(&b, 0, sizeof b);
	a.group = 3;
	b.group = 4;
	a.next = &b;
	lights.list = &a;
	CHECK(RowLights_SetGroupFlags(&lights, 3, 2, 0) == 2 && a.flags == 2 && b.flags == 0,
		"the game switches group 3 off (flag 2), other groups keep theirs");
	RowLights_EnableGroup(&lights, 3, 1);
	CHECK((a.flags & 1) == 0 && a.blight == NULL, "a light switched off by the game stays off");
	CHECK(RowLights_SetGroupFlags(&lights, 9, 0, 0) == -1, "an empty group answers -1");
}

static void test_pull(void)
{
	float width = g_RowRiverWidth;
	float centre = g_RowRiverCentre;
	float boat[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float left[4] = { -10.0f, 0.0f, 100.0f, 1.0f };
	float right[4] = { 10.0f, 0.0f, 100.0f, 1.0f };
	float behind[4] = { 10.0f, 0.0f, -100.0f, 1.0f };
	float pull;

	g_RowRiverWidth = 2000.0f;
	g_RowRiverCentre = 0.0f;
	pull = 0.0f;
	RowFloaty_AddPull(&pull, boat, left, -1);
	CHECK(pull > 0.0f, "an object ahead on the left pushes the look-ahead right");
	pull = 0.0f;
	RowFloaty_AddPull(&pull, boat, right, -1);
	CHECK(pull < 0.0f, "an object ahead on the right pushes it left");
	pull = 0.0f;
	RowFloaty_AddPull(&pull, boat, behind, -1);
	CHECK(pull == 0.0f, "an object behind does not pull");
	g_RowRiverWidth = width;
	g_RowRiverCentre = centre;
}

int main(void)
{
	test_data();
	test_gates();
	test_checkpoints();
	test_segments();
	test_helix();
	test_particles();
	test_sound_queue();
	test_lights();
	test_pull();
	fprintf(stderr, "rowing_b tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
