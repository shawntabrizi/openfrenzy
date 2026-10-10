/*
TankedUp/unplaced_entities.c  (gap functions of C:\BlitzParty\Game\Src\TankedUp\; the file is
probably CTank_Entities.cpp: 0x001ce130 prints "CTank_Entities::RouteMessage", and
tankedup_b continues that file at 0x001cee60)

CTank_Entities (CTank_Game +0xb20, "entitySystem"; tankedup_b owns most of it): the first
methods: the constructor, links, pause, the message routing to the entities' AI state
handlers, the lookups by id that the players and the camera use, the per-frame steps, and
the initialisation of a new entity.

An entity list node is { entity *, next }. Entity fields are reached at their byte offsets
(tankedup_b's tankedup_b.h names them ENT_*; the names are repeated in the comments).
Messages are 0x30 bytes: +0x00 kind (1 enter, 2 exit, 3 update, 4 start), +0x18 sender id,
+0x1c target id, +0x20 the state they go to (0: the current one, 1: the global handler),
+0x24 a copy of the kind, +0x28 a flag. The state handlers belong to tankedup_b.
*/

#include "tankedup_a.h"

/* the entity system's fields this file uses (tankedup_b: ENTS_*) */
#define TE_LIST 0x00                 /* ENTS_LIST: the first node */
#define TE_WORLD 0x04                /* ENTS_WORLD */
#define TE_CAMERA 0x20               /* ENTS_CAMERA */
#define TE_PLAYERS 0x24              /* ENTS_PLAYERS */
#define TE_MESSAGES 0x58             /* ENTS_MESSAGES: the delayed message queue */
#define TE_STARTED 0x60
#define TE_DT 0xc8                   /* ENTS_DT: the frame time */

/* entity fields (tankedup_b: ENT_*) */
#define EN_ID 0x000
#define EN_NAME 0x008
#define EN_STATE 0x088               /* the AI state now */
#define EN_NEXT_STATE 0x08c
#define EN_CHANGE_STATE 0x090        /* a state change is due */
#define EN_DEAD 0x094
#define EN_TEMPLATE 0x91c
#define EN_AI 0x924                  /* the AI type 1-10 */
#define EN_KILLS 0x92c
#define EN_HEALTH 0x930
#define EN_MINES 0x934
#define EN_ALWAYS_READY 0x938
#define EN_ON_GROUND 0x93c
#define EN_STUNNED 0x944
#define EN_ACTIVE 0x948
#define EN_SHELL 0x94c
#define EN_LAP 0x950
#define EN_ANIM_DONE 0x964
#define EN_TURNING 0x978
#define EN_PROGRESS 0x994
#define EN_ANIM_SPEED 0x998
#define EN_SPEED_TIME 0x9ac
#define EN_RADIUS 0x9b0
#define EN_CATCH_UP 0x9bc
#define EN_POS 0xa08
#define EN_HEADING 0xa2c
#define EN_TIMER 0xa88               /* a struct TuTimer (life time) */
#define EN_INST 0xad0
#define EN_TURRET_INST 0xad8
#define EN_LOW_INST 0xae0
#define EN_ANIM_SEG 0xef4

/* template fields (tankedup_b: TPL_*) */
#define TP_HEALTH 0x08
#define TP_MINES 0x10
#define TP_TYPE 0x1c                 /* 1: a tank */
#define TP_ROT 0x2c                  /* turn speed */
#define TP_LIFE 0x34
#define TP_RELOAD 0x50

/* a message (0x30 bytes) */
struct TuMessage {
	int kind;                    /* +0x00 */
	int unknown04[5];            /* +0x04 */
	int sender;                  /* +0x18 */
	int target;                  /* +0x1c */
	int state;                   /* +0x20 */
	int kindCopy;                /* +0x24 */
	int flag;                    /* +0x28 */
	int unknown2c;               /* +0x2c */
};
OF_ASSERT_SIZE(struct TuMessage, 0x30);

/* a delayed message in the queue (tankedup_b): +0x00 kind, +0x08 the tick it is due,
   +0x10 the tick the game was paused, +0x1c the target id */
#define TQ_KIND 0x00
#define TQ_DUE 0x08
#define TQ_PAUSED 0x10
#define TQ_TARGET 0x1c

/* the AI state handlers of tankedup_b: (entities, entity, state, message + 0x18) */
typedef int (*TuStateHandler)(void *, void *, int, void *);
#define TUB_State_Default ((TuStateHandler)FUN_001d8820)

