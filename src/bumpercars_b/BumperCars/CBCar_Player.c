/*
BumperCars/CBCar_Player.c  <-  C:\BlitzParty\Game\Src\BumperCars\CBCar_Player.cpp

The 4 players of BumperCars (struct BCarPlayer at 0x002efba8): the player file (models,
start and end points, characters), the pad input maps, braking and boosting, the engine and
skid sounds, the "lead player" marker and the drawing of the cars with their drivers.

0x00096d90, 0x000982f0 and 0x0009a170 are tagged with CBCar_Player.cpp; the functions
between them are bracketed. The gap functions 0x00096680-0x00096d00 before them work on the
same player table (file inferred).

Objects of other modules reached here (thiscall `this` values of the original):
  0x002f0778  the BumperCars sound object (bumpercars_a CBCar_Sound)
  0x002ed4b4  the flame effects (bumpercars_a CBCar_Flame)
  0x002efa98  the game object (bumpercars_a: ranks, camera, debug switches)
  0x002ecfac  the HUD (classlib_ui)
*/

#include "bumpercars_b.h"
#include "bumpercars_b_internal.h"

/* 0x00096680  (gap; file inferred)
Starts the engine sound of every player (sample 0xc at the minimum engine volume). */
void BCarPlayers_StartEngineSounds(void)
{
	int i;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
		g_BCarPlayers[i].engineChannel = BCarSound_Play(BCB_SOUND, 0xc, g_EngineVolumeMin, 0, -1);
}

/* 0x000966b0  (gap; file inferred)
Clears a player before the player file is read. */
void BCarPlayer_Clear(struct BCarPlayer *self)
{
	int i;

	self->carModelLow = NULL;
	self->carModel = NULL;
	self->driverModel[1] = NULL;
	self->driverModel[0] = NULL;
	self->driverInst[1] = NULL;
	self->driverInst[0] = NULL;
	self->animDrive = NULL;
	self->animWinLoop = NULL;
	self->animLoseLoop = NULL;
	self->animVictoryStart = NULL;
	self->animVictoryLoop = NULL;
	self->animVictoryEnd = NULL;
	self->unk250 = -1;
	self->brakeChannel = -1;
	self->engineChannel = -1;
	self->unk2b0 = 0;
	self->aiLevel[2] = 0;
	self->aiLevel[1] = 0;
	self->aiLevel[0] = 0;
	self->boostReadyTime = 0.0f;
	self->boostCount = 0;
	self->brakeModel = NULL;
	self->brakeInst = NULL;
	self->braking = 0;
	self->boostModel = NULL;
	self->boostInst = NULL;
	self->boostReady = 0;
	self->disableModel = NULL;
	self->entity = NULL;
	self->human = 0;
	self->overlay = 0;
	self->skidPuffTime = 0.0f;
	self->lowDetail = 0;
	self->headlightTexture = NULL;
	self->headlights[0] = NULL;
	self->headlights[1] = NULL;
	self->headlights[2] = NULL;
	self->headlightsOn = 0;
	self->scale = 1.0f;
	self->skidPuffs = 6;
	for (i = 0; i < 0x34; i++)
		((int *)&self->params)[i] = 0;
}

/* 0x000967b0  (gap; file inferred)
Creates the input map of player `index`: steering, accelerating, boost and brake on the pad
of the player (DAT_002baee0 holds the port bits) and a keyboard layout for each player. */
void BCarPlayer_CreateInput(int index)
{
	struct BCarPlayer *player = &g_BCarPlayers[index];
	unsigned int port = (unsigned int)DAT_002baee0[index * 10];
	struct biChannel *channel;
	unsigned int key;

	player->inputMap = (struct biMap *)biCreateMap(6);
	player->steer = biCreateChannel(player->inputMap, "left/right", 3, 0, 0, 0, 0);
	player->accel = biCreateChannel(player->inputMap, "accelerate", 3, 0, 0, 0, 0);
	player->steer2 = biCreateChannel(player->inputMap, "left/right", 3, 0, 0, 0, 0);
	player->accel2 = biCreateChannel(player->inputMap, "accelerate", 3, 0, 0, 0, 0);
	player->boost = biCreateChannel(player->inputMap, "boost", 1, 1, 0, 0, 0);
	player->brake = biCreateChannel(player->inputMap, "brake", 1, 0, 0, 0, 0);
	biBindInput(player->steer, port | 0xc0, port | 0xbf);
	biBindInput(player->accel, port | 0xbe, port | 0xbd);
	biBindInput(player->steer, port | 0xb9, 0);
	biBindInput(player->accel, port | 0xba, 0);
	biBindInput(player->steer2, port | 0xbb, 0);
	biBindInput(player->accel2, port | 0xbc, 0);
	biBindInput(player->boost, port | 0xc5, 0);
	biBindInput(player->brake, port | 0xc7, 0);
	biBindInput(player->brake, port | 0xc6, 0);
	switch (index)
	{
	case 0:
		biBindInput(player->steer, 0x9d, 0x9c);
		biBindInput(player->accel, 0x9b, 0x9a);
		biBindInput(player->boost, 0x69, 0);
		key = 0x67;
		break;
	case 1:
		biBindInput(player->steer, 0x48, 0x45);
		biBindInput(player->accel, 0x57, 0x5b);
		biBindInput(player->boost, 0x68, 0);
		key = 0x66;
		break;
	case 2:
		biBindInput(player->steer, 0x50, 0x4e);
		biBindInput(player->accel, 0x4f, 0x4d);
		biBindInput(player->boost, 0x4c, 0);
		key = 0x52;
		break;
	case 3:
		biBindInput(player->steer, 0x88, 0x86);
		biBindInput(player->accel, 0x84, 0x8a);
		biBindInput(player->boost, 0x83, 0);
		key = 0x82;
		break;
	default:
		goto done;
	}
	channel = player->brake;
	biBindInput(channel, key, 0);
done:
	player->unk15c = 0;
	player->unk158 = 0;
}

/* 0x00096b00  (gap; file inferred)
Deletes the input map of player `index`. */
void BCarPlayer_DeleteInput(int index)
{
	struct BCarPlayer *player = &g_BCarPlayers[index];
	struct biMap *map;

	if (player->steer != NULL)
		biDeleteChannel(player->steer);
	if (player->accel != NULL)
		biDeleteChannel(player->accel);
	if (player->steer2 != NULL)
		biDeleteChannel(player->steer2);
	if (player->accel2 != NULL)
		biDeleteChannel(player->accel2);
	if (player->brake != NULL)
		biDeleteChannel(player->brake);
	if (player->boost != NULL)
		biDeleteChannel(player->boost);
	map = player->inputMap;
	player->boost = NULL;
	player->brake = NULL;
	player->accel = NULL;
	player->steer = NULL;
	player->accel2 = NULL;
	player->steer2 = NULL;
	if (map != NULL)
		biDeleteMap(map);
	player->inputMap = NULL;
}

/* 0x00096bd0  (gap; file inferred)
Creates the input maps of the human players. */
void BCarPlayers_CreateInputs(void)
{
	int i;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		if (g_BCarPlayers[i].human != 0)
			BCarPlayer_CreateInput(i);
	}
}

/* 0x00096c30  (gap; file inferred)
The top speed of a player: ENTITYSPEED plus the catch-up bonus (WP_LAG_SPEED_OFFSET per
waypoint behind, at most WP_LAG_MAX_OFFSET), times the entity's speed scale (powerups) and, for the AI,
its speed scale. */
float BCarPlayer_GetMaxSpeed(struct BCarPlayer *self)
{
	int index = 0;
	int lag;
	float bonus;
	float speed;

	while (index < BCB_NUM_PLAYERS && &g_BCarPlayers[index] != self)
		index++;
	lag = self->entity->lag;
	if (g_WpLagMaxOffset <= lag)
		lag = g_WpLagMaxOffset;
	speed = BCarWorld_GetEntitySpeed();
	bonus = (float)lag * g_WpLagSpeedOffset;
	if (DAT_00283e58 != 0)
		bonus = bonus * 1.5f;                     /* 0x002538a0 */
	speed = (bonus + speed) * self->entity->speedScale;
	if (self->human == 0)
		speed = speed * g_AISpeedScale[index];
	return speed;
}

/* 0x00096cb0  (gap; file inferred)
Shuts the AI drivers down and deletes the input maps of the human players. */
void BCarPlayers_ShutdownInput(void)
{
	int i;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
		BCB_CALL(FUN_00089710, void (*)(void *))(g_BCarPlayers[i].ai);
	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		if (g_BCarPlayers[i].human != 0)
			BCarPlayer_DeleteInput(i);
	}
}

