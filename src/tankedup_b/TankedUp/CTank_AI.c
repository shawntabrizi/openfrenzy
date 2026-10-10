/*
TankedUp/CTank_AI.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_AI.cpp

The entity behaviours ("AI") of TankedUp. tankedup_a's dispatcher 0x001cde40 calls the
handler of an entity's AI number (ENT_AI) with (CTank_Entities, entity, event, data):
`event` 1 is a message for the entity (event 1 also goes to the think handler
0x001d8820), 3 a think tick, and so on; `data` is the message from +0x18: data[0], data[1]
its two ids, data[2] the event, data[3] the message type, data[4], data[5] extra words.
0x001d9bb0 and the later functions are tagged with CTank_AI.cpp; 0x001d7d80-0x001d9b50
(gap) are the same kind of code (file inferred).

Entity fields used here (tankedup_b.h for the others):
  +0x004 owner id     +0x08c AI state    +0x090 AI state changed   +0x098 carries a flag
  +0x920 bump sound   +0x928 hits friends  +0x92c score            +0x938 loaded
  +0x944 frozen       +0x948 protected   +0x94c triple shot        +0x95c respawning
  +0x964 animation done  +0x96c-0x974 ambient colour (-1: none)    +0x978 resting
  +0x9ac speed-up time   +0x9cc particle emitter   +0x9d0 the flag carried
  +0x9d4 the entity last touched
*/

#include "tankedup_b.h"
#include "tankedup_b_data.h"
#include "tankedup_b_ext.h"

#define ENT_OWNER 0x004
#define ENT_AI_STATE 0x08c
#define ENT_AI_CHANGED 0x090
#define ENT_HAS_FLAG 0x098
#define ENT_BUMP 0x920
#define ENT_SCORE 0x92c
#define ENT_LOADED 0x938
#define ENT_FROZEN 0x944
#define ENT_PROTECTED 0x948
#define ENT_TRIPLE 0x94c
#define ENT_RESPAWNING 0x95c
#define ENT_ANIM_DONE 0x964
#define ENT_AMBIENT 0x96c
#define ENT_RESTING 0x978
#define ENT_SPEEDUP 0x9ac
#define ENT_EMITTER 0x9cc
#define ENT_FLAG 0x9d0
#define ENT_TOUCHED 0x9d4

/* the message words of an AI handler (see the header) */
#define AI_ID0 0
#define AI_ID1 1
#define AI_EVENT 2
#define AI_TYPE 3
#define AI_ARG 4

/* the entity with id `id` (tankedup_a 0x001ce100, thiscall on CTank_Entities) */
#define TA_GetEntity(ents, id) TU_CALL(FUN_001ce100, char *(*)(void *, int))(ents, id)
/* the nearest enemy tank of `id` from `pos` (tankedup_a 0x001ce7d0): its id in *out */
#define TA_NearestEnemy(ents, pos, id, out) \
	TU_CALL(FUN_001ce7d0, void (*)(void *, float *, int, int *))(ents, pos, id, out)
/* tankedup_a 0x001cdc30: an entity back to its template's health, timers, ... */
#define TA_ResetEntity(ents, ent) TU_CALL(FUN_001cdc30, void (*)(void *, void *))(ents, ent)

/* classlib_fx: the particle emitter calls (their table names come from bumpercars_b) */
#define TU_EmitterSetOn(emitter, on) AnimSwitch_Set((int *)(emitter), (on))
#define TU_EmitterSetColour(emitter, colour) AnimSwitch_SetValue((int)(intptr_t)(emitter), (int)(colour))

/* bumpercars_a: the debris type "RESPAWNCHAR" */
extern unsigned char DAT_00291734[];
extern int DAT_00290e8c;            /* classlib_fx: the debris colour (character) */
extern float DAT_00315100;          /* the mass factor while sped up */
extern float DAT_00315104;          /* the speed-up time */
extern int DAT_00315174;            /* the command point burst ("Goal Size") */
extern unsigned char DAT_003150b0[]; /* the CTank_Debug object: +0 "Respawning inv" */

/* the player index (0-3) of an entity named "player1".."player4", else `fallback` */
static int tu_ai_player(const char *name, int fallback)
{
	int index = fallback;

	if (strcmp(name, "player1") == 0)
		index = 0;
	if (strcmp(name, "player2") == 0)
		index = 1;
	if (strcmp(name, "player3") == 0)
		index = 2;
	if (strcmp(name, "player4") == 0)
		index = 3;
	return index;
}

/* the character colour byte `c` (0 red, 1 green, 2 blue) of player `player` */
static unsigned int tu_player_colour(int player, int c)
{
	return DAT_00290718[DAT_002baed4[player * 10] * 0x60 + c];
}

/* SendDelayedMessage with the usual words: (id0, id1, event, -, 0, 0) */
static void tu_send(void *ents, int type, int id0, int id1, int event, float delay)
{
	TankEnts_SendDelayedMessage(ents, type, (unsigned int)id0, (unsigned int)id1,
		(unsigned int)event, 0, 0, 0, delay);
}

/* |a - b|^2 <= (ra + rb)^2: two entities touch */
static int tu_touching(char *a, char *b)
{
	float r = TU_F32(a, ENT_RADIUS) + TU_F32(b, ENT_RADIUS);
	float dx = TU_F32(b, ENT_POS) - TU_F32(a, ENT_POS);
	float dy = TU_F32(b, ENT_POS + 4) - TU_F32(a, ENT_POS + 4);
	float dz = TU_F32(b, ENT_POS + 8) - TU_F32(a, ENT_POS + 8);

	return dx * dx + dz * dz + dy * dy <= r * r;
}

