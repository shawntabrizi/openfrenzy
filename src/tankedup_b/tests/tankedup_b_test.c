/*
tankedup_b module tests: one guest image (src/tankedup_b/tests/run_tests.sh builds it from
the game build's objects of src/tankedup_b and the generated weak stubs).

The game image globs every .c under src/tankedup_b, so this file compiles to nothing unless
OPENFRENZY_TANKEDUP_B_TEST is defined. Only this module's objects are linked: functions of
other modules are the weak stubs (they do nothing and return 0), other modules' data are
zero slots made by run_tests.sh, and this file gives strong test versions of the bkernel
heap and timer, of the engine's sample player and of bumpercars_b's quoted-name parser, so
that the code under test can allocate, read names, and its sound calls can be observed.

What the tests pin down, and why it matters to the game:
- ranks (0x001d62d0): equal scores share a rank and the best is 1. Respawn delays, the
  fire-reload time and the AI's protection time depend on the rank.
- the text reader (0x001d7c90): a line comes back without leading white space and the
  line count goes up. The reader skips the rest of the following line after a line end, so
  the TankedUp data files only work with CR LF line ends; with LF ends every second line
  is lost. That is the original behaviour and the port keeps it (the disc files are CR LF).
- the entity file (0x001cef60): templates take their values (ROT n is a turn of 2 pi / n
  a tick, SCORELOST s keeps 1 - s of the score), a bad value is refused and the default
  stays (MASS 0 keeps 10), entities get their name, template name and position.
- delayed messages (0x001d1ac0, 0x001d1b40): the queue stays sorted by due time and
  messages due at the same time keep their order; removing works by (type, entity) only.
  The AI is a chain of such messages; a wrong order changes what the AI does.
- the move queue (0x001de780-0x001de940): one put stores the move 3 times per frame unit,
  one get returns the oldest and drops one per frame unit but never the last. Its length
  is the AI's reaction time by difficulty.
- barrier check (0x001d7d80): a barrier (type 3) or a raised wall (type 8 not in state 0xc)
  within 890 blocks a crate drop point; a lowered wall does not.
- decal points (0x001de140): the first point is always added; later ones only 100 or more
  from the last or after a direction change of 0.2 or more. Too many points eat the ring
  (240 points) and the tracks vanish early; too few make them jagged.
- steering (0x001d1f90, 0x001d1e80): a stick straight ahead drives both tracks forward, a
  stick past the turn band but in front spins the tank on the spot (left forward, right
  back), and the tracks' texture scroll follows the track speeds.
- the sound bank (0x001d5a50): the volume is clamped to 255, a pitch offset is added to the
  sample's own frequency, an unloaded bank or a negative volume plays nothing.
*/

#ifdef OPENFRENZY_TANKEDUP_B_TEST

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

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

/* ---------- strong test versions of other modules' functions */

void *bkHeapAlloc(uint size, const char *file, int line, uint flags)
{
	(void)file;
	(void)line;
	(void)flags;
	return calloc(1, size);
}

void bkHeapFree(void *pointer)
{
	free(pointer);
}

/* bumpercars_b 0x000a4e00: the text between the first two quotes (as there) */
int BCarText_GetName(char *line, char *out, int size)
{
	int n = 0;

	line = strchr(line, '"');
	if (line == NULL) {
		*out = 0;
		return 0;
	}
	for (line++; *line != 0 && *line != '"' && n < size; line++)
		out[n++] = *line;
	out[n] = 0;
	return 1;
}

static unsigned long long t_now;

ulonglong bkTimerRead(void)
{
	return t_now;
}

static struct {
	int calls;
	int volume;
	int frequency;
} t_play;

int bsPlaySample(struct BSample *sample, int volume, int pan, int frequency, unsigned int flags)
{
	(void)sample;
	(void)pan;
	(void)flags;
	t_play.calls++;
	t_play.volume = volume;
	t_play.frequency = frequency;
	return 3;
}

/* ---------- ranks */

