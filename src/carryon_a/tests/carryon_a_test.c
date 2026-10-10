/*
CARRYON_A_TEST.C

carryon_a module tests (src/carryon_a/tests/run_tests.sh builds this as the main() of a small
guest image: the carryon_a objects, the generated stubs and the platform layer). They check
rules of the Fireworks minigame that the decompiled code must keep:

- a conveyor belt starts with every slot filled, the owners' fireworks in turn (1..4), so
  each player has the same share;
- the belt position always wraps into [0, slots), and a passed firework changes owner only
  when its 18-frame flight ends;
- a firework is "at" a place on the belt only inside the visible window and the width;
- a fuse/paint piece moves its head at its speed, its tail starts after the delay, and the
  piece stops at the end of its path;
- rumble reaches only human players, for the five known events;
- a sound kind plays one of its own sounds and nothing for an unknown or empty kind;
- the camera jumps to its goal in the first frames (no swoop from the origin) and then eases
  5% per frame.

The game build compiles every .c under src/carryon_a: without OPENFRENZY_CARRYON_A_TEST
(set by run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_CARRYON_A_TEST
#include "carryon_a.h"

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

/* the other modules' globals that the carryon_a objects use (their objects are not in the
test image); types as carryon_a declares them */
char s_BGeffects_00283b18[0x800];
float DAT_002855f0[16];
float DAT_00285634[4];
float DAT_0028fda8;
float DAT_0028ffa8;
float DAT_0028ffac;
float DAT_00292c60;
float DAT_00292c64;
float DAT_00292c68;
float DAT_00292c6c;
float DAT_00293a38;
float DAT_00293e94;
float DAT_00293e98;
float DAT_00293efc;
float DAT_00293f00;
float DAT_00293f04;
float DAT_00294b64;
float DAT_00294b98[4];
float DAT_00294ba8[4];
float DAT_00294bb8;
float DAT_002ea478[4];
float DAT_002f7a2c;
int DAT_00283e58;
int DAT_00283ea4;
int DAT_00292c4c;
int DAT_00292c50;
int DAT_00292c78;
int DAT_00293928;
int DAT_00293eb0;
int DAT_00293eb4;
int DAT_00293eb8[4];
int DAT_00293f08;
int DAT_00293f0c;
int DAT_002b9c58[4];
int DAT_002b9c68[4];
int DAT_002ba56c;
int DAT_002bae6c;
int DAT_002bae70;
int DAT_002bae7c;
int DAT_002bae94;
int DAT_002bae98;
int DAT_002baeb8;
int DAT_002baebc;
int DAT_002baed4[4][10];
int DAT_002baee8[0x800];
int DAT_002baf10, DAT_002baf38, DAT_002baf60;
int DAT_002ecdec;
long long DAT_002cd7a0;
unsigned char DAT_00290718[0x800];
unsigned char DAT_00292ca0[0x800];
unsigned char DAT_002b9968[0x800];
unsigned char DAT_002ba750[0x800];
unsigned char DAT_002ecd90[0x800];
unsigned char DAT_002ed324[0x800];
unsigned int DAT_00285630;
unsigned int DAT_002bc48c;
unsigned int DAT_002bc490[256], DAT_002bd640[256], DAT_002bc898[256], DAT_002bce18[256];
unsigned int DAT_002bca94;
void **DAT_002ba95c;
void *DAT_00293ea0[4];
void *DAT_002ba9a0;
void *DAT_002ea488;
void *DAT_002ecfac;
void *DAT_002f26c4;
void *DAT_002f27c4;
void *DAT_002f27d8;
void *DAT_002f2878[6];
void *DAT_002f289c[4];

/* ---------- replacements for other modules' functions (strong, over the weak stubs) */

void bmVectorSetLength(float *out, const float *v, float length)
{
	float l = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);

	out[0] = v[0] / l * length;
	out[1] = v[1] / l * length;
	out[2] = v[2] / l * length;
}

void bmVectorLerp(float *out, const float *a, const float *b, float t)
{
	int i;

	for (i = 0; i < 4; i++)
		out[i] = a[i] + (b[i] - a[i]) * t;
}

