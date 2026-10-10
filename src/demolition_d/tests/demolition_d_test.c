/*
demolition_d module tests: one guest image (src/demolition_d/tests/run_tests.sh builds it from
the game build's objects of src/demolition_d and the generated weak stubs).

The game image globs every .c under src/demolition_d, so this file compiles to nothing unless
OPENFRENZY_DEMOLITION_D_TEST is defined.

What the tests pin down, and why it matters to the game:
- crate fall: a crate in the air falls, lands on the ground (never sinks below it), bounces
  back with half its speed and comes to rest. A crate that sinks or never rests breaks the
  arena (players stand on crates).
- crate ground check: a resting crate starts to fall only when the ground is below it.
- crate damage: an explosion smashes a crate only when it is within its radius of the crate's
  middle (half its height up), and the smash advances the shared random seed exactly once, as
  the original did (the game's later random events depend on the sequence).
- AI table thunks: the CharLib AI tables call condition n through virtual slot 0x4c + 4n and
  state n through slot 0x94 + 4n, and pass the result / the "entered" flag through. A wrong
  slot makes a computer player do the wrong thing.
- event queue: higher priority events come first, an event with the same id only updates
  the queued one, and a full queue drops an event of lower priority than its head.
*/

#ifdef OPENFRENZY_DEMOLITION_D_TEST

#include "demolition_d.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* data of modules that the test image does not link (bkernel, bdisplay, demolition_a,
   classlib_fx); only their addresses matter here */
unsigned int DAT_00285630 = 1;
char *DAT_002ff85c;
float DAT_002855f0[16];
unsigned int DAT_002bc490[256];
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];
unsigned int DAT_00298d30[64];
int DAT_002ff9c8;
void *DAT_002ff9cc;
void *DAT_002ff9d0;
unsigned int DAT_002ff9d4[16];

static int checks;
static int failures;

#define CHECK(cond, what) do { \
	checks++; \
	if (!(cond)) { \
		failures++; \
		fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, what); \
	} \
} while (0)

/* a crate with a fake actor; the ground (DcWorld_GroundHeight, a weak stub here) is 0 */
static DemoCrate *new_crate(float y)
{
	DemoCrate *c = calloc(1, sizeof(DemoCrate));

	c->actor = calloc(1, 0x470);
	c->pos[1] = y;
	c->height = 200.0f;
	c->enabled = 1;
	c->visible = 1;
	c->collides = 1;
	return c;
}

static void test_crate_fall(void)
{
	DemoCrate *c = new_crate(100.0f);
	float impact = 0.0f;
	float bounce = 0.0f;
	int frames;
	int below = 0;

	c->state = 1;
	for (frames = 0; frames < 1000 && c->state == 1; frames++) {
		float before = c->vel[1];

		DemoCrate_Fall(c, 1.0f);
		if (c->pos[1] < 0.0f)
			below = 1;
		if (impact == 0.0f && c->vel[1] > 0.0f) {
			impact = before - 1.2f; /* speed when it hit the ground */
			bounce = c->vel[1];
		}
	}
	CHECK(!below, "a falling crate never goes below the ground");
	CHECK(c->state == 0, "a falling crate comes to rest");
	CHECK(c->pos[1] == 0.0f && c->vel[1] == 0.0f, "a resting crate sits on the ground, still");
	CHECK(impact < 0.0f && bounce == -impact * 0.5f, "a crate bounces back with half its speed");
}

static void test_crate_ground_check(void)
{
	DemoCrate *c = new_crate(50.0f);

	DemoCrate_CheckGround(c, 1.0f);
	CHECK(c->state == 1, "a crate above the ground starts to fall");
	CHECK(c->ground == 0.0f, "the ground height is kept");
	c = new_crate(0.0f);
	DemoCrate_CheckGround(c, 1.0f);
	CHECK(c->state == 0, "a crate on the ground stays at rest");
}