static void test_ranks(void)
{
	int scores[4] = { 5, 9, 5, 1 };
	int ranks[4] = { 0, 0, 0, 0 };

	TankRanks(scores, ranks);
	CHECK(ranks[1] == 1, "the best score ranks 1");
	CHECK(ranks[0] == 2 && ranks[2] == 2, "equal scores share a rank");
	CHECK(ranks[3] == 4, "the worst of four ranks 4");
}

/* ---------- the text reader */

/* a reader over `text` (tankedup_b.h RD_*) */
static void t_reader(char *reader, const char *text)
{
	memset(reader, 0, 0x214);
	TankReader_Ctor(reader);
	TU_AT(reader, RD_CUR, const char *) = text;
	TU_AT(reader, RD_END, const char *) = text + strlen(text);
	strcpy(reader + RD_NAME, "test.txt");
}

static void test_reader(void)
{
	char reader[0x214];
	char line[512];
	char *got;
	static const char crlf[] = "  first\r\n\tsecond line\r\nthird\r\n";
	static const char lf[] = "one\ntwo\nthree\n";

	t_reader(reader, crlf);
	got = TankReader_GetLine(reader, line);
	CHECK(strcmp(got, "first") == 0, "leading spaces are skipped");
	got = TankReader_GetLine(reader, line);
	CHECK(strcmp(got, "second line") == 0, "a leading tab is skipped, inner spaces stay");
	got = TankReader_GetLine(reader, line);
	CHECK(strcmp(got, "third") == 0, "the third CR LF line");
	CHECK(TU_I32(reader, RD_LINE) == 3, "three lines counted");
	CHECK(!TU_READER_MORE(reader), "the reader is at the end");

	t_reader(reader, lf);
	got = TankReader_GetLine(reader, line);
	CHECK(strcmp(got, "one") == 0, "the first LF line");
	got = TankReader_GetLine(reader, line);
	CHECK(strcmp(got, "three") == 0, "with LF line ends the line after a line is skipped");
}

/* ---------- the entity file */

static void test_entity_file(void)
{
	static const char text[] =
		"// a comment\r\n"
		"TEMPLATE {\r\n"
		"NAME \"tank\"\r\n"
		"MASS 0\r\n"
		"RADIUS 50\r\n"
		"TYPE 1\r\n"
		"ROT 30\r\n"
		"SCORELOST 0.25\r\n"
		"MENU\r\n"
		"}\r\n"
		"ENTITY {\r\n"
		"NAME \"player1\"\r\n"
		"TEMPLATE \"tank\"\r\n"
		"POSITION 1 2 3\r\n"
		"}\r\n"
		"SETTINGS {\r\n"
		"3Pt_TK 7\r\n"
		"}\r\n";
	char ents[0x100];
	char reader[0x214];
	char *tpl;
	char *ent;

	memset(ents, 0, sizeof ents);
	t_reader(reader, text);
	TankEnts_LoadEntities(ents, reader);
	tpl = (char *)TU_PTR(ents, ENTS_TEMPLATES);
	CHECK(tpl != NULL, "a template was read");
	if (tpl != NULL) {
		CHECK(strcmp(tpl + TPL_NAME, "tank") == 0, "template name");
		CHECK(near(TU_F32(tpl, TPL_MASS), 10.0f), "MASS 0 is refused: the default 10 stays");
		CHECK(near(TU_F32(tpl, TPL_RADIUS), 50.0f), "RADIUS");
		CHECK(TU_I32(tpl, TPL_TYPE) == 1, "TYPE");
		CHECK(near(TU_F32(tpl, TPL_ROT), 6.28318548f / 30.0f), "ROT 30 is a turn of 2 pi / 30");
		CHECK(near(TU_F32(tpl, TPL_SCORELOST), 0.75f), "SCORELOST 0.25 keeps 0.75 of the score");
		CHECK(TU_I32(tpl, TPL_MENU) == 1, "MENU gives the template a tweak menu");
		CHECK(near(TU_F32(tpl, TPL_SCALE), 1.0f), "the default scale is 1");
	}
	CHECK(TU_PTR(ents, ENTS_LIST) != NULL, "an entity was read");
	if (TU_PTR(ents, ENTS_LIST) != NULL) {
		ent = (char *)((void **)TU_PTR(ents, ENTS_LIST))[0];
		CHECK(strcmp(ent + ENT_NAME, "player1") == 0, "entity name");
		CHECK(strcmp(ent + ENT_TEMPLATE_NAME, "tank") == 0, "entity template name");
		CHECK(near(TU_F32(ent, ENT_POS), 1.0f) && near(TU_F32(ent, ENT_POS + 4), 2.0f) &&
			near(TU_F32(ent, ENT_POS + 8), 3.0f) && near(TU_F32(ent, ENT_POS + 12), 1.0f),
			"entity position (w = 1)");
	}
	CHECK(TU_I32(ents, ENTS_PT3) == 7, "SETTINGS 3Pt_TK sets the most 3-point tokens");
}