/* tankedup_b: an entity's animation callback (missing from the function table) */
int FUN_001d7d60(int reason, void *instance, void *node, void *anim, void *context);

/* classlib_fx: deletes every token */
#define TU_TOKENS_DELETE_ALL() CToken_DeleteAll()

static void *ents_first(void *self)
{
	return TU_PTR(self, TE_LIST);
}

#define NODE_ENTITY(node) TU_PTR(node, 0)
#define NODE_NEXT(node) TU_PTR(node, 4)

/* the entity with id `id`, or NULL (the node may hold a NULL entity) */
static void **ents_find_node(void *self, int id)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1])
		if (TU_I32(node[0], EN_ID) == id)
			return node;
	return NULL;
}

/* 0x001cdb20  TankedUp (gap)
The constructor. */
void *CTankEntities_Ctor(void *self)
{
	TU_I32(self, 0x58) = 0;
	TU_I32(self, 0x00) = 0;
	TU_I32(self, 0x04) = 0;
	TU_I32(self, 0x5c) = 0;
	TU_I32(self, 0x28) = 0;
	TU_I32(self, 0xf0) = 0;
	return self;
}

/* 0x001cdb40  TankedUp (gap)
Links the camera and the player system. */
void CTankEntities_SetLinks(void *self, void *camera, void *players)
{
	TU_PTR(self, TE_CAMERA) = camera;
	TU_PTR(self, TE_PLAYERS) = players;
}

/* 0x001cdb60  TankedUp (gap)
Starts the living entities: the life timers (LIFE) and the animations (looped through the
entity's animation callback). */
void CTankEntities_Start(void *self)
{
	void **node;

	TU_I32(self, TE_STARTED) = 0;
	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		void *ent = node[0];

		TU_DPRINTF("'%s' starts at [%.0f,%.0f,%.0f]\n", (char *)TU_P(ent, EN_NAME),
			(double)TU_F32(ent, EN_POS), (double)TU_F32(ent, EN_POS + 4),
			(double)TU_F32(ent, EN_POS + 8));
		if (TU_I32(ent, EN_DEAD) != 0)
			continue;
		if (TU_F32(TU_PTR(ent, EN_TEMPLATE), TP_LIFE) != 0.0f) {
			CTankTimer_SetLength((struct TuTimer *)TU_P(ent, EN_TIMER),
				TU_F32(TU_PTR(ent, EN_TEMPLATE), TP_LIFE));
			CTankTimer_StartOrResume((struct TuTimer *)TU_P(ent, EN_TIMER));
		}
		TU_I32(ent, EN_ANIM_DONE) = 0;
		if (TU_PTR(ent, EN_ANIM_SEG) != NULL) {
			void *instance = TU_PTR(ent, EN_INST);

			baQueueAnimation((struct BActorInstance *)instance,
				(struct BActorNode *)TU_PTR(instance, 0x134), (struct BAnim *)TU_PTR(ent, EN_ANIM_SEG),
				0x400, -1, 0, (int (*)())FUN_001d7d60, ent);
		}
	}
}

/* 0x001cdc30  TankedUp (gap)
Brings an entity back to life from its template: health, mines, the life timer. (A
__thiscall method that does not use `this`; written without it, like 0x001cebd0.) */
void CTankEntities_Revive(void *ent)
{
	void *template = TU_PTR(ent, EN_TEMPLATE);

	CTankTimer_Reset((struct TuTimer *)TU_P(ent, EN_TIMER));
	TU_I32(ent, EN_HEALTH) = TU_I32(template, TP_HEALTH);
	TU_I32(ent, 0x95c) = 0;
	TU_I32(ent, EN_DEAD) = 0;
	TU_I32(ent, EN_MINES) = TU_I32(template, TP_MINES);
	if (TU_F32(template, TP_LIFE) != 0.0f) {
		CTankTimer_SetLength((struct TuTimer *)TU_P(ent, EN_TIMER), TU_F32(template, TP_LIFE));
		CTankTimer_StartOrResume((struct TuTimer *)TU_P(ent, EN_TIMER));
	}
}

/* 0x001cdca0  TankedUp (gap)
Pauses the life timers and notes the pause time of every delayed message. */
void CTankEntities_Pause(void *self)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1])
		CTankTimer_Pause((struct TuTimer *)TU_P(node[0], EN_TIMER));
	for (node = (void **)TU_PTR(self, TE_MESSAGES); node != NULL; node = (void **)node[1])
		TU_AT(node[0], TQ_PAUSED, unsigned long long) = bkTimerRead();
}