static int g_rumbles, g_rumblePlayer[8];
static const char *g_rumbleName[8];
void CLRumble_PlayNamed(int self, int player, char *name)
{
	(void)self;
	if (g_rumbles < 8)
	{
		g_rumblePlayer[g_rumbles] = player;
		g_rumbleName[g_rumbles] = name;
	}
	g_rumbles++;
}

static void *g_playedSample;
static int g_playedVolume;
int bsPlaySample(struct BSample *sample, int volume, int pan, int frequency, unsigned int flags)
{
	(void)pan;
	(void)frequency;
	(void)flags;
	g_playedSample = sample;
	g_playedVolume = volume;
	return 42;
}

/* ---------- tests */

static struct CFireResource g_res;

static void test_belt_start(void)
{
	static struct CFireBelt belt;
	int i;
	int ok = 1;

	CFire_Belt_Ctor(&belt);
	CFire_Belt_Init(&belt, NULL, NULL, 0.016f, 3, 24, 14);
	for (i = 0; i < 24; i++)
	{
		if (belt.types[i] < 1 || belt.types[i] > 4)
			ok = 0;
		if (i > 0 && belt.types[i] != belt.types[i - 1] % 4 + 1)
			ok = 0;
	}
	CHECK(ok, "the belt starts full, the owners 1..4 in turn");
	CHECK(belt.count == 24 && belt.window == 14 && belt.points == 3 && belt.pos == 0.0f,
		"Init keeps the slot count, the window and the score");
}

static void test_belt_wrap_and_pass(void)
{
	static struct CFireBelt belt;
	int i;

	CFire_Belt_Ctor(&belt);
	CFire_Belt_Init(&belt, NULL, NULL, 1.0f, 3, 24, 14);
	CFire_Belt_Update(&belt, 30.0f);
	CHECK(belt.pos >= 0.0f && belt.pos < 24.0f && belt.pos == 6.0f, "30 slots on 24 wrap to 6");
	CFire_Belt_Update(&belt, -13.0f);
	CHECK(belt.pos == 17.0f, "moving back wraps below 0 to 17");

	for (i = 0; i < 24; i++)
		belt.types[i] = 1;      /* full: no new firework can appear */
	belt.timers[3] = 5.0f;
	belt.pending[3] = 2;
	belt.speed = 0.0f;
	CFire_Belt_Update(&belt, 4.0f);
	CHECK(belt.types[3] == 1 && belt.timers[3] == 1.0f, "a passed firework is still in flight");
	CFire_Belt_Update(&belt, 4.0f);
	CHECK(belt.types[3] == 2 && belt.timers[3] == 0.0f, "it changes owner when it lands");
}

static void test_belt_place(void)
{
	static struct CFireBelt belt;

	CFire_Belt_Ctor(&belt);
	belt.count = 24;
	belt.window = 14;
	belt.types[7] = 2;
	belt.types[20] = 3;
	CHECK(CFire_Belt_HasItemAt(&belt, 0.5f, 1.0f, 2) == 1, "type 2 is in the middle of the window");
	CHECK(CFire_Belt_HasItemAt(&belt, 0.5f, 1.0f, -1) == 1, "-1 finds any type");
	CHECK(CFire_Belt_HasItemAt(&belt, 0.5f, 1.0f, 3) == 0, "type 3 is not there");
	CHECK(CFire_Belt_HasItemAt(&belt, 0.0f, 1.0f, 2) == 0, "nor is type 2 at the start");
	CHECK(CFire_Belt_HasItemAt(&belt, 20.0f / 14.0f, 1.0f, 3) == 0,
		"a firework outside the window is never found");
	belt.pos = 6.0f;            /* slot 20 is now at place 14 - still outside */
	CHECK(CFire_Belt_HasItemAt(&belt, 1.0f, 1.0f, 3) == 0, "the window ends before place 14");
	belt.pos = 7.0f;            /* slot 20 at place 13 */
	CHECK(CFire_Belt_HasItemAt(&belt, 13.0f / 14.0f, 1.0f, 3) == 1, "it enters at place 13");
}

