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
  +0x004 owner id     +0x08c AI state    +0x090 AI state changed   +0x098 AI drives
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
#define ENT_AI_ON 0x098      /* the AI drives (messages 4 and 5) */
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
		TU_I32(ent, ENT_AI_ON) = 1;
		TU_I32(ent, ENT_AI_STATE) = 2;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	case 5:
		TU_I32(ent, ENT_AI_ON) = 0;
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
		if (TU_I32(ent, ENT_AI_ON) != 0)
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

#define ENT_TOKEN_VALUE 0x97c   /* 1, 3, 5 points; -1 mine, -2 health, -3 speed-up, -4 protection,
                                   -5 win switch, -6 fire-up */
#define ENT_TOKEN_SLOT 0x984    /* the spawn point of a token */
#define ENT_TOKEN_TAKER 0xf04   /* who took a speed-up token */

/* a token's messages to the taker: type, then words (token, taker, 1, -, w4, slot) */
static void tu_token_send(void *self, char *token, int type, int taker, int w4, float delay)
{
	TankEnts_SendDelayedMessage(self, type, TU_U32(token, ENT_ID), (unsigned int)taker, 1, 0,
		(unsigned int)w4, TU_U32(token, ENT_TOKEN_SLOT), delay);
}

/* 0x001da1c0  TankedUp/CTank_AI.cpp
AI 4, "TOKEN": a token in the arena. Point tokens get a spinning CToken; a tank that
touches a token takes it: sparkles in its colour, maybe a voice, the token's message
(score 0x11, triple shot 0x14, speed-up 0x12, protection 0x13, health 0xf, mine 0x10; the
win switch picks 0x14 one time in three, else 0x13 for the leader and 0x12 for the rest),
then the token tells the Token Master (0x11). A speed-up token waits (state 7) until the
taker's boost wears off before it tells. */
int TankAI_Token(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *tank;
	void **node;
	void *p;
	float vel[4];
	unsigned int colour;
	int player;
	int type;
	int w4;
	int character;

	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 10;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("Starting 'TOKEN' AI for %s\n", ent + ENT_NAME);
		TU_I32(ent, ENT_AMBIENT) = 0x7f;
		TU_I32(ent, ENT_AMBIENT + 4) = 0x7f;
		TU_I32(ent, ENT_AMBIENT + 8) = 0x7f;
		switch (TU_I32(ent, ENT_TOKEN_VALUE)) {
		case 1:
			p = bkHeapAlloc(0x290, TU_FILE_AI, 0x37d, 0x1001);
			TU_PTR(ent, ENT_TOKEN) = p != NULL ? CToken_Init((struct CToken *)p, TU_FP(ent, ENT_POS), 0, 2.0f,
				NULL) : NULL;
			break;
		case 3:
			p = bkHeapAlloc(0x290, TU_FILE_AI, 0x37e, 0x1001);
			TU_PTR(ent, ENT_TOKEN) = p != NULL ? CToken_Init((struct CToken *)p, TU_FP(ent, ENT_POS), 1, 2.0f,
				NULL) : NULL;
			break;
		case 5:
			p = bkHeapAlloc(0x290, TU_FILE_AI, 0x37f, 0x1001);
			TU_PTR(ent, ENT_TOKEN) = p != NULL ? CToken_Init((struct CToken *)p, TU_FP(ent, ENT_POS), 2, 2.0f,
				NULL) : NULL;
			break;
		}
		if (TU_I32(ent, ENT_TOKEN_VALUE) < 1)
			TU_F32(ent, ENT_POS + 4) = 220.0f; /* power-ups float */
		CProjector_SpiralAt(TU_FP(ent, ENT_POS), -1);
		return 1;
	}
	if (event == 7) {
		if (data[AI_TYPE] != 0x1a)
			return 0;
		tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 7, 1.0f);
		tank = TA_GetEntity(self, TU_I32(ent, ENT_TOKEN_TAKER));
		TU_DEBUG("waiting for boost to wareoff '%s' before resporning\n", tank + ENT_NAME);
		if (tank == NULL) {
			tu_token_send(self, ent, 0x11, TU_I32(ent, ENT_OWNER), TU_I32(ent, ENT_TOKEN_VALUE), 0.0f);
			TU_I32(ent, ENT_RESPAWNING) = 0;
			/* the original goes on to read the boost of the missing tank */
			return 1;
		}
		TU_DEBUG("'%s' boost = %.6f\n", tank + ENT_NAME, (double)TU_F32(tank, ENT_SPEEDUP));
		if (TU_F32(tank, ENT_SPEEDUP) == 0.0f) {
			tu_token_send(self, ent, 0x11, TU_I32(ent, ENT_OWNER), TU_I32(ent, ENT_TOKEN_VALUE), 0.0f);
			TU_I32(ent, ENT_RESPAWNING) = 0;
		}
		return 1;
	}
	if (event != 10 || data[AI_TYPE] != 3)
		return 0;

	if (TU_I32(ent, ENT_ANIM_DONE) != 0 && TU_PTR(ent, ENT_ANIM_SEG) != NULL)
		baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_INST),
			(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_INST), 0x134),
			(struct BAnim *)TU_PTR(ent, ENT_ANIM_SEG), 0x400, -1, 0, (int (*)())FUN_001d7d60, ent);
	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		tank = (char *)node[0];
		if (tank == ent || TU_I32(tank, ENT_DEAD) != 0 || TU_ENT_TYPE(tank) != 1)
			continue;
		if (tu_touching(ent, tank))
			break;
	}
	if (node == NULL)
		return 1;

	/* taken by `tank` */
	tank = (char *)node[0];
	player = tu_ai_player(tank + ENT_NAME, 0);
	character = DAT_002baed4[player * 10] * 0x60;
	colour = ((unsigned int)DAT_00290718[character] << 16) | ((unsigned int)DAT_00290719[character] << 8) |
		DAT_0029071a[character];
	vel[0] = 0.0f;
	vel[1] = 5.0f;
	vel[2] = 0.0f;
	vel[3] = 1.0f;
	if (TU_I32(ent, ENT_TOKEN_VALUE) == 1)
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 3, TU_FP(tank, ENT_POS), vel, 0, colour);
	else if (TU_I32(ent, ENT_TOKEN_VALUE) == 3)
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 4, TU_FP(tank, ENT_POS), vel, 0, colour);
	else if (TU_I32(ent, ENT_TOKEN_VALUE) == 5)
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 5, TU_FP(tank, ENT_POS), vel, 0, colour);
	w4 = 0;
	switch (TU_I32(ent, ENT_TOKEN_VALUE)) {
	case 5:
		if (tu_rand() % 3 == 1)
			CTankPlayers_PlayVoice(player, 4, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		/* fall through */
	default:
		w4 = TU_I32(ent, ENT_TOKEN_VALUE);
		type = 0x11;
		break;
	case -6:
		if (tu_rand() % 4 == 1)
			CTankPlayers_PlayVoice(player, 5, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		type = 0x14;
		break;
	case -5:
		if (tu_rand() % 4 == 1)
			CTankPlayers_PlayVoice(player, 5, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		if (tu_rand() % 3 == 0)
			type = 0x14;
		else if (TU_CALL(FUN_001c83f0, int (*)(void *, int))(TU_PTR(self, ENTS_PLAYERS),
			TU_I32(tank, ENT_ID)) == 0)
			type = 0x12;
		else
			type = 0x13;
		break;
	case -4:
		if (tu_rand() % 4 == 1)
			CTankPlayers_PlayVoice(player, 5, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		type = 0x13;
		break;
	case -3:
		if (tu_rand() % 4 == 1)
			CTankPlayers_PlayVoice(player, 5, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		type = 0x12;
		break;
	case -2:
		if (tu_rand() % 4 == 1)
			CTankPlayers_PlayVoice(player, 5, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		type = 0xf;
		break;
	case -1:
		if (tu_rand() % 4 == 1)
			CTankPlayers_PlayVoice(player, 5, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
		type = 0x10;
		break;
	}
	tu_token_send(self, ent, type, TU_I32(tank, ENT_ID), w4, 0.0f);
	TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x10, TU_FP(ent, ENT_POS), 0x80, 1, 0);
	if (TU_I32(ent, ENT_TOKEN_VALUE) == -3) {
		TU_I32(ent, ENT_TOKEN_TAKER) = TU_I32(tank, ENT_ID);
		TU_I32(ent, ENT_DEAD) = 1;
		TU_I32(ent, ENT_RESPAWNING) = 1;
		tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 7, 1.0f);
		TU_I32(ent, ENT_AI_STATE) = 7;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	tu_token_send(self, ent, 0x11, TU_I32(ent, ENT_OWNER), TU_I32(ent, ENT_TOKEN_VALUE), 1.0f);
	TU_I32(ent, ENT_DEAD) = 1;
	return 1;
}

#define ENT_SPOT 0x960          /* the tracking spot light of an entity (CTankLights) */
#define ENT_HITS 0x940          /* what the world move hit this frame (bit 0: a wall) */
#define ENT_PUSH_FORCE 0x988

/* the crate's exit point in CTank_Entities: +0xd0 its position, +0xe0 its marker id */
#define ENTS_EXIT_POS 0xd0
#define ENTS_EXIT_ID 0xe0

/* tankedup_a 0x001ce270: a new entity id (++0x00315398) */
#define TA_NewEntityId(ents) TU_CALL(FUN_001ce270, int (*)(void *))(ents)

/* a spot of the light system following an entity */
static void tu_spot(void *self, char *ent, int r, int g, int b)
{
	CTankLights_SetSpot((struct TuLight *)TU_PTR(self, ENTS_LIGHTS), &TU_I32(ent, ENT_SPOT),
		TU_FP(ent, ENT_POS), r, g, b);
}

/* 0x001dab40  TankedUp/CTank_AI.cpp (bracketed)
AI 6, "DropBox": the crate of Drop Zone. States: 0xb new (appears at a random clear
"DropPoint Enter", an "ExitPoint" marker at a random clear "DropPoint Exit"), 0xc on the
ground (a tank that is not frozen picks it up), 0xd carried (follows its carrier; touching
a drop zone (type 8) scores and resets it), 0xe thrown (from a frozen tank). */
int TankAI_DropBox(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *other;
	char *carrier;
	char *marker;
	void **node;
	float *point;
	float rot[4];
	float f;
	int count;
	int id;
	int player;
	int r;

	if (event == 1) {
		if (data[AI_TYPE] != 3)
			return 0;
		if (TU_I32(self, ENTS_VARIANT) > 4)
			TU_I32(ent, ENT_DEAD) = 1;
		if (TU_I32(ent, ENT_ANIM_DONE) != 0 && TU_PTR(ent, ENT_ANIM_SEG) != NULL) {
			baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_INST),
				(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_INST), 0x134),
				(struct BAnim *)TU_PTR(ent, ENT_ANIM_SEG), 0x400, -1, 0, (int (*)())FUN_001d7d60, ent);
			return 1;
		}
		return 1;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 0xb;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("Starting 'DropBox' AI for %s\n", ent + ENT_NAME);
		return 1;
	}
	if (event == 0xb) {
		if (data[AI_TYPE] != 1)
			return 0;
		count = TA_SpawnGetCount(TU_PTR(self, ENTS_SPAWN), "DropPoint Enter");
		carrier = TA_GetEntity(self, TU_I32(ent, ENT_OWNER));
		if (carrier != NULL)
			TU_I32(carrier, ENT_FLAG) = 0;
		TU_I32(ent, ENT_OWNER) = 0;
		TU_F32(ent, ENT_VEL) = 0.0f;
		TU_F32(ent, ENT_VEL + 4) = 0.0f;
		TU_F32(ent, ENT_VEL + 8) = 0.0f;
		TU_F32(ent, ENT_VEL + 12) = 1.0f;
		TU_F32(ent, ENT_ROT + 12) = 1.0f;
		TU_F32(ent, ENT_ROT) = 0.0f;
		TU_F32(ent, ENT_ROT + 4) = 0.0f;
		TU_F32(ent, ENT_ROT + 8) = 0.0f;
		TU_F32(ent, ENT_POS + 12) = 1.0f;
		TU_F32(ent, ENT_POS) = 0.0f;
		TU_F32(ent, ENT_POS + 4) = 0.0f;
		TU_F32(ent, ENT_POS + 8) = 0.0f;
		TU_I32(ent, ENT_AMBIENT) = -1;
		TU_I32(ent, ENT_AMBIENT + 4) = -1;
		TU_I32(ent, ENT_AMBIENT + 8) = -1;
		TU_DEBUG("DropBox: restarting....\n");
		TU_DEBUG("num = %d\n", count);
		if (count == 0) {
			TU_DEBUG("DropBox: no drop points\n");
			TU_F32(ent, ENT_POS + 12) = 1.0f;
			TU_F32(ent, ENT_POS) = 0.0f;
			TU_F32(ent, ENT_POS + 4) = 1000.0f;
			TU_F32(ent, ENT_POS + 8) = 0.0f;
			TU_F32(self, ENTS_EXIT_POS + 12) = 1.0f;
			TU_F32(self, ENTS_EXIT_POS) = 0.0f;
			TU_F32(self, ENTS_EXIT_POS + 4) = 0.0f;
			TU_F32(self, ENTS_EXIT_POS + 8) = 0.0f;
			return 1;
		}
		tu_rand();
		do {
			do {
				point = (float *)TA_SpawnFind(TU_PTR(self, ENTS_SPAWN), "DropPoint Exit", tu_rand() % count);
			} while (point == NULL);
			memcpy(TU_ADDR(self, ENTS_EXIT_POS), point, 16);
		} while (TankAI_PositionClear(self, TU_FP(self, ENTS_EXIT_POS), ent) == 0);
		rot[0] = 0.0f;
		rot[1] = 0.0f;
		rot[2] = 0.0f;
		rot[3] = 1.0f;
		id = TankEnts_CreateEntity(self, 0, "ExitPoint", TU_FP(self, ENTS_EXIT_POS), rot, 0.0f);
		marker = TA_GetEntity(self, id);
		id = TA_NewEntityId(self);
		/* the original writes through a null marker when it was not made */
		if (marker != NULL) {
			TU_I32(marker, ENT_ID) = id;
			TU_F32(marker, ENT_POS + 4) = 0.0f;
		}
		TU_I32(self, ENTS_EXIT_ID) = id;
		do {
			do {
				point = (float *)TA_SpawnFind(TU_PTR(self, ENTS_SPAWN), "DropPoint Enter", tu_rand() % count);
			} while (point == NULL);
		} while (TankAI_PositionClear(self, point, ent) == 0);
		Core_Vec4Copy(TU_FP(ent, ENT_POS), point);
		TU_DEBUG("DropPoint Enter\n");
		r = tu_rand() % 2;
		if (r == 0)
			TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x11, TU_FP(ent, ENT_POS), 0x80, 1, 0);
		else if (r == 1)
			TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x12, TU_FP(ent, ENT_POS), 0x80, 1, 0);
		CProjector_SpiralAt(TU_FP(ent, ENT_POS), -1);
		TU_I32(ent, ENT_AI_STATE) = 0xc;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 0xc) {
		if (data[AI_TYPE] == 1) {
			TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_GRAVITY) = TA_WorldGravity(TU_PTR(self, ENTS_WORLD));
			TU_I32(ent, ENT_AMBIENT) = -1;
			TU_I32(ent, ENT_AMBIENT + 4) = -1;
			TU_I32(ent, ENT_AMBIENT + 8) = -1;
			carrier = TA_GetEntity(self, TU_I32(ent, ENT_OWNER));
			if (carrier != NULL)
				TU_I32(carrier, ENT_FLAG) = 0;
			TU_I32(ent, ENT_OWNER) = 0;
			return 1;
		}
		if (data[AI_TYPE] != 3)
			return 0;
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			other = (char *)node[0];
			if (other == ent || TU_I32(other, ENT_DEAD) != 0 || TU_ENT_TYPE(other) != 1 ||
				TU_I32(other, ENT_FLAG) != 0)
				continue;
			if (tu_touching(ent, other))
				break;
		}
		if (node == NULL) {
			tu_spot(self, ent, 0x7f, 0x7f, 0x7f);
			return 1;
		}
		if (TU_I32(other, ENT_FROZEN) != 0) {
			TU_I32(ent, ENT_AI_STATE) = 0xe;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		TU_I32(ent, ENT_OWNER) = TU_I32(other, ENT_ID);
		TU_DEBUG("picked up\n");
		TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x10, TU_FP(ent, ENT_POS), 0x80, 1, 0);
		TU_I32(other, ENT_FLAG) = TU_I32(ent, ENT_ID);
		TU_I32(ent, ENT_AI_STATE) = 0xd;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 0xd) {
		if (data[AI_TYPE] == 1) {
			carrier = TA_GetEntity(self, TU_I32(ent, ENT_OWNER));
			if (carrier != NULL && TU_I32(carrier, ENT_DEAD) == 0) {
				TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_GRAVITY) = 0.0f;
				/* the original keeps the name pointer as the index when it is no player's */
				player = tu_ai_player(carrier + ENT_NAME, 0);
				tu_spot(self, ent, (int)tu_player_colour(player, 0), (int)tu_player_colour(player, 1),
					(int)tu_player_colour(player, 2));
				return 1;
			}
		} else {
			if (data[AI_TYPE] != 3)
				return 0;
			carrier = TA_GetEntity(self, TU_I32(ent, ENT_OWNER));
			if (carrier != NULL && TU_I32(carrier, ENT_DEAD) == 0) {
				/* held 90 in front of the carrier */
				Core_Vec4Copy(TU_FP(ent, ENT_POS), TU_FP(carrier, ENT_POS));
				TU_F32(ent, ENT_POS + 8) = (float)sin((double)(-TU_F32(carrier, ENT_ROT + 4) - 1.57079625f)) *
					90.0f + TU_F32(ent, ENT_POS + 8); /* 0x00255180 */
				TU_F32(ent, ENT_POS) = (float)cos((double)(-TU_F32(carrier, ENT_ROT + 4) - 1.57079625f)) *
					90.0f + TU_F32(ent, ENT_POS);
				Core_Vec4Copy(TU_FP(ent, ENT_ROT), TU_FP(carrier, ENT_ROT));
				Core_Vec4Copy(TU_FP(ent, ENT_VEL), TU_FP(carrier, ENT_VEL));
				for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
					other = (char *)node[0];
					if (TU_ENT_TYPE(other) != 8 || !tu_touching(carrier, other))
						continue;
					/* delivered to a drop zone */
					TankEnts_SendDelayedMessage(self, 0x11, TU_U32(ent, ENT_ID), TU_U32(ent, ENT_OWNER), 1, 0,
						TU_U32(TU_PTR(ent, ENT_TEMPLATE), TPL_SCOREGIVE), 0, 0.0f);
					TankSound_Play(TU_PTR(self, ENTS_SOUND), 0xf, TU_FP(other, ENT_POS), 0x80, 1, 0);
					/* the original keeps the entity pointer as the index when the name is no
					   player's */
					player = tu_ai_player(carrier + ENT_NAME, 0);
					if (tu_rand() % 4 == 1)
						CTankPlayers_PlayVoice(player, 4, (int)(intptr_t)TU_FP(ent, ENT_POS), 0xff);
					TankEnts_SendDelayedMessage(self, 0x1a, TU_U32(ent, ENT_ID), TU_U32(other, ENT_ID), 1, 0,
						(unsigned int)player, 0, 0.0f);
					TU_F32(other, ENT_POS + 4) = -1000.0f;
					TU_I32(ent, ENT_AI_STATE) = 0xb;
					TU_I32(ent, ENT_AI_CHANGED) = 1;
				}
				tu_spot(self, ent, -1, -1, -1);
				return 1;
			}
		}
		TU_I32(ent, ENT_AI_STATE) = 0xc;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event != 0xe)
		return 0;
	if (data[AI_TYPE] == 1) {
		/* thrown out of a frozen tank's hands */
		TU_F32(TU_PTR(ent, ENT_TEMPLATE), TPL_GRAVITY) = TA_WorldGravity(TU_PTR(self, ENTS_WORLD));
		TU_I32(ent, ENT_AMBIENT) = -1;
		TU_I32(ent, ENT_AMBIENT + 4) = -1;
		TU_I32(ent, ENT_AMBIENT + 8) = -1;
		TU_F32(ent, ENT_ROT) = 0.0f;
		TU_F32(ent, ENT_ROT + 4) = 0.0f;
		TU_F32(ent, ENT_ROT + 8) = 0.0f;
		TU_F32(ent, ENT_ROT + 12) = 1.0f;
		f = Core_RandUnit();
		TU_F32(ent, ENT_VEL + 8) = (f + f) - 1.0f;
		TU_F32(ent, ENT_VEL + 4) = Core_RandUnit() * 0.1f; /* 0x002536cc */
		f = Core_RandUnit();
		TU_F32(ent, ENT_VEL) = (f + f) - 1.0f;
		TU_F32(ent, ENT_VEL + 12) = 1.0f;
		cmb_vec3_scale(TU_FP(ent, ENT_VEL), TU_FP(ent, ENT_VEL), TU_F32(ent, ENT_PUSH_FORCE));
		TU_I32(ent, ENT_RESTING) = 0;
		tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 0xe, 0.3f);
		return 1;
	}
	if (data[AI_TYPE] == 0x1a) {
		TU_I32(ent, ENT_AI_STATE) = 0xc;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (data[AI_TYPE] != 3)
		return 0;
	tu_spot(self, ent, 0x7f, 0x7f, 0x7f);
	return 1;
}

