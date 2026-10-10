/*
ROLLERBALL_B_TEST.C

rollerball_b module tests (src/rollerball_b/tests/run_tests.sh builds this as the main()
of a small guest image: the rollerball_b objects, rollerball_a's RollerBall .data block,
the generated stubs and the platform layer). They check behaviour other code relies on:

- the entity event queue (CRollerBall_EntityEvent.cpp) keeps events in priority order,
  replaces the data of a queued "unique" event, falls back to 16 slots for a size that
  is not a power of 2, and drops a low-priority event when full (the players' collision
  sounds, animations and rumble all go through it);
- a race path record (0x00198130/0x001981e0) answers "inside" only for points within the
  polygon (the race variations count laps and falls with it);
- the quaternion normalise (0x00198590, also used by Demolition) gives unit length and
  leaves a zero quaternion alone;
- the race controller never keeps its lane when it picks a new one;
- the goo pool ring (0x00197150) makes only the annulus water;
- the ball forces (0x001924c0) are drag + force / time scale + push when the ball rolls
  straight along the ground normal (no rolling friction);
- the goal system reset (0x001915c0) re-arms every goal (state idle, radius 80, enabled).

The game build compiles every .c under src/rollerball_b: without
OPENFRENZY_ROLLERBALL_B_TEST (set by run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_ROLLERBALL_B_TEST

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rollerball_b.h"

/* ---------- globals of other modules that the rollerball_b objects refer to (this image
   does not link their owners) */

__attribute__((weak)) unsigned char *DAT_0030ef04;
__attribute__((weak)) unsigned int DAT_00285630;
__attribute__((weak)) int DAT_00283ea4;
__attribute__((weak)) int DAT_00293928;
__attribute__((weak)) uint32_t DAT_002ae578[16];
__attribute__((weak)) int DAT_002ba56c;
__attribute__((weak)) int DAT_002baeb8;
__attribute__((weak)) int DAT_002baebc;
__attribute__((weak)) int DAT_002baed4[40];
__attribute__((weak)) unsigned int DAT_002bc490[256];
__attribute__((weak)) unsigned int DAT_002bc898[256];
__attribute__((weak)) unsigned int DAT_002bce18[256];
__attribute__((weak)) unsigned int DAT_002bd640[256];
__attribute__((weak)) float DAT_002be570[3];
__attribute__((weak)) unsigned int DAT_002be57c;
__attribute__((weak)) float DAT_002be588;
__attribute__((weak)) float DAT_002be58c;
__attribute__((weak)) int DAT_002ecfac;

/* rollerball_a functions that are missing from the function table (no weak stubs) */
__attribute__((weak)) void FUN_00187c70(void *self, int arg)
{
	(void)self;
	(void)arg;
}
__attribute__((weak)) void FUN_00188fa0(void *self)
{
	(void)self;
}
__attribute__((weak)) void FUN_001890e0(void *self, int run)
{
	(void)self;
	(void)run;
}
__attribute__((weak)) void FUN_001892e0(void *self, int draw)
{
	(void)self;
	(void)draw;
}

/* ---------- test doubles of bKernel's heap (the game uses bkHeapAlloc / bkHeapFree) */

void *bkHeapAlloc(uint size, const char *file, int line, uint flags)
{
	(void)file;
	(void)line;
	(void)flags;
	return malloc(size);
}