/* 0x00096d00  (gap; file inferred)
Resets the AI drivers, takes who is human from the player table (core) and makes their
input maps. */
void BCarPlayers_ResetInput(void)
{
	int i;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		BCB_CALL(FUN_00089730, void (*)(void *))(g_BCarPlayers[i].ai);
		if (g_BCarPlayers[i].human == 0 && DAT_002baed8[i * 10] != 0)
			BCB_CALL(FUN_000890f0, void (*)(void *))(g_BCarPlayers[i].ai);
	}
	g_BCarPlayers[0].human = DAT_002baed8[0];
	g_BCarPlayers[1].human = DAT_002baed8[10];
	g_BCarPlayers[2].human = DAT_002baed8[20];
	g_BCarPlayers[3].human = DAT_002baed8[30];
	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		if (g_BCarPlayers[i].human != 0)
			BCarPlayer_CreateInput(i);
	}
}

/* report a line of the player file that was not understood (status 0) or not used right
(status 2): the original repeats this code at every block */
static void bcb_player_bad_line(struct BCarTextReader *reader, char *line, char *word,
	int status, int lineInvalid, int lineBad)
{
	if (_stricmp(line, "}") == 0)               /* 0x00263ccc */
		return;
	sscanf(line, "%s", word);                     /* 0x002576ec */
	if (status == 0)
		bkPrintf("%s(%d) : Player_LoadInformation(\"%s\", %d): Invalid Command \"%s\"\n",
			BCB_SRC_PLAYER, lineInvalid, reader->name, reader->line, word);
	else if (status == 2)
		bkPrintf("%s(%d) : Player_LoadInformation(\"%s\", %d): Bad use of Command \"%s\" (%s)\n",
			BCB_SRC_PLAYER, lineBad, reader->name, reader->line, word, line);
}

/* load an actor resource named by the rest of `line` into *out; returns the status */
static int bcb_player_load_actor(char *line, char *name, struct BActor **out, const char *error)
{
	if (BCarText_GetName(line, name, 0x1ff) == 0)
		return 2;
	*out = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
	if (*out == NULL)
		bkPrintf(error, name);
	return 1;
}

/* 0x00096d90  BumperCars/CBCar_Player.cpp
Player_LoadInformation: reads the blocks of the player file: player1-4 { }, PUCK { },
KEEPSAKE { }, POWERUP { }, character1-6 { }, startpoint1-4 { } and endpoint1-4 { }. */
void BCarPlayers_LoadInformation(struct BCarTextReader *reader)
{
	char *line;
	int status;
	int i;
	int n;
	struct BCarPlayer *player;
	struct BCarCharacter *character;
	float angle;
	char name[512];
	char buffer[512];

	for (i = 0; i < 4; i++)
	{
		g_StartPoints[i].pos[0] = 0.0f;
		g_StartPoints[i].pos[1] = 0.0f;
		g_StartPoints[i].pos[2] = 0.0f;
		g_StartPoints[i].pos[3] = 1.0f;
		g_EndPoints[i].pos[0] = 0.0f;
		g_EndPoints[i].pos[1] = 0.0f;
		g_EndPoints[i].pos[2] = 0.0f;
		g_EndPoints[i].pos[3] = 1.0f;
		g_StartPoints[i].angle = 0.0f;
		g_EndPoints[i].angle = 0.0f;
		g_Characters[i].driverLow = NULL;
		g_Characters[i].car = NULL;
		g_Characters[i].driver = NULL;
		g_Characters[i].carLow = NULL;
	}
	for (i = 4; i < BCB_NUM_CHARACTERS; i++)
	{
		g_Characters[i].driverLow = NULL;
		g_Characters[i].car = NULL;
		g_Characters[i].driver = NULL;
		g_Characters[i].carLow = NULL;
	}

	while (reader->cursor < reader->end)
	{
		line = BCarText_ReadLine(reader, buffer);
		BCarText_NextLine(reader);
		if (strncmp(line, "//", 2) == 0)
			continue;

		/* player1 { } .. player4 { } */
		for (i = 0; i < BCB_NUM_PLAYERS; i++)
		{
			player = &g_BCarPlayers[i];
			snprintf(name, 0x1ff, "player%d {", i + 1);
			if (_stricmp(line, name) != 0)
				continue;
			BCarPlayer_Clear(player);
			while (reader->cursor < reader->end)
			{
				if (_stricmp(line, "}") == 0)
					break;
				line = BCarText_ReadLine(reader, buffer);
				BCarText_NextLine(reader);
				if (strncmp(line, "//", 2) == 0)
					continue;
				status = 0;
				if (_strnicmp(line, "DISABLEMODEL ", 0xd) == 0)
				{
					status = 2;
					if (BCarText_GetName(line, name, 0x1ff) != 0)
					{
						player->disableModel = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
						status = 1;
					}
				}
				if (_strnicmp(line, "BRAKEMODEL ", 0xb) == 0)
				{
					status = 2;
					if (BCarText_GetName(line, name, 0x1ff) != 0)
					{
						player->brakeModel = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
						if (player->brakeModel == NULL)
							bkPrintf("Player_Init: Could not load Brake Model [%s]\n", name);
						else
						{
							player->brakeInst = baInitActorInstance(NULL, player->brakeModel, 1,
								BCB_SRC_PLAYER, 0x1a9);
							if (player->brakeInst != NULL)
							{
								baPrepActorInstance(player->brakeInst, 1);
								status = 1;
							}
						}
					}
				}
				if (_strnicmp(line, "BOOSTMODEL ", 0xb) == 0)
				{
					status = 2;
					if (BCarText_GetName(line, name, 0x1ff) != 0)
					{
						player->boostModel = (struct BActor *)bLoadResource(NULL, name, 1, 0xdefa);
						if (player->boostModel == NULL)
							bkPrintf("Player_Init: Could not load Boost Model [%s]\n", name);
						else
						{
							player->boostInst = baInitActorInstance(NULL, player->boostModel, 1,
								BCB_SRC_PLAYER, 0x1bf);
							if (player->boostInst != NULL)
							{
								baPrepActorInstance(player->boostInst, 1);
								status = 1;
							}
						}
					}
				}
				if (_strnicmp(line, "HEADLIGHT ", 10) == 0)
				{
					status = 2;
					if (BCarText_GetName(line, name, 0x1ff) != 0)
					{
						player->headlightTexture =
							(struct bdTexture *)bLoadResource(NULL, name, 0, 0xdefa);
						if (player->headlightTexture == NULL)
							bkPrintf("Player_Init: Could not load Headlight Texture [%s]\n", name);
						else
							status = 1;
					}
				}
				if (_strnicmp(line, "SCALE ", 6) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f", &player->scale) == 1)
						status = 1;
				}
				if (_strnicmp(line, "ANGLE ", 6) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f", &angle) == 1)
					{
						player->params.angle = angle * 0.017453292f; /* 0x002640f8 */
						status = 1;
					}
				}
				if (_strnicmp(line, "POSITION ", 9) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f%f%f", &player->params.pos[0], &player->params.pos[1],
						&player->params.pos[2]) == 3)
						status = 1;
				}
				if (_strnicmp(line, "MASS ", 5) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f", &player->params.mass) == 1)
						status = 1;
				}
				if (_strnicmp(line, "RESPAWN_DELAY ", 0xe) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f", &player->params.respawnDelay) == 1)
						status = 1;
				}
				if (_strnicmp(line, "RADIUS ", 7) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f", &player->params.radius) == 1)
						status = 1;
				}
				if (_strnicmp(line, "DRAWOFFSET ", 0xb) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%f", &player->params.drawOffset) == 1)
						status = 1;
				}
				if (_strnicmp(line, "AI_EASY ", 8) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%d", &player->aiLevel[0]) == 1)
						status = 1;
				}
				if (_strnicmp(line, "AI_MEDIUM ", 10) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%d", &player->aiLevel[1]) == 1)
						status = 1;
				}
				if (_strnicmp(line, "AI_HARD ", 8) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%d", &player->aiLevel[2]) == 1)
						status = 1;
				}
				if (_strnicmp(line, "OVERLAY ", 8) == 0)
				{
					status = 2;
					if (sscanf(line, "%*s%d", &player->overlay) == 1)
						status = 1;
				}
				bcb_player_bad_line(reader, line, name, status, 0x236, 0x238);
			}
			player->params.carriedOffset = player->params.drawOffset;
		}

		if (_stricmp(line, "PUCK {") == 0)
		{
			BCarPuck_Load(reader);
			*line = '\0';
		}
		if (_stricmp(line, "KEEPSAKE {") == 0)
		{
			BCarKeepsake_Load(reader);
			*line = '\0';
		}
		if (_stricmp(line, "POWERUP {") == 0)
		{
			BCarPowerup_Load(reader);
			*line = '\0';
		}

		/* character1 { } .. character6 { } */
		for (i = 0; i < BCB_NUM_CHARACTERS; i++)
		{
			character = &g_Characters[i];
			snprintf(name, 0x1ff, "character%d {", i + 1);
			if (_stricmp(line, name) != 0)
				continue;
			while (reader->cursor < reader->end)
			{
				if (_stricmp(line, "}") == 0)
					break;
				line = BCarText_ReadLine(reader, buffer);
				BCarText_NextLine(reader);
				if (strncmp(line, "//", 2) == 0)
					continue;
				status = 0;
				if (_strnicmp(line, "PLAYER ", 7) == 0)
					status = bcb_player_load_actor(line, name, &character->driver,
						"Player_Init: Could not load High Detail Player Model [%s]\n");
				if (_strnicmp(line, "MODEL ", 6) == 0)
					status = bcb_player_load_actor(line, name, &character->car,
						"Player_Init: Could not load High Detail Car Model [%s]\n");
				if (_strnicmp(line, "LOWPLAYER ", 10) == 0)
					status = bcb_player_load_actor(line, name, &character->driverLow,
						"Player_Init: Could not load Low Detail Player Model [%s]\n");
				if (_strnicmp(line, "LOWMODEL ", 9) == 0)
					status = bcb_player_load_actor(line, name, &character->carLow,
						"Player_Init: Could not load Low Detail Car Model [%s]\n");
				bcb_player_bad_line(reader, line, name, status, 0x2ae, 0x2b0);
			}
		}

		/* startpoint1 { } / endpoint1 { } .. startpoint4 { } / endpoint4 { } */
		for (n = 1; n <= 4; n++)
		{
			struct BCarPoint *start = &g_StartPoints[n - 1];
			struct BCarPoint *end = &g_EndPoints[n - 1];

			snprintf(name, 0x1ff, "startpoint%d {", n);
			if (_stricmp(line, name) == 0)
			{
				while (reader->cursor < reader->end)
				{
					if (_stricmp(line, "}") == 0)
						break;
					line = BCarText_ReadLine(reader, buffer);
					BCarText_NextLine(reader);
					if (strncmp(line, "//", 2) == 0)
						continue;
					status = 0;
					if (_strnicmp(line, "POSITION ", 9) == 0)
					{
						status = 2;
						if (sscanf(line, "%*s%f%f%f", &start->pos[0], &start->pos[1],
							&start->pos[2]) == 3)
						{
							status = 1;
							start->pos[3] = 1.0f;
						}
						else
							bkPrintf("Could not load starting point [%d]\n", n);
					}
					if (_strnicmp(line, "ANGLE ", 6) == 0)
					{
						status = 2;
						if (sscanf(line, "%*s%f", &angle) == 1)
						{
							start->angle = angle * 0.017453292f;  /* 0x002640f8 */
							status = 1;
						}
					}
					bcb_player_bad_line(reader, line, name, status, 0x2f2, 0x2f4);
				}
			}
			snprintf(name, 0x1ff, "endpoint%d {", n);
			if (_stricmp(line, name) == 0)
			{
				while (reader->cursor < reader->end)
				{
					if (_stricmp(line, "}") == 0)
						break;
					line = BCarText_ReadLine(reader, buffer);
					BCarText_NextLine(reader);
					if (strncmp(line, "//", 2) == 0)
						continue;
					status = 0;
					if (_strnicmp(line, "POSITION ", 9) == 0)
					{
						status = 2;
						if (sscanf(line, "%*s%f%f%f", &end->pos[0], &end->pos[1],
							&end->pos[2]) == 3)
						{
							status = 1;
							end->pos[3] = 1.0f;
						}
						else
							bkPrintf("Could not load ending point [%d]\n", n);
					}
					if (_strnicmp(line, "ANGLE ", 6) == 0)
					{
						status = 2;
						if (sscanf(line, "%*s%f", &angle) == 1)
						{
							end->angle = angle * 0.017453292f;    /* 0x002640f8 */
							status = 1;
						}
					}
					bcb_player_bad_line(reader, line, name, status, 0x332, 0x334);
				}
			}
		}
	}
}

