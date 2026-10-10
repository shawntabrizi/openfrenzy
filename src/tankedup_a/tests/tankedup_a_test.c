/*
tankedup_a module tests: one guest image (src/tankedup_a/tests/run_tests.sh builds it from the
game build's objects of src/tankedup_a, the generated weak stubs and the platform layer).

The game image globs every .c under src/tankedup_a, so this file compiles to nothing unless
OPENFRENZY_TANKEDUP_A_TEST is defined. Only this module's objects are linked: other modules'
functions are the weak stubs (they do nothing and return 0) except the test doubles below,
and other modules' data are zero slots made by run_tests.sh.

Test doubles (strong definitions that replace the weak stubs in this image only): the
bKernel heap (calloc/free), the bKernel tick counter (a variable the tests move), resources
(a dummy non-NULL object), files (texts held here), and the text reader functions of
tankedup_b and bumpercars_b (copies of their behaviour: lines need CR LF ends, names are in
quotes).

What the tests pin down, and why it matters to the game:
- the data blocks: core constructs CTank_Game at DAT_00312f50 and tankedup_b's debug object
  at DAT_003150b0; both names must be the module's block, or the game object and the debug
  settings that this module reads are two different memories. The debris types point at
  their names ("MID1"...) and the debris sprites at "debris\sprites\aisteam"; a wrong
  pointer gives invisible debris.
- the timer: TIMER seconds give the game's length; the frame step (1.0 = 1/60 s) drives
  every system; the Turbo Mode cheat makes frames 1.5 times as long; paused time does not
  count.
- the spawn file: lists are found by name without case; points are added at the head, so the
  last point in the file is point 0 (the player who asks for point n gets that one).
- the object file: LAPS decides between a battle (0) and a race; an orientation outside 0-360
  degrees is refused; checkpoints keep the file's order (the race counts them in order); a
  boundary without a collision mesh is dropped.
- the lights: there are only four; a flash frees its light when its life ends, and a spot
  light gets no light (index -1) when all four are in use.
- the booms: a scorch lives 10 s; the boom is freed after it.
- the entities: tanks are found by name and id; the catch-up factor stays in 0.7..1; the race
  progress is never negative; a new entity gets the next id and the global AI state.
- the camera file: shots keep the file's order (a shot passes to the next one), END_TIME and
  LINK_TIME are seconds in the file and frames in the shot, and "FPCAM..." is the single
  player's shot.
- the players: a tank is on its own team; with teams on, the team word decides.
*/

#ifdef OPENFRENZY_TANKEDUP_A_TEST

#include "tankedup_a.h"

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

/* ---------- test doubles */

static unsigned long long test_ticks;
static unsigned char test_resource[64];
static const char *test_file_name;
static const char *test_file_text;

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

ulonglong bkTimerRead(void)
{
	return test_ticks;
}

ulonglong bkTimerDelta(ulonglong start, ulonglong end)
{
	return end - start;
}

struct bkResource *bLoadResource(struct bkPackage *package, const char *name, int type, int group)
{
	(void)package;
	(void)name;
	(void)type;
	(void)group;
	return (struct bkResource *)test_resource;
}

void *bkLoadFile(struct bkPackage *package, const char *name, void *buffer, uint *size, uint *tags,
	int extra)
{
	(void)package;
	(void)buffer;
	(void)tags;
	(void)extra;
	if (test_file_name == NULL || strcmp(name, test_file_name) != 0)
		return NULL;
	*size = (uint)strlen(test_file_text);
	return (void *)test_file_text;
}

void BCarTextReader_Init(struct BCarTextReader *self, char *buffer, int size, const char *name)
{
	struct TuTextReader *reader = (struct TuTextReader *)self;

	reader->cur = buffer;
	reader->end = buffer + size;
	snprintf(reader->name, sizeof reader->name, "%s", name);
	reader->line = 0;
}

int BCarText_GetName(char *line, char *out, int size)
{
	int n;

	while (*line != '"') {
		if (*line == 0) {
			*out = 0;
			return 0;
		}
		line++;
	}
	for (n = 0; line[1 + n] != 0 && line[1 + n] != '"' && n < size; n++)
		out[n] = line[1 + n];
	out[n] = 0;
	return 1;
}

