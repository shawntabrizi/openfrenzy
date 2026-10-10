/*
BumperCars/CBCar_World.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_World.cpp

The arena of BumperCars: the world file (sets, sky, collision mesh, scenic actor, texture
animations, physics settings, spawn points, flames and waypoints files, the lead marker),
the score file (the points of every event), the spawn points, the ground height, and the
drawing of the sets with the floor reflection.

0x0009e690, 0x0009ee20, 0x0009f9b0 and 0x000a0620 are tagged with CBCar_World.cpp; the
functions between them are bracketed; 0x0009e3b0-0x0009e610 before them are gap functions
of the same globals (file inferred).
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* 0x0009e3b0  (gap; file inferred)
World_Reset: clears the names and resources of the world and sets the default settings. */
void BCarWorld_Reset(void)
{
	int i;

	g_WorldSkyName[0] = '\0';
	g_WorldSetName[0] = '\0';
	g_WorldSet2Name[0] = '\0';
	g_WorldAlphaSetName[0] = '\0';
	g_WorldAlphaSet2Name[0] = '\0';
	g_WorldCollisionName[0] = '\0';
	g_WorldSceneryName[0] = '\0';
	g_WorldScenicsName[0] = '\0';
	g_WorldSceneAnimName[0] = '\0';
	g_WorldSceneAnimLoopName[0] = '\0';
	for (i = 0; i < BCB_NUM_TEXANIMS; i++)
	{
		g_WorldTexAnimName(i)[0] = '\0';
		g_WorldTexAnims[i] = NULL;
	}
	g_WorldSky = NULL;
	g_WorldSet = NULL;
	g_WorldSet2 = NULL;
	g_WorldAlphaSet = NULL;
	g_WorldAlphaSet2 = NULL;
	g_WorldCollision = NULL;
	g_WorldScenery = NULL;
	g_WorldScenicActor = NULL;
	g_WorldScenicInst = NULL;
	g_WorldScenicAnim = NULL;
	g_WorldScenicAnimLoop = NULL;
	g_ReflectBrightness = 0x50;
	g_WorldSpawns = NULL;
	g_WorldGravity[0] = 0.0f;
	g_WorldGravity[1] = 0.0f;
	g_WorldGravity[2] = 0.0f;
	g_EntityRot = 0.06283186f;                                  /* 0x3d80adfd */
	g_EntitySpeed = 3.0f;
	g_EntityBounce = 1.0f;
	g_KillHeight = -500.0f;
	g_WorldFriction = 0.0f;
	g_WorldFrozen = 0;
	g_WorldRestitution = 0.0f;
	g_MaxSpeed = 20.0f;
	g_HitTimeout = 0.0f;
	g_BrakeHitTimeout = 0.0f;
	g_RestartWait = 0.0f;
	g_BrakeValue = 0.0f;
	g_BoostValue = 0.0f;
	g_BoostLife = 0.5f;
	g_BoostRecharge = 0.0f;
	g_BoostLimit = -1;
	g_EntCollisionVel = 0.0f;
	g_DisableVel = -1.0f;
	g_WallCollisionVel = 0.0f;
	g_Laps = -1;
	g_Lives = -1;
	g_EntityCollisionBias = 0.5f;
	g_WorldEdgeFriction = 0.0f;
	g_MaxVertVelocity = 2.1474836e+09f;                        /* 0x4f000000 */
	g_GainRadius = 0.0f;
	g_LoseRadius = 2.1474836e+09f;
	BCB_VAR(int, 0x002efa8c) = 1;
	g_WpLagMaxOffset = 0;
	g_WpLagSpeedOffset = 0.0f;
	g_FlatspotAngle = 0.0f;
}

/* 0x0009e5a0  (gap; file inferred)
MAXSPEED (x1.5 with the turbo cheat). */
float BCarWorld_GetMaxSpeed(void)
{
	if (DAT_00283e58 != 0)
		return g_MaxSpeed * 1.5f;                              /* 0x002538a0 */
	return g_MaxSpeed;
}

/* 0x0009e5c0  (gap; file inferred)
BOOSTVALUE (x1.5 with the turbo cheat). */
float BCarWorld_GetBoostValue(void)
{
	if (DAT_00283e58 != 0)
		return g_BoostValue * 1.5f;
	return g_BoostValue;
}

/* 0x0009e5e0  (gap; file inferred)
ENTITYSPEED (x1.5 with the turbo cheat). */
float BCarWorld_GetEntitySpeed(void)
{
	if (DAT_00283e58 != 0)
		return g_EntitySpeed * 1.5f;
	return g_EntitySpeed;
}

/* 0x0009e600  (gap; file inferred)
WORLDRESTITUTION. */
float BCarWorld_GetRestitution(void)
{
	return g_WorldRestitution;
}