/* 0x00097b20  BumperCars/CBCar_Player.cpp (bracketed)
Stops braking: silences the brake sound and ends the skid mark. */
void BCarPlayer_StopBrake(struct BCarPlayer *self)
{
	struct BCarEntity *entity;
	float delta[3];
	float pos[4];

	bsStopChannel(self->brakeChannel);
	bsReserveChannel(self->brakeChannel, 0);
	entity = self->entity;
	self->brakeChannel = -1;
	self->skidPuffTime = 0.0f;
	pos[0] = entity->pos[0];
	pos[2] = entity->pos[2];
	pos[3] = entity->pos[3];
	pos[1] = entity->pos[1] + entity->drawOffset;
	delta[0] = entity->pos[0] - entity->oldPos[0];
	delta[1] = entity->pos[1] - entity->oldPos[1];
	delta[2] = entity->pos[2] - entity->oldPos[2];
	if (sqrtf(delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2]) != 0.0f)
		BCarTrack_AddPoint(self->skidTrack, pos, entity->groundNormal, delta, 0.0f);
	BCarTrack_End(self->skidTrack);
	self->skidPuffs = 6;
}

/* 0x00097c10  BumperCars/CBCar_Player.cpp (bracketed)
Brakes: slows the car by BRAKEVALUE, plays the skid sound (louder when faster), lays a skid
mark and blows smoke puffs. Stops braking when the car leaves the ground or slows down. */
void BCarPlayer_Brake(struct BCarPlayer *self)
{
	struct BCarEntity *entity = self->entity;
	float volume;
	float size;
	float alpha;
	float pos[4];
	float delta[4];
	int channel;

	if (entity->dead != 0 || entity->respawnTime != 0.0f)
		return;
	if (g_BrakeValue == 0.0f || entity->spinTime != 0.0f)
	{
		self->braking = 0;
		return;
	}
	volume = (cmb_vec3_length(entity->vel) / BCarWorld_GetMaxSpeed()) * 175.0f + 80.0f; /* 0x00265570, 0x0026556c */
	size = (cmb_vec3_length(self->entity->vel) / BCarWorld_GetMaxSpeed()) * 40.0f + 40.0f; /* 0x00255548 */
	if (!(volume <= 255.0f))                    /* 0x002537d0 */
		volume = 255.0f;
	if (!(size <= 80.0f))                       /* 0x0026556c */
		size = 80.0f;
	entity = self->entity;
	self->braking = 1;
	if (entity->onGround == 0 || entity->falling != 0)
	{
		BCarPlayer_StopBrake(self);
		return;
	}
	entity->force[0] = entity->force[0] - g_BrakeValue * entity->vel[0];
	entity = self->entity;
	entity->force[1] = entity->force[1] - g_BrakeValue * entity->vel[1];
	entity = self->entity;
	entity->force[2] = entity->force[2] - g_BrakeValue * entity->vel[2];
	if (cmb_vec3_length(self->entity->vel) <= 30.0f)    /* 0x002553a8 */
	{
		bsStopChannel(self->brakeChannel);
		entity = self->entity;
		pos[0] = entity->pos[0];
		pos[2] = entity->pos[2];
		pos[3] = entity->pos[3];
		pos[1] = entity->pos[1] + entity->drawOffset;
		Core_Vec3Sub(delta, entity->pos, entity->oldPos);
		if (cmb_vec3_length(delta) != 0.0f)
			BCarTrack_AddPoint(self->skidTrack, pos, entity->groundNormal, delta, 0.0f);
		BCarTrack_End(self->skidTrack);
		self->skidPuffs = 6;
		return;
	}
	channel = self->brakeChannel;
	if (channel == -1)
	{
		channel = BCarSound_Play(BCB_SOUND, (int)Core_Rand() % 2 + 10, BCB_FTOL(volume), 0, -1);
		self->brakeChannel = channel;
		bsReserveChannel(channel, 1);
	}
	else if (BCarSound_IsFree(BCB_SOUND, channel) != 0)
		BCarSound_Play(BCB_SOUND, (int)Core_Rand() % 2 + 10, BCB_FTOL(volume), 0, self->brakeChannel);
	else
		bsSetChannelVolume(self->brakeChannel, (unsigned short)BCB_FTOL(volume));
	BCarEffects_Burst(BCB_EFFECTS, 0, self->entity->groundPoint, DAT_002ea478, 0, -1);
	if (self->skidPuffTime <= BCB_NOW())
	{
		entity = self->entity;
		pos[0] = entity->pos[0];
		pos[2] = entity->pos[2];
		pos[3] = entity->pos[3];
		pos[1] = entity->pos[1] + entity->drawOffset;
		if (self->skidPuffs == 0)
			alpha = size * 0.007874016f;            /* 0x00265568 */
		else
			alpha = (float)(6 - self->skidPuffs) * (size * 0.007874016f) * 0.16666667f; /* 0x00253ff0 */
		Core_Vec3Sub(delta, entity->pos, entity->oldPos);
		if (cmb_vec3_length(delta) != 0.0f)
		{
			BCarTrack_AddPoint(self->skidTrack, pos, entity->groundNormal, delta, alpha);
			if (0 < self->skidPuffs)
				self->skidPuffs--;
			self->skidPuffTime = BCB_NOW() + 0.1f;  /* 0x002536cc */
		}
	}
}