/* tankedup_b 0x001d7c90 (the original's behaviour: the text needs CR LF line ends) */
char *FUN_001d7c90(void *self, char *buffer)
{
	struct TuTextReader *reader = (struct TuTextReader *)self;
	char *out = buffer;
	char *cur;

	while (reader->cur < reader->end && *reader->cur != '\n' && *reader->cur != '\r')
		*out++ = *reader->cur++;
	*out = 0;
	cur = reader->cur + 1;
	reader->cur = cur;
	while (*buffer != 0 && (*buffer == ' ' || *buffer == '\t'))
		buffer++;
	if (cur < reader->end) {
		while (cur < reader->end && *cur != '\n' && *cur != '\r')
			reader->cur = ++cur;
		while (cur < reader->end && (*cur == '\n' || *cur == '\r'))
			reader->cur = ++cur;
	}
	reader->line++;
	return buffer;
}

/* ---------- tests */

extern unsigned char DAT_00312f50[];
extern unsigned char DAT_003150b0[];
extern unsigned int DAT_002cd7a0;
extern unsigned int DAT_002cd7a4;
extern int DAT_00283e58;

static void test_data(void)
{
	const char *const *debris = (const char *const *)TU_ADDR(0x002b7210);
	const char *const *sprite = (const char *const *)TU_ADDR(0x002b6b48);

	CHECK((void *)DAT_00312f50 == (void *)of_tu_bss, "DAT_00312f50 is the start of the TankedUp block");
	CHECK((void *)DAT_003150b0 == (void *)(of_tu_bss + 0x2160), "DAT_003150b0 follows CTank_Game");
	CHECK((void *)TU_GAME == (void *)DAT_00312f50, "TU_GAME is core's CTank_Game");
	CHECK(strcmp(debris[0], "MID1") == 0, "debris type 0 is MID1");
	CHECK(strcmp(debris[11], "MID2") == 0, "debris type 1 is MID2 (0x2c bytes later)");
	CHECK(strcmp(debris[2 * 11], "MID3") == 0, "debris type 2 is MID3");
	CHECK(strcmp(sprite[0], "debris\\sprites\\aisteam") == 0, "the first debris sprite set");
}

static void test_timer(void)
{
	struct TuTimer timer;

	DAT_002cd7a0 = 60;
	DAT_002cd7a4 = 0;
	DAT_00283e58 = 0;
	test_ticks = 1000;
	memset(&timer, 0x55, sizeof timer);
	CHECK(CTankTimer_Initialise(&timer, 10) == 1, "Initialise returns 1");
	CHECK(near(timer.limit, 10.0f) && near(timer.limitMs, 10000.0f), "TIMER 10 is 10 s");
	CHECK(near(CTankTimer_GetFrame(&timer), 0.0f), "a stopped timer has no frame");
	CHECK(near(timer.remaining, 10.0f), "a stopped timer has its whole length left");
	CTankTimer_StartOrResume(&timer);
	test_ticks += 30;
	CHECK(near(CTankTimer_GetFrame(&timer), 30.0f), "half a second is 30 frames");
	CHECK(near(timer.elapsed, 0.5f) && near(timer.remaining, 9.5f), "elapsed and remaining");
	CHECK(near(timer.fraction, 0.05f), "the part of the length that passed");
	DAT_00283e58 = 1;
	test_ticks += 30;
	CHECK(near(CTankTimer_GetFrame(&timer), 45.0f), "Turbo Mode makes frames 1.5 times as long");
	CHECK(near(timer.elapsed, 1.0f), "Turbo Mode does not change the clock");
	DAT_00283e58 = 0;
	CTankTimer_Pause(&timer);
	test_ticks += 600;
	CTankTimer_Tick(&timer);
	CHECK(near(timer.elapsed, 1.0f), "a paused timer stands still");
	CTankTimer_StartOrResume(&timer);
	CTankTimer_Tick(&timer);
	CHECK(near(timer.elapsed, 1.0f), "the paused time does not count after resuming");
	test_ticks += 60 * 20;
	CTankTimer_Tick(&timer);
	CHECK(near(timer.remaining, 0.0f) && near(timer.fraction, 1.0f), "the end clamps at 0 / 1");
}

static const char spawn_text[] =
	"// a comment\r\n"
	"SPAWNLIST {\r\n"
	"NAME \"start\"\r\n"
	"SPAWNPOINT {\r\n"
	"POSITION 1 2 3\r\n"
	"ORIENTATION 0 90 180\r\n"
	"}\r\n"
	"QUICKSPAWN 4 5 6\r\n"
	"}\r\n";