/* 0x001d7d80  TankedUp (gap; CTank_AI.cpp inferred)
1 when no barrier (type 3) and no raised wall (type 8 not in AI state 0xc) other than
`ignore` is within 890 of `pos`. */
int TankAI_PositionClear(void *self, float *pos, void *ignore)
{
	void **node;
	char *ent;
	float dx, dy, dz;
	int type;

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (ent == (char *)ignore)
			continue;
		type = TU_ENT_TYPE(ent);
		if (!((type == 8 && TU_I32(ent, ENT_AI_STATE) != 0xc) || type == 3))
			continue;
		dz = TU_F32(ent, ENT_POS + 8) - pos[2];
		dy = TU_F32(ent, ENT_POS + 4) - pos[1];
		dx = TU_F32(ent, ENT_POS) - pos[0];
		if (dz * dz + dy * dy + dx * dx <= 792100.0f) /* 0x0027bcac */
			return 0;
	}
	return 1;
}

/* 0x001d7e10  TankedUp (gap; CTank_AI.cpp inferred)
AI 7, "Command point": a point that a tank captures by touching it; it then shows the
tank's colour, bursts and drops a "test_temp" marker. */
int TankAI_CommandPoint(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *owner;
	char *other;
	char *marker;
	void **node;
	void *emitter;
	int player;
	int kind;
	int id;

	if (event == 1) {
		if (data[AI_TYPE] == 0x1a) {
			tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 1.0f);
			if (TU_I32(ent, ENT_OWNER) != 0) {
				/* the owner scores (word 4 = 1) */
				TankEnts_SendDelayedMessage(self, 0x11, TU_U32(ent, ENT_ID), TU_U32(ent, ENT_OWNER), 1,
					0, 1, 0, 0.0f);
				return 1;
			}
		} else {
			if (data[AI_TYPE] != 3)
				return 0;
			if (TU_I32(ent, ENT_ANIM_DONE) != 0 && TU_PTR(ent, ENT_ANIM_SEG) != NULL) {
				baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_INST),
					(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_INST), 0x134),
					(struct BAnim *)TU_PTR(ent, ENT_ANIM_SEG), 0x400, -1, 0, (int (*)())FUN_001d7d60, ent);
				return 1;
			}
		}
		return 1;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 0xf;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("Starting 'Command point' AI for %s\n", ent + ENT_NAME);
		tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 1.0f);
		emitter = BCarGenericGoal_AddGoal((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS),
			TU_FP(ent, ENT_POS), 1.0f, 0);
		TU_PTR(ent, ENT_EMITTER) = emitter;
		TU_EmitterSetOn(emitter, 1);
		TU_EmitterSetColour(TU_PTR(ent, ENT_EMITTER), 0x646464);
		return 1;
	}
	if (event != 0xf) {
		if (event != 9)
			return 0;
		if (data[AI_TYPE] == 0x1b) {
			if (TU_I32(self, ENTS_VARIANT) != 4) {
				tu_send(self, 0x1b, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 9, 1.0f);
				return 1;
			}
		} else {
			if (data[AI_TYPE] != 3)
				return 0;
			if (TU_I32(self, ENTS_VARIANT) != 4)
				return 1;
		}
		TU_I32(ent, ENT_AI_STATE) = 0xf;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}

	/* event 0xf: waiting for a tank */
	if (data[AI_TYPE] != 3)
		return 0;
	owner = TA_GetEntity(self, TU_I32(ent, ENT_OWNER));
	if (owner != NULL && tu_touching(ent, owner))
		return 1; /* the owner is still on it */
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		other = (char *)node[0];
		if (TU_I32(ent, ENT_OWNER) == TU_I32(other, ENT_ID) || other == ent)
			continue;
		if (TU_ENT_TYPE(other) != 1 || TU_I32(other, ENT_DEAD) != 0)
			continue;
		if (TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(other, ENT_ID), TU_I32(ent, ENT_ID)) != 0)
			continue;
		if (tu_touching(ent, other))
			break;
	}
	if (node == NULL)
		return 1;
	TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x10, TU_FP(ent, ENT_POS), 200, 1, 0);
	TU_I32(ent, ENT_OWNER) = TU_I32(other, ENT_ID);
	/* the original keeps the entity pointer as the index when the name is no player's */
	player = tu_ai_player(TU_STR(other, ENT_NAME), 0);
	TU_I32(ent, ENT_AMBIENT) = (int)tu_player_colour(player, 0);
	TU_I32(ent, ENT_AMBIENT + 4) = (int)tu_player_colour(player, 1);
	TU_I32(ent, ENT_AMBIENT + 8) = (int)tu_player_colour(player, 2);
	TU_EmitterSetColour(TU_PTR(ent, ENT_EMITTER),
		(((TU_U32(ent, ENT_AMBIENT) & 0xffu) << 8) + TU_U32(ent, ENT_AMBIENT + 4)) * 0x100 +
		TU_U32(ent, ENT_AMBIENT + 8));
	kind = -1;
	switch (DAT_00315174) {
	case 1:
		kind = 0;
		break;
	case 2:
		kind = 1;
		break;
	case 3:
		kind = 2;
		break;
	case 4:
		kind = 3;
		break;
	case 5:
		kind = 4;
		break;
	}
	if (kind != -1)
		CParticleEffect_Burst((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS),
			(struct CParticleEmitter *)TU_PTR(ent, ENT_EMITTER), (short)TU_I32(ent, ENT_AMBIENT),
			(short)TU_I32(ent, ENT_AMBIENT + 4), (short)TU_I32(ent, ENT_AMBIENT + 8), kind, 2.0f);
	tu_send(self, 0x1b, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 9, 0.1f);
	id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "test_temp", TU_FP(ent, ENT_POS),
		TU_FP(ent, ENT_ROT), 0.0f);
	marker = TA_GetEntity(self, id);
	if (marker != NULL) {
		TU_I32(marker, ENT_AMBIENT) = TU_I32(ent, ENT_AMBIENT);
		TU_I32(marker, ENT_AMBIENT + 4) = TU_I32(ent, ENT_AMBIENT + 4);
		TU_I32(marker, ENT_AMBIENT + 8) = TU_I32(ent, ENT_AMBIENT + 8);
	}
	TU_I32(ent, ENT_AI_STATE) = 9;
	TU_I32(ent, ENT_AI_CHANGED) = 1;
	return 1;
}