static void test_crate_damage(void)
{
	DemoCrate *c = new_crate(0.0f);
	float base[3] = { 0.0f, 0.0f, 0.0f };
	float middle[3] = { 0.0f, 100.0f, 5.0f };
	unsigned int seed;

	DAT_002ff85c = calloc(1, 0x8000);
	seed = DAT_00285630;
	DemoCrate_Damage(c, base, 10.0f);
	CHECK(c->enabled == 1 && c->visible == 1,
	      "an explosion at the crate's foot but far from its middle does not smash it");
	CHECK(DAT_00285630 == seed, "no smash, no random number");
	DemoCrate_Damage(c, middle, 10.0f);
	CHECK(c->enabled == 0 && c->visible == 0, "an explosion near the crate's middle smashes it");
	CHECK(DAT_00285630 == seed * 0x343fdu + 0x269ec3u, "a smash uses one random number");
}

/* an AI object whose virtual slots record what was called */
static int last_slot;
static int last_entered;
#define SLOT_FN(n) \
	static int cond_##n(void *ai) { (void)ai; last_slot = n; return 0x100 + n; } \
	static void state_##n(void *ai, int entered) { (void)ai; last_slot = n; last_entered = entered; }
SLOT_FN(7)
SLOT_FN(17)

static void test_ai_thunks(void)
{
	const void *vtable[0xdc / 4];
	const void *const *obj;
	int r;

	memset(vtable, 0, sizeof(vtable));
	vtable[(0x4c + 7 * 4) / 4] = (const void *)cond_7;
	vtable[(0x4c + 17 * 4) / 4] = (const void *)cond_17;
	vtable[(0x94 + 7 * 4) / 4] = (const void *)state_7;
	vtable[(0x94 + 17 * 4) / 4] = (const void *)state_17;
	obj = vtable;
	r = DemoCLAI_Cond07(&obj);
	CHECK(last_slot == 7 && r == 0x107, "condition 7 is virtual slot 0x68 and returns its result");
	r = DemoCLAI_Cond17(&obj);
	CHECK(last_slot == 17 && r == 0x111, "condition 17 is virtual slot 0x90");
	DemoCLAI_State07(&obj, 1);
	CHECK(last_slot == 7 && last_entered == 1, "state 7 is virtual slot 0xb0 and gets 'entered'");
	DemoCLAI_State17(&obj, 0);
	CHECK(last_slot == 17 && last_entered == 0, "state 17 is virtual slot 0xd8");
}

static void push(DemoEventQueue *q, short id, int kind, int priority, unsigned int data)
{
	DemoCharEvent ev;

	memset(&ev, 0, sizeof(ev));
	ev.id = id;
	ev.kind = kind;
	ev.priority = priority;
	ev.data[0] = data;
	DemoCharEventSystem_Push((int)q, (int)&ev);
}

static DemoCharEvent *at(DemoEventQueue *q, int i)
{
	return (DemoCharEvent *)q->events + ((q->head + i) & (q->capacity - 1));
}

static void test_event_queue(void)
{
	DemoEventQueue q;

	memset(&q, 0, sizeof(q));
	q.capacity = 4;
	q.events = calloc(4, sizeof(DemoCharEvent));
	push(&q, 1, 1, 5, 0xa);
	push(&q, 2, 1, 9, 0xb);
	push(&q, 3, 1, 1, 0xc);
	CHECK(q.count == 3, "three events queued");
	CHECK(at(&q, 0)->id == 2 && at(&q, 1)->id == 1 && at(&q, 2)->id == 3,
	      "events are ordered by priority, highest first");
	push(&q, 1, 1, 0, 0xd);
	CHECK(q.count == 3 && at(&q, 1)->id == 1 && at(&q, 1)->data[0] == 0xd,
	      "an event with a queued id only updates that event's data");
	push(&q, 1, 0, 5, 0xf);
	CHECK(q.count == 4 && at(&q, 1)->id == 1 && at(&q, 1)->data[0] == 0xf,
	      "a kind 0 event is never merged (a second id 1 event is queued)");
	push(&q, 5, 1, 2, 0x10);
	CHECK(q.count == 4 && at(&q, 0)->id == 2 && at(&q, 3)->id == 3,
	      "a full queue drops an event of lower priority than its head");
	push(&q, 6, 1, 10, 0x11);
	CHECK(q.count == 4 && at(&q, 0)->id == 6 && at(&q, 1)->id == 2,
	      "a full queue takes an event of higher priority, losing its last place");
}

int main(void)
{
	test_crate_fall();
	test_crate_ground_check();
	test_crate_damage();
	test_ai_thunks();
	test_event_queue();
	fprintf(stderr, "demolition_d tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