static void test_spawn(void)
{
	void *spawn[1] = { NULL };
	float *point;

	test_file_name = "spawn.txt";
	test_file_text = spawn_text;
	CHECK(CTankSpawn_Initialise(spawn, NULL, (char *)"spawn.txt") == 1, "Initialise returns 1");
	CHECK(CTankSpawn_GetCount(spawn, "START") == 2, "a list is found without case; two points");
	point = (float *)CTankSpawn_GetPoint(spawn, "start", 0);
	CHECK(point != NULL && near(point[0], 4.0f) && near(point[2], 6.0f) && near(point[3], 1.0f),
		"the last point of the file is point 0");
	point = (float *)CTankSpawn_GetPoint(spawn, "start", 1);
	CHECK(point != NULL && near(point[1], 2.0f), "the SPAWNPOINT is point 1");
	CHECK(point != NULL && near(point[5], 90.0f * 0.0174532924f) && near(point[6], 3.14159274f),
		"orientations are degrees in the file and radians in the point");
	CHECK(CTankSpawn_GetPoints(spawn, "other") == NULL, "no such list");
	CTankSpawn_Free(spawn);
	CHECK(spawn[0] == NULL, "Free empties the lists");
	CHECK(CTankSpawn_Initialise(spawn, NULL, (char *)"") == 1 && spawn[0] == NULL,
		"no spawn file: no lists");
}

static const char world_text[] =
	"OBJECT {\r\n"
	"NAME \"crate\"\r\n"
	"FLAGS 1\r\n"
	"HEALTH 3\r\n"
	"POSITION 10 20 30\r\n"
	"ORIENTATION 0 400 0\r\n"
	"COLLISION \"crate.col\"\r\n"
	"}\r\n"
	"SETTINGS {\r\n"
	"LAPS 3\r\n"
	"GRAVITY -9.5\r\n"
	"}\r\n"
	"CHECKPOINT {\r\n"
	"COLLISION \"cp1\"\r\n"
	"ONEWAY 1\r\n"
	"}\r\n"
	"CHECKPOINT {\r\n"
	"COLLISION \"cp2\"\r\n"
	"POSITION 5 0 0\r\n"
	"}\r\n"
	"BOUNDARY {\r\n"
	"NAME \"edge\"\r\n"
	"}\r\n";

static void test_world(void)
{
	unsigned char world[0x20];
	unsigned char *obj;
	unsigned char *checkpoint;

	memset(world, 0, sizeof world);
	test_file_name = "objects.txt";
	test_file_text = world_text;
	CHECK(CTankWorld_Initialise(world, NULL, (char *)"objects.txt", NULL, NULL) == 1,
		"Initialise returns 1");
	CHECK(CTankWorld_GetLaps(world) == 3, "LAPS 3: a race");
	CHECK(near(CTankWorld_GetGravity(world), -9.5f), "GRAVITY");
	obj = TU_PTR(world, 0x10);
	CHECK(obj != NULL && strcmp((char *)obj + 0x70, "crate") == 0, "the object is in the list");
	CHECK(obj != NULL && TU_PTR(obj, 0x260) == NULL, "the nameless boundary was dropped");
	CHECK(obj != NULL && TU_U32(obj, 0x34) == 1 && TU_I32(obj, 0) == 3, "FLAGS and HEALTH");
	CHECK(obj != NULL && near(TU_F32(obj, 0x50), 10.0f) && near(TU_F32(obj, 0x58), 30.0f),
		"POSITION");
	CHECK(obj != NULL && near(TU_F32(obj, 0x64), 0.0f), "an orientation of 400 degrees is refused");
	CHECK(obj != NULL && TU_PTR(obj, 0x248) != NULL, "the collision mesh is loaded");
	CHECK(obj != NULL && TU_I32(obj, 8) == 1, "objects start active");
	CHECK(TU_I32(world, 4) == 2, "two checkpoints");
	checkpoint = TU_PTR(world, 0x14);
	CHECK(checkpoint != NULL && TU_I32(checkpoint, 0) == 1, "the first checkpoint of the file is first");
	checkpoint = checkpoint != NULL ? TU_PTR(checkpoint, 0x2c) : NULL;
	CHECK(checkpoint != NULL && near(TU_F32(checkpoint, 8), 5.0f) && TU_PTR(checkpoint, 0x2c) == NULL,
		"the second checkpoint follows it");
	CTankWorld_Free(world);
	CHECK(TU_PTR(world, 0x10) == NULL && TU_PTR(world, 0x14) == NULL, "Free empties the lists");
}

