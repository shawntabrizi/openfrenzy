/*
BumperCars/CBCar_Powerup.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Powerup.cpp

The powerups (list at 0x002f0774, struct BCarPowerupInfo): the POWERUP { } block of the
player file, the entities (type 4, picked up by driving into them), the speed powerups (a
DATA of 1 or more speeds the car up, less than 1 is a bomb that explodes and spins it), the
pulsing light and the smoke of a bomb, respawning at a spawn point, and drawing.
0x000a16c0 is tagged with CBCar_Powerup.cpp; the gap functions after it are powerup
functions too (file inferred).
*/

#include "bcb_items.h"

static const char s_PowerupFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CBCar_Powerup.cpp";

/* 0x000a16c0  BumperCars/CBCar_Powerup.cpp
Powerup_LoadInformation: reads a POWERUP { } block (MODEL, SCALE, TYPE, LIFE, RESPAWN_DELAY,
DATA, RADIUS, DRAWOFFSET), makes the powerup's point light and adds it to the list. */
void BCarPowerup_Load(struct BCarTextReader *reader)
{
	struct BCarPowerupInfo *powerup;
	char word[512];
	char buf[512];
	char *line;
	int status;
	int i;

	powerup = (struct BCarPowerupInfo *)BCB_NEW(0xf4, s_PowerupFile, 0x18);
	if (powerup == NULL)
		return;
	powerup->actor = NULL;
	powerup->scale = 1.0f;
	powerup->entity = NULL;
	powerup->data = 0.0f;
	powerup->type = 0;
	powerup->next = NULL;
	powerup->life = -1.0f;
	powerup->lifeLeft = -1.0f;
	powerup->light = NULL;
	for (i = 0; i < 0x34; i++)
		((unsigned int *)&powerup->params)[i] = 0;
	buf[0] = '\0';
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
				powerup->actor = bLoadResource(NULL, word, 1, 0xdefa);
				status = 1;
			}
		}
		if (_strnicmp(line, "SCALE ", 6) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &powerup->scale) == 1)
				status = 1;
		}
		if (_strnicmp(line, "TYPE ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%d", &powerup->type) == 1)
				status = 1;
		}
		if (_strnicmp(line, "LIFE ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &powerup->life) == 1)
				status = 1;
		}
		if (_strnicmp(line, "RESPAWN_DELAY ", 0xe) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &powerup->params.respawnDelay) == 1)
				status = 1;
		}
		if (_strnicmp(line, "DATA ", 5) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &powerup->data) == 1)
				status = 1;
		}
		if (_strnicmp(line, "RADIUS ", 7) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &powerup->params.radius) == 1)
				status = 1;
		}
		/* DRAWOFFSET goes to the carried offset (params + 0xa8) in this file */
		if (_strnicmp(line, "DRAWOFFSET ", 0xb) == 0)
		{
			status = 2;
			if (sscanf(line, "%*s%f", &powerup->params.carriedOffset) == 1)
				status = 1;
		}
		if (_stricmp(line, "}") != 0)
		{
			sscanf(line, "%s", word);
			bcb_BadCommand(reader, s_PowerupFile, 0x76, "Powerup_LoadInformation", status, word, line);
		}
	}
	if (0.0f < powerup->life)
		powerup->lifeLeft = powerup->life;
	powerup->light = baCreateLight(NULL, 0);
	if (powerup->light != NULL)
	{
		baSetPointLight(powerup->light, DAT_002ea478, 0.001f);   /* 0x3a83126f */
		baSetLightColour(powerup->light, 0x7f, 0x7f, 0x7f);
		baEnableLight(powerup->light, 1);
	}
	powerup->next = g_PowerupList;
	g_PowerupList = powerup;
}

/* 0x000a1a70  (gap; file inferred: CBCar_Powerup.cpp)
Powerups_Delete: frees the powerups and their lights. */
void BCarPowerups_Delete(void)
{
	struct BCarPowerupInfo *powerup = g_PowerupList;

	while (powerup != NULL)
	{
		struct BCarPowerupInfo *next = powerup->next;

		if (powerup->light != NULL)
			baDeleteLight(powerup->light);
		BCB_DELETE(powerup);
		powerup = next;
	}
	g_PowerupList = NULL;
}

/* 0x000a1ac0  (gap; file inferred: CBCar_Powerup.cpp)
Powerup_Respawn: puts a powerup entity on the best spawn point, standing on the ground, and
restarts its life. */
void BCarPowerup_Respawn(struct BCarEntity *entity)
{
	struct BCarPowerupInfo *powerup;
	struct BCarSpawn *spawn;
	float y;

	for (powerup = g_PowerupList; powerup != NULL && powerup->entity != entity;
	     powerup = powerup->next)
	{
	}
	spawn = BCB_CALL(FUN_0009f030, struct BCarSpawn *(*)(struct BCarEntity *))(entity);
	if (spawn == NULL)
		return;
	entity->pos[0] = spawn->pos[0];
	entity->pos[1] = spawn->pos[1];
	entity->pos[2] = spawn->pos[2];
	entity->pos[3] = spawn->pos[3];
	y = entity->pos[1];
	entity->pos[1] = y - (BCarWorld_GetGroundHeight(entity->pos, entity->radius, NULL) - 1.0f);
	entity->heading = spawn->angle;
	entity->targetHeading = spawn->angle;
	if (powerup != NULL && 0.0f < powerup->life)
		powerup->lifeLeft = powerup->life;
}

