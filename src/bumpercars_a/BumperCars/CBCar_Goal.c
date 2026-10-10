/*
BumperCars/CBCar_Goal.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Goal.cpp   (module bumpercars_a)

The goals (CBCar_Game + 0x3310, vtable 0x00264720): quadrilaterals in XZ read from the
variation's GOALINFO file ("GOAL {" blocks of "POINT n x z"). An entity inside one scores
(bumpercars_b reads its +0x898) and a crowd sound plays. 0x00091b60 is tagged with this
file; the gap functions 0x000919e0-0x00091e30 are this class's methods.
*/

#include "bumpercars_a.h"
#include <strings.h>

static const char s_GoalFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Goal.cpp";

/* a goal: four corners (x, z) and the next goal (0x24 bytes) */
struct BCarGoalArea {
	float corner[4][2];         /* 0x00 */
	struct BCarGoalArea *next;  /* 0x20 */
};
OF_ASSERT_SIZE(struct BCarGoalArea, 0x24);

/* the goals (0x10 bytes in the game object) */
struct CBCar_Goal {
	const void *const *vtable;  /* 0x0 0x00264720 */
	struct BCarGoalArea *first; /* 0x4 */
	int unknown8[2];            /* 0x8 */
};

#define READ_LINE(r, buf) BCA_FN(FUN_001386c0, char *, (void *, char *))((r), (buf))
#define NEXT_LINE(r) BCA_FN(FUN_00138720, void, (void *))(r)

/* 0x00264720  CBCar_Goal vtable */
const void *const vtable_00264720[1] = {
	(const void *)FUN_00091b40, /* [0] scalar deleting destructor */
};

/* 0x000919e0  (gap): CBCar_Goal::CBCar_Goal */
void *CBCar_Goal_Ctor(struct CBCar_Goal *self)
{
	self->vtable = vtable_00264720;
	self->first = NULL;
	return self;
}

/* 0x000919f0  (gap): CBCar_Goal::~CBCar_Goal */
void CBCar_Goal_Dtor(struct CBCar_Goal *self)
{
	self->vtable = vtable_00264720;
}

/* 0x00091a00  (gap): the goal that contains pos (two triangles of each quadrilateral), or NULL */
struct BCarGoalArea *CBCar_Goal_Find(struct CBCar_Goal *self, float *pos)
{
	struct BCarGoalArea *goal;
	float a[4], b[4], c[4], d[4];

	/* y and w are not set in the original (the test uses x and z) */
	memset(a, 0, sizeof a);
	memset(b, 0, sizeof b);
	memset(c, 0, sizeof c);
	memset(d, 0, sizeof d);
	for (goal = self->first; goal != NULL; goal = goal->next) {
		a[0] = goal->corner[0][0];
		a[2] = goal->corner[0][1];
		b[0] = goal->corner[1][0];
		b[2] = goal->corner[1][1];
		c[0] = goal->corner[2][0];
		c[2] = goal->corner[2][1];
		d[0] = goal->corner[3][0];
		d[2] = goal->corner[3][1];
		if (CLMath_PointInTriangleXZ(pos, a, b, c) != 0 || CLMath_PointInTriangleXZ(pos, a, c, d) != 0)
			return goal;
	}
	return NULL;
}

/* 0x00091aa0  (gap): the distance from pos to the nearest goal's centre (bumpercars_b
0x000a57a0), or -1 */
float CBCar_Goal_Distance(struct CBCar_Goal *self, float *pos)
{
	struct BCarGoalArea *goal;
	float centre[4], best = 1e6f, d;

	goal = self->first;
	if (goal == NULL)
		return -1.0f;
	do {
		BCA_FN(FUN_000a57a0, void, (float *, float, float, float, float, float, float, float, float))(centre,
			goal->corner[0][0], goal->corner[0][1], goal->corner[1][0], goal->corner[1][1], goal->corner[2][0],
			goal->corner[2][1], goal->corner[3][0], goal->corner[3][1]);
		d = bmVectorDistance(centre, pos);
		if (d < best)
			best = d;
		goal = goal->next;
	} while (goal != NULL);
	if (best < 999999.0f)
		return best;
	return -1.0f;
}

/* 0x00091b30  (gap): walks the goals (nothing else) */
void CBCar_Goal_Pause(struct CBCar_Goal *self)
{
	struct BCarGoalArea *goal;

	for (goal = self->first; goal != NULL; goal = goal->next) {
	}
}

