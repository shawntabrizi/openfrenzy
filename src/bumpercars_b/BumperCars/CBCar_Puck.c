/*
BumperCars/CBCar_Puck.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Puck.cpp

The pucks of the puck variant (list at 0x002f0760, struct BCarPuckInfo): the PUCK { } block
of the player file, the entities, scoring a puck goal, and drawing (glow pass and floor
reflection). 0x000a0db0 is tagged with CBCar_Puck.cpp; the gap functions around it are puck
functions too (file inferred).
*/

#include "bcb_items.h"

static const char s_PuckFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Puck.cpp";

/* 0x000a0cc0  (gap; file inferred: CBCar_Puck.cpp)
Pucks_Init: makes the entity of every puck (type 2) and its light spot "spotN". Returns 0
when a puck has no model or its entity fails. */
int BCarPucks_Init(void)
{
	struct BCarPuckInfo *puck;
	char spot[256];
	int n = 1;

	for (puck = g_PuckList; puck != NULL; puck = puck->next)
	{
		if (puck->actor == NULL)
		{
			bkPrintf("Player_Init - Couldn't find puck actor\n");
			return 0;
		}
		puck->entity = BCarEntity_Create(puck->actor, puck->disableModel, puck->scale, &puck->params);
		if (puck->entity == NULL)
		{
			bkPrintf("Player_Init - Couldn't initialise puck entity\n");
			return 0;
		}
		BCarEntity_SetType(puck->entity, BCAR_ENT_PUCK);
		_snprintf(spot, 0xff, "spot%d", n);
		BCarLight_Attach(BCB_LIGHTS, spot, puck->entity);
		n++;
	}
	return 1;
}

/* 0x000a0db0  BumperCars/CBCar_Puck.cpp
Puck_LoadInformation: reads a PUCK { } block (MODEL, DISABLEMODEL, SCALE, RESPAWN_DELAY,
ANGLE, POSITION, MASS, RADIUS, DRAWOFFSET, PL_COLOUR_TEX) and adds the puck to the list. */
void BCarPuck_Load(struct BCarTextReader *reader)
{
	struct BCarPuckInfo *puck;
	char word[512];
	char buf[512];
	char *line;
	float angle;
	int status;
	int i;

	puck = (struct BCarPuckInfo *)BCB_NEW(0xec, s_PuckFile, 0x18);
	if (puck == NULL)
		return;
	buf[0] = '\0';
	puck->actor = NULL;
	puck->scale = 1.0f;
	puck->entity = NULL;
	puck->next = NULL;
	puck->disableModel = NULL;
	puck->frame = 0;
	puck->colourTexture = NULL;
	for (i = 0; i < 0x34; i++)
		((unsigned int *)&puck->params)[i] = 0;
	line = buf;
	while (reader->cursor < reader->end)
	{
		if (_stricmp(line, "}") == 0)
			break;
		line = BCarText_ReadLine(reader, buf);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0)
			continue;
		status = 0;
		if (_strnicmp(line, "MODEL ", 6) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, word, 0x1ff) != 0)
			{
				puck->actor = (struct BActor *)bLoadResource(NULL, word, 1, 0xdefa);
				status = 1;
			}
		}
		if (_strnicmp(line, "DISABLEMODEL ", 0xd) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, word, 0x1ff) != 0)
			{
				puck->disableModel = (struct BActor *)bLoadResource(NULL, word, 1, 0xdefa);
				status = 1;
			}
		}
		if (_strnicmp(line, "SCALE ", 6) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &puck->scale) == 1)
				status = 1;
		}
		if (_strnicmp(line, "RESPAWN_DELAY ", 0xe) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &puck->params.respawnDelay) == 1)
				status = 1;
		}
		if (_strnicmp(line, "ANGLE ", 6) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &angle) == 1)
			{
				puck->params.angle = angle * 0.017453292f;   /* 0x002640f8 */
				status = 1;
			}
		}
		if (_strnicmp(line, "POSITION ", 9) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f%f%f", &puck->params.pos[0], &puck->params.pos[1],
				    &puck->params.pos[2]) == 3)
				status = 1;
		}
		if (_strnicmp(line, "MASS ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &puck->params.mass) == 1)
				status = 1;
		}
		if (_strnicmp(line, "RADIUS ", 7) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &puck->params.radius) == 1)
				status = 1;
		}
		if (_strnicmp(line, "DRAWOFFSET ", 0xb) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &puck->params.drawOffset) == 1)
				status = 1;
		}
		if (_strnicmp(line, "PL_COLOUR_TEX ", 0xe) == 0)
		{
			status = 2;
			if (BCarText_GetName(line, word, 0x1ff) != 0)
			{
				puck->colourTexture = (struct bdTexture *)bLoadResource(NULL, word, 0, 0xdefa);
				status = 1;
			}
		}
		if (_stricmp(line, "}") != 0)
		{
			sscanf(line, "%s", word);
			bcb_BadCommand(reader, s_PuckFile, 0x98, "Puck_LoadInformation", status, word, line);
		}
	}
	puck->params.carriedOffset = puck->params.drawOffset;
	puck->next = g_PuckList;
	g_PuckList = puck;
}