/* 0x001d8440  TankedUp (gap; CTank_AI.cpp inferred)
A mine goes off: message 6 (hit) to every live entity within 650 of it. */
void TankAI_MineExplode(void *self, void *entity)
{
	char *ent = (char *)entity;
	void **node;
	char *other;

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		other = (char *)node[0];
		if (TU_I32(other, ENT_ID) == TU_I32(ent, ENT_ID) || TU_I32(other, ENT_DEAD) != 0)
			continue;
		if (bmVectorDistance(TU_FP(ent, ENT_POS), TU_FP(other, ENT_POS)) < 650.0f) /* 0x002797d8 */
			tu_send(self, 6, TU_I32(ent, ENT_ID), TU_I32(node[0], ENT_ID), 1, 0.0f);
	}
}

/* 0x001d84e0  TankedUp (gap; CTank_AI.cpp inferred)
AI 3, "MINE": arms after 2 s, goes off when hit, after 15 s, or when an enemy tank comes
within 450. */
int TankAI_Mine(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *enemy;
	int enemyId;

	if (event == 1)
		return 0;
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 9;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("Starting 'MINE' AI for %s\n", ent + ENT_NAME);
		tu_send(self, 0x19, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 9, 2.0f);
		return 1;
	}
	if (event == 9) {
		if (data[AI_TYPE] == 6)
			goto explode;
		if (data[AI_TYPE] != 0x19)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 3;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		tu_send(self, 6, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 3, 15.0f);
		return 1;
	}
	if (event != 3)
		return 0;
	if (data[AI_TYPE] == 6)
		goto explode;
	if (data[AI_TYPE] != 3)
		return 0;
	TA_NearestEnemy(self, TU_FP(ent, ENT_POS), TU_I32(ent, ENT_ID), &enemyId);
	enemy = TA_GetEntity(self, enemyId);
	if (enemy != NULL && bmVectorDistance(TU_FP(ent, ENT_POS), TU_FP(enemy, ENT_POS)) < 450.0f)
		tu_send(self, 6, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 3, 0.5f); /* 0x00272b60 */
	return 1;
explode:
	TankAI_MineExplode(self, ent);
	TU_I32(ent, ENT_DEAD) = 1;
	return 1;
}

/* 0x001d86d0  TankedUp (gap; CTank_AI.cpp inferred; its text names
CTank_Entities::GetRespawnPos)
Moves an entity to a point of its SPAWNLIST, chosen at random weighted by the distance to
the nearest entity. The point list (tankedup_a CTank_Spawn): pos[4], rot[4], +0x20 weight,
+0x24 next. */
void TankEnts_GetRespawnPos(void *self, void *entity)
{
	char *ent = (char *)entity;
	float *points;
	float *point;
	float total;
	float d;
	float pick;
	void **node;

	if (ent == NULL)
		return;
	points = (float *)TA_SpawnGetPoints(TU_PTR(self, ENTS_SPAWN), ent + ENT_SPAWNLIST);
	total = 0.0f;
	if (points == NULL)
		return;
	point = points;
	do {
		point[8] = 1e7f;
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			d = bmVectorDistance(point, TU_FP(node[0], ENT_POS));
			if (!(point[8] <= d))
				d = bmVectorDistance(point, TU_FP(node[0], ENT_POS));
			else
				d = point[8];
			point[8] = d;
		}
		total = total + point[8];
		point = *(float **)&point[9];
	} while (point != NULL);
	pick = (float)tu_rand() * total * 3.05185094e-05f; /* 0x00253740 */
	point = points;
	while (point[8] < pick) {
		pick = pick - point[8];
		point = *(float **)&point[9];
		if (point == NULL) {
			TU_DEBUG("CTank_Entities::GetRespawnPos - Respawn failed to get position for entity [%s]\n",
				ent + ENT_NAME);
			return;
		}
	}
	memcpy(ent + ENT_POS, point, 16);
	memcpy(ent + ENT_ROT, point + 4, 16);
}

/* the colour of a player's character for the light flash of a respawn */
static void tu_respawn_flash(void *self, char *ent, int player)
{
	int character = DAT_002baed4[player * 10] * 0x60;

	CTankLights_AddFlash((struct TuLight *)TU_PTR(self, ENTS_LIGHTS), TU_FP(ent, ENT_POS),
		DAT_00290718[character], DAT_00290719[character], DAT_0029071a[character]);
}