/* 0x00091b40  (gap): CBCar_Goal scalar deleting destructor */
struct CBCar_Goal *CBCar_Goal_Destroy(struct CBCar_Goal *self, byte flags)
{
	self->vtable = vtable_00264720;
	if ((flags & 1) != 0)
		BCA_DELETE(self);
	return self;
}

/* 0x00091b60  BumperCars/CBCar_Goal.cpp: CBCar_Goal::LoadGoals */
void CBCar_Goal_LoadGoals(struct CBCar_Goal *self, struct BCarTextReader *reader)
{
	char line[512], word[512];
	char *text;
	struct BCarGoalArea *goal;
	int status, n;
	float x, z;

	while (reader->cursor < reader->end) {
		text = READ_LINE(reader, line);
		NEXT_LINE(reader);
		if (strncmp(text, "//", 2) == 0 || strcasecmp(text, "GOAL {") != 0)
			continue;
		goal = (struct BCarGoalArea *)BCA_NEW(0x24, s_GoalFile, 0x5a, 0x1001);
		if (goal == NULL)
			return;
		memset(goal->corner, 0, sizeof goal->corner);
		while (reader->cursor < reader->end) {
			status = 0;
			if (strcasecmp(text, "}") == 0)
				break;
			text = READ_LINE(reader, line);
			NEXT_LINE(reader);
			if (strncmp(text, "//", 2) == 0)
				continue;
			/* the original compares 5 characters of "POINT " */
			if (strncasecmp(text, "POINT ", 5) == 0) {
				status = 2;
				if (sscanf(text, "%*s%d%f%f", &n, &x, &z) == 3 && n > 0 && n < 5) {
					status = 1;
					goal->corner[n - 1][0] = x;
					goal->corner[n - 1][1] = z;
				}
			}
			if (strcasecmp(text, "}") != 0) {
				sscanf(text, "%s", word);
				if (status == 0)
					BCA_Printf("%s(%d) : LoadGoals(\"%s\", %d): Invalid Command \"%s\"\n", s_GoalFile, 0x86,
						reader->name, reader->line, word);
				else if (status == 2)
					BCA_Printf("%s(%d) : LoadGoals(\"%s\", %d): Bad use of Command \"%s\" (%s)\n", s_GoalFile,
						0x88, reader->name, reader->line, word, text);
			}
		}
		goal->next = self->first;
		self->first = goal;
	}
}

/* 0x00091d70  (gap): frees the goals */
void CBCar_Goal_Shutdown(struct CBCar_Goal *self)
{
	struct BCarGoalArea *goal;

	while (self->first != NULL) {
		goal = self->first;
		self->first = goal->next;
		BCA_DELETE(goal);
	}
	self->first = NULL;
}

/* 0x00091db0  (gap): marks the entities inside a goal (+0x898) and cheers for each */
void CBCar_Goal_Update(struct CBCar_Goal *self, int playing)
{
	unsigned char *e;
	struct BCarGoalArea *goal;

	if (playing == 0)
		return;
	for (e = BCG(0x330c, unsigned char *); e != NULL; e = BCA_AT(e, 0x8b8, unsigned char *)) {
		goal = CBCar_Goal_Find(self, (float *)(e + 0x50));
		BCA_AT(e, 0x898, struct BCarGoalArea *) = goal;
		if (goal != NULL)
			FUN_00092120(g_BCarGame->sound, bca_rand() < 20000 ? 0x19 : 0x1a, 0xff, 0, -1);
	}
}

/* 0x00091e30  (gap): CBCar_Goal::Initialise: loads the goal file. Returns 1 when loaded. */
int CBCar_Goal_Initialise(struct CBCar_Goal *self, struct bkPackage *package, char *file)
{
	struct BCarTextReader reader;
	uint size;
	void *text;

	BCA_FN(FUN_000a5be0, void, (void *))(&reader);
	text = bkLoadFile(package, file, NULL, &size, NULL, 0);
	if (text != NULL) {
		BCA_FN(FUN_000a5c00, void, (void *, void *, uint, const char *))(&reader, text, size, file);
		CBCar_Goal_LoadGoals(self, &reader);
	}
	BCA_FN(FUN_000a5bf0, void, (void *))(&reader);
	return text != NULL;
}