/* 0x0009e610  (gap; file inferred)
No points for any event. */
void BCarWorld_ResetScores(void)
{
	unsigned int address;

	for (address = 0x002efa14; address <= 0x002efa44; address += 4)
		g_Score(address) = 0;
	for (address = 0x002efa54; address <= 0x002efa80; address += 4)
		g_Score(address) = 0;
}

/* a keyword of a configuration line: when `line` starts with the first `length` characters
of `keyword`, reads one value with `format` into `out` and returns 1 (read) or 2 (bad);
otherwise returns `status` */
static int bcb_keyword(const char *line, const char *keyword, int length, const char *format,
	void *out, int status)
{
	if (_strnicmp(line, keyword, (size_t)length) != 0)
		return status;
	if (sscanf(line, format, out) == 1)
		return 1;
	return 2;
}

/* report a line of a world or score file that was not understood (status 0) or not used
right (status 2) */
static void bcb_world_bad_line(struct BCarTextReader *reader, char *line, char *word, int status,
	const char *what, int lineInvalid, int lineBad)
{
	sscanf(line, "%s", word);                                  /* 0x002576ec */
	if (status == 0)
		bkPrintf("%s(%d) : %s(\"%s\", %d): Invalid Command \"%s\"\n", BCB_SRC_WORLD, lineInvalid,
			what, reader->name, reader->line, word);
	else if (status == 2)
		bkPrintf("%s(%d) : %s(\"%s\", %d): Bad use of Command \"%s\" (%s)\n", BCB_SRC_WORLD,
			lineBad, what, reader->name, reader->line, word, line);
}

/* the keywords of the score file in the original order, each with the length compared
(LEAD_KEEPER_GAIN is compared on 12 characters only, as in the original) */
static const struct {
	const char *keyword;
	int length;
	unsigned int address;
} bcb_score_keywords[] = {
	{ "SELF_GOAL ", 10, 0x002efa14 },
	{ "OTHER_GOAL ", 0xb, 0x002efa18 },
	{ "PUCK_GOAL ", 10, 0x002efa1c },
	{ "LEAD_SELF_GOAL ", 0xf, 0x002efa54 },
	{ "LEAD_OTHER_GOAL ", 0x10, 0x002efa58 },
	{ "OTHER_HIT_FRONT ", 0x10, 0x002efa20 },
	{ "OTHER_HIT_SIDE ", 0xf, 0x002efa24 },
	{ "OTHER_HIT_BACK ", 0xf, 0x002efa28 },
	{ "SELF_HIT_FRONT ", 0xf, 0x002efa2c },
	{ "SELF_HIT_SIDE ", 0xe, 0x002efa30 },
	{ "SELF_HIT_BACK ", 0xe, 0x002efa34 },
	{ "GATE ", 5, 0x002efa44 },
	{ "LEAD_OTHER_HIT_FRONT ", 0x15, 0x002efa5c },
	{ "LEAD_OTHER_HIT_SIDE ", 0x14, 0x002efa60 },
	{ "LEAD_OTHER_HIT_BACK ", 0x14, 0x002efa64 },
	{ "LEAD_SELF_HIT_FRONT ", 0x14, 0x002efa68 },
	{ "LEAD_SELF_HIT_SIDE ", 0x13, 0x002efa6c },
	{ "LEAD_SELF_HIT_BACK ", 0x13, 0x002efa70 },
	{ "KING_GAIN ", 10, 0x002efa38 },
	{ "KING_LOSE ", 10, 0x002efa3c },
	{ "LEAD_KING_GAIN ", 0xf, 0x002efa74 },
	{ "LEAD_KING_LOSE ", 0xf, 0x002efa78 },
	{ "KEEPER_GAIN ", 0xc, 0x002efa40 },
	{ "LEAD_KEEPER_GAIN ", 0xc, 0x002efa7c },
	{ "LEAD_GATE ", 10, 0x002efa80 },
	{ "SELF_KILL ", 10, 0x002efa48 },
	{ "OTHER_KILL ", 0xb, 0x002efa4c },
	{ "PUCK_KILL ", 10, 0x002efa50 },
	{ "LEAD_SELF_KILL ", 0xf, 0x002efa84 },
	{ "LEAD_OTHER_KILL ", 0x10, 0x002efa88 },
};

/* 0x0009e690  BumperCars/CBCar_World.cpp
World_LoadScoreInformation: reads the points of every event from the score file. */
void BCarWorld_LoadScoreInformation(struct BCarTextReader *reader)
{
	char *line;
	int status;
	unsigned int i;
	char word[512];
	char buffer[512];

	BCarWorld_ResetScores();
	while (reader->cursor < reader->end)
	{
		line = BCarText_ReadLine(reader, buffer);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0)
			continue;
		status = 0;
		for (i = 0; i < sizeof bcb_score_keywords / sizeof bcb_score_keywords[0]; i++)
			status = bcb_keyword(line, bcb_score_keywords[i].keyword, bcb_score_keywords[i].length,
				"%*s%d", BCB_ADDR(bcb_score_keywords[i].address), status);
		bcb_world_bad_line(reader, line, word, status, "World_LoadScoreInformation", 0x4e5, 0x4e7);
	}
}