/* 0x001cdcf0  TankedUp (gap)
Resumes the life timers and delays every delayed message by the time of the pause. */
void CTankEntities_Resume(void *self)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1])
		CTankTimer_StartOrResume((struct TuTimer *)TU_P(node[0], EN_TIMER));
	for (node = (void **)TU_PTR(self, TE_MESSAGES); node != NULL; node = (void **)node[1]) {
		void *message = node[0];

		TU_AT(message, TQ_DUE, unsigned long long) +=
			bkTimerDelta(TU_AT(message, TQ_PAUSED, unsigned long long), bkTimerRead());
	}
}

/* 0x001cdd50  TankedUp (gap; no `this`)
Draws a glow sprite (200 x 200) at `pos`, turned by `angle`. */
void CTankEntities_DrawGlow(float *pos, float angle, void *texture)
{
	bdSetWorldMatrix(DAT_002855f0);
	bdPushRenderState(3, 0, 0);
	bdPushRenderState(6, 1, 0);
	bdPushRenderState(7, 2, 0);
	bdPushRenderState(10, 1, 0);
	bdPushRenderState(5, 0, 0);
	bdSetDepthOffset(-1.0f);
	bdDrawSprite3D(pos, 200.0f, 200.0f, angle, (struct bdTexture *)texture, 0, 0, 0x7f, 0x7f, 0x7f,
		0x30, NULL, 0.0f);
	bdPopRenderState(3);
	bdPopRenderState(6);
	bdPopRenderState(10);
	bdPopRenderState(7);
	bdPopRenderState(5);
	bdSetDepthOffset(0.0f);
}

/* 0x001cde00  TankedUp (gap)
The id of the entity called `name` (0 when there is none). */
int CTankEntities_FindId(void *self, const char *name)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1])
		if (_stricmp((char *)TU_P(node[0], EN_NAME), name) == 0)
			return TU_I32(node[0], EN_ID);
	return 0;
}

/* the state handler of AI type `ai` (1-10; other types have none) */
static TuStateHandler ents_handler(int ai)
{
	switch (ai) {
	case 1:
		return (TuStateHandler)FUN_001db5a0;
	case 2:
		return (TuStateHandler)FUN_001dcde0;
	case 3:
		return (TuStateHandler)FUN_001d84e0;
	case 4:
		return (TuStateHandler)FUN_001da1c0;
	case 5:
		return (TuStateHandler)FUN_001d9d30;
	case 6:
		return (TuStateHandler)FUN_001dab40;
	case 7:
		return (TuStateHandler)FUN_001d7e10;
	case 8:
		return (TuStateHandler)FUN_001dbd20;
	case 9:
		return (TuStateHandler)FUN_001dba50;
	case 10:
		return (TuStateHandler)FUN_001dcbd0;
	}
	return NULL;
}

/* 0x001cde40  TankedUp (gap)
Hands message `message` to entity `ent` in state `state`: to its AI type's handler, which
for the global state (1) is followed by the default handler; without an AI type, to the
default handler. Returns the last handler's result. */
int CTankEntities_SendMessageNow(void *self, void *ent, int state, void *message)
{
	struct TuMessage *msg = (struct TuMessage *)message;
	TuStateHandler handler;

	msg->kindCopy = msg->kind;
	handler = ents_handler(TU_I32(ent, EN_AI));
	if (state != 1) {
		if (handler != NULL)
			return handler(self, ent, state, &msg->sender);
		return TUB_State_Default(self, ent, state, &msg->sender);
	}
	if (handler != NULL)
		handler(self, ent, 1, &msg->sender);
	return TUB_State_Default(self, ent, 1, &msg->sender);
}

/* 0x001ce100  TankedUp (gap)
The entity with id `id`, or NULL. */
void *CTankEntities_Get(void *self, int id)
{
	void **node = ents_find_node(self, id);

	return node != NULL ? node[0] : NULL;
}