/* 0x001d8820  TankedUp (gap; CTank_AI.cpp inferred)
The think system: the messages every entity handles (event 1), whatever its AI.
  1 created (rocks get a grey ambient)   2 removed      0xf, 0x10 more health / mines
  0x11 score data[4] (playing only)      0x17 the game asks to end
  0x12 speed-up    0xb thaw    0xc unprotected    0x14 triple shot (5 s)   0x15 its end
  0x13 protected (5 s in a race, else by rank)    6 hit by data[0]   9 killed
  8 killed data[0] (scores via the owner; flags change hands)   7 killed by data[0]
  10 reloaded      0x18 bump sound on      4, 5 picked up / dropped a flag
  0xd respawn (effect, then 0xe after 0.75 s)     0xe respawned     3 think tick */
int TankAI_Think(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *other;
	char *root;
	char *killer;
	char *flag;
	void **node;
	float pos[4];
	float dir[4];
	float delay;
	float f;
	int rootId;
	int player;
	int i;
	void *debris;

	if (event != 1)
		return 0;
	switch (data[AI_TYPE]) {
	case 1:
		TU_DEBUG("ENTITY [%s] ENTERED THINK SYSTEM\n", ent + ENT_NAME);
		if (TU_ENT_TYPE(ent) == 9) {
			TU_I32(ent, ENT_AMBIENT) = 0x7f;
			TU_I32(ent, ENT_AMBIENT + 4) = 0x7f;
			TU_I32(ent, ENT_AMBIENT + 8) = 0x78;
		}
		return 1;
	case 2:
		TU_DEBUG("ENTITY [%s] EXITED THINK SYSTEM\n", ent + ENT_NAME);
		return 1;
	case 0xf:
		if (TU_ENT_TYPE(ent) == 1 &&
			TU_I32(ent, ENT_HEALTH) < TU_I32(TU_PTR(ent, ENT_TEMPLATE), TPL_HEALTH))
			TU_I32(ent, ENT_HEALTH) = TU_I32(ent, ENT_HEALTH) + 1;
		return 1;
	case 0x10:
		if (TU_ENT_TYPE(ent) == 1 &&
			TU_I32(ent, ENT_MINES) < TU_I32(TU_PTR(ent, ENT_TEMPLATE), TPL_MINES))
			TU_I32(ent, ENT_MINES) = TU_I32(ent, ENT_MINES) + 1;
		return 1;
	case 0x11:
		if (TU_I32(self, ENTS_VARIANT) != 4)
			return 1;
		if (data[AI_ARG] == 0)
			TU_DEBUG("%s: That's odd I have just got zero points from %d\n", ent + ENT_NAME,
				data[AI_ID0]);
		if (TU_ENT_TYPE(ent) == 1)
			TU_I32(ent, ENT_SCORE) = TU_I32(ent, ENT_SCORE) + data[AI_ARG];
		return 1;
	case 0x17:
		TU_I32(self, 0x60) = 1;
		return 1;
	case 0x12:
		TU_F32(ent, ENT_SPEEDUP) = DAT_00315104;
		TU_DEBUG("%s gets a speed up!!!\n", ent + ENT_NAME);
		TU_I32(ent, ENT_TRIPLE) = 0;
		return 1;
	case 0xb:
		TU_I32(ent, ENT_FROZEN) = 0;
		TU_I32(ent, ENT_AMBIENT) = -1;
		TU_I32(ent, ENT_AMBIENT + 4) = -1;
		TU_I32(ent, ENT_AMBIENT + 8) = -1;
		return 1;
	case 0xc:
		TU_I32(ent, ENT_PROTECTED) = 0;
		return 1;
	case 0x14:
		TU_I32(ent, ENT_TRIPLE) = 1;
		TankEnts_RemoveMessages(self, TU_I32(ent, ENT_ID), 0x15);
		tu_send(self, 0x15, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 5.0f);
		TU_F32(ent, ENT_SPEEDUP) = 0.0f;
		return 1;
	case 0x15:
		TU_I32(ent, ENT_TRIPLE) = 0;
		return 1;
	case 0x13:
		TU_I32(ent, ENT_PROTECTED) = 1;
		TankEnts_RemoveMessages(self, TU_I32(ent, ENT_ID), 0xc);
		if (TA_WorldLaps(TU_PTR(self, ENTS_WORLD)) != 0) {
			tu_send(self, 0xc, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 5.0f);
			return 1;
		}
		switch (TA_PlayersRank(TU_PTR(self, ENTS_PLAYERS), TU_I32(ent, ENT_ID))) {
		case 1:
			tu_send(self, 0xc, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 2.0f);
			return 1;
		case 2:
		case 3:
			tu_send(self, 0xc, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 3.0f);
			return 1;
		case 4:
			tu_send(self, 0xc, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 4.0f);
			return 1;
		}
		return 1;
	case 6:
		/* hit by data[0] */
		TA_PlayersShake(TU_PTR(self, ENTS_PLAYERS), TU_I32(ent, ENT_ID), 3);
		if (TU_ENT_TYPE(ent) == 9) {
			/* a rock: the root owner of what hit it is protected for 1 s and hurts the
			   tanks within 800 */
			root = TA_GetEntity(self, data[AI_ID0]);
			while (root != NULL && TU_I32(root, ENT_OWNER) != 0)
				root = TA_GetEntity(self, TU_I32(root, ENT_OWNER));
			rootId = 0;
			if (root != NULL) {
				rootId = TU_I32(root, ENT_ID);
				TU_I32(root, ENT_PROTECTED) = 1;
				TankEnts_RemoveMessages(self, rootId, 0xc);
				tu_send(self, 0xc, TU_I32(root, ENT_ID), TU_I32(root, ENT_ID), 1, 1.0f);
			}
			for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
				other = (char *)node[0];
				if (TU_I32(other, ENT_ID) == rootId || other == ent || TU_ENT_TYPE(other) != 1)
					continue;
				if (bmVectorDistance(TU_FP(other, ENT_POS), TU_FP(ent, ENT_POS)) < 800.0f) /* 0x002686f8 */
					tu_send(self, 6, rootId, TU_I32(node[0], ENT_ID), 1, 0.0f);
			}
		}
		if (TU_I32(ent, ENT_PROTECTED) != 0) {
			if (TA_GetEntity(self, data[AI_ID0]) != NULL)
				TA_GetEntity(self, data[AI_ID0]);
			TU_DEBUG("protected\n");
			return 1;
		}
		TU_F32(ent, ENT_SPEEDUP) = 0.0f;
		FUN_001ec260();
		tu_send(self, 8, TU_I32(ent, ENT_ID), data[AI_ID0], 1, 0.0f);
		player = tu_ai_player(ent + ENT_NAME, -1);
		if (tu_rand() % 3 == 1)
			CTankPlayers_PlayVoice(player, 2, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		if ((TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x10) != 0) {
			/* freezes: half the ambient light, thawed (0xb) after a time by rank */
			CTankLights_GetAmbient((struct TuLight *)TU_PTR(self, ENTS_LIGHTS), &TU_I32(ent, ENT_AMBIENT),
				&TU_I32(ent, ENT_AMBIENT + 4), &TU_I32(ent, ENT_AMBIENT + 8));
			TU_I32(ent, ENT_AMBIENT) = TU_I32(ent, ENT_AMBIENT) / 2;
			TU_I32(ent, ENT_AMBIENT + 4) = TU_I32(ent, ENT_AMBIENT + 4) / 2;
			TU_I32(ent, ENT_AMBIENT + 8) = TU_I32(ent, ENT_AMBIENT + 8) / 2;
			TU_I32(ent, ENT_FROZEN) = 1;
			TankEnts_RemoveMessages(self, TU_I32(ent, ENT_ID), 0xb);
			delay = -1.0f;
			switch (TA_PlayersRank(TU_PTR(self, ENTS_PLAYERS), TU_I32(ent, ENT_ID))) {
			case 1:
				delay = 3.0f;
				break;
			case 2:
			case 3:
				delay = 2.0f;
				break;
			case 4:
				delay = 1.0f;
				break;
			}
			if (delay >= 0.0f)
				tu_send(self, 0xb, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, delay);
		}
		if (TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_HIT_PRO) != 0.0f &&
			(TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x20) != 0) {
			TU_I32(ent, ENT_PROTECTED) = 1;
			TankEnts_RemoveMessages(self, TU_I32(ent, ENT_ID), 0xc);
			tu_send(self, 0xc, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1,
				TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_HIT_PRO));
		}
		if (TU_I32(TU_PTR(ent, ENT_TEMPLATE), TPL_HEALTH) > 0) {
			TU_I32(ent, ENT_HEALTH) = TU_I32(ent, ENT_HEALTH) - 1;
			if (TU_I32(ent, ENT_HEALTH) < 1) {
				killer = TA_GetEntity(self, data[AI_ID0]);
				tu_send(self, 7, TU_I32(ent, ENT_ID), data[AI_ID0], 1, 0.0f);
				TU_DEBUG("%s has been killed\n", ent + ENT_NAME);
				TU_I32(ent, ENT_SCORE) = TU_FTOL((float)TU_I32(ent, ENT_SCORE) *
					TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_SCORELOST));
				if (TA_WorldLaps(TU_PTR(self, ENTS_WORLD)) != 0 && killer != NULL) {
					Core_Vec4Copy(TU_FP(ent, ENT_POS), TU_FP(killer, ENT_POS));
					TU_F32(ent, ENT_POS + 12) = 1.0f;
				}
				TU_I32(ent, ENT_DEAD) = 1;
			}
		}
		if (TA_GetEntity(self, data[AI_ID0]) != NULL)
			TA_GetEntity(self, data[AI_ID0]);
		TU_DEBUG("hit\n");
		return 1;
	case 9:
		if (TA_GetEntity(self, data[AI_ID0]) != NULL)
			TA_GetEntity(self, data[AI_ID0]);
		TU_DEBUG("killed\n");
		return 1;
	case 8:
		killer = TA_GetEntity(self, data[AI_ID0]);
		if (killer == NULL)
			return 1;
		if (TU_I32(ent, ENT_OWNER) != 0) {
			/* a shell: the score goes to its owner */
			TU_DEBUG("passing a kill to the owner of %s\n", ent + ENT_NAME);
			tu_send(self, 8, data[AI_ID0], TU_I32(ent, ENT_OWNER), 1, 0.0f);
			return 1;
		}
		if (TU_I32(killer, ENT_FLAG) == 0)
			return 1;
		flag = TA_GetEntity(self, TU_I32(killer, ENT_FLAG));
		if (flag == NULL)
			return 1;
		if (TU_I32(ent, ENT_FLAG) != 0) {
			TU_I32(flag, ENT_OWNER) = 0;
			TU_I32(killer, ENT_FLAG) = 0;
			return 1;
		}
		TU_DEBUG("the flag changes hands\n");
		TU_I32(ent, ENT_FLAG) = TU_I32(killer, ENT_FLAG);
		TU_I32(killer, ENT_FLAG) = 0;
		TU_I32(flag, ENT_OWNER) = TU_I32(ent, ENT_ID);
		player = tu_ai_player(ent + ENT_NAME, -1);
		if (tu_rand() % 2 == 1)
			CTankPlayers_PlayVoice(player, 6, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		return 1;
	case 7:
		if (TU_I32(ent, ENT_OWNER) != 0) {
			TU_DEBUG("passing a kill to the owner of %s\n", ent + ENT_NAME);
			tu_send(self, 7, data[AI_ID0], TU_I32(ent, ENT_OWNER), 1, 0.0f);
			return 1;
		}
		killer = TA_GetEntity(self, data[AI_ID0]);
		if (killer != NULL) {
			TU_DEBUG("%s scores\n", ent + ENT_NAME);
			if (TU_I32(ent, ENT_ID) != TU_I32(killer, ENT_ID))
				TU_I32(ent, ENT_SCORE) = TU_I32(ent, ENT_SCORE) +
					TU_I32(TU_PTR(killer, ENT_TEMPLATE), TPL_SCOREGIVE);
		}
		return 1;
	case 10:
		TU_I32(ent, ENT_LOADED) = 1;
		return 1;
	case 0x18:
		TU_I32(ent, ENT_BUMP) = 1;
		return 1;
	case 4:
		TU_I32(ent, ENT_HAS_FLAG) = 1;
		TU_I32(ent, ENT_AI_STATE) = 2;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	case 5:
		TU_I32(ent, ENT_HAS_FLAG) = 0;
		TU_I32(ent, ENT_AI_STATE) = 1;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	case 0xd:
		/* respawn: the effect in the player's colour, then 0xe */
		TankSound_Play(TU_PTR(self, ENTS_SOUND), 0xe, TU_FP(ent, ENT_POS), 0xff, 1, 0);
		/* the original keeps the entity pointer as the index when the name is no player's */
		player = tu_ai_player(ent + ENT_NAME, 0);
		pos[0] = TU_F32(ent, ENT_POS);
		pos[2] = TU_F32(ent, ENT_POS + 8);
		DAT_00290e8c = DAT_002baed4[player * 10];
		pos[1] = 0.0f;
		pos[3] = 1.0f;
		debris = Debris_Add((struct DebrisType *)DAT_00291734, pos, DAT_002ea478, 0, 0xffffffffu);
		TU_F32(debris, 0x8c) = 3.0f;
		tu_send(self, 0xe, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1, 0.75f);
		return 1;
	case 0xe:
		/* respawned */
		TU_F32(ent, ENT_SPEEDUP) = 0.0f;
		TU_I32(ent, ENT_AI_STATE) = 1;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TA_ResetEntity(self, ent);
		Core_Vec4Copy(TU_FP(ent, ENT_ROT_START), TU_FP(ent, ENT_ROT));
		TU_I32(ent, ENT_RESPAWNING) = 0;
		TU_I32(ent, ENT_TRIPLE) = 0;
		if ((TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x20) != 0) {
			TU_I32(ent, ENT_PROTECTED) = 1;
			tu_send(self, 0xb, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1,
				TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_HIT_PRO));
		}
		if (TU_I32(ent, ENT_HAS_FLAG) != 0)
			tu_send(self, 4, 0, TU_I32(ent, ENT_ID), 1, 0.0f);
		player = tu_ai_player(ent + ENT_NAME, 0);
		TU_F32((void *)(intptr_t)DAT_002ecfac, 0xb8 + player * 4) = 180.0f;
		/* anything alive under the respawned tank dies */
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			other = (char *)node[0];
			if (other == ent || TU_I32(other, ENT_DEAD) != 0 || TU_ENT_TYPE(other) != 1)
				continue;
			f = TU_F32(ent, ENT_RADIUS) + TU_F32(other, ENT_RADIUS);
			if ((TU_F32(other, ENT_POS + 8) - TU_F32(ent, ENT_POS + 8)) *
				(TU_F32(other, ENT_POS + 8) - TU_F32(ent, ENT_POS + 8)) +
				(TU_F32(other, ENT_POS + 4) - TU_F32(ent, ENT_POS + 4)) *
				(TU_F32(other, ENT_POS + 4) - TU_F32(ent, ENT_POS + 4)) +
				(TU_F32(other, ENT_POS) - TU_F32(ent, ENT_POS)) *
				(TU_F32(other, ENT_POS) - TU_F32(ent, ENT_POS)) <= f * f)
				TU_I32(other, ENT_DEAD) = 1;
		}
		if (TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_HIT_PRO) != 0.0f &&
			(TU_U8(TU_PTR(ent, ENT_TEMPLATE), TPL_FLAGS) & 0x20) != 0) {
			TU_I32(ent, ENT_PROTECTED) = 1;
			TankEnts_RemoveMessages(self, TU_I32(ent, ENT_ID), 0xc);
			tu_send(self, 0xc, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 1,
				TU_F32(DAT_003150b0, 0)); /* CTank_Debug "Respawning inv" */
		}
		tu_respawn_flash(self, ent, player);
		return 1;
	case 3:
		break;
	default:
		return 0;
	}

	/* 3: the think tick */
	if (TU_ENT_TYPE(ent) == 9) {
		if (TU_I32(ent, ENT_ANIM_DONE) != 0 && TU_PTR(ent, ENT_ANIM_SEG) != NULL)
			baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_INST),
				(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_INST), 0x134),
				(struct BAnim *)TU_PTR(ent, ENT_ANIM_SEG), 0x400, -1, 0, (int (*)())FUN_001d7d60, ent);
		if (TU_I32(ent, ENT_TOUCHED) != 0) {
			other = TA_GetEntity(self, TU_I32(ent, ENT_TOUCHED));
			if (other != NULL && TU_ENT_TYPE(other) == 1)
				tu_send(self, 6, 0, TU_I32(ent, ENT_ID), 1, 0.0f);
		}
	}
	if (TU_I32(ent, ENT_FROZEN) != 0) {
		/* ice sparkles */
		for (i = 0; i < 3; i++) {
			dir[0] = Core_RandUnit() - 0.5f;
			dir[1] = 1.0f;
			dir[2] = Core_RandUnit() - 0.5f;
			dir[3] = 1.0f;
			bmVectorNormalise(dir, dir);
			cmb_vec3_scale(dir, dir, Core_RandUnit() * 10.0f); /* 0x002536d8 */
			if (i == 1) {
				if (tu_rand() % 10 == 1)
					TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 0, TU_FP(ent, ENT_POS), dir, 0, 0xffffffffu);
			}
			if (i == 1 || i == 2) {
				if (tu_rand() % 10 == 1)
					TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 1, TU_FP(ent, ENT_POS), dir, 0, 0xffffffffu);
			}
			if (i == 1 || i == 2 || i == 3) {
				if (tu_rand() % 10 == 1)
					TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 2, TU_FP(ent, ENT_POS), dir, 0, 0xffffffffu);
			}
		}
	}
	/* the speed-up runs out (by 1/180 of a frame time unit; not on odd frame times) */
	if (TU_F32(self, ENTS_DT) <= 0.0f || 20.0f <= TU_F32(self, ENTS_DT)) {
		TU_DEBUG("odd frame time\n");
	} else {
		f = TU_F32(ent, ENT_SPEEDUP) - TU_F32(self, ENTS_DT) * 0.00555555569f; /* 0x00263518 */
		TU_F32(ent, ENT_SPEEDUP) = f;
		if (0.0f <= f)
			TU_DEBUG("speed up left %f\n", (double)f);
		else
			TU_F32(ent, ENT_SPEEDUP) = 0.0f;
	}
	if (TU_F32(ent, ENT_SPEEDUP) <= 0.0f) {
		TU_I32(ent, ENT_MASS) = TU_I32(TU_PTR(ent, ENT_TEMPLATE), TPL_MASS);
		return 1;
	}
	TU_F32(ent, ENT_MASS) = DAT_00315100 * TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_MASS);
	return 1;
}

