/*
BACTOR_TEST.INC (a C file; not named .c so the game image does not compile it)

Tests of the bactor module as a guest image. Run with
src/bactor/tests/run_tests.sh <build dir>. Exit code: the number of failed
checks. Only BACTOR is linked; every other game function is its weak stub
(so nothing here may depend on bKernel or bDisplay doing real work).

What they protect:
	- key tracks decode quantised keys (value * scale + base) and find the
	  right pair of keys (the binary search with its odd end cases), because
	  every animated node is posed from them;
	- an animation that reaches its end is reported once to its callback and
	  frees its slot; a looping one wraps its time; nodes follow the track;
	- shader fragments get their own constant registers: unskinned shaders
	  start at c2, skinned ones after the bone matrices (c48), and %cN is
	  relative to the fragment; text substitution of %N;
	- a full extra system that recycles drops its oldest item to make room
	  and never grows past its size;
	- lights get distinct D3D light indices (with the original's quirk: a new
	  light's own stale index counts as used).
*/

#include <stdio.h>
#include <string.h>

#include "bactor.h"

static int failures;

#define CHECK(condition) check((condition), #condition, __LINE__)
#define NEAR(a, b) ((a) - (b) < 0.001f && (b) - (a) < 0.001f)

static void check(int ok, const char *text, int line)
{
	if (!ok)
	{
		failures++;
		printf("FAIL line %d: %s\n", line, text);
	}
}

/* ---------- key tracks */

static void test_key_tracks(void)
{
	static struct BKey3 keys[3] = {
		{ 0, { 0, 100, 4 } },
		{ 10, { 10, 200, 4 } },
		{ 20, { 30, 300, 4 } },
	};
	struct BKeyTrack3 track;
	float v[3];

	memset(&track, 0, sizeof(track));
	track.numKeys = 3;
	track.keys = keys;
	track.base[0] = 1.0f;
	track.base[1] = 0.0f;
	track.base[2] = -2.0f;
	track.scale[0] = 0.5f;
	track.scale[1] = 0.01f;
	track.scale[2] = 1.0f;

	bGetKey3(v, &track, 0);
	CHECK(NEAR(v[0], 1.0f) && NEAR(v[1], 1.0f) && NEAR(v[2], 2.0f));
	/* exactly on the middle key */
	bGetKey3(v, &track, 10 << 10);
	CHECK(NEAR(v[0], 6.0f) && NEAR(v[1], 2.0f));
	/* half way between keys 0 and 1 */
	bGetKey3(v, &track, 5 << 10);
	CHECK(NEAR(v[0], 3.5f) && NEAR(v[1], 1.5f));
	/* a quarter of the way from key 1 to key 2 */
	bGetKey3(v, &track, (10 << 10) + (10 << 10) / 4);
	CHECK(NEAR(v[0], 6.0f + 0.25f * 10.0f) && NEAR(v[1], 2.25f));
	/* past the last key: the last key */
	bGetKey3(v, &track, 40 << 10);
	CHECK(NEAR(v[0], 16.0f) && NEAR(v[1], 3.0f));
}

/* ---------- animation slots */

static struct BAnim *s_endedAnim;
static int s_endedCount;
static int s_endedReason;

static int anim_ended(int reason, void *instance, void *node, struct BAnim *anim, void *context)
{
	(void)instance;
	(void)node;
	(void)context;
	s_endedAnim = anim;
	s_endedReason = reason;
	s_endedCount++;
	return 0;
}