/* 0x001ce130  TankedUp/CTank_Entities.cpp (inferred: "CTank_Entities::RouteMessage")
Delivers a message to its target entity: state 0 to the current state (the global state
when that does not take it), state 1 to the global state and then the current one, another
state only when the entity is in it. */
void CTankEntities_RouteMessage(void *self, void *message)
{
	struct TuMessage *msg = (struct TuMessage *)message;
	void **node = ents_find_node(self, msg->target);
	void *ent;

	if (node == NULL || (ent = node[0]) == NULL) {
		TU_DPRINTF("CTank_Entities::RouteMessage - target entity doesnt exist anymore\n");
		return;
	}
	if (msg->state == 0) {
		if (CTankEntities_SendMessageNow(self, ent, TU_I32(ent, EN_STATE), msg) == 0)
			CTankEntities_SendMessageNow(self, ent, 1, msg);
	} else if (msg->state == 1) {
		CTankEntities_SendMessageNow(self, ent, 1, msg);
		if (TU_I32(ent, EN_STATE) != 1)
			CTankEntities_SendMessageNow(self, ent, TU_I32(ent, EN_STATE), msg);
	} else if (msg->state == TU_I32(ent, EN_STATE)) {
		CTankEntities_SendMessageNow(self, ent, TU_I32(ent, EN_STATE), msg);
	}
}

/* 0x001ce1f0  TankedUp (gap)
Changes an entity's AI state to its next state: "exit" to the old state (unless it is the
global state), "enter" to the new one. */
void CTankEntities_ChangeState(void *self, void *ent)
{
	struct TuMessage msg;
	int next;

	memset(&msg, 0, sizeof msg);   /* the other words are uninitialised in the original */
	msg.sender = TU_I32(ent, EN_ID);
	msg.flag = 0;
	TU_I32(ent, EN_CHANGE_STATE) = 0;
	msg.target = msg.sender;
	if (TU_I32(ent, EN_STATE) != 1) {
		msg.kind = 2;
		CTankEntities_SendMessageNow(self, ent, TU_I32(ent, EN_STATE), &msg);
	}
	next = TU_I32(ent, EN_NEXT_STATE);
	TU_I32(ent, EN_STATE) = next;
	msg.kind = 1;
	if (next != 1)
		CTankEntities_SendMessageNow(self, ent, next, &msg);
}

/* 0x001ce270  TankedUp (gap; no `this`)
Counts up the entity id counter (0x00315398) without making an entity. */
void CTankEntities_SkipId(void)
{
	TU_VAR(int, 0x00315398) = TU_VAR(int, 0x00315398) + 1;
}

/* 0x001ce280  TankedUp (gap)
An entity's destructor part: its life timer's (empty, 0x001d5950). */
void CTankEntities_EntityDtor(void *ent)
{
	(void)ent;
}

/* 0x001ce290  TankedUp (gap)
Sends "enter" and then "start" to the global state of every entity. */
void CTankEntities_Setup(void *self)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		struct TuMessage msg;

		memset(&msg, 0, sizeof msg);
		msg.flag = 0;
		msg.target = TU_I32(node[0], EN_ID);
		msg.sender = TU_I32(node[0], EN_ID);
		msg.state = 1;
		msg.kind = 1;
		CTankEntities_SendMessageNow(self, node[0], 1, &msg);
		msg.kind = 4;
		CTankEntities_SendMessageNow(self, node[0], 1, &msg);
	}
}

/* 0x001ce300  TankedUp (gap)
Deletes the tokens and sends "exit" to the global state of every entity. */
void CTankEntities_Shutdown(void *self)
{
	void **node;

	TU_TOKENS_DELETE_ALL();
	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		struct TuMessage msg;

		memset(&msg, 0, sizeof msg);
		msg.flag = 0;
		msg.target = TU_I32(node[0], EN_ID);
		msg.sender = TU_I32(node[0], EN_ID);
		msg.kind = 2;
		CTankEntities_SendMessageNow(self, node[0], 1, &msg);
	}
}

/* 0x001ce350  TankedUp (gap)
Whether entity `id` is alive. */
int CTankEntities_IsAlive(void *self, int id)
{
	void **node = ents_find_node(self, id);

	if (node != NULL && node[0] != NULL)
		return TU_I32(node[0], EN_DEAD) == 0;
	return 0;
}

/* 0x001ce390  TankedUp (gap)
Entity `id`'s kills, health, mines, laps, and whether it finished the race (its laps are
the world's LAPS). Nothing is written when there is no such entity. */
void CTankEntities_GetStats(void *self, int id, int *kills, int *health, int *mines, int *laps,
	int *finished)
{
	void **node = ents_find_node(self, id);
	void *ent;

	if (node == NULL || (ent = node[0]) == NULL)
		return;
	*kills = TU_I32(ent, EN_KILLS);
	*health = TU_I32(ent, EN_HEALTH);
	*mines = TU_I32(ent, EN_MINES);
	*laps = TU_I32(ent, EN_LAP);
	*finished = TU_I32(ent, EN_LAP) == CTankWorld_GetLaps(TU_PTR(self, TE_WORLD));
}