/* 0x001d9b50  TankedUp (gap; CTank_AI.cpp inferred)
The score and position of every tank, in list order. */
void TankEnts_GetTanks(void *self, float *positions, int *scores)
{
	void **node;
	char *ent;

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		ent = (char *)node[0];
		if (TU_ENT_TYPE(ent) != 1)
			continue;
		*scores++ = TU_I32(ent, ENT_SCORE);
		memcpy(positions, TU_FP(node[0], ENT_POS), 16);
		positions += 4;
	}
}

/* the Token Master's tables in CTank_Entities: +0x64 the token type of each spawn point
   (0: free), +0x68 + type * 4 the tokens of each type in the arena, +0x98 + type * 4 the
   most of each type (SETTINGS) */
#define ENTS_TOKEN_SLOTS 0x64
#define ENTS_TOKEN_COUNT 0x68
#define ENTS_TOKEN_MAX 0x98

/* 0x001d9bb0  TankedUp/CTank_AI.cpp
A free point of "TK_spawn_list", chosen at random: its position and rotation, and its
index, or -1. (`unused` is the caller's token type; the tanks' scores and positions are
gathered and not used.) */
int TankAI_GetTokenSpawn(void *self, int *unused, float *pos, float *rot)
{
	float *points;
	float *point;
	int *scores;
	float *positions;
	float total;
	float pick;
	int i;

	(void)unused;
	points = (float *)TA_SpawnGetPoints(TU_PTR(self, ENTS_SPAWN), "TK_spawn_list");
	scores = (int *)bkHeapCalloc((uint)(DAT_002bae94 * 4), 0, TU_FILE_AI, 0x295, 0x2001);
	positions = (float *)bkHeapCalloc((uint)(DAT_002bae94 << 4), 0, TU_FILE_AI, 0x296, 0x2001);
	TankEnts_GetTanks(self, positions, scores);
	if (points == NULL) {
		TU_DEBUG("Could not age spawn list!!\n");
		bkHeapFree(scores);
		bkHeapFree(positions);
		return -1;
	}
	total = 0.0f;
	i = 0;
	point = points;
	do {
		point[8] = 0.0f;
		if (TU_I32(TU_PTR(self, ENTS_TOKEN_SLOTS), i * 4) == 0) {
			total = total + 1.0f;
			point[8] = 1.0f;
		}
		point = *(float **)&point[9];
		i++;
	} while (point != NULL);
	pick = total * (float)tu_rand() * 3.05185094e-05f; /* 0x00253740 */
	i = 0;
	point = points;
	do {
		if (pick <= point[8])
			break;
		pick = pick - point[8];
		point = *(float **)&point[9];
		i++;
	} while (point != NULL);
	bkHeapFree(scores);
	bkHeapFree(positions);
	if (point != NULL && TU_I32(TU_PTR(self, ENTS_TOKEN_SLOTS), i * 4) == 0) {
		memcpy(pos, point, 16);
		memcpy(rot, point + 4, 16);
		return i;
	}
	return -1;
}