static void test_animation(void)
{
	static struct BKey3 posKeys[2] = { { 0, { 0, 0, 0 } }, { 10, { 10, 0, 0 } } };
	static struct BKey3 scaleKeys[1] = { { 0, { 1, 1, 1 } } };
	static struct BKey4 rotKeys[1] = { { 0, { 0, 0, 0, 1 } } };
	static struct BActorNodeDef def;
	static struct
	{
		struct BActorNode node;
		struct BAnimSlot slots[2];
	} nodes;
	static struct BActorInstance inst;
	struct BAnim anim = { 0, 0, 10 << 10, 0 };
	int active;

	memset(&def, 0, sizeof(def));
	def.posTrack.numKeys = 2;
	def.posTrack.keys = posKeys;
	def.posTrack.scale[0] = 1.0f;
	def.scaleTrack.numKeys = 1;
	def.scaleTrack.keys = scaleKeys;
	def.scaleTrack.scale[0] = def.scaleTrack.scale[1] = def.scaleTrack.scale[2] = 1.0f;
	def.rotTrack.numKeys = 1;
	def.rotTrack.keys = rotKeys;
	def.rotTrack.scale[3] = 1.0f;
	def.next = &def;
	def.prev = &def;
	memset(&nodes, 0, sizeof(nodes));
	memset(&inst, 0, sizeof(inst));

	CHECK(bCountNodeDefs(&def) == 1);
	CHECK(bInitNodes(&def, &nodes.node, NULL, 2) == (struct BActorNode *)&nodes.slots[2]);
	CHECK(nodes.node.next == &nodes.node && nodes.node.numSlots == 2);
	CHECK(nodes.node.slots == nodes.slots);
	inst.root = &nodes.node;
	inst.rot[3] = 1.0f;
	inst.scale[0] = inst.scale[1] = inst.scale[2] = 1.0f;

	/* play: the time advances by speed (default 0x400 per tick) * dt */
	baQueueAnimation(&inst, NULL, &anim, 0, -1, 0, anim_ended, NULL);
	CHECK(nodes.slots[0].active == 1 && nodes.slots[0].mode == BANIM_PLAY);
	CHECK(nodes.slots[0].speed == 0x400 && nodes.slots[0].time == 0);
	CHECK(baIsAnimating(&inst, NULL, 0, NULL) == 1);
	active = baUpdateAnimations(&inst, inst.root, 5.0f, NULL);
	CHECK(active == 1);
	CHECK(nodes.slots[0].time == (5 << 10));
	CHECK(NEAR(nodes.node.pos[0], 5.0f));
	CHECK(s_endedCount == 0);

	/* past the end: reported once, the slot is free, the node rests on the last frame */
	baUpdateAnimations(&inst, inst.root, 10.0f, NULL);
	CHECK(s_endedCount == 1 && s_endedAnim == &anim && s_endedReason == 0);
	CHECK(nodes.slots[0].active == 0);
	CHECK(nodes.node.curSlot == 1);
	CHECK(NEAR(nodes.node.pos[0], 10.0f));
	CHECK(baIsAnimating(&inst, NULL, 0, NULL) == 0);
	baUpdateAnimations(&inst, inst.root, 10.0f, NULL);
	CHECK(s_endedCount == 1);

	/* a loop wraps: 12 ticks into a 10-tick animation is frame 12 - 10 - 1 / 1025 */
	baQueueAnimation(&inst, NULL, &anim, 0, -1, BANIMF_LOOP, anim_ended, NULL);
	CHECK(nodes.slots[1].active == 1);
	baUpdateAnimations(&inst, inst.root, 12.0f, NULL);
	CHECK(nodes.slots[1].active == 1);
	CHECK(nodes.slots[1].time == ((12 << 10) - (10 << 10)) % ((10 << 10) + 1));
	CHECK(s_endedCount == 1);

	/* flushing calls the callback with reason 1 and empties the queue */
	baFlushAnimations(&inst, inst.root, 1, 0);
	CHECK(s_endedCount == 2 && s_endedReason == 1);
	CHECK(nodes.slots[1].active == 0 && nodes.node.curSlot == 0);
}

/* ---------- shader fragments */

