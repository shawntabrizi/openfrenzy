/*
BumperCars/CBCar_Waypoints.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Waypoints.cpp

The waypoint gates (up to 40 at 0x002f0a80, struct BCarWaypoint): in the race variant every
car passes the gates in order (laps, finish places, the lag behind the leader that the AI and
the speed boost use); in the keeper variant some gates are goals (CBCar_GenericGoal) and one
of them at a time is active, marked by a shell and scoring GATE when a car carries a keepsake
through it. Also the two particle fountains of the race end.
0x000a3eb0 and 0x000a48d0 are tagged with CBCar_Waypoints.cpp, 0x000a41e0 is bracketed; the
gap functions 0x000a3c40 and 0x000a4c20 are waypoint functions too (file inferred).

BCarEntity.unk048 (+0x48) is the index of the next gate of an entity, BCarEntity.team (+0x44)
its finish place (-1 while racing).
*/

#include "bcb_items.h"

static const char s_WaypointFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Waypoints.cpp";

/* classlib_fx: the goal effect (CParticleEffect) and its emitters, and the particle systems */
#define bcb_GoalsUpdate(goals, dt) BCB_FN(FUN_000a85f0, void, (void *, float))((goals), (dt))
#define bcb_GoalsDraw(goals, mode, actors) BCB_FN(FUN_000a86e0, void, (void *, int, unsigned int))((goals), (mode), (actors))
#define bcb_GoalsCollide(goals, goal, sphere, dir) \
	BCB_FN(FUN_000a8a00, int, (void *, void *, float *, float *))((goals), (goal), (sphere), (dir))
#define bcb_GoalsBurst(goals, goal, r, g, b, kind, strength) \
	BCB_FN(FUN_000a7e50, void, (void *, void *, short, short, short, int, float))((goals), (goal), (r), (g), (b), (kind), (strength))
#define bcb_GoalSwitch(goal, on) BCB_FN(FUN_000a7dc0, void, (void *, int))((goal), (on))
#define bcb_ParticleStart(ps, r, g, b) BCB_FN(FUN_000aa020, void, (void *, short, short, short))((ps), (r), (g), (b))

/* a virtual function of a particle system (classlib_fx vtables 0x00266cd8...) */
#define BCB_VCALL(obj, slot, type) ((type)((*(void ***)(obj))[slot]))

/* 0x000a3c40  (gap; file inferred: CBCar_Waypoints.cpp)
Waypoints_Draw(glow): the goals and the active gate's shell; in the glow pass also the race
end fountains and the gates (a flat quad between their 4 points, pulsing orange). */
void BCarWaypoints_Draw(int glow)
{
	int i;

	BCB_DEBUG_GET(FUN_0008c000);
	if (g_GenericGoals != NULL)
		bcb_GoalsDraw(g_GenericGoals, glow, 1);
	if (g_GateShell != NULL)
	{
		bdPushRenderState(6, 1, 0);
		bdPushRenderState(10, 1, 0);
		bdSetWorldIdentity();
		baPrepActorInstance(g_GateShell, 1);
		baDrawActorInstance(g_GateShell, 7);
		bdPopRenderState(6);
		bdPopRenderState(10);
	}
	if (glow == 0)
		return;
	for (i = 0; i < 2; i++)
	{
		void *ps = g_LapEffects[i];

		if (ps != NULL)
			BCB_VCALL(ps, 2, void (*)(void *))(ps);
	}
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(8, 0, 0);
	bdSetWorldIdentity();
	for (i = 0; i < g_NumWaypoints; i++)
	{
		struct BCarWaypoint *wp = &g_Waypoints[i];
		struct BCarVertex *v = (struct BCarVertex *)BCB_ADDR(0x002f2220);
		float t;
		int r, g, b;
		unsigned int colour;

		if (wp->keeperGate != 0 || BCB_DEBUG_GET(FUN_0008c020) == 0)
			continue;
		bdSetTexture(0, NULL);
		t = (float)abs(DAT_002ba99c % 0x3c - 0x1e) * 0.033333335f;    /* 0x00255b64 */
		r = 0xdb - BCB_FTOL(t * -6.0f);                               /* 0x00266654 */
		g = 0x7b - BCB_FTOL(t * -102.0f);                             /* 0x00266650 */
		b = 0x14 - BCB_FTOL(t * -205.0f);                             /* 0x0026664c */
		colour = DAT_002bd640[b] | DAT_002bc490[g] | DAT_002bce18[r] | DAT_002bc898[0x3c];   /* 0x002bc988 */
		v[0].colour = colour;
		v[1].colour = colour;
		v[2].colour = colour;
		v[3].colour = colour;
		v[0].pos[0] = wp->points[0][0];
		v[0].pos[1] = wp->points[0][1];
		v[0].pos[2] = wp->points[0][2];
		v[1].pos[0] = wp->points[1][0];
		v[1].pos[1] = wp->points[1][1];
		v[1].pos[2] = wp->points[1][2];
		v[2].pos[0] = wp->points[3][0];
		v[2].pos[1] = wp->points[3][1];
		v[2].pos[2] = wp->points[3][2];
		v[3].pos[0] = wp->points[2][0];
		v[3].pos[1] = wp->points[2][1];
		v[3].pos[2] = wp->points[2][2];
		bdDrawPrimitiveFVF(4, v, 4, 0x152);
	}
	bdPopRenderState(6);
	bdPopRenderState(5);
	bdPopRenderState(10);
	bdPopRenderState(8);
	bdPopRenderState(3);
}