/* 0x001db5a0  TankedUp/CTank_AI.cpp (bracketed)
AI 1, "NoFloor": a tank shell. It leaves a trail in its owner's colour, glides a fixed
height (50) over the ground and sounds when it hits a wall; when it goes it sounds by what
it hit (a protected tank, a tank, or anything else). */
int TankAI_NoFloor(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *owner;
	float trail[4];
	float zero[4];
	float normal[4];
	float speed;
	float height;
	float d;
	float e;
	float f;
	unsigned int colour;
	int player;
	int i;

	if (event == 1) {
		if (data[AI_TYPE] != 2)
			return 0;
		if (TA_GetEntity(self, TU_I32(ent, ENT_TOUCHED)) == NULL) {
			switch (tu_rand() % 3) {
			case 0:
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 4, TU_FP(ent, ENT_POS), 0x80, 1, 0);
				return 1;
			case 1:
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 5, TU_FP(ent, ENT_POS), 0x80, 1, 0);
				return 1;
			case 2:
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 6, TU_FP(ent, ENT_POS), 0x80, 1, 0);
				return 1;
			}
		} else {
			if (TU_I32(ent, ENT_PROTECTED) != 0) {
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x15, TU_FP(ent, ENT_POS), 0x80, 1, 0);
				return 1;
			}
			switch (tu_rand() % 3) {
			case 0:
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 7, TU_FP(ent, ENT_POS), 200, 1, 0);
				return 1;
			case 1:
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 8, TU_FP(ent, ENT_POS), 200, 1, 0);
				return 1;
			case 2:
				TankSound_Play(TU_PTR(self, ENTS_SOUND), 9, TU_FP(ent, ENT_POS), 200, 1, 0);
				return 1;
			}
		}
		return 1;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 3;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("Starting 'NoFloor' AI for %s (for shell radius %d)\n", ent + ENT_NAME,
			(double)TU_F32(ent, ENT_RADIUS));
		TU_I32(ent, ENT_AMBIENT) = 0x7f;
		TU_I32(ent, ENT_AMBIENT + 4) = 0x7f;
		TU_I32(ent, ENT_AMBIENT + 8) = 0x7f;
		TU_I32(ent, ENT_TOKEN_VALUE) = 0;
		return 1;
	}
	if (event != 3 || data[AI_TYPE] != 3)
		return 0;
	owner = TA_GetEntity(self, TU_I32(ent, ENT_OWNER));
	if (owner != NULL) {
		/* the original keeps the name pointer as the index when it is no player's */
		player = tu_ai_player(owner + ENT_NAME, 0);
		colour = ((tu_player_colour(player, 0) << 8 | tu_player_colour(player, 1)) << 8) +
			tu_player_colour(player, 2);
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 7, TU_FP(ent, ENT_POS), DAT_002ea478, 0, colour);
		/* a second puff back along the velocity, past the frame's steps */
		i = 0;
		f = 0.0f;
		while (f < TU_F32(self, ENTS_DT)) {
			i++;
			f = (float)i;
		}
		trail[3] = TU_F32(ent, ENT_VEL + 12);
		f = (-1.0f / TU_F32(self, ENTS_DT)) * (float)(i + 1); /* 0x00253854 */
		trail[0] = TU_F32(ent, ENT_VEL) * f + TU_F32(ent, ENT_POS);
		trail[1] = TU_F32(ent, ENT_VEL + 4) * f + TU_F32(ent, ENT_POS + 4);
		trail[2] = TU_F32(ent, ENT_VEL + 8) * f + TU_F32(ent, ENT_POS + 8);
		TA_DebrisAdd(TU_PTR(self, ENTS_DEBRIS), 7, trail, DAT_002ea478, 0, colour);
	}
	TU_F32(ent, ENT_VEL + 4) = 0.0f;
	height = TA_WorldGroundHeight(TU_PTR(self, ENTS_WORLD), TU_FP(ent, ENT_POS), TU_F32(ent, ENT_RADIUS),
		normal);
	zero[0] = 0.0f;
	zero[1] = 0.0f;
	zero[2] = 0.0f;
	zero[3] = 1.0f;
	speed = bmVectorDistance(TU_FP(ent, ENT_VEL), zero) * 3.0f; /* 0x0025384c */
	d = 50.0f - height; /* 0x002559b8 */
	if (speed < d)
		d = speed;
	e = d;
	if (d < -speed) {
		e = 0.0f;
		if (-1000.0f < d)
			e = -speed;
	}
	TU_F32(ent, ENT_POS + 4) = e + TU_F32(ent, ENT_POS + 4);
	if ((TU_U8(ent, ENT_HITS) & 1) == 0)
		return 1;
	TankSound_Play(TU_PTR(self, ENTS_SOUND), 0x14, TU_FP(ent, ENT_POS), 0x80, 1, 0);
	TU_DEBUG("shell hit a wall\n");
	return 1;
}