/* 0x00098030  BumperCars/CBCar_Player.cpp (bracketed)
Boosts: when the boost is ready (BOOSTLIMIT not used up), gives the car BOOSTVALUE of thrust,
makes the boost flame for BOOSTLIFE seconds and rumbles the pad. */
void BCarPlayer_Boost(struct BCarPlayer *self)
{
	struct BCarEntity *entity = self->entity;
	int index;

	if (entity->dead != 0 || entity->respawnTime != 0.0f || entity->spinTime != 0.0f ||
		entity->falling != 0)
		return;
	index = 0;
	while (index < BCB_NUM_PLAYERS && &g_BCarPlayers[index] != self)
		index++;
	if (BCarWorld_GetBoostValue() == 0.0f || self->boostReady == 0)
		return;
	if (g_BoostLimit != -1 && g_BoostLimit <= self->boostCount)
		return;
	BCarSound_Play(BCB_SOUND, bcb_rand() % 3 + 6, 0xff, 0, -1);
	entity = self->entity;
	BCarEntity_Thrust(entity, BCarWorld_GetBoostValue() * entity->speedScale, 1);
	self->boostReadyTime = BCB_NOW() + g_BoostRecharge;
	self->boostCount++;
	self->boostReady = 0;
	BCarTimer_SetDuration(&self->entity->boostTimer, g_BoostLife);
	BCarTimer_Start(&self->entity->boostTimer);
	self->entity->boosting = 1;
	BCarGame_Rumble(index, 3, 1.0f);
}

/* 0x000981c0  BumperCars/CBCar_Player.cpp (bracketed)
In the team variant (4) an AI car with a goal drives at its goal: (3200, 270, -800) for team
1, (3200, 470, -1500) for team 2, braking when it is close. */
void BCarPlayer_DriveToGoal(struct BCarPlayer *self)
{
	struct BCarEntity *entity = self->entity;
	float dx;
	float dy;
	float dz;
	float heading;

	if (entity->team == -1)
		return;
	if (entity->team == 1)
	{
		dx = 3200.0f;                               /* 0x00265578 */
		dy = 270.0f;                                /* 0x00262f48 */
		dz = -800.0f;
	}
	else if (entity->team == 2)
	{
		dx = 3200.0f;
		dy = 470.0f;                                /* 0x00265574 */
		dz = -1500.0f;
	}
	else
	{
		dz = 0.0f;
		dx = 0.0f;
		dy = 0.0f;
	}
	dx = dx - entity->pos[0];
	dy = dy - entity->pos[1];
	dz = dz - entity->pos[2];
	heading = BCarMath_Atan2(dx, dz);
	if (60.0f < sqrtf(dz * dz + dx * dx + dy * dy))     /* 0x00254e98 */
	{
		self->entity->frozen = 0;
		BCarEntity_SetHeading(self->entity, heading);
		BCarEntity_Thrust(self->entity, BCarPlayer_GetMaxSpeed(self), 0);
		BCarPlayer_Brake(self);
		return;
	}
	if (self->entity->frozen == 0)
	{
		self->entity->frozen = 1;
		BCarEntity_SetHeading(self->entity, 1.5707963f); /* 0x3fc90fda */
	}
}

/* swap the car model of a player's entity to `car` and the driver to `driverIndex` (the
original repeats this code for the end of the game and back) */
static void bcb_player_swap_car(struct BCarPlayer *player, struct BActor *car, int line,
	int driverIndex)
{
	struct BCarEntity *entity = player->entity;
	struct BActorInstance *driver;
	struct BAnim *drive;
	float m[16];

	entity->actor = car;
	if (entity->inst != NULL)
		baDeleteActorInstance(entity->inst);
	entity->inst = baInitActorInstance(NULL, entity->actor, 1, BCB_SRC_PLAYER, line);
	entity = player->entity;
	bmMatrixRotationXZY(m, entity->rot[0], entity->heading + 3.1415925f, entity->rot[2]); /* 0x00256750 */
	bmMatrixToQuaternion(player->entity->inst->rot, m);
	baPrepActorInstance(player->entity->inst, 1);
	player->entity->inst->scale[2] = player->scale;
	player->entity->inst->scale[1] = player->entity->inst->scale[2];
	player->entity->inst->scale[0] = player->entity->inst->scale[1];
	driver = player->driverInst[driverIndex];
	drive = (struct BAnim *)baFindSubObject(driver, "drive");
	player->animDrive = drive;
	if (drive != NULL)
		baQueueAnimation(driver, driver->root, drive, 0x400, -1, 1, NULL, NULL);
	g_LastColourSet = 0;
}