/* 0x0009ed60  BumperCars/CBCar_World.cpp (bracketed)
Pauses the flames (the game is paused). */
void BCarWorld_Pause(void)
{
	if (g_WorldLoaded != 0)
		BCarFlame_Pause(BCB_FLAME);
}

/* 0x0009ed80  BumperCars/CBCar_World.cpp (bracketed) */
void BCarWorld_Resume(void)
{
	if (g_WorldLoaded != 0)
		BCarFlame_Resume(BCB_FLAME);
}

/* 0x0009eda0  BumperCars/CBCar_World.cpp (bracketed)
World_Shutdown: deletes the spawn points, the lead marker, the scenic actor and the flames. */
void BCarWorld_Shutdown(void)
{
	struct BCarSpawn *spawn = g_WorldSpawns;

	while (spawn != NULL)
	{
		g_WorldSpawns = spawn->next;
		BCB_DELETE(spawn);
		spawn = g_WorldSpawns;
	}
	g_WorldSpawns = NULL;
	if (g_LeadInst != NULL)
	{
		baDeleteActorInstance(g_LeadInst);
		g_LeadInst = NULL;
	}
	if (g_WorldScenicInst != NULL)
	{
		baDeleteActorInstance(g_WorldScenicInst);
		g_WorldScenicInst = NULL;
	}
	BCarFlame_Delete(BCB_FLAME);
	g_WorldLoaded = 0;
}

/* 0x0009ee20  BumperCars/CBCar_World.cpp
World_LoadSpawnInfo: reads the SPAWNPOINT { } blocks (POSITION, ANGLE) of the spawn file. */
void BCarWorld_LoadSpawnInfo(struct BCarTextReader *reader)
{
	char *line;
	int status;
	struct BCarSpawn *spawn;
	float angle;
	char word[512];
	char buffer[512];

	while (reader->cursor < reader->end)
	{
		line = BCarText_ReadLine(reader, buffer);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0 || _stricmp(line, "SPAWNPOINT {") != 0)
			continue;
		spawn = (struct BCarSpawn *)BCB_NEW(0x18, BCB_SRC_WORLD, 0x36c);
		if (spawn == NULL)
			return;
		while (reader->cursor < reader->end)
		{
			if (_stricmp(line, "}") == 0)                       /* 0x00263ccc */
				break;
			line = BCarText_ReadLine(reader, buffer);
			BCarText_NextLine(reader);
			if (strncmp(line, "//", 2) == 0)
				continue;
			status = 0;
			if (_strnicmp(line, "POSITION ", 9) == 0)
			{
				status = 2;
				if (sscanf(line, "%*s%f%f%f", &spawn->pos[0], &spawn->pos[1], &spawn->pos[2]) == 3)
					status = 1;
			}
			if (_strnicmp(line, "ANGLE ", 6) == 0)
			{
				status = 2;
				if (sscanf(line, "%*s%f", &angle) == 1)
				{
					spawn->angle = angle * 0.017453292f;           /* 0x002640f8 */
					status = 1;
				}
			}
			if (_stricmp(line, "}") != 0)
				bcb_world_bad_line(reader, line, word, status, "World_LoadSpawnInfo", 0x397, 0x399);
		}
		spawn->next = g_WorldSpawns;
		g_WorldSpawns = spawn;
	}
}

/* 0x0009f030  BumperCars/CBCar_World.cpp (bracketed)
The spawn point for `entity`: of the spawn points that are not in the flames, the one with
the largest (sum of the distances to the entities) / (largest - smallest distance), the
entities measured at height 0. NULL when there is none. */
struct BCarSpawn *BCarWorld_FindSpawn(struct BCarEntity *entity)
{
	struct BCarSpawn *spawn;
	struct BCarSpawn *best = NULL;
	struct BCarEntity *other;
	float bestScore = 0.0f;
	float sum;
	float nearest;
	float farthest;
	float distance;
	float a[4];
	float b[4];

	if (g_WorldSpawns == NULL)
		return NULL;
	for (spawn = g_WorldSpawns; spawn != NULL; spawn = spawn->next)
	{
		if (BCarFlame_Inside(BCB_FLAME, spawn->pos, entity->radius, NULL) != 0)
			continue;
		a[0] = spawn->pos[0];
		a[2] = spawn->pos[2];
		a[3] = spawn->pos[3];
		nearest = 32767.0f;
		a[1] = 0.0f;
		sum = 0.0f;
		farthest = 0.0f;
		for (other = g_Entities; other != NULL; other = other->next)
		{
			b[0] = other->pos[0];
			b[2] = other->pos[2];
			b[3] = other->pos[3];
			b[1] = 0.0f;
			distance = fabsf(bmVectorDistance(a, b));
			if (distance < nearest)
				nearest = distance;
			if (farthest < distance)
				farthest = distance;
			sum = distance + sum;
		}
		if (farthest - nearest != 0.0f)
			sum = sum / (farthest - nearest);
		if (bestScore < sum)
		{
			best = spawn;
			bestScore = sum;
		}
	}
	return best;
}