/* 0x001dba50  TankedUp/CTank_AI.cpp (bracketed)
AI 9, "DropZone": a drop zone (a particle goal); message 0x1a (a crate delivered by player
data[4]) leaves a "test_temp" marker in the player's colour, bursts, hides the emitter and
ends the zone (0x1c). */
int TankAI_DropZone(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *marker;
	void *emitter;
	float away[4];
	int character;
	int id;

	if (event == 1) {
		if (data[AI_TYPE] != 3)
			return 0;
		if (TU_I32(ent, ENT_ANIM_DONE) != 0 && TU_PTR(ent, ENT_ANIM_SEG) != NULL) {
			baQueueAnimation((struct BActorInstance *)TU_PTR(ent, ENT_INST),
				(struct BActorNode *)TU_PTR(TU_PTR(ent, ENT_INST), 0x134),
				(struct BAnim *)TU_PTR(ent, ENT_ANIM_SEG), 0x400, -1, 0, (int (*)())FUN_001d7d60, ent);
			return 1;
		}
		return 1;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 0xf;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("AI:  Starting 'DropZone' AI for ent...%s\n", ent + ENT_NAME);
		emitter = BCarGenericGoal_AddGoal((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS),
			TU_FP(ent, ENT_POS), 1.0f, 0);
		TU_PTR(ent, ENT_EMITTER) = emitter;
		TU_EmitterSetOn(emitter, 1);
		return 1;
	}
	away[0] = 0.0f;
	away[1] = -100000.0f; /* 0xc7c35000 */
	away[2] = 0.0f;
	away[3] = 1.0f;
	if (event == 0xf) {
		if (data[AI_TYPE] != 0x1a)
			return 0;
		TU_F32(ent, ENT_POS + 4) = 0.0f;
		id = TankEnts_CreateEntity(self, TU_I32(ent, ENT_ID), "test_temp", TU_FP(ent, ENT_POS),
			TU_FP(ent, ENT_ROT), 0.0f);
		marker = TA_GetEntity(self, id);
		if (marker != NULL) {
			character = DAT_002baed4[data[AI_ARG] * 10] * 0x60;
			TU_I32(marker, ENT_AMBIENT) = DAT_00290718[character];
			TU_I32(marker, ENT_AMBIENT + 4) = DAT_00290719[character];
			TU_I32(marker, ENT_AMBIENT + 8) = DAT_0029071a[character];
			CParticleEffect_Burst((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS),
				(struct CParticleEmitter *)TU_PTR(ent, ENT_EMITTER), (short)DAT_00290718[character],
				(short)DAT_00290719[character], (short)DAT_0029071a[character], 2, 4.5f);
		}
		CParticleEmitter_SetPosition((struct CParticleEmitter *)TU_PTR(ent, ENT_EMITTER), away);
		tu_send(self, 0x1c, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 0xc, 0.0f);
		TU_I32(ent, ENT_DEAD) = 1;
		TU_I32(ent, ENT_AI_STATE) = 0xc;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 0xc && data[AI_TYPE] == 0x1c) {
		CParticleEmitter_SetPosition((struct CParticleEmitter *)TU_PTR(ent, ENT_EMITTER), away);
		TU_I32(ent, ENT_DEAD) = 1;
		return 1;
	}
	return 0;
}