static void test_shader_fragments(void)
{
	struct BShaderFragment op2;
	struct BShaderFragment flat;
	static struct BShaderMode mode;
	char text[256];
	int lines;

	bShaderFragmentInit(&op2, 3, 0, "%0 %1, %2\n");
	bShaderFragmentInit(&flat, 0, 2, "mov oD0, %c0\nmad r0.xyz, r2, %c1.w, r0\n");
	CHECK(op2.numLines == 1 && flat.numLines == 2 && flat.numConstants == 2);

	memset(&mode, 0, sizeof(mode));
	text[0] = '\0';
	lines = bShaderAppendFragment(&op2, &mode, text, "mov", "r0", "v0");
	CHECK(lines == 1);
	CHECK(strcmp(text, "mov r0, v0\n") == 0);
	CHECK(mode.build.count == 1 && mode.build.constantBase[0] == 2);
	lines = bShaderAppendFragment(&flat, &mode, text);
	CHECK(lines == 2);
	CHECK(strcmp(text, "mov r0, v0\nmov oD0, c[2]\nmad r0.xyz, r2, c[3].w, r0\n") == 0);
	/* the next fragment starts after flat's two constants */
	lines = bShaderAppendFragment(&op2, &mode, text, "mov", "r1", "v1");
	CHECK(mode.build.count == 3 && mode.build.constantBase[2] == 4);

	/* skinned: the first fragment starts after the bone matrices */
	memset(&mode, 0, sizeof(mode));
	mode.numBones = 1;
	text[0] = '\0';
	bShaderAppendFragment(&flat, &mode, text);
	CHECK(mode.build.constantBase[0] == 0x30);
	CHECK(strcmp(text, "mov oD0, c[48]\nmad r0.xyz, r2, c[49].w, r0\n") == 0);
}

/* ---------- extras */

static void test_extras(void)
{
	static struct BExtraSystem system;
	static int used[2];
	static void *items[2];
	static char data[2 * 16];
	void *a;
	void *b;
	void *c;

	memset(&system, 0, sizeof(system));
	system.flags = 1; /* recycle */
	system.maxItems = 2;
	system.itemSize = 16;
	system.used = used;
	system.items = items;
	system.itemData = data;

	a = baAddExtraItem(&system);
	b = baAddExtraItem(&system);
	CHECK(a == data && b == data + 16);
	CHECK(system.numItems == 2 && system.maxUsed == 2);
	/* full: the oldest goes, its slot is reused */
	c = baAddExtraItem(&system);
	CHECK(c == data);
	CHECK(system.numItems == 2 && items[0] == b && items[1] == c);
	/* without recycling a full system refuses */
	system.flags = 0;
	CHECK(baAddExtraItem(&system) == NULL);
	CHECK(baDeleteExtraItem(&system, 0) == 1);
	CHECK(system.numItems == 1 && items[0] == c && used[1] == 0);
}

/* ---------- lights */

static void test_lights(void)
{
	static struct BLight a;
	static struct BLight b;

	/* baInitLights asks FUN_000a6fa0 (another module's "return 1", a stub here) */
	g_lightsOff.next = g_lightsOff.prev = &g_lightsOff;
	g_lightsOn.next = g_lightsOn.prev = &g_lightsOn;
	CHECK(baCreateLight(&a, 0) == &a);
	CHECK(baCreateLight(&b, 0) == &b);
	/* the new light is already in the list when its index is chosen, so its own
	(here zero) index counts as used, as in the original */
	CHECK(a.index == 1 && b.index == 2);
	CHECK(a.index != b.index);
	CHECK(g_lightsOff.next == &a && a.next == &b && b.next == &g_lightsOff);
	CHECK(a.color[0] == 0x7f && (a.flags & BLIGHTF_ALLOCATED) == 0);
}

int main(void)
{
	test_key_tracks();
	test_animation();
	test_shader_fragments();
	test_extras();
	test_lights();
	printf("bactor_test: %d failure(s)\n", failures);
	return failures;
}
