/*
META_B_TEST.C

meta_b module tests (src/meta_b/tests/run_tests.sh builds this as the main() of a small
guest image: the meta_b objects, core's tweak-file word skipper, meta_a's Meta data block,
the generated stubs and the platform layer). They check what the board's level scripts and
the other modules rely on:

- the .lnd preprocessor keeps words, drops comments, applies #defines (the #define lines
  stay, for the loader's "#define" keyword) and ends a line's last word with "\r" (PUZZLE
  lines are cut at "\r");
- name tables and tweakables are read from meta_a's copy of the Meta .data block at the
  right offsets (a wrong offset would misparse every level silently);
- key tracks are packed in the format charlib's path player reads;
- a vent's on/off cycle and the water level step behave as the original.

The game build compiles every .c under src/meta_b: without OPENFRENZY_META_B_TEST (set by
run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_META_B_TEST

#include <stdio.h>
#include <string.h>

#include "meta_b.h"
#include "meta_b_internal.h"

/* core's data block (linked for the tweak-file word skipper) points at these functions of
   modules that are not in this image */
__attribute__((weak)) void DcParticles_EmitAll(struct CPSHeader *self)
{
	(void)self;
}
__attribute__((weak)) void Shadows_SetPitchLimits(float lowest, float highest)
{
	(void)lowest;
	(void)highest;
}

/* ---------- test doubles of the engine services this image does not link (the game uses
   bKernel's heap and ClassLib's lists; these copy their behaviour) */

#include <stdlib.h>

void *bkHeapAlloc(uint size, const char *file, int line, uint flags)
{
	(void)file;
	(void)line;
	(void)flags;
	return malloc(size);
}

void *bkHeapCalloc(uint size, byte fill, const char *file, int line, uint flags)
{
	void *p = malloc(size);

	(void)file;
	(void)line;
	(void)flags;
	if (p != NULL)
		memset(p, fill, size);
	return p;
}

void bkHeapFree(void *pointer)
{
	free(pointer);
}

/* classlib_ui ClassLib/unplaced.c 0x00071050-0x00071110 */
void CLList_Init(int *list)
{
	list[1] = (int)list;
	list[0] = (int)list;
	list[2] = 0;
}