/* the driving AIs' fields */
#define ENT_THINKS 0x980        /* think ticks */
#define ENT_WANDER 0x9b4        /* a wandering steer offset */
#define ENT_NEXT_DIST 0x990     /* the distance to the next waypoint when it was chosen */
#define ENT_PROGRESS 0x994      /* the lap progress (0-1) */
#define ENT_WAYPOINTS 0x97c     /* waypoints passed */
#define ENT_AIM 0x9d8           /* the route point to aim at */
#define ENT_HUNT_POS 0xa68      /* the hunted entity's position when the route was set */
#define ENT_ATTACK_POS 0xa78    /* the attacked entity's position at the last check */
#define ENT_MOVES 0xf00         /* the move queue (CTank_MoveQue) */
#define ENT_TARGET 0xf04        /* the entity hunted */

/* the waypoints (4) at the start of the route object */
#define TU_WAYPOINT(self, i) TU_FP(TU_PTR(self, ENTS_ROUTE), (i) * 0x10)

/* tankedup_a CTank_Route (thiscall): the next route point towards waypoint `index` (its
   distance) and the route's target */
#define TA_RouteGetWaypoint(route, pos, out, index) \
	TU_CALL(FUN_001c7cd0, float (*)(void *, float *, float *, int))(route, pos, out, index)
#define TA_RouteSetTarget(route, pos, index) \
	TU_CALL(FUN_001c7ac0, void (*)(void *, float *, int))(route, pos, index)
#define TA_IsHuman(players, id) TU_CALL(FUN_001c8460, int (*)(void *, int))(players, id)
#define TA_WorldCanSee(world, from, to) \
	TU_CALL(FUN_001cb7f0, int (*)(void *, float *, float *))(world, from, to)

/* a new move queue of `count` moves for an AI tank */
static void tu_new_moves(char *ent, int line, int count)
{
	void *p = bkHeapAlloc(0x14, TU_FILE_AI, line, 0x1001);

	TU_PTR(ent, ENT_MOVES) = p != NULL ? TankMoveQue_Ctor(p, count) : NULL;
}

static void tu_free_moves(char *ent)
{
	void *p = TU_PTR(ent, ENT_MOVES);

	if (p != NULL) {
		TankMoveQue_Free(p);
		bkHeapFree(p);
	}
	TU_PTR(ent, ENT_MOVES) = NULL;
}

/* replays the oldest queued move: steer and speed, fire, reverse; 1 when it reversed */
static int tu_replay_move(void *self, char *ent)
{
	float heading, speed, fire, reverse;

	TankMoveQue_Get(TU_PTR(ent, ENT_MOVES), &heading, &speed, &fire, &reverse);
	TankEnts_DriveAnalog(self, TU_I32(ent, ENT_ID),
		CLMath_WrapAngle(CLMath_WrapAngle(heading - TU_F32(ent, ENT_ROT + 4))), speed);
	if (fire != 0.0f)
		TankEnts_FireCommand(self, TU_I32(ent, ENT_ID));
	if (reverse != 0.0f) {
		TankEnts_Reverse(self, TU_I32(ent, ENT_ID));
		return 1;
	}
	return 0;
}

/* the feelers: two rays from the sides of the tank (DAT_003151c4 out) towards `aim` turned
   by DAT_003151c0 degrees; the steer correction DAT_003151bc degrees per difference */