/* 0x001ce410  TankedUp (gap)
Whether entity `id` can act: active, alive and not stunned. */
int CTankEntities_CanAct(void *self, int id)
{
	void **node = ents_find_node(self, id);
	void *ent;

	if (node == NULL || (ent = node[0]) == NULL)
		return 0;
	if (TU_I32(ent, EN_ACTIVE) == 0)
		return 0;
	if (TU_I32(ent, EN_DEAD) != 0)
		return 0;
	if (TU_I32(ent, EN_STUNNED) != 0)
		return 0;
	return 1;
}

/* 0x001ce460  TankedUp (gap)
Sets entity `id`'s catch-up factor from its lead (in race units): 1 - 2 * lead, kept in
0.7 .. 1. */
void CTankEntities_SetLead(void *self, int id, float lead)
{
	void **node = ents_find_node(self, id);
	void *ent;
	float factor;

	if (node == NULL || (ent = node[0]) == NULL)
		return;
	factor = 1.0f - (lead + lead);
	if (factor <= 0.7f)
		factor = 0.7f;
	TU_F32(ent, EN_CATCH_UP) = factor;
	if (1.0f < factor)
		factor = 1.0f;
	TU_F32(ent, EN_CATCH_UP) = factor;
}

/* 0x001ce4d0  TankedUp (gap)
Entity `id`'s race progress (its progress times 480, at least 0; 0 when there is none). */
int CTankEntities_GetProgress(void *self, int id)
{
	void **node = ents_find_node(self, id);
	int progress;

	if (node == NULL || node[0] == NULL)
		return 0;
	progress = TU_FTOL(TU_F32(node[0], EN_PROGRESS) * 480.0f);
	return progress < 0 ? 0 : progress;
}

/* the blink of an icon that runs out: on, except in the 0.33 s gaps before 2.0, 1.33 and
   0.66 s are left */
static int ents_blink_on(float left)
{
	return (2.0f <= left || left <= 1.66f) && (1.33f <= left || left <= 1.0f)
		&& (0.66f <= left || left <= 0.33f);
}

/* 0x001ce520  TankedUp (gap)
Whether entity `id`'s shell icon shows: it has a shell, blinking while its shell message
(kind 0x15) runs out. Without such a message the original tests the id's bits as a float. */
int CTankEntities_HasShell(void *self, int id)
{
	void **node = ents_find_node(self, id);
	float left = tu_bitsf((unsigned int)id);

	if (node == NULL || node[0] == NULL || TU_I32(node[0], EN_SHELL) == 0)
		return 0;
	for (node = (void **)TU_PTR(self, TE_MESSAGES); node != NULL; node = (void **)node[1]) {
		void *message = node[0];

		if (TU_I32(message, TQ_KIND) == 0x15 && TU_I32(message, TQ_TARGET) == id) {
			long long ticks = (long long)(TU_AT(message, TQ_DUE, unsigned long long) - bkTimerRead());

			left = (float)((double)ticks
				/ (double)(long long)(((unsigned long long)DAT_002cd7a4 << 32) | DAT_002cd7a0));
		}
	}
	return ents_blink_on(left);
}

/* 0x001ce630  TankedUp (gap)
Entity `id`'s reload bar (0-1): full when it is always ready or has no RELOAD; else how far
its reload message (kind 10) has run. 0 when there is none. */
float CTankEntities_GetScoreBar(void *self, int id)
{
	void **node = ents_find_node(self, id);
	void *ent;

	if (node == NULL || (ent = node[0]) == NULL)
		return 0.0f;
	if (TU_I32(ent, EN_ALWAYS_READY) != 0 || TU_F32(TU_PTR(ent, EN_TEMPLATE), TP_RELOAD) == 0.0f)
		return 1.0f;
	for (node = (void **)TU_PTR(self, TE_MESSAGES); node != NULL; node = (void **)node[1]) {
		void *message = node[0];

		if (TU_I32(message, TQ_KIND) == 10 && TU_I32(message, TQ_TARGET) == id) {
			long long ticks = (long long)(TU_AT(message, TQ_DUE, unsigned long long) - bkTimerRead());
			double left = (double)ticks
				/ (double)(long long)(((unsigned long long)DAT_002cd7a4 << 32) | DAT_002cd7a0);

			return (float)(1.0 - left / TU_F32(TU_PTR(ent, EN_TEMPLATE), TP_RELOAD));
		}
	}
	return 0.0f;
}