/* ---------- delayed messages */

static void *t_message(int type, int id, long long due)
{
	char *msg = (char *)calloc(1, 0x30);

	TU_I32(msg, 0) = type;
	TU_I32(msg, 0x1c) = id;
	TU_AT(msg, 0x08, long long) = due;
	return msg;
}

static int t_queue_types(char *ents, int *types, int max)
{
	void **node;
	int n = 0;

	for (node = (void **)TU_PTR(ents, ENTS_MESSAGES); node != NULL && n < max; node = (void **)node[1])
		types[n++] = TU_I32(node[0], 0);
	return n;
}

static void test_messages(void)
{
	char ents[0x100];
	int types[8];
	int n;

	memset(ents, 0, sizeof ents);
	TankEnts_StoreMessage(ents, t_message(1, 10, 300));
	TankEnts_StoreMessage(ents, t_message(2, 10, 100));
	TankEnts_StoreMessage(ents, t_message(3, 11, 300));
	TankEnts_StoreMessage(ents, t_message(4, 10, 200));
	n = t_queue_types(ents, types, 8);
	CHECK(n == 4, "four messages queued");
	CHECK(n == 4 && types[0] == 2 && types[1] == 4 && types[2] == 1 && types[3] == 3,
		"sorted by due time; equal times keep their order");
	TankEnts_RemoveMessages(ents, 10, 1);
	n = t_queue_types(ents, types, 8);
	CHECK(n == 3 && types[0] == 2 && types[1] == 4 && types[2] == 3,
		"removing (type 1, entity 10) leaves type 1 of other entities and other types");
	TankEnts_RemoveMessages(ents, 11, 3);
	TankEnts_RemoveMessages(ents, 10, 2);
	TankEnts_RemoveMessages(ents, 10, 4);
	CHECK(TU_PTR(ents, ENTS_MESSAGES) == NULL, "the queue is empty");
}

/* ---------- the move queue */

static void test_move_queue(void)
{
	char queue[0x14];
	float a, b, c, d;

	DAT_0028fda8 = 1.0f;
	TankMoveQue_Ctor(queue, 4);
	TankMoveQue_Get(queue, &a, &b, &c, &d);
	CHECK(a == 0.0f && b == 0.0f && c == 0.0f && d == 0.0f, "an empty queue gives zeros");
	TankMoveQue_Put(queue, 1.0f, 2.0f, 3.0f, 4.0f);
	CHECK(TU_I32(queue, 0xc) == 3, "one put stores the move 3 times per frame unit");
	TankMoveQue_Put(queue, 5.0f, 6.0f, 7.0f, 8.0f);
	CHECK(TU_I32(queue, 0xc) == 4, "the queue stops at its capacity");
	TankMoveQue_Get(queue, &a, &b, &c, &d);
	CHECK(a == 1.0f && b == 2.0f && c == 3.0f && d == 4.0f, "get returns the oldest move");
	CHECK(TU_I32(queue, 0xc) == 3, "one move is dropped per frame unit");
	TankMoveQue_Get(queue, &a, &b, &c, &d);
	TankMoveQue_Get(queue, &a, &b, &c, &d);
	TankMoveQue_Get(queue, &a, &b, &c, &d);
	CHECK(a == 5.0f && TU_I32(queue, 0xc) == 1, "the last move is never dropped");
	TankMoveQue_Get(queue, &a, &b, &c, &d);
	CHECK(a == 5.0f, "the last move repeats");
	TankMoveQue_Free(queue);
}