static float tu_feelers(void *self, char *ent, float *aim)
{
	float left[4], right[4];
	float leftEnd[4], rightEnd[4];
	float fl, fr;

	left[0] = 0.0f;
	left[1] = 0.0f;
	left[2] = DAT_003151c4 + TU_F32(ent, ENT_RADIUS);
	left[3] = 1.0f;
	bmVectorRotateZXY(left, left, TU_F32(ent, ENT_ROT + 8), -TU_F32(ent, ENT_ROT),
		CLMath_WrapAngle(TU_F32(ent, ENT_ROT + 4) + 1.57079625f));
	cmb_vec3_add(left, left, TU_FP(ent, ENT_POS));
	TankPoint_RotateAround(leftEnd, aim, DAT_003151c0 * 0.0174532924f, left);
	TankPoint_SetDistance(leftEnd, 500.0f, left);
	right[2] = DAT_003151c4 + TU_F32(ent, ENT_RADIUS);
	right[0] = 0.0f;
	right[1] = 0.0f;
	right[3] = 1.0f;
	bmVectorRotateZXY(right, right, TU_F32(ent, ENT_ROT + 8), -TU_F32(ent, ENT_ROT),
		CLMath_WrapAngle(TU_F32(ent, ENT_ROT + 4) - 1.57079625f));
	cmb_vec3_add(right, right, TU_FP(ent, ENT_POS));
	TankPoint_RotateAround(rightEnd, aim, -(DAT_003151c0 * 0.0174532924f), right);
	TankPoint_SetDistance(rightEnd, 500.0f, right);
	fl = TankEnts_RayFraction(self, left, leftEnd, TU_I32(ent, ENT_ID));
	fr = TankEnts_RayFraction(self, right, rightEnd, TU_I32(ent, ENT_ID));
	return DAT_003151bc * 0.0174532924f * (fl - fr);
}

/* 1 when a live, unprotected tank or rock other than `ent` is in the firing cone
   (DAT_003150ec degrees) and range (DAT_003150f0) */
static int tu_target_in_sights(void *self, char *ent)
{
	void **node;
	char *other;
	int fire = 0;
	int type;

	for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
		other = (char *)node[0];
		if (other == ent || TU_I32(other, ENT_DEAD) != 0 || TU_I32(other, ENT_PROTECTED) != 0)
			continue;
		type = TU_ENT_TYPE(other);
		if (type != 1 && type != 9)
			continue;
		if (fabsf(TankEnts_AngleToPoint(self, ent, TU_FP(other, ENT_POS))) <
			DAT_003150ec * 0.0174532924f &&
			bmVectorDistance(TU_FP(ent, ENT_POS), TU_FP(node[0], ENT_POS)) < DAT_003150f0)
			fire = 1;
	}
	return fire;
}

/* what the tank touched: a rock is attacked; a tank sometimes (rand % 30000 <= frame time
   * 100); 0 when the touched entity is gone (no move is queued then) */
static int tu_touched(void *self, char *ent)
{
	char *other;

	if (TU_I32(ent, ENT_TOUCHED) == 0)
		return 1;
	other = TA_GetEntity(self, TU_I32(ent, ENT_TOUCHED));
	if (other == NULL)
		return 0;
	if (TU_ENT_TYPE(other) == 9) {
		TU_I32(ent, ENT_TARGET) = TU_I32(ent, ENT_TOUCHED);
		TU_I32(ent, ENT_AI_STATE) = 0x10;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
	}
	if (TU_ENT_TYPE(other) == 1 &&
		tu_rand() % 30000 <= TU_FTOL(TU_F32(self, ENTS_DT) * 100.0f)) { /* 0x00255544 */
		TU_I32(ent, ENT_TARGET) = TU_I32(ent, ENT_TOUCHED);
		TU_I32(ent, ENT_AI_STATE) = 0x10;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
	}
	return 1;
}

/* the fire flag of a queued move: the original stores the integer 1 in the float slot */
#define TU_FIRE_FLAG tu_bitsf(1)

