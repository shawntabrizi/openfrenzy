/*
TankedUp/CTank_Spawn.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Spawn.cpp

CTank_Spawn (CTank_Game +0xe30, "spawnSystem"): named lists of spawn points, read from the
variation's SPAWNFILE:

  SPAWNLIST {
    NAME <name>
    SPAWNPOINT {  POSITION x y z  ORIENTATION ax ay az (degrees)  }
    QUICKSPAWN x y z
  }

0x001c41a0 and 0x001c44b0 are tagged with this file; the lookups before them
(0x001c40d0-0x001c4160) and Initialise (0x001c4770, "CTank_Spawn::Initialise") are placed
here too.
*/

#include "tankedup_a.h"

/* a spawn point (0x28 bytes) */
struct TuSpawnPoint {
	float pos[4];                /* +0x00 */
	float orient[4];             /* +0x10 radians */
	int unknown20;               /* +0x20 */
	struct TuSpawnPoint *next;   /* +0x24 */
};
OF_ASSERT_SIZE(struct TuSpawnPoint, 0x28);

/* a named spawn list (0x4c bytes) */
struct TuSpawnList {
	char name[0x40];             /* +0x00 */
	int count;                   /* +0x40 */
	struct TuSpawnPoint *points; /* +0x44 */
	struct TuSpawnList *next;    /* +0x48 */
};
OF_ASSERT_SIZE(struct TuSpawnList, 0x4c);

/* the spawn list called `name`, or NULL */
static struct TuSpawnList *spawn_find(void *self, const char *name)
{
	struct TuSpawnList *list;

	for (list = *(struct TuSpawnList **)self; list != NULL; list = list->next)
		if (_stricmp(list->name, name) == 0)
			return list;
	return NULL;
}

/* 0x001c40d0  TankedUp/CTank_Spawn.cpp (inferred)
The first spawn point of the list `name`, or NULL. */
void *CTankSpawn_GetPoints(void *self, const char *name)
{
	struct TuSpawnList *list = spawn_find(self, name);

	return list != NULL ? list->points : NULL;
}

/* 0x001c4110  TankedUp/CTank_Spawn.cpp (inferred)
Spawn point `index` of the list `name` (NULL when the list is shorter). */
void *CTankSpawn_GetPoint(void *self, const char *name, int index)
{
	struct TuSpawnList *list = spawn_find(self, name);
	struct TuSpawnPoint *point;

	if (list == NULL)
		return NULL;
	point = list->points;
	if (index == 0)
		return point;
	do {
		if (point != NULL)
			point = point->next;
		index--;
	} while (index != 0);
	return point;
}

/* 0x001c4160  TankedUp/CTank_Spawn.cpp (inferred)
The number of spawn points of the list `name` (0 when there is none). */
int CTankSpawn_GetCount(void *self, const char *name)
{
	struct TuSpawnList *list = spawn_find(self, name);

	return list != NULL ? list->count : 0;
}

/* 0x001c41a0  TankedUp/CTank_Spawn.cpp
Reads a "SPAWNPOINT {" block up to "}" and adds the point at the head of *list. Returns 0
when there is no memory. (`this` is not used.) */
int CTankSpawn_LoadSpawnPoint(void *self, struct TuTextReader *reader, struct TuSpawnPoint **list)
{
	struct TuSpawnPoint *point;
	char line[512];
	char command[512];
	float x, y, z;
	float ax, ay, az;
	char *text;
	int result;

	(void)self;
	point = (struct TuSpawnPoint *)TU_NEW(0x28, TU_FILE_SPAWN, 0xe3);
	if (point == NULL) {
		TU_DPRINTF("CTank_Spawn::LoadSpawnPoint - could not get memory for new spawnnode\n");
		return 0;
	}
	point->pos[0] = 0.0f;
	point->pos[1] = 0.0f;
	point->pos[2] = 0.0f;
	point->pos[3] = 1.0f;
	point->orient[0] = 0.0f;
	point->orient[1] = 0.0f;
	point->orient[2] = 0.0f;
	point->orient[3] = 1.0f;
	point->next = NULL;
	line[0] = 0;
	text = line;
	while (reader->cur < reader->end) {
		result = 0;
		if (_stricmp(text, "}") == 0)
			break;
		text = TUB_Reader_ReadLine(reader, line);
		if (strncmp(text, "//", 2) == 0)
			continue;
		if (_strnicmp(text, "POSITION ", 9) == 0) {
			result = 2;
			if (sscanf(text, "%*s%f%f%f", &x, &y, &z) == 3) {
				point->pos[0] = x;
				point->pos[1] = y;
				point->pos[2] = z;
				point->pos[3] = 1.0f;
				result = 1;
			}
		}
		if (_strnicmp(text, "ORIENTATION ", 0xc) == 0) {
			result = 2;
			if (sscanf(text, "%*s%f%f%f", &ax, &ay, &az) == 3) {
				point->orient[3] = 1.0f;
				result = 1;
				point->orient[0] = ax * 0.0174532924f;
				point->orient[1] = ay * 0.0174532924f;
				point->orient[2] = az * 0.0174532924f;
			}
		}
		if (_stricmp(text, "}") != 0) {
			sscanf(text, "%s", command);
			if (result == 0)
				TU_DPRINTF("CTank_Spawn::LoadSpawnPoint(\"%s\", %d): Invalid Command \"%s\"\n",
					reader->name, reader->line, command);
			else if (result == 2)
				TU_DPRINTF("CTank_Spawn::LoadSpawnPoint(\"%s\", %d): Bad use of Command \"%s\"\n",
					reader->name, reader->line, command);
		}
	}
	point->next = *list;
	*list = point;
	return 1;
}