/* 0x000a3eb0  BumperCars/CBCar_Waypoints.cpp
Waypoint_LoadInformation: reads the "WAYPOINT { }" blocks (POINT n x y z, KEEPERGATE,
TEXTURE). A keeper gate gets a goal; one of the gates is chosen at random to be active. */
void BCarWaypoint_LoadInformation(struct BCarTextReader *reader)
{
	char word[512];
	char buf[512];
	char *line;
	int status;

	g_NumWaypoints = 0;
	g_NumGates = 0;
	g_GateShell = NULL;
	if (!(reader->cursor < reader->end))
	{
		g_ActiveGate = 0;
		g_PreviousGate = 0;
		return;
	}
	do
	{
		struct BCarWaypoint *wp;
		int index;

		line = BCarText_ReadLine(reader, buf);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0 || _stricmp(line, "WAYPOINT {") != 0)
			continue;
		index = g_NumWaypoints;
		g_NumWaypoints = index + 1;
		wp = &g_Waypoints[index];
		wp->keeperGate = 0;
		wp->texture = NULL;
		wp->goal = NULL;
		while (reader->cursor < reader->end)
		{
			int n;
			float x, y, z;

			status = 0;
			if (_stricmp(line, "}") == 0)
				break;
			line = BCarText_ReadLine(reader, buf);
			BCarText_NextLine(reader);
			if (strncmp(line, "//", 2) == 0)
				continue;
			if (_strnicmp(line, "POINT ", 6) == 0)
			{
				status = 2;
				if (sscanf(line, "%*s%d%f%f%f", &n, &x, &y, &z) == 4 && 0 < n && n < 5)
				{
					wp->points[n - 1][0] = x;
					wp->points[n - 1][1] = y;
					wp->points[n - 1][2] = z;
					wp->points[n - 1][3] = 1.0f;
					status = 1;
				}
			}
			if (_strnicmp(line, "KEEPERGATE ", 0xb) == 0)
			{
				status = 2;
				if (sscanf(line, "%*s%d", &wp->keeperGate) == 1)
					status = 1;
			}
			if (_strnicmp(line, "TEXTURE ", 8) == 0)
			{
				status = 2;
				if (BCarText_GetName(line, word, 0x1ff) != 0)
				{
					wp->texture = (struct bdTexture *)bLoadResource(NULL, word, 0, 0xdefa);
					status = 1;
				}
			}
			if (_stricmp(line, "}") != 0)
			{
				sscanf(line, "%s", word);
				bcb_BadCommand(reader, s_WaypointFile, 0xc6, "Waypoint_LoadInformation", status, word, line);
			}
		}
		if (wp->keeperGate != 0)
		{
			g_NumGates = g_NumGates + 1;
			/* the third argument is the float POINT 2 x (stored as the goal's scale, +0x10);
			bumpercars_b.decls types it float now, so pass the value (integration round 2) */
			wp->goal = BCarGenericGoal_AddGoal(g_GenericGoals, wp->points[0], wp->points[1][0], 0);
			/* 0x000a8020 sets the goal's texture (+0x24) */
			BCB_FN(FUN_000a8020, void, (void *, void *))(wp->goal, wp->texture);
		}
	} while (reader->cursor < reader->end);
	if (g_NumGates != 0)
	{
		g_ActiveGate = bcb_rand() % g_NumGates;
		g_PreviousGate = g_ActiveGate;
		return;
	}
	g_ActiveGate = 0;
	g_PreviousGate = 0;
}