/* 0x000a1b60  (gap; file inferred: CBCar_Powerup.cpp)
Powerup_Hit(car, item): a car drove into powerup `item`. A speed powerup scales the car's
speed by DATA; a DATA below 1 is a bomb: an explosion, a sound, a rumble and the car spins.
The powerup respawns. */
void BCarPowerup_Hit(struct BCarEntity *car, struct BCarEntity *item)
{
	struct BCarPowerupInfo *powerup;

	for (powerup = g_PowerupList; powerup != NULL; powerup = powerup->next)
	{
		int index;

		if (powerup->entity != item)
			continue;
		index = -1;
		if (g_BCarPlayers[0].entity == car)
			index = 0;
		if (g_BCarPlayers[1].entity == car)
			index = 1;
		if (g_BCarPlayers[2].entity == car)
			index = 2;
		if (g_BCarPlayers[3].entity == car)
			index = 3;
		if (powerup->type != BCAR_POWERUP_SPEED)
			continue;
		car->speedScale = powerup->data * car->speedScale;
		if (1.0f <= powerup->data)
		{
			if (index != -1 && bcb_rand() % 5 == 0)
				BCarSound_Speak(BCB_SOUND, index, 8, 0xff, car->pos, 0.0f);
		}
		else
		{
			float pos[4];

			if (index != -1 && bcb_rand() % 5 == 0)
				BCarSound_Speak(BCB_SOUND, index, 0xe, 0xff, car->pos, 0.0f);
			pos[0] = item->pos[0];
			pos[2] = item->pos[2];
			pos[1] = item->pos[1] + item->drawOffset;
			pos[3] = item->pos[3];
			BCarExplosions_Add(BCB_VAR(struct BCarExplosions *, 0x002f2040), pos, 1000.0f);
			BCarSound_Play(BCB_SOUND, bcb_rand() % 5 + 0x11, 0xff, 0, -1);
			if (index != -1)
				BCarGame_Rumble(index, 1, 1.0f);
			BCB_CALL(FUN_0009c210, void (*)(struct BCarEntity *, float))(car, 1.5f);
		}
	}
	BCarPowerup_Respawn(item);
}

/* 0x000a1d30  (gap; file inferred: CBCar_Powerup.cpp)
Powerups_Draw(draw): draws the live powerups with their floor reflection. */
void BCarPowerups_Draw(int draw)
{
	struct BCarPowerupInfo *powerup;

	if (draw == 0)
		return;
	bdPushRenderState(10, 0, 0);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(8, 1, 0);
	bdPushRenderState(3, 0, 0);
	for (powerup = g_PowerupList; powerup != NULL; powerup = powerup->next)
	{
		if (powerup->entity->dead != 0 || powerup->entity->respawnTime != 0.0f)
			continue;
		if (BCB_DEBUG_GET(FUN_0008c040) != 0)
		{
			BCB_CALL(FUN_001d6820, void (*)(void))();
			BCB_CALL(FUN_001d6860, void (*)(void))();
			BCarRender_BeginReflection(0);
			bcb_DrawReflected(powerup->entity->inst, 1);
			BCarRender_EndReflection();
			BCB_CALL(FUN_0018e640, void (*)(void))();
		}
		baPrepActorInstance(powerup->entity->inst, 1);
		baDrawActorInstance(powerup->entity->inst, 1);
	}
	bdPopRenderState(3);
	bdPopRenderState(8);
	bdPopRenderState(6);
	bdPopRenderState(10);
}

/* 0x000a1f40  (gap; file inferred: CBCar_Powerup.cpp)
Powerups_Init: makes the entity of every powerup (type 4, can be picked up, 1.3 times wider)
and places it. Returns 0 when a powerup has no model or its entity fails. */
int BCarPowerups_Init(void)
{
	struct BCarPowerupInfo *powerup;

	for (powerup = g_PowerupList; powerup != NULL; powerup = powerup->next)
	{
		if (powerup->actor == NULL)
		{
			bkPrintf("Player_Init - Couldn't find powerup actor\n");
			return 0;
		}
		powerup->entity = BCarEntity_Create(powerup->actor, NULL, powerup->scale, &powerup->params);
		if (powerup->entity == NULL)
		{
			bkPrintf("Player_Init - Couldn't initialise powerup entity\n");
			return 0;
		}
		BCarEntity_SetType(powerup->entity, BCAR_ENT_POWERUP);
		powerup->entity->pickup = 1;
		powerup->entity->inst->scale[0] = powerup->entity->inst->scale[0] * 1.3f;   /* 0x00266448 */
		powerup->entity->inst->scale[2] = powerup->entity->inst->scale[2] * 1.3f;
		BCarPowerup_Respawn(powerup->entity);
	}
	return 1;
}