/* ---------- entities for the list tests */

static char t_templates[4][0x6e0];
static char t_entities[4][0xf08];
static void *t_nodes[4][2];

/* a list of `n` entities in the CTank_Entities `ents` */
static void t_entity_list(char *ents, int n)
{
	int i;

	memset(t_templates, 0, sizeof t_templates);
	memset(t_entities, 0, sizeof t_entities);
	TU_PTR(ents, ENTS_LIST) = n > 0 ? t_nodes[0] : NULL;
	for (i = 0; i < n; i++) {
		TU_I32(t_entities[i], ENT_ID) = i + 1;
		TU_PTR(t_entities[i], ENT_TEMPLATE) = t_templates[i];
		TU_F32(t_entities[i], ENT_POS + 12) = 1.0f;
		t_nodes[i][0] = t_entities[i];
		t_nodes[i][1] = i + 1 < n ? t_nodes[i + 1] : NULL;
	}
}

static void test_position_clear(void)
{
	char ents[0x100];
	float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	memset(ents, 0, sizeof ents);
	t_entity_list(ents, 2);
	TU_I32(t_templates[0], TPL_TYPE) = 3;          /* a barrier */
	TU_F32(t_entities[0], ENT_POS) = 800.0f;
	TU_I32(t_templates[1], TPL_TYPE) = 1;          /* a tank does not block */
	TU_F32(t_entities[1], ENT_POS + 8) = 10.0f;
	CHECK(TankAI_PositionClear(ents, pos, NULL) == 0, "a barrier 800 away blocks");
	CHECK(TankAI_PositionClear(ents, pos, t_entities[0]) == 1, "the ignored entity does not block");
	TU_F32(t_entities[0], ENT_POS) = 900.0f;
	CHECK(TankAI_PositionClear(ents, pos, NULL) == 1, "a barrier 900 away does not block");
	TU_F32(t_entities[0], ENT_POS) = 100.0f;
	TU_I32(t_templates[0], TPL_TYPE) = 8;          /* a wall */
	TU_I32(t_entities[0], 0x8c) = 0xc;             /* lowered */
	CHECK(TankAI_PositionClear(ents, pos, NULL) == 1, "a lowered wall does not block");
	TU_I32(t_entities[0], 0x8c) = 9;
	CHECK(TankAI_PositionClear(ents, pos, NULL) == 0, "a raised wall blocks");
}

/* ---------- decal points */