static void test_lights(void)
{
	struct TuLight lights[4];
	float pos[4] = { 1.0f, 2.0f, 3.0f, 1.0f };
	int index;
	int i;

	memset(lights, 0, sizeof lights);
	for (i = 0; i < 4; i++)
		lights[i].light = test_resource;
	CTankLights_AddFlash(lights, pos, -1, 0, 0);
	CHECK(lights[0].state == 6 && near(lights[0].duration, 180.0f), "a flash takes the first free light");
	CHECK(near(lights[0].pos[1], 2.0f) && near(lights[0].range, 2000.0f), "the flash's place and range");
	CHECK(CTankLights_Update(lights, 100.0f, 3) == 3, "the lights keep the game state");
	CHECK(lights[0].state == 6, "the flash lives on at 100 of 180 frames");
	CTankLights_Update(lights, 100.0f, 3);
	CHECK(lights[0].state == 0, "the flash frees its light after 180 frames");
	for (i = 0; i < 4; i++)
		lights[i].state = 2;
	index = -1;
	CTankLights_SetSpot(lights, &index, pos, 1, 2, 3);
	CHECK(index == -1, "no free light: no spot light");
	lights[2].state = 0;
	CTankLights_SetSpot(lights, &index, pos, 1, 2, 3);
	CHECK(index == 2 && lights[2].state == 4, "a spot light takes the free light");
}

static void test_boom(void)
{
	void *boom[3] = { NULL, NULL, NULL };
	float pos[4] = { 0.0f, 100.0f, 0.0f, 1.0f };
	void *node;

	CTankBoom_Add(boom, pos, 50.0f);
	node = boom[0];
	CHECK(node != NULL && TU_I32(node, 0) == 1, "a boom is active");
	CHECK(node != NULL && TU_F32(node, 0x2c) >= 0.0f && TU_F32(node, 0x2c) < 6.2832f,
		"the scorch turns by a whole degree");
	CTankBoom_Update(boom, 60.0f * 5.0f, 4);
	CHECK(boom[0] != NULL, "a boom still shows its scorch after 5 s");
	CTankBoom_Update(boom, 60.0f * 6.0f, 4);
	CHECK(boom[0] == NULL, "the boom is freed after its 10 s scorch");
}

/* a fake entity list for the entity lookups */
static unsigned char test_ents[3][0xf08];
static unsigned char test_tank_template[0x6e0];
static unsigned char test_shell_template[0x6e0];
static void *test_nodes[3][2];

static void make_entities(void *system)
{
	static const char *const names[3] = { "player1", "player2", "shell" };
	int i;

	memset(test_ents, 0, sizeof test_ents);
	memset(test_tank_template, 0, sizeof test_tank_template);
	memset(test_shell_template, 0, sizeof test_shell_template);
	TU_I32(test_tank_template, 0x1c) = 1;
	TU_I32(test_shell_template, 0x1c) = 2;
	for (i = 0; i < 3; i++) {
		void *ent = test_ents[i];

		TU_I32(ent, 0) = 10 + i;
		strcpy((char *)ent + 8, names[i]);
		TU_PTR(ent, 0x91c) = i < 2 ? (void *)test_tank_template : (void *)test_shell_template;
		TU_F32(ent, 0x9b0) = 50.0f;
		TU_F32(ent, 0xa08) = 100.0f * (float)i;
		test_nodes[i][0] = ent;
		test_nodes[i][1] = i < 2 ? (void *)test_nodes[i + 1] : NULL;
	}
	TU_PTR(system, 0) = test_nodes[0];
}