/* 0x000982f0  BumperCars/CBCar_Player.cpp
Updates the players once per frame (`dt` frames; `control`: the game is running): the car
models at the end of the game, ranks and their speech, the HUD, the skid marks, falling off
the arena (variant 3), scoring and respawning, boost recharge, the engine sound and the
driving: the AI, a pad (the camera-relative or the classic steering) or the team goal. */
void BCarPlayers_Update(float dt, int control)
{
	int i;
	int j;
	int hitIndex;
	struct BCarPlayer *player;
	struct BCarEntity *entity;
	struct BCarEntity *other;
	struct BActorInstance *driver;
	float pos[4];
	float a[4];
	float b[4];
	float level;
	float steer;
	float accel;
	float heading;
	float scale;
	int speech;
	int value;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		player = &g_BCarPlayers[i];
		if (g_BCarGameState == 3 && g_BCarVictoryShown == 0)
		{
			if (player->lowDetail == 0)
			{
				bcb_player_swap_car(player, player->carModelLow, 0x4ca, 1);
				player->lowDetail = 1;
			}
			else
				player->lowDetail = 1;
		}
		else
		{
			if (player->lowDetail == 1)
				bcb_player_swap_car(player, player->carModel, 0x4e0, 0);
			player->lowDetail = 0;
		}

		if (g_BCarVariant == 4)
		{
			player->lastRank = player->rank;
			player->rank = BCarCamera_GetRank(BCB_CAMERA, i);
			if (control != 0)
			{
				if (player->rank < player->lastRank && bcb_rand() % 0x19 == 0)
					BCarSound_Speak(BCB_SOUND, i, 0xb, 0xff, player->entity->pos, 1.0f);
				if (player->lastRank < player->rank && bcb_rand() % 0x19 == 0)
					BCarSound_Speak(BCB_SOUND, i, 0xc, 0xff, player->entity->pos, 1.0f);
			}
		}

		/* the "flag" and "dead" overlays */
		if (player->human == 0 || control == 0)
		{
			BCB_I16(player->flagOverlay, 0x54) = 1;
			BCB_I16(player->deadOverlay, 0x54) = 1;
		}
		else if (player->entity->active == 0)
		{
			BCB_I16(player->deadOverlay, 0x54) = 0;
			BCB_I16(player->flagOverlay, 0x54) = 1;
		}
		else
		{
			BCB_I16(player->deadOverlay, 0x54) = 1;
			if (player->entity->carried == NULL)
				BCB_I16(player->flagOverlay, 0x54) = 1;
			else
				BCB_I16(player->flagOverlay, 0x54) = 0;
		}

		/* the laps on the HUD */
		if ((unsigned int)g_BCarGameState <= 3)
		{
			if (player->entity->team == -1)
			{
				if (g_BCarVariant == 4)
					CLHud_SetFraction(DAT_002ecfac, i, player->entity->lap + 1, g_Laps);
			}
			else if (g_BCarVariant == 4)
				CLHud_SetFraction(DAT_002ecfac, i, g_Laps + 1, g_Laps);
		}

		BCarTrack_Update(player->skidTrack, dt);

		/* falling off the edge of the arena (variant 3) */
		entity = player->entity;
		if (entity->vel[1] < -1.0f && entity->onGround == 0 && g_BCarVariant == 3 &&
			5.29e+06f < entity->pos[2] * entity->pos[2] + entity->pos[0] * entity->pos[0] &&
			entity->falling == 0)                  /* 0x00253854, 0x00265584 */
		{
			entity->falling = 1;
			if (bcb_rand() % 0x19 == 0)
				BCarSound_Speak(BCB_SOUND, i, 0xd, 200, player->entity->pos, 0.0f);
			player->entity->rotVel[0] = player->entity->rot[0];
			player->entity->rotVel[2] = player->entity->rot[2];
		}

		driver = player->driverInst[player->lowDetail];
		if (driver != NULL && player->entity->growing == 0)
			baUpdateAnimations(driver, driver->root, dt * 0.5f, NULL); /* 0x002536d0 */

		/* scoring: the car fell into a goal */
		if (g_BCarScoring != 0)
		{
			entity = player->entity;
			other = entity->lastHit;
			hitIndex = -1;
			if (g_BCarPlayers[0].entity == other)
				hitIndex = 0;
			if (g_BCarPlayers[1].entity == other)
				hitIndex = 1;
			if (g_BCarPlayers[2].entity == other)
				hitIndex = 2;
			if (g_BCarPlayers[3].entity == other)
				hitIndex = 3;
			if (entity->scored != 0 && control != 0)
			{
				pos[0] = entity->pos[0];
				pos[2] = entity->pos[2];
				pos[3] = entity->pos[3];
				pos[1] = entity->pos[1] + entity->drawOffset;
				BCarEffects_Burst(BCB_EFFECTS, 1, pos, DAT_002ea478, 0, -1);
				if (hitIndex == -1)
				{
					if (bcb_rand() % 0x19 == 0)
						BCarSound_Speak(BCB_SOUND, i, 0xe, 0xff, pos, 0.0f);
				}
				else
				{
					if (bcb_rand() % 0x19 == 0)
					{
						speech = (bcb_rand() & 1) == 0 ? 0xc : 0xe;
						BCarSound_Speak(BCB_SOUND, i, speech, 0xff, pos, 0.0f);
					}
					if (bcb_rand() % 0x19 == 0)
					{
						if ((bcb_rand() & 1) == 0)
							BCarSound_Speak(BCB_SOUND, hitIndex, 4, 0xff, pos, 0.0f);
						else
							BCarSound_Speak(BCB_SOUND, hitIndex, 0xb, 0xff, pos, 0.0f);
					}
				}
				BCarEntity_AddScore(player->entity, g_LeadPlayer == player ?
					g_Score(0x002efa54) : g_Score(0x002efa14));   /* LEAD_SELF_GOAL / SELF_GOAL */
				other = player->entity->lastHit;
				if (other != NULL)
				{
					for (j = 0; j < BCB_NUM_PLAYERS; j++)
					{
						if (j != i && other == g_BCarPlayers[j].entity)
						{
							if (BCarEntity_SameTeam(player->entity, other, NULL, NULL) == 0)
								BCarEntity_AddScore(player->entity->lastHit,
									g_LeadPlayer == player ? g_Score(0x002efa58) :
									g_Score(0x002efa18)); /* LEAD_OTHER_GOAL / OTHER_GOAL */
							break;
						}
					}
				}
				BCarEntity_Respawn(player->entity);
			}
		}

		/* the boost recharges */
		if ((g_BoostLimit == -1 || player->boostCount < g_BoostLimit) &&
			player->boostReadyTime <= BCB_NOW())
			player->boostReady = 1;
		player->braking = 0;
		if (player->entity->respawnBlink != 0 || player->entity->growing != 0)
			continue;

		/* the engine sound: volume and pitch follow the speed (or pulse at the start) */
		if (g_BCarGameState == 2)
		{
			level = fabsf(sinf(BCarTimer_GetFraction(&g_BCarTimer.timer) * 120.0f + (float)i)); /* 0x00255b50 */
			if (!(level <= 1.0f))
				level = 1.0f;
		}
		else
		{
			entity = player->entity;
			level = sqrtf(entity->vel[0] * entity->vel[0] + entity->vel[1] * entity->vel[1] +
				entity->vel[2] * entity->vel[2]) / BCarPlayer_GetMaxSpeed(player);
			if (!(level <= 1.0f))
				level = 1.0f;
		}
		bsSetChannelVolume(player->engineChannel, (unsigned short)(BCB_FTOL((float)(g_EngineVolumeMax -
			g_EngineVolumeMin) * level) + g_EngineVolumeMin));
		BCarSound_SetParam(BCB_SOUND, player->engineChannel, 0xc,
			BCB_FTOL((float)(g_EnginePitchMax - g_EnginePitchMin) * level) + g_EnginePitchMin);
		if (control == 0)
			continue;

		/* driving */
		player->unk2d8 = 0;
		if (g_BCarVariant == 4 && player->entity->team != -1)
			BCarPlayer_DriveToGoal(player);
		else if (player->human == 0)
			BCB_CALL(FUN_0008bdd0, void (*)(void *, float))(player->ai, dt);
		else
		{
			if (BCarCamera_GetViewPlayer(BCB_CAMERA, NULL) != 0)
			{
				/* classic steering: the stick turns the car */
				accel = (float)BCB_I32(player->accel, 0x14);
				if (DAT_00283dc0 != 0)
					steer = (float)BCB_I32(player->steer2, 0x14);
				else
					steer = (float)BCB_I32(player->steer, 0x14);
				if (steer != 0.0f)
				{
					steer = steer / (float)BCB_I32(player->steer, 0x1c);
					scale = steer * steer * g_EntityRot;
					if (0.0f <= steer)
						BCarEntity_Turn(player->entity, scale * 0.5f);   /* 0x002536d0 */
					else
						BCarEntity_Turn(player->entity, scale * -0.5f);  /* 0x00257b8c */
				}
				goto thrust;
			}
			if (DAT_00283dc0 != 0)
			{
				steer = (float)BCB_I32(player->steer2, 0x14);
				accel = (float)BCB_I32(player->accel, 0x14);
				if (steer != 0.0f)
					BCarEntity_Turn(player->entity, (g_EntityRot /
						(float)BCB_I32(player->steer, 0x1c)) * steer * 0.3f); /* 0x00265580 */
thrust:
				if (accel != 0.0f)
				{
					scale = (-1.0f / (float)BCB_I32(player->accel, 0x1c)) * accel;
					BCarEntity_Thrust(player->entity, BCarPlayer_GetMaxSpeed(player) * scale, 0);
				}
			}
			else
			{
				/* the stick points where the car drives, relative to the camera */
				value = BCB_I32(player->steer, 0x14);
				if (value != 0 || BCB_I32(player->accel, 0x14) != 0)
				{
					steer = (float)value;
					accel = (float)BCB_I32(player->accel, 0x14);
					heading = BCarMath_Atan2(steer, -accel);
					heading = CLMath_WrapAngle(BCarCamera_GetAngle(BCB_CAMERA) + heading);
					BCarEntity_SetHeading(player->entity, heading);
					scale = BCarPlayer_GetMaxSpeed(player);
					BCarEntity_Thrust(player->entity, sqrtf(accel * accel + steer * steer) *
						2.15845e-05f * scale, 0);     /* 0x0026557c */
				}
			}
			if (BCB_I32(player->brake, 0x14) != 0)
				BCarPlayer_Brake(player);
			if (BCB_I32(player->boost, 0x14) != 0)
				BCarPlayer_Boost(player);
		}
		if (player->braking == 0)
			BCarPlayer_StopBrake(player);
		else
		{
			bmVectorNormalise(a, player->entity->vel);
			bmVectorNormalise(b, player->entity->oldVel);
			if (b[0] * a[0] + b[1] * a[1] + b[2] * a[2] < 0.3f) /* 0x00265580 */
				BCarTrack_End(player->skidTrack);
		}
	}

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		player = &g_BCarPlayers[i];
		if (player->headlights[0] != NULL)
			BCarHeadlight_Update(player->headlights[0]);
		if (player->headlights[1] != NULL)
			BCarHeadlight_Update(player->headlights[1]);
		if (player->headlights[2] != NULL)
			BCarHeadlight_Update(player->headlights[2]);
		entity = player->entity;
		if (entity->dead == 0 && entity->team == -1 && entity->respawnTime == 0.0f)
			player->headlightsOn = (1 < g_BCarGameState);
		else
			player->headlightsOn = 0;
		if (DAT_00283ea4 != g_LastColourSet)
		{
			Core_SetCharacterColours(player->driverInst[player->lowDetail], DAT_00283ea4);
			baPrepActorInstance(player->driverInst[player->lowDetail], 1);
		}
	}
	g_LastColourSet = DAT_00283ea4;
}