/* 0x001ce700  TankedUp (gap)
Whether entity `id`'s speed icon shows (1 or 0): while its speed time runs, blinking near
its end. */
float CTankEntities_GetSpeedTime(void *self, int id)
{
	void **node = ents_find_node(self, id);
	float left;

	if (node == NULL || node[0] == NULL)
		return 0.0f;
	left = TU_F32(node[0], EN_SPEED_TIME);
	if (0.0f < left && ents_blink_on(left))
		return 1.0f;
	return 0.0f;
}

/* 0x001ce7d0  TankedUp (gap)
The tank nearest to `pos` other than `self_id` (*out = its id, 0 when none). Stops at the
first dead entity it meets, as the original does. */
void CTankEntities_FindNearestTank(void *self, float *pos, int selfId, int *out)
{
	void **node;
	float best = 0.0f;

	*out = 0;
	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		void *ent = node[0];

		if (TU_I32(ent, EN_ID) == selfId)
			continue;
		if (TU_I32(ent, EN_DEAD) != 0)
			return;
		if (TU_I32(TU_PTR(ent, EN_TEMPLATE), TP_TYPE) != 1)
			continue;
		if (*out == 0) {
			*out = TU_I32(ent, EN_ID);
			best = bmVectorDistance(pos, TU_FP(node[0], EN_POS));
		} else {
			float distance = bmVectorDistance(pos, TU_FP(ent, EN_POS));

			if (distance < best) {
				best = distance;
				*out = TU_I32(node[0], EN_ID);
			}
		}
	}
}

/* 0x001ce870  TankedUp (gap)
Turns entity `id` (on the ground, not stunned, alive) by `turn` (-1..1): faster with speed
time, scaled by its template's turn speed and the frame time. */
void CTankEntities_Turn(void *self, int id, float turn)
{
	void **node = ents_find_node(self, id);
	void *ent;
	double heading;

	if (node == NULL || (ent = node[0]) == NULL)
		return;
	if (TU_I32(ent, EN_ON_GROUND) == 0 || TU_I32(ent, EN_STUNNED) != 0 || TU_I32(ent, EN_DEAD) != 0)
		return;
	heading = CLMath_WrapAngle(TU_F32(ent, EN_HEADING));
	TU_F32(ent, EN_HEADING) = (float)heading;
	if (turn != 0.0f) {
		heading = heading - ((double)TU_F32(ent, EN_SPEED_TIME) * 0.5f + 1.0f)
			* TU_F32(TU_PTR(ent, EN_TEMPLATE), TP_ROT) * TU_F32(self, TE_DT) * turn;
		TU_F32(ent, EN_HEADING) = CLMath_WrapAngle((float)heading);
		TU_I32(ent, EN_TURNING) = 0;
	}
}

/* 0x001ce940  TankedUp (gap; no `this`)
`angle` wrapped into 0 .. 2 pi. */
float CTankEntities_WrapAngle(float angle)
{
	double a = angle;

	while (a > 6.28318548f)
		a = a - 6.28318548f;
	while (a < 0.0f)
		a = a + 6.28318548f;
	return (float)a;
}

/* 0x001ce990  TankedUp (gap)
The number of tanks whose sphere reaches within `radius` of `pos`. */
int CTankEntities_CountNear(void *self, float *pos, float radius)
{
	void **node;
	int count = 0;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		void *ent = node[0];
		float reach, dx, dy, dz;

		if (TU_I32(TU_PTR(ent, EN_TEMPLATE), TP_TYPE) != 1)
			continue;
		reach = radius + TU_F32(ent, EN_RADIUS);
		dz = pos[2] - TU_F32(ent, EN_POS + 8);
		dy = pos[1] - TU_F32(ent, EN_POS + 4);
		dx = pos[0] - TU_F32(ent, EN_POS);
		if (dz * dz + dy * dy + dx * dx <= reach * reach)
			count++;
	}
	return count;
}