/* the active gate's shell sits on the active goal */
static void bcb_PlaceShell(void)
{
	struct BActorInstance *shell = g_GateShell;

	if (shell != NULL)
	{
		float *pos = BCB_GOAL_POS(g_Waypoints[g_ActiveGate].goal);

		shell->pos[0] = pos[0];
		shell->pos[1] = pos[1];
		shell->pos[2] = pos[2];
		shell->pos[3] = pos[3];
	}
}

/* 0x000a41e0  BumperCars/CBCar_Waypoints.cpp (bracketed)
Waypoints_Update(playing, dt): the goals and fountains, then for every car: passing the next
gate (a ray along its motion against the gate's two triangles) counts its laps and finish
place; carrying a keepsake through the active keeper gate scores GATE and moves the active
gate. Last, the lag of every entity behind the leader. `playing` is not used. */
void BCarWaypoints_Update(int playing, float dt)
{
	int index;
	struct BCarEntity *e;
	int best;

	(void)playing;
	if (g_NumWaypoints == 0)
		return;
	if (g_GenericGoals != NULL)
		bcb_GoalsUpdate(g_GenericGoals, dt);
	for (index = 0; index < 2; index++)
	{
		void *ps = g_LapEffects[index];

		if (ps != NULL)
			BCB_VCALL(ps, 1, void (*)(void *, float))(ps, dt);
	}
	for (index = 0; index < BCB_NUM_PLAYERS; index++)
	{
		int *slot = &DAT_002baed4[index * 10];
		struct BRay ray;
		struct BTriangle tri1;
		struct BTriangle tri2;
		struct BCollHit hit;
		float sphere[5];
		struct BCarWaypoint *wp;
		float length;
		int count;
		int previous;
		int pick;
		int k;

		e = g_BCarPlayers[index].entity;
		ray.origin[0] = e->pos[0];
		ray.origin[1] = e->pos[1];
		ray.origin[2] = e->pos[2];
		ray.origin[3] = e->pos[3];
		ray.origin[1] = ray.origin[1] + e->drawOffset;
		ray.dir[0] = -e->vel[0];
		ray.dir[1] = -e->vel[1];
		ray.dir[2] = -e->vel[2];
		length = sqrtf(ray.dir[2] * ray.dir[2] + ray.dir[1] * ray.dir[1] + ray.dir[0] * ray.dir[0]);
		ray.length = length;
		if (length == 0.0f)
		{
			float scale;

			ray.dir[0] = 0.0f;
			ray.dir[1] = 0.0f;
			ray.dir[2] = 1.0f;
			ray.dir[3] = 1.0f;
			bmVectorRotateY(ray.dir, ray.dir, e->targetHeading);
			bmVectorNormalise(ray.dir, ray.dir);
			scale = e->radius;
			ray.dir[0] = ray.dir[0] * scale;
			ray.dir[1] = ray.dir[1] * scale;
			ray.dir[2] = ray.dir[2] * scale;
		}
		else if (length < e->radius)
		{
			float scale = e->radius / length;

			ray.dir[0] = ray.dir[0] * scale;
			ray.dir[1] = ray.dir[1] * scale;
			ray.dir[2] = ray.dir[2] * scale;
		}
		ray.dir[3] = 1.0f;
		ray.origin[3] = 1.0f;
		sphere[0] = e->pos[0];
		sphere[1] = e->pos[1] + e->drawOffset;
		sphere[2] = e->pos[2];
		sphere[3] = 1.0f;
		sphere[4] = e->radius;
		hit.flags = 0;
		wp = &g_Waypoints[e->unk048];
		for (k = 0; k < 3; k++)
		{
			tri1.v[0][k] = wp->points[0][k];
			tri1.v[1][k] = wp->points[1][k];
			tri1.v[2][k] = wp->points[2][k];
			tri2.v[0][k] = wp->points[0][k];
			tri2.v[1][k] = wp->points[2][k];
			tri2.v[2][k] = wp->points[3][k];
		}
		tri1.v[0][3] = 1.0f;
		tri1.v[1][3] = 1.0f;
		tri1.v[2][3] = 1.0f;
		tri2.v[0][3] = 1.0f;
		tri2.v[1][3] = 1.0f;
		tri2.v[2][3] = 1.0f;
		if (bmRayTriangle(&ray, &tri1, &hit) != 0 || bmRayTriangle(&ray, &tri2, &hit) != 0)
		{
			e->unk048 = e->unk048 + 1;
			if (e->unk048 == g_NumWaypoints)
			{
				if (g_Laps != -1)
				{
					e->lap = e->lap + 1;
					if (e->lap == g_Laps)
					{
						e->place = g_FinishPlace;
						g_FinishPlace = g_FinishPlace + 1;
						BCarSound_Play(BCB_SOUND, 0x1d, 0xff, 0, -1);
						if (e->place == 1)
						{
							int j;

							for (j = 0; j < 2; j++)
							{
								void *ps = g_LapEffects[j];

								if (ps != NULL)
								{
									bcb_ParticleStart(ps, BCB_CHAR_COLOUR(*slot, 0), BCB_CHAR_COLOUR(*slot, 1),
										BCB_CHAR_COLOUR(*slot, 2));
									BCB_F32(g_LapEffects[j], 0x60) = 10.0f;
									BCB_AT(g_LapEffects[j], 0x50, unsigned char) = 1;
								}
							}
							for (j = 0; j < BCB_NUM_PLAYERS; j++)
							{
								if (g_BCarPlayers[j].entity == e)
									BCarSound_Speak(BCB_SOUND, j, 9, 0xff, e->pos, 0.0f);
							}
						}
					}
				}
				e->unk048 = 0;
			}
		}

		/* the active keeper gate */
		if (g_Waypoints[g_ActiveGate].keeperGate == 0 || e->carried == NULL)
			continue;
		sphere[0] = e->lastPos[0];
		sphere[1] = e->lastPos[1] + e->drawOffset;
		sphere[2] = e->lastPos[2];
		sphere[3] = 1.0f;
		sphere[4] = e->radius;
		if (bcb_GoalsCollide(g_GenericGoals, g_Waypoints[g_ActiveGate].goal, sphere, e->lastVel) == 0)
			continue;
		BCarSound_Play(BCB_SOUND, 0x1e, 0xff, 0, -1);
		bcb_GoalsBurst(g_GenericGoals, g_Waypoints[g_ActiveGate].goal, BCB_CHAR_COLOUR(*slot, 0),
			BCB_CHAR_COLOUR(*slot, 1), BCB_CHAR_COLOUR(*slot, 2), 5, 2.0f);
		if (bcb_rand() % 0x19 == 0)
			BCarSound_Speak(BCB_SOUND, index, 4, 0xff, e->pos, 0.0f);
		bcb_GoalSwitch(g_Waypoints[g_ActiveGate].goal, 0);
		count = g_NumGates;
		if (count == 2)
		{
			previous = -1;
		}
		else
		{
			previous = g_PreviousGate;
			if (count <= 1)
				goto chosen;
		}
		g_PreviousGate = g_ActiveGate;
		/* the original tests the previous gate's KEEPERGATE (index -1 reads the word in front
		of the table), not the picked one's */
		do
		{
			pick = bcb_rand() % count;
		} while (pick == g_ActiveGate || pick == previous ||
			 BCB_VAR(int, 0x002f0a80 + previous * 0x4c) == 0);
		g_ActiveGate = pick;
	chosen:
		bcb_GoalSwitch(g_Waypoints[g_ActiveGate].goal, 1);
		bcb_PlaceShell();
		if (g_LeadPlayer != NULL && g_LeadPlayer->entity == e)
			BCarEntity_AddScore(e, g_Score(0x002efa80));
		else
			BCarEntity_AddScore(e, g_Score(0x002efa44));
	}

	/* the lag behind the leader, in gates */
	best = 0;
	for (e = g_Entities; e != NULL; e = e->next)
	{
		int progress = e->lap * g_NumWaypoints + e->unk048;

		if (best < progress)
			best = progress;
	}
	for (e = g_Entities; e != NULL; e = e->next)
		e->lag = (best - e->lap * g_NumWaypoints) - e->unk048;
}