/* 0x001dbd20  TankedUp/CTank_AI.cpp
AI 8: an AI tank of the race. It follows the four route waypoints (counting them both
ways), wanders a little, steers with its feelers, fires at what is in its sights, goes
for speed-up tokens it can see (0x11), attacks rocks and sometimes tanks it bumps into
(0x10) and backs off when stuck (0x12). Its moves pass through a queue whose length is
its reaction time by difficulty. */
int TankAI_Tank(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *other;
	void **node;
	float aim[4];
	float d;
	float f;
	float steer;
	float angle;
	float speed;
	float fire;
	int count;
	int i;

	if (event == 1) {
		if (data[AI_TYPE] == 1) {
			TU_I32(ent, ENT_AI_STATE) = 7;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			TU_I32(ent, ENT_ROUTENUM) = 0;
			f = Core_RandUnit();
			f = (f + f) - 1.0f;
			TU_I32(ent, ENT_WAYPOINTS) = 0;
			TU_F32(ent, ENT_WANDER) = DAT_003150fc * 0.0174532924f * f;
			TU_F32(ent, ENT_NEXT_DIST) = bmVectorDistance(TU_FP(ent, ENT_POS),
				TU_WAYPOINT(self, TU_I32(ent, ENT_ROUTENUM))) - 1000.0f; /* 0x00255898 */
			if (DAT_002bae70 == 0)
				tu_new_moves(ent, 0x939, 0x14);
			else if (DAT_002bae70 == 1)
				tu_new_moves(ent, 0x93a, 0xf);
			else if (DAT_002bae70 == 2)
				tu_new_moves(ent, 0x93b, 2);
			return 1;
		}
		if (data[AI_TYPE] == 2) {
			tu_free_moves(ent);
			return 1;
		}
		if (data[AI_TYPE] != 3)
			return 0;
		TU_I32(ent, ENT_THINKS) = TU_I32(ent, ENT_THINKS) + 1;
		if (bmVectorDistance(TU_FP(ent, ENT_POS), TU_WAYPOINT(self, TU_I32(ent, ENT_ROUTENUM))) < 1000.0f) {
			TU_I32(ent, ENT_ROUTENUM) = TU_I32(ent, ENT_ROUTENUM) + 1;
			TU_I32(ent, ENT_WAYPOINTS) = TU_I32(ent, ENT_WAYPOINTS) + 1;
			TU_DEBUG("next waypoint\n");
			if (TU_I32(ent, ENT_ROUTENUM) == 4)
				TU_I32(ent, ENT_ROUTENUM) = 0;
			TU_F32(ent, ENT_NEXT_DIST) = bmVectorDistance(TU_FP(ent, ENT_POS),
				TU_WAYPOINT(self, TU_I32(ent, ENT_ROUTENUM))) - 1000.0f;
		}
		if (bmVectorDistance(TU_FP(ent, ENT_POS), TU_WAYPOINT(self, (TU_I32(ent, ENT_ROUTENUM) + 2) % 4)) <
			1000.0f) {
			/* back at the previous one: the wrong way */
			TU_I32(ent, ENT_ROUTENUM) = TU_I32(ent, ENT_ROUTENUM) - 1;
			TU_I32(ent, ENT_WAYPOINTS) = TU_I32(ent, ENT_WAYPOINTS) - 1;
			TU_DEBUG("previous waypoint\n");
			if (TU_I32(ent, ENT_ROUTENUM) == -1)
				TU_I32(ent, ENT_ROUTENUM) = 3;
			TU_F32(ent, ENT_NEXT_DIST) = bmVectorDistance(TU_FP(ent, ENT_POS),
				TU_WAYPOINT(self, TU_I32(ent, ENT_ROUTENUM))) - 1000.0f;
		}
		f = (float)TU_I32(ent, ENT_WAYPOINTS) * 0.0833333358f + 0.0833333358f; /* 0x0026c594 */
		f = f - (bmVectorDistance(TU_FP(ent, ENT_POS), TU_WAYPOINT(self, TU_I32(ent, ENT_ROUTENUM))) -
			1000.0f) / (TU_F32(ent, ENT_NEXT_DIST) * 12.0f); /* 0x00254f78 */
		TU_F32(ent, ENT_PROGRESS) = f;
		if (1.0f < f)
			TU_F32(ent, ENT_PROGRESS) = 1.0f;
		if (TU_I32(ent, ENT_AI_ON) != 0)
			tu_replay_move(self, ent);
		return 1;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_DEBUG("AI tank starts\n");
		TA_RouteGetWaypoint(TU_PTR(self, ENTS_ROUTE), TU_FP(ent, ENT_POS), TU_FP(ent, ENT_AIM),
			TU_I32(ent, ENT_ROUTENUM));
		TU_I32(ent, ENT_AI_STATE) = 7;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 7) {
		if (data[AI_TYPE] != 3)
			return 0;
		f = Core_RandUnit();
		TU_F32(ent, ENT_WANDER) = TU_F32(ent, ENT_WANDER) * 0.899999976f + /* 0x002672b4 */
			DAT_003150fc * 0.0174532924f * ((f + f) - 1.0f) * 0.100000001f;
		if (TU_I32(ent, ENT_AI_ON) == 0)
			return 1;
		count = TU_I32(ent, ENT_THINKS);
		TU_I32(ent, ENT_THINKS) = count + 1;
		if (count % 5 == 3)
			TA_RouteGetWaypoint(TU_PTR(self, ENTS_ROUTE), TU_FP(ent, ENT_POS), TU_FP(ent, ENT_AIM),
				TU_I32(ent, ENT_ROUTENUM));
		aim[0] = TU_F32(ent, ENT_AIM);
		aim[1] = TU_F32(ent, ENT_AIM + 4);
		aim[2] = TU_F32(ent, ENT_AIM + 8);
		aim[3] = 1.0f;
		steer = CLMath_WrapAngle(TankEnts_AngleToPoint(self, ent, aim) + TU_F32(ent, ENT_WANDER));
		steer = CLMath_WrapAngle(tu_feelers(self, ent, aim) + steer);
		fire = 0.0f;
		speed = DAT_00315160;
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			other = (char *)node[0];
			if (other == ent)
				continue;
			if (TU_I32(ent, ENT_PROTECTED) == 0 && TU_F32(ent, ENT_SPEEDUP) == 0.0f && TU_ENT_TYPE(other) == 5) {
				/* a token worth turning for */
				d = bmVectorDistance(TU_FP(ent, ENT_POS), TU_FP(other, ENT_POS));
				angle = TankEnts_AngleToPoint(self, ent, TU_FP(node[0], ENT_POS));
				angle = CLMath_WrapAngle(angle - TankEnts_AngleToPoint(self, ent, aim));
				if (3.1415925f < angle)
					angle = angle - 6.28318548f;
				if (fabsf(angle * 0.318309903f * DAT_00315168) + d < DAT_00315164 && /* 0x002649e8 */
					TA_WorldCanSee(TU_PTR(self, ENTS_WORLD), TU_FP(ent, ENT_POS), TU_FP(node[0], ENT_POS)) != 0) {
					TU_I32(ent, ENT_TARGET) = TU_I32(node[0], ENT_ID);
					TU_I32(ent, ENT_AI_STATE) = 0x11;
					TU_I32(ent, ENT_AI_CHANGED) = 1;
				}
			}
			/* the original tests the aim point here, not the other entity */
			if (TU_I32(node[0], ENT_PROTECTED) == 0 &&
				(TU_ENT_TYPE(node[0]) == 1 || TU_ENT_TYPE(node[0]) == 9) &&
				fabsf(TankEnts_AngleToPoint(self, ent, aim)) < DAT_003150ec * 0.0174532924f &&
				bmVectorDistance(TU_FP(ent, ENT_POS), aim) < DAT_003150f0)
				fire = TU_FIRE_FLAG;
		}
		if (tu_touched(self, ent) == 0)
			return 1;
		TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), steer + TU_F32(ent, ENT_ROT + 4), speed, fire, 0.0f);
		return 1;
	}
	if (event == 0x11) {
		/* going for a token */
		if (data[AI_TYPE] == 1) {
			tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 0x11, 1.0f);
			return 1;
		}
		if (data[AI_TYPE] == 0x1a) {
			TU_I32(ent, ENT_AI_STATE) = 7;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		if (data[AI_TYPE] != 3)
			return 0;
		other = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
		if (other == NULL) {
			TU_I32(ent, ENT_AI_STATE) = 7;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		angle = TankEnts_AngleToPoint(self, ent, TU_FP(other, ENT_POS));
		speed = DAT_00315160;
		fire = tu_target_in_sights(self, ent) ? TU_FIRE_FLAG : 0.0f;
		if (TU_I32(ent, ENT_TOUCHED) != 0) {
			other = TA_GetEntity(self, TU_I32(ent, ENT_TOUCHED));
			if (other == NULL || TU_ENT_TYPE(other) != 9)
				return 1;
			TU_I32(ent, ENT_TARGET) = TU_I32(ent, ENT_TOUCHED);
			TU_I32(ent, ENT_AI_STATE) = 0x10;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
		}
		TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), angle + TU_F32(ent, ENT_ROT + 4), speed, fire, 0.0f);
		return 1;
	}
	if (event == 0x12) {
		/* backing off */
		if (data[AI_TYPE] == 1) {
			tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 0x12, 0.5f);
			TU_F32(ent, ENT_ATTACK_POS) = 0.0f;
			TU_F32(ent, ENT_ATTACK_POS + 4) = 0.0f;
			TU_F32(ent, ENT_ATTACK_POS + 8) = 0.0f;
			TU_F32(ent, ENT_ATTACK_POS + 12) = 1.0f;
			return 1;
		}
		if (data[AI_TYPE] == 0x1a) {
			TU_I32(ent, ENT_AI_STATE) = 7;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		if (data[AI_TYPE] != 3)
			return 0;
		TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), TU_F32(ent, ENT_ROT + 4), 0.0f, 0.0f, TU_FIRE_FLAG);
		return 1;
	}
	if (event == 0x10) {
		/* attacking what it bumped into */
		if (data[AI_TYPE] == 1) {
			tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 0x10, 1.0f);
			other = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
			if (other == NULL) {
				TU_I32(ent, ENT_AI_STATE) = 7;
				TU_I32(ent, ENT_AI_CHANGED) = 1;
				return 1;
			}
			if (100.0f <= bmVectorDistance(TU_FP(ent, ENT_ATTACK_POS), TU_FP(other, ENT_POS))) {
				Core_Vec4Copy(TU_FP(ent, ENT_ATTACK_POS), TU_FP(other, ENT_POS));
				return 1;
			}
			/* it has not moved: back off */
			TU_I32(ent, ENT_AI_STATE) = 0x12;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		if (data[AI_TYPE] == 0x1a) {
			TankEnts_FireCommand(self, TU_I32(ent, ENT_ID));
		} else {
			if (data[AI_TYPE] != 3)
				return 0;
			other = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
			if (other != NULL) {
				angle = TankEnts_AngleToPoint(self, ent, TU_FP(other, ENT_POS));
				TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), angle + TU_F32(ent, ENT_ROT + 4), DAT_00315160, 0.0f,
					0.0f);
				speed = DAT_00315160;
				TankEnts_DriveAnalog(self, TU_I32(ent, ENT_ID),
					TankEnts_AngleToPoint(self, ent, TU_FP(other, ENT_POS)), speed);
				fire = tu_target_in_sights(self, ent) ? TU_FIRE_FLAG : 0.0f;
				TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), angle + TU_F32(ent, ENT_ROT + 4), speed, fire, 0.0f);
				return 1;
			}
		}
		TU_I32(ent, ENT_AI_STATE) = 7;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 8 && data[AI_TYPE] == 3)
		return 1;
	(void)i;
	return 0;
}

/* 0x001dcbd0  TankedUp/CTank_AI.cpp (bracketed)
AI 10: drives to route waypoint 4 (until within 200) without firing; on creation a goal
effect at (202, 213, -2215) bursts in the player's colour. */
int TankAI_Layer(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	float point[4];
	void *emitter;
	int player;
	int character;

	if (event != 1)
		return 0;
	if (data[AI_TYPE] == 1) {
		point[0] = 202.0f;
		point[1] = 213.0f;
		point[2] = -2215.0f;
		point[3] = 1.0f;
		emitter = BCarGenericGoal_AddGoal((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS), point, 1.0f,
			data[AI_TYPE]);
		TU_PTR(ent, ENT_EMITTER) = emitter;
		TU_EmitterSetOn(emitter, 0);
		TU_EmitterSetColour(TU_PTR(ent, ENT_EMITTER), 0x646464);
		/* the original keeps the entity pointer as the index when the name is no player's */
		player = tu_ai_player(ent + ENT_NAME, 0);
		character = DAT_002baed4[player * 10] * 0x60;
		CParticleEffect_Burst((struct CParticleEffect *)TU_PTR(self, ENTS_EFFECTS),
			(struct CParticleEmitter *)TU_PTR(ent, ENT_EMITTER), (short)DAT_00290718[character],
			(short)DAT_00290719[character], (short)DAT_0029071a[character], 4, 4.5f);
		return 1;
	}
	if (data[AI_TYPE] == 2) {
		tu_free_moves(ent);
		return 1;
	}
	if (data[AI_TYPE] != 3)
		return 0;
	TU_F32(ent, ENT_VEL) = 0.0f;
	TU_F32(ent, ENT_VEL + 4) = 0.0f;
	TU_F32(ent, ENT_VEL + 8) = 0.0f;
	TU_F32(ent, ENT_VEL + 12) = 1.0f;
	if (200.0f < TA_RouteGetWaypoint(TU_PTR(self, ENTS_ROUTE), TU_FP(ent, ENT_POS), point, 4)) /* 0x002536f0 */
		TankEnts_DriveAnalog(self, TU_I32(ent, ENT_ID),
			CLMath_WrapAngle(TankEnts_AngleToPoint(self, ent, point)), 1.0f);
	TU_I32(ent, ENT_LOADED) = 0;
	return 1;
}