static void test_decal(void)
{
	char decal[0x3c];
	float normal[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
	float dir[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
	float turned[4] = { 0.5f, 0.0f, 1.0f, 1.0f };
	float pos[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	memset(decal, 0, sizeof decal);
	TankDecal_Ctor(decal, 1, 90.0f, 4.0f, NULL, NULL);
	CHECK(TU_I32(decal, 0x0c) == 1, "fewer than 2 points disables the decal");
	memset(decal, 0, sizeof decal);
	TankDecal_Ctor(decal, 3, 90.0f, 4.0f, NULL, NULL);
	CHECK(TU_I32(decal, 0x0c) == 0 && near(TU_F32(decal, 0x1c), 240.0f),
		"3 points; a life of 4 s is 240 frames");
	TankDecal_AddPoint(decal, pos, normal, dir, 0.5f);
	CHECK(TU_I32(decal, 0x08) == 1, "the first point is added");
	pos[2] = 50.0f;
	TankDecal_AddPoint(decal, pos, normal, dir, 0.5f);
	CHECK(TU_I32(decal, 0x08) == 1, "a point 50 from the last on the same heading is not added");
	pos[2] = 100.0f;
	TankDecal_AddPoint(decal, pos, normal, dir, 0.5f);
	CHECK(TU_I32(decal, 0x08) == 2, "a point 100 from the last is added");
	pos[2] = 110.0f;
	TankDecal_AddPoint(decal, pos, normal, turned, 0.5f);
	CHECK(TU_I32(decal, 0x08) == 0 && TU_I32(decal, 0x04) == 1,
		"a turn adds a point; the ring wraps and drops the oldest");
	CHECK(TU_I32((char *)TU_PTR(decal, 0x20) + 0 * 0x3c, 0) == 1, "points after the first join the track");
	free(TU_PTR(decal, 0x20));
}

/* ---------- steering */

static void test_steering(void)
{
	char ents[0x100];
	char *tank;

	memset(ents, 0, sizeof ents);
	t_entity_list(ents, 1);
	tank = t_entities[0];
	TU_I32(ents, ENTS_VARIANT) = 4;               /* playing */
	DAT_003150b4 = 60.0f;                          /* the turn band, degrees */
	TankEnts_DriveAnalog(ents, 1, 0.0f, 1.0f);
	CHECK(near(TU_F32(tank, 0x9c4), 1.0f) && near(TU_F32(tank, 0x9c8), 1.0f),
		"stick straight ahead: both tracks forward at full speed");
	TU_F32(tank, 0x9c4) = 0.0f;
	TU_F32(tank, 0x9c8) = 0.0f;
	TankEnts_DriveAnalog(ents, 1, 1.57079625f, 1.0f);
	CHECK(near(TU_F32(tank, 0x9c4), 1.0f) && near(TU_F32(tank, 0x9c8), -1.0f),
		"stick to the side past the band: the tank spins (left forward, right back)");
	TU_F32(tank, 0x9c4) = 0.0f;
	TU_F32(tank, 0x9c8) = 0.0f;
	TU_I32(ents, ENTS_VARIANT) = 3;
	TankEnts_DriveAnalog(ents, 1, 0.0f, 1.0f);
	CHECK(TU_F32(tank, 0x9c4) == 0.0f && TU_F32(tank, 0x9c8) == 0.0f,
		"before the game starts the tracks do not move");
}

/* ---------- the sound bank */

static void test_sound(void)
{
	char bank[0xec];
	char sample[0x40];

	memset(bank, 0, sizeof bank);
	memset(sample, 0, sizeof sample);
	TU_I32(sample, 0x20) = 22050;
	TU_PTR(bank, 0x70 + 5 * 4) = sample;
	t_play.calls = 0;
	CHECK(TankSound_Play(bank, 5, NULL, 200, 1, 0) == -1 && t_play.calls == 0,
		"an unloaded bank plays nothing");
	TU_I32(bank, 0xe0) = 1;
	CHECK(TankSound_Play(bank, 5, NULL, -1, 1, 0) == -1 && t_play.calls == 0,
		"a negative volume plays nothing");
	CHECK(TankSound_Play(bank, 5, NULL, 300, 1, 0) == 3 && t_play.volume == 0xff && t_play.frequency == 0,
		"the volume is clamped to 255; no pitch keeps the recorded rate (0)");
	TankSound_Play(bank, 5, NULL, 100, 1, 1000);
	CHECK(t_play.volume == 100 && t_play.frequency == 23050, "a pitch offset is added to the sample rate");
	CHECK(TankSound_Play(bank, 6, NULL, 100, 1, 0) == -1, "a missing sample plays nothing");
}

int main(void)
{
	test_ranks();
	test_reader();
	test_entity_file();
	test_messages();
	test_move_queue();
	test_position_clear();
	test_decal();
	test_steering();
	test_sound();
	fprintf(stderr, "tankedup_b tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