/* 0x000a11b0  (gap; file inferred: CBCar_Puck.cpp)
Pucks_Update(playing): the colour frame of each puck follows the car that hit it last; a
puck in a goal scores PUCK_GOAL for that car (with a cheer) and respawns. */
void BCarPucks_Update(int playing)
{
	struct BCarPuckInfo *puck;

	for (puck = g_PuckList; puck != NULL; puck = puck->next)
	{
		struct BCarEntity *e = puck->entity;
		float pos[4];

		puck->frame = 0;
		if (e->lastHit != NULL)
		{
			if (g_BCarPlayers[0].entity == e->lastHit)
				puck->frame = DAT_002baed4[0] + 1;
			if (g_BCarPlayers[1].entity == e->lastHit)
				puck->frame = DAT_002baed4[10] + 1;
			if (g_BCarPlayers[2].entity == e->lastHit)
				puck->frame = DAT_002baed4[20] + 1;
			if (g_BCarPlayers[3].entity == e->lastHit)
				puck->frame = DAT_002baed4[30] + 1;
		}
		if (e->scored == 0 || playing == 0)
			continue;
		pos[0] = e->pos[0];
		pos[2] = e->pos[2];
		pos[3] = e->pos[3];
		pos[1] = e->pos[1] + puck->entity->drawOffset;
		BCarEffects_Burst(BCB_EFFECTS, 1, pos, DAT_002ea478, 0, -1);
		if (puck->entity->lastHit != NULL)
			BCarEntity_AddScore(puck->entity->lastHit, g_Score(0x002efa1c));
		if (bcb_rand() % 0x19 == 0)
		{
			int i;

			for (i = 0; i < BCB_NUM_PLAYERS; i++)
			{
				struct BCarEntity *car = g_BCarPlayers[i].entity;

				if (car == puck->entity->lastHit)
					BCarSound_Speak(BCB_SOUND, i, (bcb_rand() % 2 == 0) ? 4 : 0xb, 0xff, car->pos, 0.0f);
			}
		}
		BCarEntity_Respawn(puck->entity);
	}
}

/* 0x000a1390  (gap; file inferred: CBCar_Puck.cpp)
Pucks_Delete: frees the puck list. */
void BCarPucks_Delete(void)
{
	struct BCarPuckInfo *puck = g_PuckList;

	while (puck != NULL)
	{
		struct BCarPuckInfo *next = puck->next;

		BCB_DELETE(puck);
		puck = next;
	}
	g_PuckList = NULL;
}

/* 0x000a13d0  (gap; file inferred: CBCar_Puck.cpp)
Pucks_Draw(glow, glowPass): draws the live pucks in the colour of the last car that hit
them, with their floor reflection; in the glow pass only the fast ones, brighter with speed. */
void BCarPucks_Draw(int glow, int glowPass)
{
	struct BCarPuckInfo *puck;
	int passes = glow != 0 ? 6 : 1;

	bdPushRenderState(10, 0, 0);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(8, 0, 0);
	bdPushRenderState(3, 1, 0);
	for (puck = g_PuckList; puck != NULL; puck = puck->next)
	{
		struct BCarEntity *e = puck->entity;

		if (e->dead != 0 || e->respawnTime != 0.0f)
			continue;
		if (glowPass != 0 && bcb_SpeedBrightness(puck->entity) == 0)
			continue;
		if (puck->colourTexture != NULL)
			bdTextureSetFrame(puck->colourTexture, puck->frame);
		if (BCB_DEBUG_GET(FUN_0008c040) != 0 && glowPass == 0)
		{
			BCB_CALL(FUN_001d6820, void (*)(void))();
			BCB_CALL(FUN_001d6860, void (*)(void))();
			BCarRender_BeginReflection(0);
			bcb_DrawReflected(puck->entity->inst, passes);
			BCarRender_EndReflection();
			BCB_CALL(FUN_0018e640, void (*)(void))();
		}
		baPrepActorInstance(puck->entity->inst, 1);
		baDrawActorInstance(puck->entity->inst, passes);
		if (puck->colourTexture != NULL)
			bdTextureSetFrame(puck->colourTexture, 0);
	}
	if (glowPass != 0)
		baSetBrightness(0x100);
	bdPopRenderState(3);
	bdPopRenderState(8);
	bdPopRenderState(6);
	bdPopRenderState(10);
}