/* the smoke and spark velocity of a bomb: three rand() steps, the last one scales it */
static void bcb_BombVelocity(float *vel, float y)
{
	float r;

	bcb_rand();
	bcb_rand();
	r = (float)bcb_rand();
	vel[0] = 0.00015259255f * r;   /* 0x00266454 */
	vel[3] = 1.0f;
	vel[1] = y * r;
	vel[2] = r * 0.00015564437f;   /* 0x00266450 */
}

/* 0x000a2020  (gap; file inferred: CBCar_Powerup.cpp)
Powerups_Update(playing, dt): the life of a powerup runs out (it explodes and moves);
a bomb smokes and flickers red, a speed powerup pulses white. */
void BCarPowerups_Update(int playing, float dt)
{
	struct BCarPowerupInfo *powerup;

	for (powerup = g_PowerupList; powerup != NULL; powerup = powerup->next)
	{
		struct BCarEntity *e = powerup->entity;
		float pos[4];
		float vel[4];
		float fraction;
		int value;

		pos[0] = e->pos[0];
		pos[2] = e->pos[2];
		pos[3] = e->pos[3];
		pos[1] = (e->pos[1] + powerup->entity->drawOffset) - BCB_F32(powerup->actor, 0x5c);
		if (playing != 0 && 0.0f < powerup->life)
		{
			powerup->lifeLeft = powerup->lifeLeft - dt * 0.016666668f;   /* 0x00262b7c */
			if (powerup->lifeLeft <= 0.0f)
			{
				e = powerup->entity;
				pos[0] = e->pos[0];
				pos[2] = e->pos[2];
				pos[3] = e->pos[3];
				pos[1] = e->pos[1] + powerup->entity->drawOffset;
				BCarExplosions_Add(BCB_VAR(struct BCarExplosions *, 0x002f2040), pos, 300.0f);
				BCarSound_Play(BCB_SOUND, bcb_rand() % 5 + 0x11, 0x7f, 0, -1);
				BCarPowerup_Respawn(powerup->entity);
			}
		}
		fraction = powerup->lifeLeft / powerup->life;
		if (powerup->type != BCAR_POWERUP_SPEED || !(powerup->data < 1.0f))
		{
			/* a good powerup: a white pulse */
			if (powerup->light != NULL)
			{
				value = BCB_FTOL(sinf((float)DAT_002ba99c * 0.05f) * (fraction * 64.0f)) + 0x40; /* 0x00258b54, 0x00253f38 */
				baSetPointLight(powerup->light, powerup->entity->inst->pos, 0.001f);
				baSetLightColour(powerup->light, value, value, value);
			}
		}
		else
		{
			unsigned int colour;
			int n;

			/* a bomb: smoke, sparks and a red flicker */
			if (bcb_rand() % 3 == 0)
			{
				bcb_BombVelocity(vel, 0.00091555528f);   /* 0x00265804 */
				BCB_F32(bcb_EffectsAdd(4, pos, vel, 0, -1), 0x8c) = 0.2f;
			}
			if (bcb_rand() % 3 == 0)
			{
				bcb_BombVelocity(vel, 0.00091555528f);
				BCB_F32(bcb_EffectsAdd(0, pos, vel, 0, -1), 0x8c) = 0.2f;
			}
			if (powerup->life > 0.0f)
				colour = (unsigned int)(((BCB_FTOL((1.0f - fraction) * 127.0f) & 0xff) * 0x100 +
					(BCB_FTOL(fraction * 127.0f) & 0xff)) * 0x100);    /* 0x00255348 */
			else
				colour = 0xffffffffu;
			for (n = 4; n != 0; n--)
			{
				bcb_BombVelocity(vel, 0.0018311106f);  /* 0x0026644c */
				BCB_F32(bcb_EffectsAdd(10, pos, vel, 0, (int)colour), 0x8c) = 0.3f;
			}
			if (powerup->light != NULL)
			{
				float t = (float)DAT_002ba99c * 0.05f;

				baSetPointLight(powerup->light, powerup->entity->inst->pos, 0.001f);
				value = BCB_FTOL(sinf(t) * (fraction * 64.0f)) + 0x40;
				baSetLightColour(powerup->light, value, 0, 0);
			}
		}
	}
}

/* 0x000a24c0  (gap; file inferred: CBCar_Powerup.cpp)
Entity_DropCarried: the entity carried by `self` respawns and can be picked up again. */
void BCarEntity_DropCarried(struct BCarEntity *self)
{
	if (self->carried != NULL)
	{
		BCarEntity_Respawn(self->carried);
		self->carried->pickup = 1;
		self->carried = NULL;
	}
}