static void test_piece_path(void)
{
	static struct CoConveyorItem item;
	static float points[3][4] = { { 0, 0, 0, 1 }, { 100, 0, 0, 1 }, { 200, 0, 0, 1 } };
	static struct CoAsePath path = { &points[0][0], 3, 0, { 0, 0, 0 } };
	int i;

	g_fireRes = &g_res;
	memset(&item, 0, sizeof item);
	item.path = &path;
	item.speed = 50.0f;
	CFire_ConveyorItem_Start(&item, 1, 1);
	CHECK(item.active == 1 && item.pos[0] == 0.0f && item.point == 1 && item.delay == 12.0f,
		"a piece starts at the first point, its tail 12 frames behind");
	CFire_ConveyorItem_Start(&item, 2, 0);
	CHECK(item.player == 1 && item.paint == 1, "a running piece cannot be restarted");
	CFire_ConveyorItem_Update(&item, 1.0f, NULL, NULL, NULL, 0);
	CHECK(item.pos[0] == 50.0f && item.pos2[0] == 0.0f && item.delay == 11.0f,
		"the head moves at its speed, the tail waits");
	for (i = 0; i < 3 && item.active; i++)
		CFire_ConveyorItem_Update(&item, 1.0f, NULL, NULL, NULL, 0);
	CHECK(item.active == 0 && item.pos[0] == 200.0f, "the piece stops at the end of its path");
}

static void test_rumble(void)
{
	memset(DAT_002baed4, 0, sizeof DAT_002baed4);
	DAT_002baed4[0][1] = 1;
	DAT_002baed4[2][1] = 1;
	g_rumbles = 0;
	CFire_Resource_Rumble(&g_res, -1, 1);
	CHECK(g_rumbles == 2 && g_rumblePlayer[0] == 0 && g_rumblePlayer[1] == 2,
		"every player: only the human pads rumble");
	CHECK(g_rumbles >= 1 && strcmp(g_rumbleName[0], "FWRocketBang") == 0, "event 1 is the rocket bang");
	g_rumbles = 0;
	CFire_Resource_Rumble(&g_res, 1, 0);
	CHECK(g_rumbles == 0, "a CPU player's pad does not rumble");
	CFire_Resource_Rumble(&g_res, 0, 4);
	CHECK(g_rumbles == 1 && strcmp(g_rumbleName[0], "FWKO-ed") == 0, "event 4 is the knock-out");
	CFire_Resource_Rumble(&g_res, 0, 5);
	CHECK(g_rumbles == 1, "unknown events do nothing");
}

static void test_sounds(void)
{
	static void *samples[0x2e];
	static int sound;

	memset(&g_res, 0, sizeof g_res);
	g_res.samples = samples;
	g_res.firstSample[3] = 5;
	g_res.numSamples[3] = 1;
	samples[5] = &sound;
	g_playedSample = NULL;
	CHECK(CFire_Resource_FindSample(&g_res, 3, -1, 0x80) == 42 && g_playedSample == &sound
		&& g_playedVolume == 0x80, "a kind with one sound plays it");
	g_playedSample = NULL;
	CHECK(CFire_Resource_FindSample(&g_res, 4, -1, 0x80) == -1 && g_playedSample == NULL,
		"a kind without sounds plays nothing");
	CHECK(CFire_Resource_FindSample(&g_res, 15, 0, 0x80) == -1 && g_playedSample == NULL,
		"kinds above 14 are rejected");
	samples[5] = NULL;
	CHECK(CFire_Resource_FindSample(&g_res, 3, 0, 0x80) == -1, "a sound that failed to load is skipped");
}

static void test_camera_ease(void)
{
	static struct CFireCamera cam;

	memset(&cam, 0, sizeof cam);
	cam.eyeGoal[0] = 100.0f;
	cam.targetGoal[2] = 40.0f;
	CFire_Camera_Lerp(&cam, 1.0f);
	CHECK(cam.eye[0] == 100.0f && cam.target[2] == 40.0f, "the first frames jump to the goal");
	cam.frames = 3;
	cam.eyeGoal[0] = 200.0f;
	CFire_Camera_Lerp(&cam, 2.0f);
	CHECK(cam.eye[0] > 109.99f && cam.eye[0] < 110.01f, "then it eases 5% per frame of the step");
}

int main(void)
{
	test_belt_start();
	test_belt_wrap_and_pass();
	test_belt_place();
	test_piece_path();
	test_rumble();
	test_sounds();
	test_camera_ease();
	fprintf(stderr, "carryon_a tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