/* 0x000991f0  BumperCars/CBCar_Player.cpp (bracketed)
Chooses the lead player (who carries the lead marker): in the team variant the player ranked
first; otherwise the best scorer when he leads by LEADSCORE, for LEADTIMEOUT seconds. Says
so (speech 0xb, or the lead voice 0x14) when the leader changes. */
void BCarPlayers_UpdateLead(int speak)
{
	struct BCarPlayer *previous = g_LeadPlayer;
	struct BCarEntity *entity;
	int i;
	int lead;
	int best;
	int second;
	int score;

	if (g_BCarVariant == 4)
	{
		lead = -1;
		for (i = 0; i < 4; i++)
		{
			if (BCarCamera_GetRank(BCB_CAMERA, i) == 1)
				lead = i;
		}
		if (lead == -1)
			return;
		g_LeadPlayer = &g_BCarPlayers[lead];
		if (speak != 0 && g_BCarPlayers[lead].entity->team == -1 && g_LeadSpoken == 0 &&
			bcb_rand() % 0x96 == 0)
		{
			BCarSound_Speak(BCB_SOUND, lead, 0xb, 0xff, g_BCarPlayers[lead].entity->pos, 1.0f);
			g_LeadSpoken = 1;
		}
		if (g_LeadPlayer != previous && DAT_002baebc == 0)
		{
			g_LeadSpoken = 0;
			if (bcb_rand() % 0x19 == 0)
				BCarSound_PlayVoice(BCB_SOUND, 0x14, DAT_002baed4[lead * 10], 0xff);
		}
		return;
	}
	if (g_LeadTimeout == 0.0f)
		return;
	if (g_LeadPlayer != NULL)
	{
		g_LeadAngle = CLMath_WrapAngle(g_LeadSpin + g_LeadAngle);
		entity = g_LeadPlayer->entity;
		g_LeadPos[0] = entity->pos[0];
		g_LeadPos[2] = entity->pos[2];
		g_LeadPos[1] = entity->pos[1] + g_LeadOffset;
		g_LeadPos[3] = entity->pos[3];
		if (g_LeadEndTime <= BCB_NOW())
			g_LeadPlayer = NULL;
	}

	/* the best score and the best of the others */
	second = 0;
	best = 0;
	if (0 < g_BCarPlayers[0].entity->score)
		best = g_BCarPlayers[0].entity->score;
	lead = 0;
	if (best < g_BCarPlayers[1].entity->score)
	{
		best = g_BCarPlayers[1].entity->score;
		lead = 1;
	}
	if (best < g_BCarPlayers[2].entity->score)
	{
		lead = 2;
		best = g_BCarPlayers[2].entity->score;
	}
	if (best < g_BCarPlayers[3].entity->score)
	{
		lead = 3;
		best = g_BCarPlayers[3].entity->score;
	}
	score = g_BCarPlayers[0].entity->score;
	if (0 < score && score <= best && lead != 0)
		second = score;
	score = g_BCarPlayers[1].entity->score;
	if (second < score && score <= best && lead != 1)
		second = score;
	score = g_BCarPlayers[2].entity->score;
	if (second < score && score <= best && lead != 2)
		second = score;
	score = g_BCarPlayers[3].entity->score;
	if (second < score && score <= best && lead != 3)
		second = score;
	if (best < g_LeadScore + second)
		return;

	g_LeadPlayer = &g_BCarPlayers[lead];
	g_LeadEndTime = BCB_NOW() + g_LeadTimeout;
	entity = g_LeadPlayer->entity;
	g_LeadPos[0] = entity->pos[0];
	g_LeadPos[2] = entity->pos[2];
	g_LeadPos[1] = entity->pos[1] + g_LeadOffset;
	g_LeadPos[3] = entity->pos[3];
	if (speak != 0 && g_BCarPlayers[lead].entity->team == -1 && g_LeadSpoken == 0 &&
		bcb_rand() % 0x96 == 0)
	{
		BCarSound_Speak(BCB_SOUND, lead, 0xb, 0xff, g_BCarPlayers[lead].entity->pos, 1.0f);
		g_LeadSpoken = 1;
	}
	if (g_LeadPlayer != previous && DAT_002baebc == 0)
	{
		g_LeadSpoken = 0;
		if (bcb_rand() % 0x19 == 0)
			BCarSound_PlayVoice(BCB_SOUND, 0x14, DAT_002baed4[lead * 10], 0xff);
	}
}

/* draw `inst` again mirrored in the floor (the floor's reflection): flipped in y about the
ground under it, tilted to the ground normal, then restored */
static void bcb_draw_reflected(struct BActorInstance *inst, int passMode)
{
	float rot[4];
	float normal[4];
	float height;

	rot[0] = inst->rot[0];
	rot[1] = inst->rot[1];
	rot[2] = inst->rot[2];
	rot[3] = inst->rot[3];
	height = BCarWorld_GetGroundHeight(inst->pos, 0.0f, normal);
	BCarMath_TiltToNormal(inst->rot, inst->rot, normal);
	inst->scale[1] = inst->scale[1] * -1.0f;             /* 0x00253854 */
	inst->pos[1] = inst->pos[1] - (height + height);
	baPrepActorInstance(inst, 1);
	baDrawActorInstance(inst, passMode);
	inst->pos[1] = (height + height) + inst->pos[1];
	inst->scale[1] = inst->scale[1] * -1.0f;
	inst->rot[0] = rot[0];
	inst->rot[1] = rot[1];
	inst->rot[2] = rot[2];
	inst->rot[3] = rot[3];
}

/* 0x000994b0  BumperCars/CBCar_Player.cpp (bracketed)
Draws the lead marker over the lead player (and its reflection when the debug switch at
0x0008c040 is on). `shadow` selects the second render pass. */
void BCarPlayers_DrawLead(int shadow)
{
	float m[16];

	if (g_LeadPlayer == NULL || g_LeadPlayer->entity->dead != 0 ||
		g_LeadPlayer->entity->respawnTime != 0.0f)
		return;
	bdSetRenderState(8, 0, 0);
	bdSetRenderState(6, shadow != 0, 0);
	if (g_LeadInst != NULL)
	{
		bdSetWorldIdentity();
		bmMatrixRotationXZY(m, g_LeadPlayer->entity->rot[0], g_LeadAngle,
			g_LeadPlayer->entity->rot[2]);
		bmMatrixToQuaternion(g_LeadInst->rot, m);
		g_LeadInst->pos[0] = g_LeadPos[0];
		g_LeadInst->pos[1] = g_LeadPos[1];
		g_LeadInst->pos[2] = g_LeadPos[2];
		g_LeadInst->pos[3] = g_LeadPos[3];
		if (BCB_DEBUG_GET(FUN_0008c040) != 0)
		{
			FUN_001d6820();
			FUN_001d6860();
			BCarRender_BeginReflection(0);
			bcb_draw_reflected(g_LeadInst, shadow != 0 ? 6 : 1);
			BCarRender_EndReflection();
			FUN_0018e640();
		}
		baPrepActorInstance(g_LeadInst, 1);
		baDrawActorInstance(g_LeadInst, shadow != 0 ? 6 : 1);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(8, BCB_DEBUG_GET(FUN_0008c060), 0);
}

/* place `inst` on a player's car: the car's orientation (heading + pi) and position, raised by
the draw offset */
static void bcb_place_on_car(struct BActorInstance *inst, struct BCarEntity *entity)
{
	float m[16];

	bmMatrixRotationXZY(m, entity->rot[0], entity->heading + 3.1415925f, entity->rot[2]); /* 0x00256750 */
	bmMatrixToQuaternion(inst->rot, m);
	inst->pos[0] = entity->pos[0];
	inst->pos[1] = entity->pos[1];
	inst->pos[2] = entity->pos[2];
	inst->pos[3] = entity->pos[3];
	inst->pos[1] = entity->drawOffset + inst->pos[1];
}

/* 0x00099710  BumperCars/CBCar_Player.cpp (bracketed)
Draws the players: the skid marks, then for every live car not viewed from inside, the
driver, the brake and the boost models with their reflections in the floor, and the
headlights. `shadow` selects the second render pass (the headlights are drawn in it, the
driver is not). */
void BCarPlayers_Draw(int shadow)
{
	int i;
	int view;
	struct BCarPlayer *player;
	struct BCarEntity *entity;
	struct BActorInstance *driver;
	int pass = shadow != 0 ? 6 : 1;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		player = &g_BCarPlayers[i];
		bdSetRenderState(6, 1, 0);
		bdPushRenderState(10, 1, 0);
		bdPushRenderState(3, 0, 0);
		bdSetDepthOffset(-1.0f);
		FUN_001d6820();
		FUN_001d6860();
		BCarTrack_Draw(player->skidTrack);
		FUN_0018e640();
		bdSetDepthOffset(0.0f);
		bdPopRenderState(3);
		bdPopRenderState(10);
		entity = player->entity;
		if (entity->dead != 0 || entity->respawnTime != 0.0f)
			continue;
		if (BCarCamera_GetViewPlayer(BCB_CAMERA, &view) != 0 && i == view)
			continue;

		driver = player->driverInst[player->lowDetail];
		if (driver != NULL && shadow == 0)
			bcb_place_on_car(driver, player->entity);
		if (player->braking != 0 && player->brakeInst != NULL)
			bcb_place_on_car(player->brakeInst, player->entity);
		if (player->boostReady != 0 && player->boostInst != NULL)
			bcb_place_on_car(player->boostInst, player->entity);
		bdSetWorldIdentity();
		if (BCB_DEBUG_GET(FUN_0008c040) != 0)
		{
			FUN_001d6820();
			FUN_001d6860();
			BCarRender_BeginReflection(0);
			if (player->driverInst[player->lowDetail] != NULL &&
				BCB_DEBUG_GET(FUN_0008c080) != 0)
			{
				BCarLight_Enable(BCB_LIGHTS, 1);
				bcb_draw_reflected(player->driverInst[player->lowDetail], 1);
				BCarLight_Enable(BCB_LIGHTS, 0);
			}
			BCarRender_EndReflection();
			BCarRender_BeginReflection(1);
			if (player->braking != 0 && player->brakeInst != NULL)
				bcb_draw_reflected(player->brakeInst, pass);
			if (player->boostReady != 0 && player->boostInst != NULL)
				bcb_draw_reflected(player->boostInst, pass);
			BCarRender_EndReflection();
			FUN_0018e640();
		}
		bdSetRenderState(10, 0, 0);
		baSetBrightness(BCB_FTOL(player->entity->alpha * 255.0f)); /* 0x002537d0 */
		if (player->headlightsOn != 0 && BCB_DEBUG_GET(FUN_0008c090) != 0 && shadow != 0)
		{
			if (player->headlights[0] != NULL)
				BCarHeadlight_Draw(player->headlights[0], player->entity->alpha * 0.5f);  /* 0x002536d0 */
			if (player->headlights[1] != NULL)
				BCarHeadlight_Draw(player->headlights[1], player->entity->alpha * 0.35f); /* 0x00265590 */
			if (player->headlights[2] != NULL)
				BCarHeadlight_Draw(player->headlights[2], player->entity->alpha * 0.2f);  /* 0x00257c7c */
		}
		if (player->driverInst[player->lowDetail] != NULL && shadow == 0 &&
			BCB_DEBUG_GET(FUN_0008c080) != 0)
		{
			BCarLight_Enable(BCB_LIGHTS, 1);
			baPrepActorInstance(player->driverInst[player->lowDetail], 1);
			baDrawActorInstance(player->driverInst[player->lowDetail], 1);
			BCarLight_Enable(BCB_LIGHTS, 0);
		}
		bdSetRenderState(6, 1, 0);
		bdSetRenderState(10, 1, 0);
		bdSetRenderState(8, 0, 0);
		if (player->braking != 0 && player->brakeInst != NULL)
		{
			baPrepActorInstance(player->brakeInst, 1);
			baDrawActorInstance(player->brakeInst, pass);
		}
		if (player->boostReady != 0 && player->boostInst != NULL)
		{
			baPrepActorInstance(player->boostInst, 1);
			baDrawActorInstance(player->boostInst, pass);
		}
		if (player->human == 0)
			BCB_CALL(FUN_001ec260, void (*)(int))(shadow);
		bdSetRenderState(8, BCB_DEBUG_GET(FUN_0008c060), 0);
		bdSetRenderState(10, 0, 0);
		baSetBrightness(0x100);
	}
}