static void test_entities(void)
{
	unsigned char system[0xf8];
	float origin[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int id;

	memset(system, 0, sizeof system);
	make_entities(system);
	CHECK(CTankEntities_FindId(system, "PLAYER2") == 11, "a tank is found by name without case");
	CHECK(CTankEntities_FindId(system, "nobody") == 0, "an unknown name gives id 0");
	CHECK(CTankEntities_Get(system, 12) == test_ents[2] && CTankEntities_Get(system, 99) == NULL,
		"entities by id");
	CHECK(CTankEntities_CountNear(system, origin, 60.0f) == 2,
		"tanks within the radius plus their own radius count, shells do not");
	CTankEntities_SetLead(system, 10, 0.5f);
	CHECK(near(TU_F32(test_ents[0], 0x9bc), 0.7f), "a leader is held back to 0.7 at most");
	CTankEntities_SetLead(system, 10, -1.0f);
	CHECK(near(TU_F32(test_ents[0], 0x9bc), 1.0f), "a tank behind runs at full speed (1)");
	TU_F32(test_ents[1], 0x994) = 0.5f;
	CHECK(CTankEntities_GetProgress(system, 11) == 240, "progress 0.5 is 240");
	TU_F32(test_ents[1], 0x994) = -1.0f;
	CHECK(CTankEntities_GetProgress(system, 11) == 0, "progress is never negative");
	CHECK(near(CTankEntities_WrapAngle(7.0f), 7.0f - 6.28318548f), "angles wrap below 2 pi");
	CHECK(near(CTankEntities_WrapAngle(-1.0f), 6.28318548f - 1.0f), "angles wrap above 0");
	id = TU_VAR(int, 0x00315398);
	CTankEntities_InitEntity(test_ents[2]);
	CHECK(TU_I32(test_ents[2], 0) == id + 1 && TU_VAR(int, 0x00315398) == id + 1,
		"a new entity gets the next id");
	CHECK(TU_I32(test_ents[2], 0x88) == 1 && TU_I32(test_ents[2], 0x8c) == 1,
		"a new entity starts in the global AI state");
	CHECK(near(TU_F32(test_ents[2], 0x98c), 100.0f) && near(TU_F32(test_ents[2], 0x9b0), -1.0f)
		&& ((char *)test_ents[2])[8] == 0, "the defaults: mass 100, radius -1, no name");
}

static const char camera_text[] =
	"CAMERA {\r\n"
	"NAME \"INTO CAM\"\r\n"
	"END_TIME 2\r\n"
	"LINK_TIME 0.5\r\n"
	"FROM_LOC 1 2 3\r\n"
	"}\r\n"
	"CAMERA {\r\n"
	"NAME \"FPCAM1\"\r\n"
	"}\r\n";

static void test_camera(void)
{
	unsigned char camera[0xf0];
	struct TuTextReader reader;
	unsigned char *shot;

	memset(camera, 0, sizeof camera);
	BCarTextReader_Init((struct BCarTextReader *)&reader, (char *)camera_text,
		(int)strlen(camera_text), "camera.txt");
	CTankCamera_LoadCameraInfo(camera, &reader);
	shot = TU_PTR(camera, 0x90);
	CHECK(shot != NULL && strcmp((char *)shot, "INTO CAM") == 0, "the first shot of the file is first");
	CHECK(shot != NULL && near(TU_F32(shot, 0x140), 120.0f) && near(TU_F32(shot, 0x144), 30.0f),
		"END_TIME and LINK_TIME: seconds become frames");
	CHECK(shot != NULL && near(TU_F32(shot, 0x10c), 1.0f) && near(TU_F32(shot, 0x114), 3.0f),
		"FROM_LOC");
	shot = shot != NULL ? TU_PTR(shot, 0x148) : NULL;
	CHECK(shot != NULL && TU_PTR(camera, 0x94) == shot, "the FPCAM shot is the single player's");
	CHECK(shot != NULL && near(TU_F32(shot, 0x120), 30.0f) && near(TU_F32(shot, 0x144), -1.0f)
		&& near(TU_F32(shot, 0x134), 0.01f), "a new shot's defaults");
}

extern int DAT_002baebc;
extern int DAT_002baef4[];

static void test_players(void)
{
	unsigned char players[0xa4];
	unsigned char records[4][0x48];
	int i;

	memset(players, 0, sizeof players);
	memset(records, 0, sizeof records);
	for (i = 0; i < 4; i++)
		TU_I32(records[i], 0xc) = 20 + i;
	TU_PTR(players, 4) = records;
	TU_I32(records[2], 8) = 1;
	CHECK(CTankPlayers_SameTeam(players, 21, 21) == 1, "a tank is on its own team");
	DAT_002baebc = 0;
	CHECK(CTankPlayers_SameTeam(players, 20, 21) == 0, "without teams two tanks are enemies");
	DAT_002baebc = 1;
	DAT_002baef4[0] = 1;
	DAT_002baef4[10] = 1;
	DAT_002baef4[20] = 2;
	CHECK(CTankPlayers_SameTeam(players, 20, 21) == 1, "players 1 and 2 on team 1");
	CHECK(CTankPlayers_SameTeam(players, 20, 22) == 0, "player 3 on team 2");
	DAT_002baebc = 0;
	CHECK(CTankPlayers_IsWinner(players, 22) == 1 && CTankPlayers_IsWinner(players, 21) == 0,
		"the winner flag of the tank's player");
	CHECK(CTankPlayers_IsWinner(players, 99) == 0, "a tank of no player is no winner");
}

int main(void)
{
	test_data();
	test_timer();
	test_spawn();
	test_world();
	test_lights();
	test_boom();
	test_entities();
	test_camera();
	test_players();
	fprintf(stderr, "tankedup_a tests: %d checks, %d failures\n", checks, failures);
	return failures != 0;
}

#endif