/* 0x0009f180  BumperCars/CBCar_World.cpp (bracketed)
Reads the score file `name` of `package`. Returns 0 when it is not there. */
int BCarWorld_LoadScoreFile(struct bkPackage *package, const char *name)
{
	struct BCarTextReader reader;
	char *file;
	uint size;

	BCarTextReader_Construct(&reader);
	file = (char *)bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (file == NULL)
	{
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	BCarTextReader_Init(&reader, file, (int)size, name);
	BCarWorld_LoadScoreInformation(&reader);
	BCarTextReader_Destruct(&reader);
	return 1;
}

/* 0x0009f250  BumperCars/CBCar_World.cpp (bracketed)
World_Update: the flames and the scenic actor's animation. */
void BCarWorld_Update(float dt, int control)
{
	if (g_WorldLoaded == 0)
		return;
	BCarFlame_Update(BCB_FLAME, dt, control);
	if (g_WorldScenicInst != NULL)
	{
		baUpdateAnimations(g_WorldScenicInst, g_WorldScenicInst->root, dt * 0.5f, NULL); /* 0x002536d0 */
		baPrepActorInstance(g_WorldScenicInst, 1);
	}
	bcb_rand();
	bcb_rand();
	bcb_rand();
}

/* 0x0009f2e0  BumperCars/CBCar_World.cpp (bracketed)
Draws the arena: the texture animations follow the game timer; the sky (bobbing up and down
except in variant 3), the sets, the scenic actor, the alpha sets and the scenery.
`reflection`: the pass of the floor's reflection; `mirrored`: draw everything mirrored in the
floor (y flipped, turned with the camera). */
void BCarWorld_DrawSets(int reflection, int mirrored)
{
	int i;
	float world[16];
	float scale[16];
	float turn[16];

	baSetBrightness(0x100);
	bdSetRenderState(4, 1, 0);
	for (i = 0; i < BCB_NUM_TEXANIMS; i++)
	{
		if (g_WorldTexAnims[i] != NULL)
			bdTextureSetTime(g_WorldTexAnims[i], BCB_FTOL(BCB_NOW() * 1000.0f)); /* 0x00255898 */
	}
	if (g_BCarVariant == 3)
		memcpy(world, DAT_002855f0, sizeof world);
	else
		bmMatrixTranslation(world, 0.0f,
			(float)(sin((double)g_BCarFrameCount * 0.017453292f + (double)g_BCarFrameCount *
				0.017453292f) * 150.0), 0.0f);                  /* 0x002640f8, 0x00262df8 */
	if (mirrored == 0)
	{
		BCB_CALL(FUN_000a57f0, void (*)(int))(BCB_DEBUG_GET(FUN_0008c030) != 0 ?
			g_ReflectBrightness : 0);
		BCB_CALL(FUN_000a5870, void (*)(void))();
		FUN_0018e640();
	}
	else
	{
		bmMatrixScaling(scale, 1.0f, -1.0f, 1.0f);
		bmMatrixRotationY(turn, BCarCamera_GetAngle(BCB_CAMERA));
		bmMatrixMultiplyCopy(scale, world, scale);
		bmMatrixMultiplyCopy(world, turn, scale);
		bdSetRenderState(4, 0, 0);
	}
	if (reflection == 0)
	{
		bdSetRenderState(6, 0, 0);
		bdSetRenderState(10, 0, 0);
		bdSetRenderState(8, 0, 0);
		if (mirrored == 0)
			bdSetWorldIdentity();
		else
			bdSetWorldMatrix(world);
		if (g_WorldSky != NULL)
			baDrawSet(g_WorldSky, 1);
		bdSetRenderState(8, BCB_DEBUG_GET(FUN_0008c060), 0);
		bdSetWorldMatrix(world);
		if (g_WorldScenery != NULL)
			baDrawSet(g_WorldScenery, 1);
		bdSetWorldIdentity();
		if (g_WorldScenicInst != NULL)
		{
			baDrawActorInstance(g_WorldScenicInst, 1);
			baDrawActorInstance(g_WorldScenicInst, 6);
		}
		if (mirrored != 0)
			goto done;
		if (g_WorldSet != NULL)
			baDrawSet(g_WorldSet, 1);
		if (g_WorldSet2 != NULL)
			baDrawSet(g_WorldSet2, 1);
	}
	else
	{
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(8, 0, 0);
		bdSetRenderState(10, 1, 0);
		if (mirrored == 0)
			bdSetWorldIdentity();
		else
			bdSetWorldMatrix(world);
		if (g_WorldSky != NULL)
			baDrawSet(g_WorldSky, 4);
		if (mirrored == 0)
		{
			bdSetRenderState(8, BCB_DEBUG_GET(FUN_0008c060), 0);
			bdSetRenderState(5, 0, 0);
			if (g_WorldSet != NULL)
				baDrawSet(g_WorldSet, 4);
			if (g_WorldSet2 != NULL)
				baDrawSet(g_WorldSet2, 4);
			bdSetRenderState(8, 0, 0);
			bdSetRenderState(10, 0, 0);
			FUN_001d6860();
			if (g_WorldAlphaSet != NULL)
			{
				baDrawSet(g_WorldAlphaSet, 1);
				baDrawSet(g_WorldAlphaSet, 4);
			}
			bdSetRenderState(10, 1, 0);
			if (g_WorldAlphaSet2 != NULL)
			{
				baDrawSet(g_WorldAlphaSet2, 1);
				baDrawSet(g_WorldAlphaSet2, 4);
			}
			FUN_0018e640();
		}
		bdSetRenderState(8, BCB_DEBUG_GET(FUN_0008c060), 0);
		if (mirrored != 0)
			goto done;
	}
	FUN_001bea60();
done:
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(4, 0, 0);
}

/* 0x0009f6d0  BumperCars/CBCar_World.cpp (bracketed)
The height of `pos` (lowered by `radius`) above the collision mesh: a ray straight down; the
nearest hit's normal goes to `normal` (when not NULL). 1e6 when nothing is below or `pos`
is outside the mesh's bounds. */
float BCarWorld_GetGroundHeight(float *pos, float radius, float *normal)
{
	struct BCollMesh *mesh = g_WorldCollision;
	struct BRay ray;
	struct BCollHit hits[60];
	float best = 1e+06f;
	float top = pos[1];
	float height;
	int count;

	ray.origin[0] = pos[0];
	ray.origin[2] = pos[2];
	ray.origin[3] = pos[3];
	if (!(BCB_F32(mesh, 0x40) <= ray.origin[0] + radius) ||
		!(ray.origin[0] - radius <= BCB_F32(mesh, 0x50)) ||
		!(BCB_F32(mesh, 0x48) <= ray.origin[2] + radius) ||
		!(ray.origin[2] - radius <= BCB_F32(mesh, 0x58)))
		return 1e+06f;                                         /* 0x00265b60 */
	ray.length = (float)sqrt(1e+12);                           /* 0x00265b68 */
	count = 0x3c;
	hits[0].flags = 0xc;
	ray.dir[0] = 0.0f;
	ray.dir[1] = -1e+06f;
	ray.dir[2] = 0.0f;
	ray.dir[3] = 1.0f;
	ray.origin[1] = top - radius;
	if (bmRayMesh(&ray, mesh, &count, hits) != 0 && 0 < count)
	{
		do
		{
			count--;
			height = (top - radius) - hits[count].point[1];
			if (height < best)
			{
				best = height;
				if (normal != NULL)
				{
					normal[0] = hits[count].normal[0];
					normal[1] = hits[count].normal[1];
					normal[2] = hits[count].normal[2];
					normal[3] = hits[count].normal[3];
				}
			}
		} while (count != 0);
	}
	return best;
}

/* 0x0009f860  BumperCars/CBCar_World.cpp (bracketed)
Reads the spawn file `name` of `package`. Returns 0 when it is not there. */
int BCarWorld_LoadSpawnFile(struct bkPackage *package, const char *name)
{
	struct BCarTextReader reader;
	char *file;
	uint size;

	BCarTextReader_Construct(&reader);
	file = (char *)bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (file == NULL)
	{
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	BCarTextReader_Init(&reader, file, (int)size, name);
	BCarWorld_LoadSpawnInfo(&reader);
	BCarTextReader_Destruct(&reader);
	return 1;
}

/* 0x0009f930  BumperCars/CBCar_World.cpp (bracketed)
World_Draw: `pass` 0 draws the sets and the flames, 1 the sets mirrored in the floor (no
flames), 2 the flames only; the
collision mesh in wire frame when the debug switch 0x0008c070 is on. */
void BCarWorld_Draw(int reflection, int pass)
{
	if (g_WorldLoaded == 0)
		return;
	if (pass != 2)
		BCarWorld_DrawSets(reflection, pass);
	if (BCB_DEBUG_GET(FUN_0008c070) != 0)
	{
		bdSetWorldIdentity();
		BCB_CALL(FUN_000a5050, void (*)(struct BCollMesh *, unsigned int, int))(g_WorldCollision,
			0x407f007fu, 0);
	}
	if (pass != 1)
		BCarFlame_Draw(BCB_FLAME, reflection, pass == 2);
}

/* a name keyword: the quoted name after it goes to `out` (`size` characters at most) */
static int bcb_keyword_name(char *line, const char *keyword, int length, char *out, int size,
	int status)
{
	if (_strnicmp(line, keyword, (size_t)length) != 0)
		return status;
	if (BCarText_GetName(line, out, size) != 0)
		return 1;
	return 2;
}

/* a float keyword whose value must be in [0, 1) */
static int bcb_keyword_fraction(const char *line, const char *keyword, int length, float *out,
	int status)
{
	float value;

	if (_strnicmp(line, keyword, (size_t)length) != 0)
		return status;
	if (sscanf(line, "%*s%f", &value) == 1 && 0.0f <= value && value < 1.0f)
	{
		*out = value;
		return 1;
	}
	return 2;
}

/* 0x0009f9b0  BumperCars/CBCar_World.cpp
World_LoadInformation: reads the world file: the set, sky, collision and scenery names, the
texture animations, the physics, scoring and boost settings, the lead marker, and the
spawn point, flame and waypoint files (read from `package` at once). */
void BCarWorld_LoadInformation(struct bkPackage *package, struct BCarTextReader *reader)
{
	char *line;
	int status;
	int i;
	int value;
	float f;
	char name[512];
	char buffer[512];

	BCarWorld_Reset();
	while (reader->cursor < reader->end)
	{
		line = BCarText_ReadLine(reader, buffer);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0)
			continue;
		status = 0;
		status = bcb_keyword_name(line, "SKY ", 4, g_WorldSkyName, 0x1ff, status); /* 0x00265ec0 */
		status = bcb_keyword_name(line, "MAPSET ", 7, g_WorldSetName, 0x1ff, status);
		status = bcb_keyword_name(line, "MAPSET2 ", 8, g_WorldSet2Name, 0x1ff, status);
		status = bcb_keyword_name(line, "MAPALPHA2 ", 10, g_WorldAlphaSet2Name, 0x1ff, status);
		status = bcb_keyword_name(line, "MAPALPHA ", 9, g_WorldAlphaSetName, 0x1ff, status);
		status = bcb_keyword_name(line, "MAPCOL ", 7, g_WorldCollisionName, 0x1ff, status);
		status = bcb_keyword_name(line, "SCENERY ", 8, g_WorldSceneryName, 0x1ff, status);
		status = bcb_keyword_name(line, "SCENICS ", 8, g_WorldScenicsName, 0x1ff, status);
		for (i = 0; i < BCB_NUM_TEXANIMS; i++)
		{
			_snprintf(name, 0x1ff, "TEXANIM%d ", i + 1);
			status = bcb_keyword_name(line, name, (int)strlen(name), g_WorldTexAnimName(i), 0x1ff,
				status);
		}
		status = bcb_keyword_name(line, "SCENEANIM ", 10, g_WorldSceneAnimName, 0x3f, status);
		status = bcb_keyword_name(line, "SCENEANIMLOOP ", 0xe, g_WorldSceneAnimLoopName, 0x3f,
			status);
		status = bcb_keyword(line, "KILLHEIGHT ", 0xb, "%*s%f", &g_KillHeight, status);
		if (_strnicmp(line, "GRAVITY ", 8) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &f) == 1)
			{
				g_WorldGravity[0] = 0.0f;
				g_WorldGravity[1] = f;
				g_WorldGravity[2] = 0.0f;
				status = 1;
			}
		}
		status = bcb_keyword_fraction(line, "FRICTION ", 9, &g_WorldFriction, status);
		status = bcb_keyword_fraction(line, "WORLDEDGEFRICTION ", 0x12, &g_WorldEdgeFriction, status);
		status = bcb_keyword(line, "WORLDRESTITUTION ", 0x11, "%*s%f", &g_WorldRestitution, status);
		status = bcb_keyword(line, "ENTITYBOUNCE ", 0xb, "%*s%f", &g_EntityBounce, status);
		status = bcb_keyword_fraction(line, "ENTITYCOLLISIONBIAS ", 0x14, &g_EntityCollisionBias,
			status);
		status = bcb_keyword(line, "MAXSPEED ", 9, "%*s%f", &g_MaxSpeed, status);
		status = bcb_keyword(line, "TIMEOUT ", 8, "%*s%f", &g_HitTimeout, status);
		status = bcb_keyword(line, "BRAKE_HIT_TIMEOUT ", 0x12, "%*s%f", &g_BrakeHitTimeout, status);
		status = bcb_keyword(line, "RESTARTWAIT ", 0xc, "%*s%f", &g_RestartWait, status);
		if (_strnicmp(line, "SPAWNPOINTS ", 0xc) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, name, 0x1ff) != 0 && BCarWorld_LoadSpawnFile(package, name) != 0)
				status = 1;
		}
		if (_strnicmp(line, "FLAMES ", 7) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, name, 0x1ff) != 0 &&
				BCarFlame_Initialise(BCB_FLAME, package, name) != 0)
				status = 1;
		}
		if (_strnicmp(line, "WAYPOINTS ", 10) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, name, 0x1ff) != 0 &&
				BCB_CALL(FUN_000a48d0, int (*)(struct bkPackage *, const char *))(package, name) != 0)
				status = 1;
		}
		status = bcb_keyword(line, "ENTITYSPEED ", 0xc, "%*s%f", &g_EntitySpeed, status);
		if (_strnicmp(line, "ENTITYROT ", 10) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &f) == 1 && 0.0f < f)
			{
				status = 1;
				g_EntityRot = 6.2831855f / f;                   /* 0x00255930 */
			}
		}
		status = bcb_keyword(line, "BRAKEVALUE ", 0xb, "%*s%f", &g_BrakeValue, status);
		status = bcb_keyword(line, "BOOSTVALUE ", 0xb, "%*s%f", &g_BoostValue, status);
		status = bcb_keyword(line, "BOOSTLIFE ", 10, "%*s%f", &g_BoostLife, status);
		if (_strnicmp(line, "BOOSTLIMIT ", 0xb) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%d", &value) == 1 && -1 < value)
			{
				g_BoostLimit = value;
				status = 1;
			}
		}
		status = bcb_keyword(line, "BOOSTRECHARGE ", 0xe, "%*s%f", &g_BoostRecharge, status);
		status = bcb_keyword(line, "LEADTIMEOUT ", 0xc, "%*s%f", &g_LeadTimeout, status);
		status = bcb_keyword(line, "LEADSCORE ", 10, "%*s%d", &g_LeadScore, status);
		if (_strnicmp(line, "LEADMODEL ", 10) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, name, 0x1ff) != 0)
			{
				g_LeadActor = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
				if (g_LeadActor == NULL)
					bkPrintf("World_Init: Could not load Lead Player model [%s]\n", name);
				status = 1;
			}
		}
		if (_strnicmp(line, "LEADSPIN ", 9) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &f) == 1)
			{
				g_LeadSpin = f * 0.017453292f;                  /* 0x002640f8 */
				status = 1;
			}
		}
		status = bcb_keyword(line, "LEADOFFSET ", 0xb, "%*s%f", &g_LeadOffset, status);
		status = bcb_keyword(line, "ENT_COLLISION_VEL ", 0x12, "%*s%f", &g_EntCollisionVel, status);
		status = bcb_keyword(line, "DISABLE_VEL ", 0xc, "%*s%f", &g_DisableVel, status);
		status = bcb_keyword(line, "WALL_COLLISION_VEL ", 0x13, "%*s%f", &g_WallCollisionVel, status);
		status = bcb_keyword(line, "MAXVERTVELOCITY ", 0x10, "%*s%f", &g_MaxVertVelocity, status);
		status = bcb_keyword(line, "GAINRADIUS ", 0xb, "%*s%f", &g_GainRadius, status);
		status = bcb_keyword(line, "LOSERADIUS ", 0xb, "%*s%f", &g_LoseRadius, status);
		if (_strnicmp(line, "LAPS ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%d", &value) == 1 && 0 < value)
			{
				g_Laps = value;
				status = 1;
			}
		}
		if (_strnicmp(line, "LIVES ", 6) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%d", &value) == 1 && 0 < value)
			{
				g_Lives = value;
				status = 1;
			}
		}
		status = bcb_keyword(line, "WP_LAG_SPEED_OFFSET ", 0x14, "%*s%f", &g_WpLagSpeedOffset, status);
		status = bcb_keyword(line, "WP_LAG_MAX_OFFSET ", 0x12, "%*s%d", &g_WpLagMaxOffset, status);
		status = bcb_keyword(line, "FLATSPOT_ANGLE ", 0xf, "%*s%f", &g_FlatspotAngle, status);
		bcb_world_bad_line(reader, line, name, status, "World_LoadInformation", 0x319, 0x31b);
	}
}