/* make a 90x90 HUD overlay of a texture, hidden, under the HUD's overlay of player slot */
static int bcb_player_overlay(int parent, const char *texture)
{
	int desc[0xec];
	int overlay;
	float f;

	memset(desc, 0, sizeof desc);
	desc[0xa2] = 0;
#define BCB_DESC_F(index, value) (f = (value), memcpy(&desc[index], &f, 4))
	BCB_DESC_F(0x94, 90.0f);
	BCB_DESC_F(0x95, 90.0f);
	BCB_DESC_F(0x96, 90.0f);
	BCB_DESC_F(0x97, 255.0f);
	BCB_DESC_F(0xad, 17.0f);
	BCB_DESC_F(0xae, 20.0f);
	desc[0xaf] = 0;
	BCB_DESC_F(0xc2, 32.0f);
	BCB_DESC_F(0xc3, 32.0f);
#undef BCB_DESC_F
	desc[0xa4] = 0x80;
	desc[1] = (int)bLoadResource(NULL, texture, 0, 0xdefa);
	overlay = COverlay_CreateChild(parent, (undefined4 *)desc);
	BCB_I16(overlay, 0x54) = 1;
	return overlay;
}

/* 0x00099fc0  BumperCars/CBCar_Player.cpp (bracketed)
Makes the "flag" and "dead" HUD overlays of the players (hidden). */
void BCarPlayers_CreateOverlays(void)
{
	int i;
	int parent;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		parent = BCB_I32(DAT_002ecfac, 0xf0 + i * 4);
		g_BCarPlayers[i].flagOverlay = bcb_player_overlay(parent, "debris\\textures\\flag.bmp");
		parent = BCB_I32(DAT_002ecfac, 0xf0 + i * 4);
		g_BCarPlayers[i].deadOverlay = bcb_player_overlay(parent, "debris\\textures\\dead.bmp");
		g_BCarPlayers[i].unk2cc = 0;
	}
}

/* a random speed scale of the AI for the difficulty and the variant */
static float bcb_ai_speed_scale(int difficulty, int variant, int *set)
{
	*set = 1;
	if (difficulty == 0)
	{
		if (variant == 4)
			return (float)bcb_rand() * 9.15555e-06f + 0.4f;     /* 0x0026572c, 0x00262dbc */
		if (variant == 3)
			return (float)bcb_rand() * 6.1037e-06f + 0.3f;      /* 0x00265734, 0x00265580 */
		if (variant == 2)
			return Core_RandUnit() * 0.3f + 0.4f;
		return Core_RandUnit() * 0.3f + 0.3f;
	}
	if (difficulty == 1)
	{
		if (variant == 4)
			return (float)bcb_rand() * 6.1037e-06f + 0.4f;
		if (variant == 3)
			return (float)bcb_rand() * 6.1037e-06f + 0.37f;     /* 0x00265730 */
		if (variant == 2)
			return Core_RandUnit() * 0.15f + 0.4f;              /* 0x0026573c */
		return Core_RandUnit() * 0.2f + 0.4f;                   /* 0x00257c7c */
	}
	if (difficulty == 2)
	{
		if (variant == 4)
			return (float)bcb_rand() * 4.57778e-06f + 0.8f;     /* 0x00265748, 0x00257c78 */
		if (variant == 3)
			return (float)bcb_rand() * 3.05185e-06f + 0.65f;    /* 0x00265744, 0x00265740 */
		if (variant == 2)
			return Core_RandUnit() * 0.15f + 0.65f;
		return Core_RandUnit() * 0.1f + 0.6f;                   /* 0x002536cc, 0x00265738 */
	}
	*set = 0;
	return 0.0f;
}

/* make a headlight of an entity: range = scale * `range`, width `width` */
static struct BCarHeadlight *bcb_player_headlight(struct BCarPlayer *player, int line,
	float range, float width)
{
	void *memory = BCB_NEW(0x40, BCB_SRC_PLAYER, line);
	struct BCarHeadlight *light = NULL;

	if (memory != NULL)
		light = BCarHeadlight_Construct((struct BCarHeadlight *)memory, player->entity->instPos,
			player->entity->instRot, player->headlightTexture, player->scale * range, width, 8);
	BCarHeadlight_SetFade(light, 15.0f);
	return light;
}