/* aims the route at an entity (10 above it) and remembers its position */
static void tu_hunt(void *self, char *ent, char *target)
{
	float aim[4];

	aim[0] = TU_F32(target, ENT_POS);
	aim[2] = TU_F32(target, ENT_POS + 8);
	aim[1] = TU_F32(target, ENT_POS + 4) + 10.0f; /* 0x002536d8 */
	aim[3] = TU_F32(target, ENT_POS + 12);
	TA_RouteSetTarget(TU_PTR(self, ENTS_ROUTE), aim, TU_I32(ent, ENT_ROUTENUM));
	memcpy(ent + ENT_HUNT_POS, target + ENT_POS, 16);
}

/* 0x001dcde0  TankedUp/CTank_AI.cpp
AI 2, "Hunter": an AI tank of the arena games. It picks a target (4: tokens, enemy
command points, flags; 5: the nearest drop zone while it carries the crate), hunts it along
the route with its feelers (7), fires at enemies in its sights, avenges hits now and then,
attacks rocks and tanks it bumps into (0x10) and waits while frozen (0xf). */
int TankAI_Hunter(void *self, void *entity, int event, int *data)
{
	char *ent = (char *)entity;
	char *target;
	char *other;
	void **node;
	float aim[4];
	float best;
	float d;
	float steer;
	float speed;
	float fire;
	int count;
	int type;
	int id;

	if (event == 1) {
		switch (data[AI_TYPE]) {
		case 0x1e:
			if (TU_I32(ent, ENT_TOUCHED) == data[AI_ID0]) {
				target = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
				if (target != NULL) {
					tu_hunt(self, ent, target);
					TU_I32(ent, ENT_AI_STATE) = 7;
					TU_I32(ent, ENT_AI_CHANGED) = 1;
					return 1;
				}
			}
			return 1;
		case 0x1d:
			if (TU_I32(ent, ENT_TOUCHED) != 0 &&
				TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(ent, ENT_TOUCHED), TU_I32(ent, ENT_ID)) == 0) {
				TankEnts_FireCommand(self, TU_I32(ent, ENT_ID));
				if (TU_I32(ent, ENT_TOUCHED) == data[AI_ID0]) {
					tu_send(self, 0x1e, TU_I32(ent, ENT_TOUCHED), TU_I32(ent, ENT_ID), 1, Core_RandUnit());
					return 1;
				}
			}
			return 1;
		case 1:
			if (DAT_002bae70 == 0)
				tu_new_moves(ent, 0x75c, 0x1e);
			else if (DAT_002bae70 == 1)
				tu_new_moves(ent, 0x75d, 2);
			else if (DAT_002bae70 == 2)
				tu_new_moves(ent, 0x75e, 2);
			return 1;
		case 2:
			tu_free_moves(ent);
			return 1;
		case 3:
			TU_I32(ent, ENT_THINKS) = TU_I32(ent, ENT_THINKS) + 1;
			if (TU_I32(ent, ENT_AI_ON) == 0)
				return 1;
			if (TU_I32(ent, ENT_FROZEN) != 0) {
				TU_I32(ent, ENT_AI_STATE) = 0xf;
				TU_I32(ent, ENT_AI_CHANGED) = 1;
				return 1;
			}
			if (TU_I32(ent, ENT_TOUCHED) != 0)
				tu_send(self, 0x1d, TU_I32(ent, ENT_TOUCHED), TU_I32(ent, ENT_ID), 1, Core_RandUnit());
			if (TU_I32(TU_PTR(self, ENTS_WORLD), 0) != 0) {
				target = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
				if (target == NULL) {
					TU_I32(ent, ENT_AI_STATE) = 4;
					TU_I32(ent, ENT_AI_CHANGED) = 1;
					return 1;
				}
				TA_RouteSetTarget(TU_PTR(self, ENTS_ROUTE), TU_FP(target, ENT_POS), TU_I32(ent, ENT_ROUTENUM));
				Core_Vec4Copy(TU_FP(ent, ENT_HUNT_POS), TU_FP(target, ENT_POS));
			}
			tu_replay_move(self, ent);
			return 1;
		}
		return 0;
	}
	if (event == 2) {
		if (data[AI_TYPE] != 1)
			return 0;
		TU_I32(ent, ENT_AI_STATE) = 4;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TU_DEBUG("AI:  Starting 'Hunter' AI for tank...%s\n", ent + ENT_NAME);
		return 1;
	}
	if (event == 4) {
		/* choose a target */
		if (data[AI_TYPE] != 3)
			return 0;
		best = 999999.0f;
		TU_I32(ent, ENT_TARGET) = 0;
		if (TU_I32(ent, ENT_FLAG) != 0) {
			TU_I32(ent, ENT_AI_STATE) = 5;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			other = (char *)node[0];
			if (TU_I32(other, ENT_DEAD) != 0)
				continue;
			if (TU_ENT_TYPE(other) == 7 &&
				TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(other, ENT_OWNER), TU_I32(ent, ENT_ID)) == 0 &&
				TA_IsHuman(TU_PTR(self, ENTS_PLAYERS), TU_I32(node[0], ENT_ID)) != 0 && tu_rand() % 2 == 1)
				continue;
			type = TU_ENT_TYPE(node[0]);
			if (!(type == 5 || (type == 7 && TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS),
				TU_I32(node[0], ENT_OWNER), TU_I32(ent, ENT_ID)) == 0) || TU_ENT_TYPE(node[0]) == 3))
				continue;
			other = (char *)node[0];
			if (TU_ENT_TYPE(other) == 3) {
				id = TU_I32(other, ENT_ID);
				if (id == TU_I32(ent, ENT_ID)) {
					TU_I32(ent, ENT_AI_STATE) = 5;
					TU_I32(ent, ENT_AI_CHANGED) = 1;
					return 1;
				}
				if (TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), id, TU_I32(ent, ENT_ID)) != 0)
					continue;
			}
			aim[0] = TU_F32(other, ENT_POS);
			aim[2] = TU_F32(other, ENT_POS + 8);
			aim[1] = TU_F32(other, ENT_POS + 4) + 10.0f;
			aim[3] = TU_F32(other, ENT_POS + 12);
			d = bmVectorDistance(aim, TU_FP(ent, ENT_POS));
			d = Core_RandUnit() * DAT_003150f4 + d;
			other = (char *)node[0];
			if (TU_ENT_TYPE(other) == 5 && TU_I32(other, ENT_TOKEN_VALUE) > 0 &&
				TU_I32(other, ENT_TOKEN_VALUE) != 1) {
				if (TU_I32(other, ENT_TOKEN_VALUE) == 3)
					d = d * 0.5f;
				else if (TU_I32(other, ENT_TOKEN_VALUE) == 5)
					d = d * 0.333333343f; /* 0x0025df94 */
			}
			if (d < best) {
				TU_I32(ent, ENT_TARGET) = TU_I32(other, ENT_ID);
				best = d;
			}
		}
		target = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
		if (target == NULL) {
			TU_DEBUG("Hunter: no target\n");
			return 1;
		}
		aim[0] = TU_F32(target, ENT_POS);
		aim[2] = TU_F32(target, ENT_POS + 8);
		aim[1] = TU_F32(target, ENT_POS + 4) + 10.0f;
		aim[3] = TU_F32(target, ENT_POS + 12);
		TA_RouteSetTarget(TU_PTR(self, ENTS_ROUTE), aim, TU_I32(ent, ENT_ROUTENUM));
		memcpy(ent + ENT_HUNT_POS, target + ENT_POS, 16);
		TU_I32(ent, ENT_AI_STATE) = 7;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 5) {
		/* carrying the crate: the nearest drop zone */
		if (data[AI_TYPE] != 3)
			return 0;
		TU_I32(ent, ENT_TARGET) = 0;
		best = 999999.0f;
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			other = (char *)node[0];
			if (TU_ENT_TYPE(other) != 8)
				continue;
			aim[0] = TU_F32(other, ENT_POS);
			aim[2] = TU_F32(other, ENT_POS + 8);
			aim[1] = TU_F32(other, ENT_POS + 4) + 100.0f; /* 0x00255544 */
			aim[3] = TU_F32(other, ENT_POS + 12);
			d = bmVectorDistance(aim, TU_FP(ent, ENT_POS));
			if (d < best) {
				TU_I32(ent, ENT_TARGET) = TU_I32(node[0], ENT_ID);
				best = d;
			}
		}
		target = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
		if (target == NULL) {
			TU_DEBUG("Hunter: no drop zone\n");
			return 1;
		}
		aim[0] = TU_F32(target, ENT_POS);
		aim[2] = TU_F32(target, ENT_POS + 8);
		aim[1] = TU_F32(target, ENT_POS + 4) + 10.0f;
		aim[3] = TU_F32(target, ENT_POS + 12);
		TA_RouteSetTarget(TU_PTR(self, ENTS_ROUTE), aim, TU_I32(ent, ENT_ROUTENUM));
		memcpy(ent + ENT_HUNT_POS, target + ENT_POS, 16);
		TU_I32(ent, ENT_AI_STATE) = 7;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		TA_RouteGetWaypoint(TU_PTR(self, ENTS_ROUTE), TU_FP(ent, ENT_POS), aim, TU_I32(ent, ENT_ROUTENUM));
		return 1;
	}
	if (event == 0xf) {
		/* frozen */
		if (data[AI_TYPE] != 3)
			return 0;
		if (TU_I32(ent, ENT_FROZEN) == 0) {
			TU_I32(ent, ENT_AI_STATE) = 4;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
		}
		return 1;
	}
	if (event == 7) {
		/* hunting */
		if (data[AI_TYPE] == 6) {
			/* hit: now and then (Vindictiveness) go for the root owner of what hit it */
			if (TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(ent, ENT_ID), data[AI_ID0]) != 0 ||
				!(Core_RandUnit() < DAT_003150f8))
				return 1;
			TU_I32(ent, ENT_TARGET) = data[AI_ID0];
			other = TA_GetEntity(self, data[AI_ID0]);
			while (other != NULL) {
				id = TU_I32(other, ENT_OWNER);
				if (id == 0)
					return 1;
				TU_I32(ent, ENT_TARGET) = id;
				other = TA_GetEntity(self, id);
			}
			return 1;
		}
		if (data[AI_TYPE] == 1) {
			TankEnts_RemoveMessages(self, TU_I32(ent, ENT_ID), 0x1a);
			tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 7, 4.0f);
			return 1;
		}
		if (data[AI_TYPE] == 0x1a) {
			TU_I32(ent, ENT_AI_STATE) = 4;
			TU_I32(ent, ENT_AI_CHANGED) = 1;
			return 1;
		}
		if (data[AI_TYPE] != 3)
			return 0;
		fire = 0.0f;
		target = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
		if (target == NULL || TU_I32(target, ENT_DEAD) != 0 || TU_I32(target, ENT_FROZEN) != 0)
			goto rethink;
		if (TU_ENT_TYPE(target) == 7 &&
			TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(target, ENT_OWNER), TU_I32(ent, ENT_ID)) != 0 &&
			!(tu_rand() % 0x1e != 0xe && DAT_002bae70 == 0))
			goto rethink;
		if (TU_ENT_TYPE(target) == 3) {
			if (TU_I32(target, ENT_OWNER) == TU_I32(ent, ENT_ID)) {
				TU_I32(ent, ENT_AI_STATE) = 5;
				TU_I32(ent, ENT_AI_CHANGED) = 1;
				return 1;
			}
			if (TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(target, ENT_OWNER), TU_I32(ent, ENT_ID)) != 0)
				goto rethink;
		}
		if (TU_ENT_TYPE(target) == 8 && TU_I32(ent, ENT_FLAG) == 0)
			goto rethink;
		if (50.0f < bmVectorDistance(TU_FP(ent, ENT_HUNT_POS), TU_FP(target, ENT_POS))) { /* 0x002559b8 */
			TA_RouteSetTarget(TU_PTR(self, ENTS_ROUTE), TU_FP(target, ENT_POS), TU_I32(ent, ENT_ROUTENUM));
			Core_Vec4Copy(TU_FP(ent, ENT_HUNT_POS), TU_FP(target, ENT_POS));
		}
		if (tu_touching(ent, target))
			goto rethink;
		count = TU_I32(ent, ENT_THINKS);
		TU_I32(ent, ENT_THINKS) = count + 1;
		if (count % 5 == 3) {
			d = TA_RouteGetWaypoint(TU_PTR(self, ENTS_ROUTE), TU_FP(ent, ENT_POS), TU_FP(ent, ENT_AIM),
				TU_I32(ent, ENT_ROUTENUM));
			if (d < 10.0f) {
				TU_I32(ent, ENT_AI_STATE) = 4;
				TU_I32(ent, ENT_AI_CHANGED) = 1;
				return 1;
			}
			if (999999.0f < d) /* 0x002640e4 */
				goto rethink;
		}
		memcpy(aim, ent + ENT_AIM, 16);
		steer = CLMath_WrapAngle(TankEnts_AngleToPoint(self, ent, aim));
		steer = CLMath_WrapAngle(tu_feelers(self, ent, aim) + steer);
		speed = DAT_00315160;
		for (node = (void **)TU_PTR(self, ENTS_LIST); node != NULL; node = (void **)node[1]) {
			other = (char *)node[0];
			if (other == ent || TU_I32(other, ENT_DEAD) != 0 || TU_I32(other, ENT_PROTECTED) != 0)
				continue;
			if (TA_IsFriendly(TU_PTR(self, ENTS_PLAYERS), TU_I32(other, ENT_ID), TU_I32(ent, ENT_ID)) != 0 ||
				TU_ENT_TYPE(node[0]) != 1)
				continue;
			/* the original tests the aim point here, not the other tank */
			if (!(bmVectorDistance(TU_FP(ent, ENT_POS), aim) < DAT_003150f0))
				continue;
			if (fabsf(TankEnts_AngleToPoint(self, ent, aim)) < DAT_003150ec * 0.0174532924f ||
				(char *)node[0] == target) {
				fire = TU_FIRE_FLAG;
			} else if (DAT_002bae70 == 2 && TA_IsHuman(TU_PTR(self, ENTS_PLAYERS), TU_I32(node[0], ENT_ID)) != 0) {
				/* hard: sometimes switch to a human nearby */
				if (bmVectorDistance(TU_FP(ent, ENT_POS), aim) < 1000.0f && TU_ENT_TYPE(target) == 7 &&
					tu_rand() % 10 == 3)
					TU_I32(ent, ENT_TARGET) = TU_I32(node[0], ENT_ID);
				if (TU_ENT_TYPE(target) == 5 && tu_rand() % 10 == 3)
					TU_I32(ent, ENT_TARGET) = TU_I32(node[0], ENT_ID);
			}
			if ((char *)node[0] == target &&
				fabsf(TankEnts_AngleToPoint(self, ent, aim)) < DAT_003150ec * 3.0f * 0.0174532924f)
				fire = TU_FIRE_FLAG;
		}
		if (tu_touched(self, ent) == 0)
			return 1;
		TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), steer + TU_F32(ent, ENT_ROT + 4), speed, fire, 0.0f);
		return 1;
	rethink:
		TU_I32(ent, ENT_AI_STATE) = 4;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 0x10) {
		/* attacking what it bumped into */
		if (data[AI_TYPE] == 1) {
			tu_send(self, 0x1a, TU_I32(ent, ENT_ID), TU_I32(ent, ENT_ID), 0x10, 1.0f);
			return 1;
		}
		if (data[AI_TYPE] == 0x1a) {
			TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), TU_F32(ent, ENT_ROT + 4), 0.0f, TU_FIRE_FLAG, 0.0f);
		} else {
			if (data[AI_TYPE] != 3)
				return 0;
			target = TA_GetEntity(self, TU_I32(ent, ENT_TARGET));
			if (target != NULL) {
				TankEnts_DriveAnalog(self, TU_I32(ent, ENT_ID),
					TankEnts_AngleToPoint(self, ent, TU_FP(target, ENT_POS)), DAT_00315160);
				steer = TankEnts_AngleToPoint(self, ent, TU_FP(target, ENT_POS));
				speed = DAT_00315160;
				fire = tu_target_in_sights(self, ent) ? TU_FIRE_FLAG : 0.0f;
				TankMoveQue_Put(TU_PTR(ent, ENT_MOVES), steer + TU_F32(ent, ENT_ROT + 4), speed, fire, 0.0f);
				return 1;
			}
		}
		TU_I32(ent, ENT_AI_STATE) = 4;
		TU_I32(ent, ENT_AI_CHANGED) = 1;
		return 1;
	}
	if (event == 8 && data[AI_TYPE] == 3)
		return 1;
	return 0;
}