/* 0x001cea10  TankedUp (gap)
Per frame: the entities' life timers and animations (body, low detail body, turret); an
entity whose life ran out or whose health is gone dies. */
void CTankEntities_Think(void *self)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		void *ent = node[0];
		void *template = TU_PTR(ent, EN_TEMPLATE);
		void *instance;

		if (0.0f < TU_F32(template, TP_LIFE))
			CTankTimer_Tick((struct TuTimer *)TU_P(ent, EN_TIMER));
		instance = TU_PTR(ent, EN_INST);
		if (instance != NULL && TU_PTR(ent, EN_ANIM_SEG) != NULL)
			baUpdateAnimations((struct BActorInstance *)instance,
				(struct BActorNode *)TU_PTR(instance, 0x134),
				TU_F32(ent, EN_ANIM_SPEED) * TU_F32(self, TE_DT), NULL);
		instance = TU_PTR(ent, EN_LOW_INST);
		if (instance != NULL && TU_PTR(ent, EN_ANIM_SEG) != NULL)
			baUpdateAnimations((struct BActorInstance *)instance,
				(struct BActorNode *)TU_PTR(instance, 0x134),
				TU_F32(ent, EN_ANIM_SPEED) * TU_F32(self, TE_DT), NULL);
		instance = TU_PTR(ent, EN_TURRET_INST);
		if (instance != NULL)
			baUpdateAnimations((struct BActorInstance *)instance,
				(struct BActorNode *)TU_PTR(instance, 0x134), TU_F32(self, TE_DT), NULL);
		template = TU_PTR(ent, EN_TEMPLATE);
		if ((TU_F32(template, TP_LIFE) != 0.0f
				&& BCarTimer_GetRemaining((struct BCarTimer *)TU_P(ent, EN_TIMER)) == 0.0f)
			|| (TU_I32(TU_PTR(ent, EN_TEMPLATE), TP_HEALTH) != 0 && TU_I32(ent, EN_HEALTH) == 0))
			TU_I32(ent, EN_DEAD) = 1;
	}
}

/* 0x001ceb30  TankedUp (gap)
Sends "update" to every living entity: to the global state, then the current state. */
void CTankEntities_UpdateStates(void *self)
{
	struct TuMessage msg;
	void **node;

	memset(&msg, 0, sizeof msg);
	msg.kind = 3;
	msg.state = 1;
	msg.flag = 0;
	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1]) {
		void *ent = node[0];

		if (TU_I32(ent, EN_DEAD) != 0)
			continue;
		msg.target = TU_I32(ent, EN_ID);
		msg.sender = TU_I32(ent, EN_ID);
		CTankEntities_SendMessageNow(self, ent, 1, &msg);
		CTankEntities_SendMessageNow(self, ent, TU_I32(ent, EN_STATE), &msg);
	}
}

/* 0x001ceba0  TankedUp (gap)
Carries out the state changes that are due. */
void CTankEntities_ChangeStates(void *self)
{
	void **node;

	for (node = (void **)ents_first(self); node != NULL; node = (void **)node[1])
		if (TU_I32(node[0], EN_CHANGE_STATE) != 0)
			CTankEntities_ChangeState(self, node[0]);
}