/* 0x0009a170  BumperCars/CBCar_Player.cpp
Player_Init: reads the player file `name` of `package`, sets the AI speed scales for the
difficulty, places the players on their start points (in the rank order of the score
table), makes their drivers, animations, entities, AI, skid tracks, spot lights and
headlights, then the pucks, keepsakes and powerups. The debug option "players" sets how
many players are human. Returns 1 on success. */
int BCarPlayers_Init(struct bkPackage *package, const char *name)
{
	struct BCarTextReader reader;
	struct BCarPlayer *player;
	struct BCarCharacter *character;
	struct BActorInstance *inst;
	struct bdTexture *texture;
	unsigned char *colour;
	char *option;
	char *file;
	uint size;
	int humans;
	int i;
	int k;
	int order;
	int set;
	float value;
	void *memory;
	char spot[256];

	BCarTextReader_Construct(&reader);
	humans = -1;
	option = BCarOptions_Get("players");
	if (option != NULL)
		humans = atol(option);
	file = (char *)bkLoadFile(package, name, NULL, &size, NULL, 0);
	if (file == NULL)
	{
		bkPrintf("Player_Init - Couldn't open player file [%s]\n", name);
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	BCarTextReader_Init(&reader, file, (int)size, name);
	if (DAT_002baeb8 != 0)
	{
		g_SavedDifficulty = DAT_002bae70;
		DAT_002bae70 = 2;
	}
	BCarPlayers_LoadInformation(&reader);

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		value = bcb_ai_speed_scale(DAT_002bae70, g_BCarVariant, &set);
		if (set != 0)
			g_AISpeedScale[i] = value;
	}

	CScoreTable_Rank((struct CScoreTable *)DAT_002b9968);
	for (i = 0; i < 4; i++)
	{
		order = DAT_002b9c58[i];
		if (g_StartPoints[i].pos[3] != 1.0f)
		{
			bkPrintf("Player_Init - Player [%d] has no starting position. Startpoint [%d] not defined\n",
				order, i);
			BCarTextReader_Destruct(&reader);
			return 0;
		}
		player = &g_BCarPlayers[order];
		player->slot = i;
		player->rank = i;
		player->lastRank = i;
		player->params.pos[0] = g_StartPoints[i].pos[0];
		player->params.pos[1] = g_StartPoints[i].pos[1];
		player->params.pos[2] = g_StartPoints[i].pos[2];
		player->params.pos[3] = g_StartPoints[i].pos[3];
		player->params.angle = g_StartPoints[i].angle;
	}
	g_LeadPlayer = &g_BCarPlayers[DAT_002b9c64];

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		player = &g_BCarPlayers[i];
		character = &g_Characters[DAT_002baed4[i * 10]];
		for (k = 0; k < 2; k++)
		{
			if (k == 0)
			{
				player->carModel = character->car;
				player->driverModel[0] = character->driver;
			}
			else
			{
				player->carModelLow = character->carLow;
				player->driverModel[1] = character->driverLow;
			}
			if (player->driverModel[k] == NULL)
				continue;
			inst = baInitActorInstance(NULL, player->driverModel[k], 6, BCB_SRC_PLAYER, 0xaa);
			player->driverInst[k] = inst;
			if (inst == NULL)
			{
				bkPrintf("***** ERROR: Could not load player [%d] actor instance\n", i);
				DbgBreakPoint();
				continue;
			}
			inst->rot[0] = DAT_00285634[0];
			inst->rot[1] = DAT_00285634[1];
			inst->rot[2] = DAT_00285634[2];
			inst->rot[3] = DAT_00285634[3];
			baPrepActorInstance(inst, 1);
			baFlushAnimations(inst, inst->root, 1, 0);
		}
		inst = player->driverInst[0];
		if (inst == NULL)
		{
			player->animDrive = NULL;
			player->animWinLoop = NULL;
			player->animLoseLoop = NULL;
			player->animVictoryStart = NULL;
			player->animVictoryLoop = NULL;
			player->animVictoryEnd = NULL;
		}
		else
		{
			player->animDrive = (struct BAnim *)baFindSubObject(inst, "drive");
			player->animWinLoop = (struct BAnim *)baFindSubObject(player->driverInst[0], "winloop");
			player->animLoseLoop = (struct BAnim *)baFindSubObject(player->driverInst[0], "loseloop");
			player->animVictoryStart = (struct BAnim *)baFindSubObject(player->driverInst[0], "victory_start");
			player->animVictoryLoop = (struct BAnim *)baFindSubObject(player->driverInst[0], "victory_loop");
			player->animVictoryEnd = (struct BAnim *)baFindSubObject(player->driverInst[0], "victory_end");
		}
		if (player->driverModel[0] != NULL && player->driverInst[1] != NULL)
			baSetInstanceAnimations(player->driverModel[0], player->driverInst[1]);
		if (player->animDrive != NULL)
			baQueueAnimation(player->driverInst[0], player->driverInst[0]->root, player->animDrive,
				0x400, -1, 1, NULL, NULL);
		if (player->carModel == NULL || player->carModelLow == NULL)
		{
			bkPrintf("Player_Init - Couldnt find player actor(s)\n");
			BCarTextReader_Destruct(&reader);
			return 0;
		}
		player->entity = BCarEntity_Create(player->carModel, player->disableModel, player->scale,
			&player->params);
		if (player->entity == NULL)
		{
			bkPrintf("Player_Init - Couldnt initialise player entity\n");
			BCarTextReader_Destruct(&reader);
			return 0;
		}
		player->entity->player = player;
		BCarEntity_SetType(player->entity, BCAR_ENT_CAR);
		if (humans == -1)
		{
			if (DAT_002baed8[i * 10] != 0)
			{
				player->human = 1;
				DAT_002baed8[i * 10] = 1;
			}
		}
		else if (-1 < humans && i < humans)
		{
			player->human = 1;
			DAT_002baed8[i * 10] = 1;
		}
		BCB_CALL(FUN_00089770, void (*)(void *, struct BCarPlayer *, int *))(player->ai, player,
			player->aiLevel);

		texture = (struct bdTexture *)bLoadResource(NULL, "debris\\textures\\skid.bmp", 0, 0xdefa);
		memory = BCB_NEW(0x2c, BCB_SRC_PLAYER, 0xf2);
		if (memory == NULL)
			player->skidTrack = NULL;
		else
			player->skidTrack = BCarTrack_Construct((struct BCarTrack *)memory, 700,
				player->params.radius, 30.0f, texture, texture);
		_snprintf(spot, 0xff, "playerspot%d", player->slot + 1);
		colour = &DAT_00290718[DAT_002baed4[player->slot * 10] * 0x60];
		BCarLight_SetColour(BCB_LIGHTS, BCarLight_Attach(BCB_LIGHTS, spot, player->entity),
			colour[0], colour[1], colour[2]);
		if (player->headlightTexture != NULL)
		{
			player->headlights[0] = bcb_player_headlight(player, 0x100, 1700.0f, 50.0f); /* 0x00265630 */
			player->headlights[1] = bcb_player_headlight(player, 0x103, 1500.0f, 60.0f); /* 0x0026562c */
			player->headlights[2] = bcb_player_headlight(player, 0x106, 1200.0f, 70.0f); /* 0x002553a4 */
		}
		player->boostReadyTime = 0.0f;
		player->boostCount = 0;
		if (player->boostInst != NULL)
		{
			player->boostInst->scale[3] = 1.0f;
			player->boostInst->scale[0] = player->scale * 1.2f;    /* 0x00265628 */
			player->boostInst->scale[1] = player->scale * 1.2f;
			player->boostInst->scale[2] = player->scale * 1.2f;
		}
		for (k = 0; k < 2; k++)
		{
			inst = player->driverInst[k];
			if (inst != NULL)
			{
				inst->scale[0] = player->scale;
				inst->scale[1] = player->scale;
				inst->scale[3] = 1.0f;
				inst->scale[2] = player->scale;
			}
		}
		if (player->brakeInst != NULL)
		{
			player->brakeInst->scale[0] = player->scale;
			player->brakeInst->scale[1] = player->scale;
			player->brakeInst->scale[3] = 1.0f;
			player->brakeInst->scale[2] = player->scale;
		}
	}
	if (BCarPucks_Init() == 0 || BCarKeepsakes_Init() == 0)
	{
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	if (BCarPowerups_Init() == 0)
	{
		BCarTextReader_Destruct(&reader);
		return 0;
	}
	g_LastColourSet = 0;
	BCarTextReader_Destruct(&reader);
	return 1;
}

/* 0x0009abb0  (gap; file inferred)
Player_Shutdown: shuts the AI drivers down and deletes what Player_Init made. */
void BCarPlayers_Shutdown(void)
{
	int i;
	struct BCarPlayer *player;
	struct BCarTrack *track;
	struct BCarHeadlight *light;
	int k;

	for (i = 0; i < BCB_NUM_PLAYERS; i++)
	{
		player = &g_BCarPlayers[i];
		BCB_CALL(FUN_000890d0, void (*)(void *))(player->ai);
		if (player->brakeInst != NULL)
			baDeleteActorInstance(player->brakeInst);
		if (player->boostInst != NULL)
			baDeleteActorInstance(player->boostInst);
		if (player->driverInst[0] != NULL)
			baDeleteActorInstance(player->driverInst[0]);
		if (player->driverInst[1] != NULL)
			baDeleteActorInstance(player->driverInst[1]);
		track = player->skidTrack;
		if (track != NULL)
		{
			BCarTrack_Destruct(track);
			BCB_DELETE(track);
		}
		if (player->brakeChannel != -1)
			bsReserveChannel(player->brakeChannel, 0);
		for (k = 0; k < 3; k++)
		{
			light = player->headlights[k];
			if (light != NULL)
			{
				BCarHeadlight_Destruct(light);
				BCB_DELETE(light);
			}
		}
	}
	if (DAT_002baeb8 != 0)
		DAT_002bae70 = g_SavedDifficulty;
}