void CLList_Insert(int *list, int *node, int at_tail)
{
	int *last;
	int first;

	if (at_tail != 0)
	{
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
	if (node[0] != 0)
	{
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

extern const char *DAT_002b2428[], *DAT_002b2590[], *DAT_002b262c[], *DAT_002b2668[];
extern char DAT_0030a09c[];

static void test_preprocess(void)
{
	static const char src[] =
		"#define HIGH 120\n"
		"/* a comment\n spanning lines */ LANDSCAPE site.obe 0 HIGH 0\n"
		"PLAY_TIME 4800 // the round\n"
		"PUZZLE first one\n";
	char *text;
	char *out;
	int size;

	/* the #define list (core's static constructor builds it in the game) */
	CLList_Init((int *)DAT_0030a09c);
	size = (int)strlen(src);
	text = (char *)bkHeapAlloc(size + 2, "test", 0, 0x2001);
	memcpy(text, src, size + 1);
	out = LevelLoad_Preprocess(text, &size);
	CHECK(out != NULL, "the preprocessor returns a buffer");
	if (out == NULL)
		return;
	out[size] = '\0';
	CHECK(strstr(out, "comment") == NULL, "block comments are dropped");
	CHECK(strstr(out, "round") == NULL, "line comments are dropped");
	/* #define lines stay (their name replaced too); the loader skips them with its own
	   "#define" keyword that reads two words */
	CHECK(strstr(out, "#define 120 120\r") != NULL, "#define lines stay for the loader to skip");
	CHECK(strstr(out, "LANDSCAPE site.obe 0 120 0\r") != NULL,
		"defines are replaced and the line's last word ends with \\r");
	CHECK(strstr(out, "PUZZLE first one\r") != NULL, "a PUZZLE line keeps its words for strtok");
	CHECK(size == (int)strlen(out), "the size is the text written");
}

static void test_tables(void)
{
	char flags[] = "HURT_WHEN_ON+PUSH_ONLY";

	CHECK(strcmp(DAT_002b2428[0], "DEBRIS") == 0, "action 0 is DEBRIS");
	CHECK(strcmp(DAT_002b2428[0x49], "ENABLE_SLIDDING_TEXTURE_MODEL") == 0,
		"action 0x49 is the last action");
	CHECK(LevelLoad_Lookup("BOUNCE", DAT_002b2590) == 2, "path behaviour BOUNCE is 2");
	CHECK(LevelLoad_Lookup("NOPE", DAT_002b2590) == -1, "an unknown name is -1");
	CHECK(LevelLoad_ParseFlags(flags, DAT_002b262c) == ((1u << 1) | (1u << 4)),
		"vent hazard flags combine with +");
	CHECK(strcmp(DAT_002b2668[0], "ON_WATER") == 0, "emitter flag table");
	CHECK(MB_META(0x002b0c90, float) == -0.5f, "water splash depth tweakable");
	CHECK(MB_META(0x002b0cac, int) == 1000, "rain splash block size");
	CHECK(strcmp(DAT_002b0be8[0].name, "on") == 0 && strcmp(DAT_002b0be8[1].name, "off") == 0,
		"big red button animations");
	CHECK(DAT_002b0be8[0].speed == 1.0f, "animation speed");
	CHECK(strcmp(DAT_002b0a90[3].name, "missile.obe") == 0, "bomb models by kind");
	CHECK(strcmp(DAT_002b0a64[1].name, "scorch2.bmp") == 0 && DAT_002b0a64[1].size == 60.0f,
		"scorch mark types");
	CHECK(DAT_002b0a4c == 24.0f && DAT_002b0a50 == 10, "lightning spiral tweakables (aliases)");
}

static void test_pack_track(void)
{
	float keys[3][4] = {
		{ 10.0f, 0.0f, -5.0f, 0.0f },
		{ 20.0f, 4.0f, -5.0f, 1024.0f },
		{ 15.0f, 2.0f, -5.0f, 2048.0f },
	};
	struct MbKeyTrack track;

	memset(&track, 0, sizeof track);
	track.numKeys = 3;
	LevelLoad_PackTrack(&track, &keys[0][0]);
	CHECK(track.min[0] == 10.0f && track.min[1] == 0.0f && track.min[2] == -5.0f,
		"the minimum of each axis");
	CHECK(track.scale[2] == 0.0f, "a flat axis has no scale");
	CHECK(track.keys != NULL, "keys allocated");
	if (track.keys == NULL)
		return;
	CHECK(track.keys[4 + 0] == 1 && track.keys[8 + 0] == 2, "times are stored / 1024");
	CHECK(track.keys[4 + 1] == (short)(int)(10.0f / ((20.0f - 10.0f) * 1.5259022e-05f)),
		"positions in 1/65535 of the range");
	CHECK(track.keys[3] == 0 && track.keys[7] == 0, "a flat axis packs to 0");
}

static void test_vent_cycle(void)
{
	static struct CMetaVent vent;

	memset(&vent, 0, sizeof vent);
	CLList_Init((int *)&DAT_00308e60);
	CMetaVent_Init(&vent, NULL, 0, 7, 0, 0.0f);
	CHECK(vent.colour[0] == -1, "colour 0 means the particle type's colour");
	CHECK(CMetaVent_AddCycle(&vent, 2, 3) == 1, "a cycle step is added");
	CMetaVent_SetActive(&vent, 1);
	CHECK(vent.active == 1 && vent.on == 0 && vent.timer == 1, "switched on: off for one frame");
	CMetaVent_Update(&vent);
	CHECK(vent.on == 1 && vent.timer == 3, "then on for the step's on time");
	CMetaVent_Update(&vent);
	CMetaVent_Update(&vent);
	CMetaVent_Update(&vent);
	CHECK(vent.on == 0 && vent.timer == 2, "then off for the step's off time");
	CHECK(MetaVents_SetActive(7, 0) == 1 && vent.active == 0, "vents are switched by tag");
	MB_LIST_REMOVE(&DAT_00308e60, &vent);
}

static void test_water_clamp(void)
{
	CHECK(MetaWater_Clamp(5.0f, 2.0f) == 2.0f, "a step is limited to the speed");
	CHECK(MetaWater_Clamp(-5.0f, 2.0f) == -2.0f, "with the step's sign");
	CHECK(MetaWater_Clamp(1.0f, -2.0f) == 1.0f, "a small step is kept (the speed's sign does not matter)");
}

int main(void)
{
	test_preprocess();
	test_tables();
	test_pack_track();
	test_vent_cycle();
	test_water_clamp();
	fprintf(stderr, "meta_b tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