void bkHeapFree(void *pointer)
{
	free(pointer);
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

/* ---------- the event queue */

struct TestEvent {
	const void *vtbl;
	short id;
	unsigned char unique;
	unsigned char prio;
	int data[9];
};
OF_ASSERT_SIZE(struct TestEvent, 0x2c);

static void post(void *sys, short id, unsigned char prio, unsigned char unique, int value)
{
	struct TestEvent e;

	memset(&e, 0, sizeof e);
	e.id = id;
	e.prio = prio;
	e.unique = unique;
	e.data[0] = value;
	CRollerBallEntityEventSystem_Post(sys, &e);
}

static struct TestEvent *queued(void *sys, int k)
{
	struct TestEvent *events = RBB_P(sys, 0x04);
	int mask = RBB_US(sys, 0x08) - 1;

	return &events[(RBB_US(sys, 0x0a) + k) & mask];
}

static void test_event_queue(void)
{
	unsigned char sys[0x10];
	int i;

	CRollerBallEntityEventSystem_Ctor(sys);
	CRollerBallEntityEventSystem_InitQueue(sys, 3);
	CHECK(RBB_US(sys, 0x08) == 16, "a queue size that is not a power of 2 becomes 16");
	CHECK(RBB_P(sys, 0x04) != NULL && RBB_I(RBB_P(sys, 0x04), -4) == 16,
		"the event array carries its count in front (C++ new[])");

	post(sys, 1, 5, 0, 100);
	post(sys, 2, 9, 0, 200);
	post(sys, 3, 1, 0, 300);
	CHECK(RBB_US(sys, 0x0c) == 3, "three events queued");
	CHECK(queued(sys, 0)->id == 2 && queued(sys, 1)->id == 1 && queued(sys, 2)->id == 3,
		"events are kept highest priority first");

	post(sys, 1, 5, 1, 111);
	CHECK(RBB_US(sys, 0x0c) == 3, "a unique event with a queued id is not added again");
	CHECK(queued(sys, 1)->data[0] == 111, "it replaces the queued event's data");

	post(sys, 4, 5, 0, 400);
	CHECK(queued(sys, 1)->id == 4 && queued(sys, 2)->id == 1,
		"an event goes before the queued ones of its priority (the newest first)");

	for (i = RBB_US(sys, 0x0c); i < 16; i++)
		post(sys, (short)(10 + i), 3, 0, i);
	CHECK(RBB_US(sys, 0x0c) == 16, "the queue fills up");
	post(sys, 99, 0, 0, 0);
	CHECK(RBB_US(sys, 0x0c) == 16 && queued(sys, 15)->id != 99,
		"a full queue drops an event of lower priority than its first slot");

	CRollerBallEntityEventSystem_Dtor(sys);
}

/* ---------- race path records */

static void test_path(void)
{
	float square[4][4] = {
		{ 0.0f, 0.0f, 0.0f, 1.0f },
		{ 100.0f, 0.0f, 0.0f, 1.0f },
		{ 100.0f, 0.0f, 100.0f, 1.0f },
		{ 0.0f, 0.0f, 100.0f, 1.0f },
	};
	float in[4] = { 50.0f, 0.0f, 50.0f, 1.0f };
	float out_x[4] = { 150.0f, 0.0f, 50.0f, 1.0f };
	float out_z[4] = { 50.0f, 0.0f, -1.0f, 1.0f };
	float a[2] = { 1.0f, 1.0f };
	float line[4] = { 0.0f, 0.0f, 2.0f, 0.0f };
	unsigned char path[0x30];

	CRollerBallPath_Ctor(path);
	CRollerBallPath_SetPoints(path, square[0], 4);
	CHECK(RBB_F(path, 0x04) == 0.0f && RBB_F(path, 0x14) == 100.0f && RBB_F(path, 0x1c) == 100.0f,
		"the bounding box spans the points");
	CHECK(CRollerBallPath_Contains(path, in) == 1, "a point in the square is inside");
	CHECK(CRollerBallPath_Contains(path, out_x) == 0, "a point beyond x is outside");
	CHECK(CRollerBallPath_Contains(path, out_z) == 0, "a point before z is outside");
	CHECK(CRollerBallPath_SideOfLine(a, line) > 0.0f, "side of a line: the cross product's sign");
	FUN_00198110(path);
	CHECK(RBB_P(path, 0x24) == NULL, "the destructor forgets the points");
}

/* ---------- quaternion normalise */

static void test_quaternion(void)
{
	float q[4] = { 0.0f, 0.0f, 3.0f, 4.0f };
	float z[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	FUN_00198590(q);
	CHECK(fabsf(q[2] - 0.6f) < 1e-6f && fabsf(q[3] - 0.8f) < 1e-6f, "a quaternion is scaled to length 1");
	FUN_00198590(z);
	CHECK(z[0] == 0.0f && z[3] == 0.0f, "a zero quaternion stays zero (no division by 0)");
}

/* ---------- race lanes */

static void test_lanes(void)
{
	unsigned char ctl[0x5c];
	int i;
	int ok = 1;

	memset(ctl, 0, sizeof ctl);
	RBB_I(ctl, 0x54) = -1;
	DAT_00285630 = 12345;
	for (i = 0; i < 64; i++) {
		unsigned int before = RBB_U(ctl, 0x54);
		CRollerBallControlRace_PickLane(ctl);
		if (RBB_U(ctl, 0x54) == before || RBB_U(ctl, 0x54) > 3)
			ok = 0;
	}
	CHECK(ok, "a new lane is 0-3 and never the old one");
}

/* ---------- goo pool ring */

static void test_ring(void)
{
	unsigned char water[0x194];
	unsigned char *field = calloc(1, 0x9090);
	unsigned char flags[16];
	int i;

	memset(water, 0, sizeof water);
	memset(flags, 0xee, sizeof flags);
	RBB_P(water, 0x190) = field;
	RBB_F(field, 0x04) = -1.5f;   /* x0 */
	RBB_F(field, 0x0c) = -1.5f;   /* z0 */
	RBB_I(field, 0x18) = 4;       /* nx */
	RBB_I(field, 0x1c) = 4;       /* nz */
	RBB_P(field, 0x44) = flags;
	RBB_F(field, 0x904c) = 1.0f;
	RBB_F(field, 0x9050) = 1.0f;
	CRollerBallWater_SetRing(water, 1.0f, 2.0f);
	/* cell centres -1.5, -0.5, 0.5, 1.5: |(0.5, 0.5)|^2 = 0.5 (inside the inner circle),
	   |(1.5, 0.5)|^2 = 2.5 (in the ring), |(1.5, 1.5)|^2 = 4.5 (beyond the outer circle) */
	CHECK(flags[1 * 4 + 1] == 1, "the centre is fixed (not water)");
	CHECK(flags[1 * 4 + 0] == 0, "the ring is water");
	CHECK(flags[0 * 4 + 0] == 1, "the corners are fixed");
	for (i = 0; i < 16; i++)
		CHECK(flags[i] == 0 || flags[i] == 1, "every cell gets a flag");
	free(field);
}

/* ---------- ball forces */

static void test_forces(void)
{
	unsigned char *e = calloc(1, 0x427c);

	RBB_F(e, RBE_VEL + 4) = 2.0f;        /* straight along the normal */
	RBB_F(e, RBE_NORMAL + 4) = 1.0f;
	RBB_F(e, RBE_DRAG) = 0.5f;
	RBB_F(e, RBE_TIME_SCALE) = 2.0f;
	RBB_F(e, RBE_FORCE + 4) = -4.0f;
	RBB_F(e, RBE_PUSH + 0) = 3.0f;
	CRollerballEntity_ComputeForces(e);
	CHECK(RBB_F(e, RBE_ACCEL + 0) == 3.0f, "x: the push");
	CHECK(RBB_F(e, RBE_ACCEL + 4) == -1.0f - 2.0f, "y: drag (-0.5 * 2) plus force / time scale (-4 / 2)");
	CHECK(RBB_F(e, RBE_ACCEL + 8) == 0.0f, "z: nothing");
	CRollerballEntity_Integrate(e, 0.5f);
	CHECK(RBB_F(e, RBE_VEL + 4) == 2.0f + 0.5f * 2.0f * -3.0f, "velocity += accel * dt * time scale");
	free(e);
}

/* ---------- goal reset */

static void test_goal_reset(void)
{
	unsigned char sys[0x7e0];
	unsigned char goals[2][0x78];

	memset(sys, 0, sizeof sys);
	memset(goals, 0x55, sizeof goals);
	RBB_P(sys, RBGS_GOAL_LIST) = goals[0];
	RBB_P(goals[0], RBG_NEXT) = goals[1];
	RBB_P(goals[1], RBG_NEXT) = NULL;
	DAT_00285630 = 7;
	CRollerballGoalSystem_ResetGoals(sys);
	CHECK(RBB_I(sys, RBGS_ACTIVE) == 1, "the goal system is switched on");
	CHECK(RBB_I(sys, RBGS_PATTERN) >= 0 && RBB_I(sys, RBGS_PATTERN) <= 2, "the pattern is 0-2");
	CHECK(RBB_S(goals[1], RBG_STATE) == RBG_STATE_IDLE && RBB_F(goals[1], RBG_RADIUS) == 80.0f &&
		RBB_S(goals[1], RBG_ENABLED) == 1 && RBB_I(goals[1], RBG_FADE) == 0, "every goal is re-armed");
	CHECK(RBB_B(goals[0], RBG_ANIM0_ON) == 0 && RBB_B(goals[0], RBG_ANIM1_ON) == 0,
		"the goal animations are stopped");
}

int main(void)
{
	test_event_queue();
	test_path();
	test_quaternion();
	test_lanes();
	test_ring();
	test_forces();
	test_goal_reset();
	fprintf(stderr, "rollerball_b tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