/* 0x001d9d30  TankedUp/CTank_AI.cpp
AI 5, "TOKEN MASTER": keeps the arena stocked with tokens (health, mines, points,
speed-up, protection, win switch, fire-up) up to the SETTINGS maxima, one per spawn
point. Event 1 messages 1 and 2 set up and free the spawn table, 0x1f is ignored; event 10
message 0x11 hears that a token (value data[4]) at spawn point data[5] was taken, message 3
restocks. The number of spawn points is a function-local static (0x0031598c, guard
0x00315990). */
int TankAI_TokenMaster(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *token;
	int *count;
	int type;
	int value;
	int id;
	int slot;
	int i;
	float pos[4];
	float rot[4];

	if ((DAT_00315990 & 1) == 0) {
		DAT_00315990 = DAT_00315990 | 1;
		DAT_0031598c = TA_SpawnGetCount(TU_PTR(self, ENTS_SPAWN), ent + ENT_SPAWNLIST);
	}
	if (event == 1) {
		if (data[AI_TYPE] == 1) {
			TU_I32(self, 0xe4) = TU_I32(ent, ENT_ID);
			DAT_0031598c = TA_SpawnGetCount(TU_PTR(self, ENTS_SPAWN), ent + ENT_SPAWNLIST);
			TU_PTR(self, ENTS_TOKEN_SLOTS) = bkHeapAlloc((uint)(DAT_0031598c * 4), TU_FILE_AI, 0x2d1, 0x2001);
			return 1;
		}
		if (data[AI_TYPE] == 2) {
			if (TU_PTR(self, ENTS_TOKEN_SLOTS) != NULL) {
				bkHeapFree(TU_PTR(self, ENTS_TOKEN_SLOTS));
				TU_PTR(self, ENTS_TOKEN_SLOTS) = NULL;
				return 1;
			}
			return 1;
		}
		if (data[AI_TYPE] == 0x1f)
			return 1;
		return 0;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 10;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("Starting 'TOKEN MASTER' AI for %s\n", ent + ENT_NAME);
		for (i = 0; i < DAT_0031598c; i++)
			TU_I32(TU_PTR(self, ENTS_TOKEN_SLOTS), i * 4) = 0;
		memset(TU_ADDR(self, ENTS_TOKEN_COUNT), 0, 0xb * 4);
		return 1;
	}
	if (event != 10)
		return 0;
	if (data[AI_TYPE] == 0x11) {
		/* a token was taken */
		value = data[AI_ARG];
		type = 0;
		if (value == -6)
			type = 10;
		if (value == -5)
			type = 9;
		if (value == -4)
			type = 8;
		if (value == -3)
			type = 6;
		if (value == -2)
			type = 1;
		if (value == -1)
			type = 2;
		if (value == 1)
			type = 3;
		if (value == 3)
			type = 4;
		if (value == 5)
			type = 5;
		else if (type == 0)
			TU_DEBUG("Token Master:  ERROR: NONE token returning info\n");
		count = &TU_I32(self, ENTS_TOKEN_COUNT + type * 4);
		*count = *count - 1;
		if (TU_I32(TU_PTR(self, ENTS_TOKEN_SLOTS), data[5] * 4) != type)
			TU_DEBUG("Token Master:  ERROR: TOKEN AT WRONGE POS\n");
		TU_I32(TU_PTR(self, ENTS_TOKEN_SLOTS), data[5] * 4) = 0;
		return 1;
	}
	if (data[AI_TYPE] != 3)
		return 0;
	/* restock: one token of each type that is short (types 1-10), at a free point */
	id = 1;
	value = 1;
	for (type = 1; type < 0xb; type++) {
		count = &TU_I32(self, ENTS_TOKEN_COUNT + type * 4);
		if (!(*count < TU_I32(self, ENTS_TOKEN_MAX + type * 4)))
			continue;
		slot = TankAI_GetTokenSpawn(self, &type, pos, rot);
		if (slot == -1)
			continue;
		switch (type) {
		case 1:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "health_TK", pos, rot, 0.0f);
			value = -2;
			break;
		case 2:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "mine_TK", pos, rot, 0.0f);
			value = -1;
			break;
		case 3:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "1Point_TK", pos, rot, 0.0f);
			value = 1;
			TU_DEBUG("1points: count = %d, max = %d\n", TU_I32(self, 0x74), TU_I32(self, 0xa4));
			break;
		case 4:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "3Points_TK", pos, rot, 0.0f);
			TU_DEBUG("3points: count = %d, max = %d\n", TU_I32(self, 0x78), TU_I32(self, 0xa8));
			value = 3;
			break;
		case 5:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "5Points_TK", pos, rot, 0.0f);
			TU_DEBUG("5points: count = %d, max = %d\n", TU_I32(self, 0x7c), TU_I32(self, 0xac));
			value = 5;
			break;
		case 6:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "speedup_TK", pos, rot, 0.0f);
			value = -3;
			break;
		case 8:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "pro_TK", pos, rot, 0.0f);
			value = -4;
			break;
		case 9:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "WIN_SWITCH_TK", pos, rot, 0.0f);
			value = -5;
			break;
		case 10:
			id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "FIRE_UP_TK", pos, rot, 0.0f);
			value = -6;
			break;
		}
		token = TA_GetEntity(self, id);
		/* the original writes through a null token when it was not made */
		if (token != NULL) {
			TU_I32(token, 0x984) = slot;
			TU_I32(token, 0x97c) = value;
		}
		TU_I32(TU_PTR(self, ENTS_TOKEN_SLOTS), slot * 4) = type;
		*count = *count + 1;
	}
	return 1;
}