/* load the set `name` (resource type 6) into *out unless the name is empty */
static void bcb_world_load_set(char *name, struct BSet **out, const char *error)
{
	if (name[0] == '\0')
		return;
	*out = (struct BSet *)bLoadResource(NULL, name, 6, 0xdefa);
	if (*out == NULL)
		bkPrintf(error, name);
}

/* 0x000a0620  BumperCars/CBCar_World.cpp
World_Init: reads the world file `name` of `package` and loads what it names: the sets (the
main set is required), the sky, the alpha sets, the scenery, the collision mesh (required),
the scenic actor with its animations, the texture animations and the lead marker. Then
it stirs the random numbers and runs the debris for 360 frames (so the arena starts
"lived in"). Returns 1 on success. */
int BCarWorld_Init(struct bkPackage *package, const char *name)
{
	struct BCarTextReader reader;
	char *file;
	uint size;
	int i;

	BCarTextReader_Construct(&reader);
	g_WorldLoaded = 0;
	file = (char *)bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (file == NULL)
	{
		bkPrintf("World_Init: Configuration file '%s' not found\n", name);
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	BCarTextReader_Init(&reader, file, (int)size, name);
	BCarWorld_LoadInformation(package, &reader);
	if (g_WorldSetName[0] == '\0')
	{
		bkPrintf("World_Init: Set not specified\n");
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	g_WorldSet = (struct BSet *)bLoadResource(NULL, g_WorldSetName, 6, 0xdefa);
	if (g_WorldSet == NULL)
	{
		bkPrintf("World_Init: Set [%s] could not be loaded\n", g_WorldSetName);
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	bcb_world_load_set(g_WorldSkyName, &g_WorldSky, "World_Init: Sky [%s] could not be loaded\n");
	bcb_world_load_set(g_WorldAlphaSetName, &g_WorldAlphaSet,
		"World_Init: Alpha Set [%s] could not be loaded\n");
	bcb_world_load_set(g_WorldSet2Name, &g_WorldSet2, "World_Init: Set2 [%s] could not be loaded\n");
	bcb_world_load_set(g_WorldAlphaSet2Name, &g_WorldAlphaSet2,
		"World_Init: Alpha Set2 [%s] could not be loaded\n");
	bcb_world_load_set(g_WorldSceneryName, &g_WorldScenery,
		"World_Init: Scenery [%s] could not be loaded\n");
	if (g_WorldCollisionName[0] == '\0')
	{
		bkPrintf("World_Init: Collision Mesh not specified\n");
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	g_WorldCollision = (struct BCollMesh *)bLoadResource(NULL, g_WorldCollisionName, 7, 0xdefa);
	if (g_WorldCollision == NULL)
	{
		bkPrintf("World_Init: Collision Mesh [%s] not found\n", g_WorldCollisionName);
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	if (g_WorldScenicsName[0] != '\0')
	{
		g_WorldScenicActor = (struct BActor *)bLoadResource(NULL, g_WorldScenicsName, 1, 0xdefa);
		if (g_WorldScenicActor == NULL)
			bkPrintf("World_Init: Scenic Actor [%s] could not be loaded\n", g_WorldScenicsName);
		else
		{
			g_WorldScenicInst = baInitActorInstance(NULL, g_WorldScenicActor, 3, BCB_SRC_WORLD, 0x73);
			if (g_WorldScenicInst == NULL)
				bkPrintf("World_Init: Scenic Actor Instance could not be created\n");
			else
			{
				baPrepActorInstance(g_WorldScenicInst, 1);
				if (g_WorldSceneAnimName[0] != '\0')
				{
					g_WorldScenicAnim = (struct BAnim *)baFindSubObject(g_WorldScenicInst,
						g_WorldSceneAnimName);
					if (g_WorldScenicAnim == NULL)
						bkPrintf("World_Init: Scenic Actor Animation [%s] could not be found\n",
							g_WorldSceneAnimName);
				}
				if (g_WorldSceneAnimLoopName[0] != '\0')
				{
					g_WorldScenicAnimLoop = (struct BAnim *)baFindSubObject(g_WorldScenicInst,
						g_WorldSceneAnimLoopName);
					if (g_WorldScenicAnimLoop == NULL)
						bkPrintf("World_Init: Scenic Actor Animation [%s] could not be found\n",
							g_WorldSceneAnimLoopName);
				}
			}
		}
	}
	for (i = 0; i < BCB_NUM_TEXANIMS; i++)
	{
		if (g_WorldTexAnimName(i)[0] == '\0')
			continue;
		g_WorldTexAnims[i] = (struct bdTexture *)bLoadResource(NULL, g_WorldTexAnimName(i), 0, 0xdefa);
		if (g_WorldTexAnims[i] == NULL)
			bkPrintf("World_Init: Generic Texture Animation [%s] could not be loaded\n",
				g_WorldTexAnimName(i));
	}
	if (g_WorldScenicInst != NULL)
		baFlushAnimations(g_WorldScenicInst, g_WorldScenicInst->root, 1, 0);
	if (g_WorldScenicAnim != NULL)
		baQueueAnimation(g_WorldScenicInst, g_WorldScenicInst->root, g_WorldScenicAnim, 0x400, -1,
			0, NULL, NULL);
	if (g_WorldScenicAnimLoop != NULL)
		baQueueAnimation(g_WorldScenicInst, g_WorldScenicInst->root, g_WorldScenicAnimLoop, 0x400,
			-1, 1, NULL, NULL);
	if (g_LeadTimeout != 0.0f)
	{
		if (g_LeadActor == NULL)
			bkPrintf("World_Init: Lead player actor not found\n");
		else
		{
			g_LeadInst = baInitActorInstance(NULL, g_LeadActor, 1, BCB_SRC_WORLD, 0xa7);
			baPrepActorInstance(g_LeadInst, 1);
		}
	}
	g_WorldLoaded = 1;
	for (i = 0; i < 800; i++)
	{
		bcb_rand();
		bcb_rand();
		bcb_rand();
	}
	for (i = 0; i < 0x168; i++)
		BCarEffects_Update(BCB_EFFECTS, 1.0f);
	BCarTextReader_Destruct(&reader);
	return 1;
}