/* 0x000a48d0  BumperCars/CBCar_Waypoints.cpp
Waypoint_LoadFile(package, file): the goals, the waypoint file, the active gate's shell
(bumperalpha.obe), the goal textures and, in the race variant, the two fountains at the
finish. Returns 0 when the file is missing. */
int BCarWaypoint_LoadFile(struct bkPackage *package, char *file)
{
	struct BCarTextReader reader;
	unsigned int size;
	char *text;
	void *mem;
	struct BActor *actor;
	int i;

	BCarTextReader_Construct(&reader);
	text = bkLoadFile(package, file, NULL, &size, NULL, 0);
	if (text == NULL)
	{
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	BCarTextReader_Init(&reader, text, (int)size, file);
	mem = BCB_NEW(0x28, s_WaypointFile, 0x1d);
	if (mem == NULL)
		g_GenericGoals = NULL;
	else
		g_GenericGoals = BCarGenericGoal_Construct((struct CParticleEffect *)mem, 1);
	BCarWaypoint_LoadInformation(&reader);
	if (0 < g_NumGates)
	{
		actor = (struct BActor *)bLoadResource(NULL, "world\\models\\bumperalpha.obe", 1, 0xdefa);
		if (actor == NULL)
		{
			bkPrintf("%s(%d) : Waypoint_LoadFile - Could not find active shell actor\n", s_WaypointFile, 0x35);
		}
		else
		{
			g_GateShell = baInitActorInstance(NULL, actor, 1, s_WaypointFile, 0x29);
			if (g_GateShell != NULL)
			{
				struct BActorInstance *shell = g_GateShell;

				shell->rot[0] = DAT_00285634[0];
				shell->rot[1] = DAT_00285634[1];
				shell->rot[2] = DAT_00285634[2];
				shell->rot[3] = DAT_00285634[3];
				baPrepActorInstance(g_GateShell, 1);
			}
			else
			{
				bkPrintf("%s(%d) : Waypoint_LoadFile - Could not create active shell actor instance\n",
					s_WaypointFile, 0x30);
			}
		}
	}
	if (g_Waypoints[g_ActiveGate].keeperGate != 0)
	{
		bcb_GoalSwitch(g_Waypoints[g_ActiveGate].goal, 1);
		bcb_PlaceShell();
	}
	g_HelixTexture = (struct bdTexture *)bLoadResource(NULL, "genericgoal\\textures\\helixtwist.bmp", 0, 0xdefa);
	g_SparkTexture = (struct bdTexture *)bLoadResource(NULL, "genericgoal\\textures\\sparkburst.bmp", 0, 0xdefa);
	if (g_Laps == 0 || g_NumWaypoints < 1 || g_NumWaypoints == g_NumGates)
	{
		g_LapEffects[0] = NULL;
		g_LapEffects[1] = NULL;
	}
	else
	{
		for (i = 0; i < 2; i++)
		{
			void *ps;
			float pos[4];

			mem = BCB_NEW(0xa8, s_WaypointFile, 0x4f);
			ps = (mem == NULL) ? NULL : bcb_ParticleConstruct(mem);
			g_LapEffects[i] = ps;
			if (ps == NULL)
				continue;
			bcb_ParticleInit(ps, NULL, 800);
			bcb_ParticleSetTexture2(ps, g_HelixTexture);
			bcb_ParticleSetTexture(ps, g_SparkTexture);
			pos[3] = 1.0f;
			pos[1] = 1295.0f;           /* 0x44a1e000 */
			pos[0] = 1891.0f;           /* 0x44ec6000 */
			pos[2] = (i % 2 == 0) ? -4391.0f : -2558.0f;   /* 0xc5893800, 0xc51fe000 */
			bcb_ParticleSetPosition(ps, pos);
		}
	}
	BCarTextReader_Destruct(&reader);
	return 1;
}

/* 0x000a4c20  (gap; file inferred: CBCar_Waypoints.cpp)
Waypoints_Delete: the goals, the shell and the fountains. */
void BCarWaypoints_Delete(void)
{
	struct CParticleEffect *goals = g_GenericGoals;
	int i;

	if (g_NumWaypoints != 0)
		g_NumWaypoints = 0;
	if (g_GenericGoals != NULL)
	{
		BCarGenericGoal_Destruct(goals);
		BCB_DELETE(goals);
		g_GenericGoals = NULL;
	}
	if (g_GateShell != NULL)
	{
		baDeleteActorInstance(g_GateShell);
		g_GateShell = NULL;
	}
	for (i = 0; i < 2; i++)
	{
		void *ps = g_LapEffects[i];

		if (ps != NULL)
		{
			BCB_VCALL(ps, 0, void (*)(void *))(ps);
			if (g_LapEffects[i] != NULL)
				BCB_VCALL(g_LapEffects[i], 5, void *(*)(void *, int))(g_LapEffects[i], 1);
			g_LapEffects[i] = NULL;
		}
	}
}