/* 0x001c43c0  TankedUp/CTank_Spawn.cpp (inferred)
Frees every spawn list and its points. */
void CTankSpawn_Free(void *self)
{
	struct TuSpawnList **first = (struct TuSpawnList **)self;

	while (*first != NULL) {
		struct TuSpawnList *list = *first;

		*first = list->next;
		while (list->points != NULL) {
			struct TuSpawnPoint *point = list->points;

			list->points = point->next;
			TU_DELETE(point);
		}
		TU_DELETE(list);
	}
	*first = NULL;
}

/* 0x001c4420  TankedUp/CTank_Spawn.cpp (inferred)
A random spawn point of the list `name` (the C library's rand()); `out` is (0, 0, 0, 1)
when there is no such list. */
void CTankSpawn_GetRandomPoint(void *self, const char *name, float *out)
{
	struct TuSpawnList *list;
	struct TuSpawnPoint *point;
	int steps;

	out[0] = 0.0f;
	out[1] = 0.0f;
	out[2] = 0.0f;
	out[3] = 1.0f;
	list = spawn_find(self, name);
	if (list == NULL)
		return;
	steps = tu_rand();
	point = list->points;
	for (steps = steps % list->count; steps != 0; steps--)
		if (point->next != NULL)
			point = point->next;
	out[0] = point->pos[0];
	out[1] = point->pos[1];
	out[2] = point->pos[2];
	out[3] = point->pos[3];
}

/* 0x001c44b0  TankedUp/CTank_Spawn.cpp
Reads every "SPAWNLIST {" block of the spawn file. */
void CTankSpawn_LoadSpawnInfo(void *self, struct TuTextReader *reader)
{
	struct TuSpawnList **first = (struct TuSpawnList **)self;
	char command[512];
	char line[512];
	float x, y, z;
	char *text;
	int result;

	if (reader->end <= reader->cur)
		return;
	do {
		text = TUB_Reader_ReadLine(reader, line);
		if (strncmp(text, "//", 2) != 0 && _stricmp(text, "SPAWNLIST {") == 0) {
			struct TuSpawnList *list = (struct TuSpawnList *)TU_NEW(0x4c, TU_FILE_SPAWN, 0x78);

			if (list == NULL) {
				TU_DPRINTF("CTank_Spawn::LoadSpawnInfo - could not get memory for new spawnlist\n");
				return;
			}
			list->name[0] = 0;
			list->count = 0;
			list->points = NULL;
			list->next = NULL;
			while (reader->cur < reader->end) {
				if (_stricmp(text, "}") == 0)
					break;
				text = TUB_Reader_ReadLine(reader, line);
				if (strncmp(text, "//", 2) == 0)
					continue;
				result = 0;
				if (_strnicmp(text, "NAME ", 5) == 0) {
					result = 2;
					if (BCarText_GetName(text, list->name, 0x3f) != 0)
						result = 1;
				}
				if (_strnicmp(text, "SPAWNPOINT {", 0xc) == 0) {
					result = 2;
					if (CTankSpawn_LoadSpawnPoint(self, reader, &list->points) != 0) {
						result = 1;
						list->count++;
					}
					/* the point's "}" must not end the list */
					*text = 0;
				}
				if (_strnicmp(text, "QUICKSPAWN ", 0xb) == 0) {
					result = 2;
					if (sscanf(text, "%*s%f%f%f", &x, &y, &z) == 3) {
						struct TuSpawnPoint *point =
							(struct TuSpawnPoint *)TU_NEW(0x28, TU_FILE_SPAWN, 0xae);

						if (point == NULL) {
							TU_DPRINTF("CTank_Spawn::LoadSpawnInfo - could not get memory for new "
								"'quick' spawnnode\n");
						} else {
							point->pos[0] = x;
							point->pos[1] = y;
							point->pos[3] = 1.0f;
							point->pos[2] = z;
							point->orient[0] = 0.0f;
							point->orient[1] = 0.0f;
							point->orient[2] = 0.0f;
							point->orient[3] = 1.0f;
							point->next = list->points;
							list->points = point;
							result = 1;
							list->count++;
						}
					}
				}
				if (_stricmp(text, "}") != 0) {
					sscanf(text, "%s", command);
					if (result == 0)
						TU_DPRINTF("CTank_Spawn::LoadSpawnInfo(\"%s\", %d): Invalid Command \"%s\"\n",
							reader->name, reader->line, command);
					else if (result == 2)
						TU_DPRINTF("CTank_Spawn::LoadSpawnInfo(\"%s\", %d): Bad use of Command \"%s\"\n",
							reader->name, reader->line, command);
				}
			}
			list->next = *first;
			*first = list;
		}
	} while (reader->cur < reader->end);
}

/* 0x001c4770  TankedUp/CTank_Spawn.cpp (inferred: "CTank_Spawn::Initialise")
Loads the spawn file `file` from `package` (nothing when the name is empty). Always
returns 1. */
int CTankSpawn_Initialise(void *self, void *package, char *file)
{
	struct TuTextReader reader;
	unsigned int size;
	void *buffer;

	TUB_Reader_Ctor(&reader);
	*(struct TuSpawnList **)self = NULL;
	if (file[0] != 0) {
		buffer = bkLoadFile((struct bkPackage *)package, file, NULL, &size, NULL, 0);
		if (buffer == NULL) {
			TU_DPRINTF("CTank_Spawn::Initialise - Configuration file [%s] not found\n", file);
		} else {
			BCarTextReader_Init((struct BCarTextReader *)&reader, (char *)buffer, (int)size, file);
			CTankSpawn_LoadSpawnInfo(self, &reader);
		}
	}
	TUB_Reader_Dtor(&reader);
	return 1;
}