/* 0x001cebd0  TankedUp (gap)
Initialises a new entity: the next id (0x00315398), the global AI state, empty names and
models, unit vectors, and the defaults (mass 100, radius -1, ...). (A __thiscall method
that does not use `this`; written without it, as tankedup_b calls it.) */
void CTankEntities_InitEntity(void *ent)
{
	int id = TU_VAR(int, 0x00315398) + 1;

	TU_I32(ent, 0x088) = 1;
	TU_I32(ent, 0x08c) = 1;
	TU_I32(ent, 0x090) = 0;
	TU_VAR(int, 0x00315398) = id;
	TU_I32(ent, 0x000) = id;
	TU_I32(ent, 0x004) = 0;
	_snprintf((char *)TU_P(ent, 0x008), 0x3f, "NEW-%d", id);
	TU_I32(ent, 0x980) = TU_I32(ent, 0x000);
	TU_I32(ent, 0x098) = 0;
	TU_U8(ent, 0x008) = 0;
	TU_U8(ent, 0x8dc) = 0;
	TU_U8(ent, 0x6dc) = 0;
	TU_U8(ent, 0x09c) = 0;
	TU_U8(ent, 0x0dc) = 0;
	TU_U8(ent, 0x2dc) = 0;
	TU_U8(ent, 0x4dc) = 0;
	TU_F32(ent, 0x98c) = 100.0f;
	TU_I32(ent, 0xac8) = 0;
	TU_I32(ent, 0xacc) = 0;
	TU_I32(ent, 0xad0) = 0;
	TU_I32(ent, 0xad4) = 0;
	TU_I32(ent, 0xad8) = 0;
	TU_I32(ent, 0xeec) = 0;
	TU_I32(ent, 0xef0) = 0;
	TU_I32(ent, 0xef4) = 0;
	TU_I32(ent, 0x91c) = 0;
	TU_I32(ent, 0xefc) = 0;
	TU_I32(ent, 0x938) = 1;
	TU_I32(ent, 0x9a0) = 0;
	TU_I32(ent, 0x93c) = 0;
	TU_I32(ent, 0x954) = 0;
	TU_I32(ent, 0x958) = 0;
	TU_U8(ent, 0x048) = 0;
	TU_I32(ent, 0x92c) = 0;
	TU_I32(ent, 0x928) = 1;
	TU_I32(ent, 0x9ac) = 0;
	TU_I32(ent, 0x940) = 0;
	TU_F32(ent, 0x99c) = 1.0f;
	TU_F32(ent, 0x998) = 1.0f;
	TU_I32(ent, 0x968) = -1;
	TU_I32(ent, 0x924) = -1;
	TU_I32(ent, 0x950) = -1;
	TU_I32(ent, 0x960) = -1;
	TU_I32(ent, 0x96c) = -1;
	TU_I32(ent, 0x970) = -1;
	TU_I32(ent, 0x974) = -1;
	TU_F32(ent, 0x9b0) = -1.0f;
	TU_I32(ent, 0xa18) = 0;
	TU_I32(ent, 0xa1c) = 0;
	TU_I32(ent, 0xa20) = 0;
	TU_F32(ent, 0xa24) = 1.0f;
	TU_I32(ent, 0xa28) = 0;
	TU_I32(ent, 0xa2c) = 0;
	TU_I32(ent, 0xa30) = 0;
	TU_F32(ent, 0xa34) = 1.0f;
	TU_I32(ent, 0xa08) = 0;
	TU_I32(ent, 0xa0c) = 0;
	TU_I32(ent, 0xa10) = 0;
	TU_F32(ent, 0xa14) = 1.0f;
	TU_I32(ent, 0xa38) = 0;
	TU_I32(ent, 0xa3c) = 0;
	TU_I32(ent, 0xa40) = 0;
	TU_F32(ent, 0xa44) = 1.0f;
	TU_I32(ent, 0xa48) = 0;
	TU_I32(ent, 0xa4c) = 0;
	TU_I32(ent, 0xa50) = 0;
	TU_F32(ent, 0xa54) = 1.0f;
	TU_I32(ent, 0xa58) = 0;
	TU_I32(ent, 0xa5c) = 0;
	TU_I32(ent, 0xa60) = 0;
	TU_F32(ent, 0xa64) = 1.0f;
	TU_I32(ent, 0x9b8) = 0;
	TU_I32(ent, 0x9d4) = 0;
	TU_I32(ent, 0x930) = 0;
	TU_I32(ent, 0x95c) = 0;
	TU_I32(ent, 0x094) = 0;
	TU_I32(ent, 0x944) = 0;
	TU_I32(ent, 0x948) = 0;
	TU_I32(ent, 0x978) = 0;
	TU_I32(ent, 0x920) = 1;
	TU_I32(ent, 0x9c0) = 0;
	TU_I32(ent, 0x9d0) = 0;
	TU_I32(ent, 0xef8) = 0;
	TU_I32(ent, 0x9cc) = 0;
	TU_I32(ent, 0xf00) = 0;
	TU_I32(ent, 0x94c) = 0;
	TU_U8(ent, 0xaec) = 0;
	TU_U8(ent, 0xcec) = 0;
	TU_I32(ent, 0xadc) = 0;
	TU_I32(ent, 0xae0) = 0;
	TU_I32(ent, 0xae4) = 0;
	TU_I32(ent, 0xae8) = 0;
	TU_F32(ent, 0x9bc) = 1.0f;
	TU_F32(ent, 0x988) = -1.0f;
	TU_F32(ent, 0x9c4) = 1000.0f;
	TU_F32(ent, 0x9c8) = 1000.0f;
	TU_I32(ent, 0xa78) = 0;
	TU_I32(ent, 0xa7c) = 0;
	TU_I32(ent, 0xa80) = 0;
	TU_F32(ent, 0xa84) = 1.0f;
}
