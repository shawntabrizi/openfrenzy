/*
TankedUp/CTank_Players.c  <-  C:\BlitzParty\Game\Src\TankedUp\CTank_Players.cpp

CTank_Players (CTank_Game +0xc38, "playerSystem", vtable 0x00279fd8): the players' tanks
("player1".."player4" entities), their pads (tracks, two fire buttons), rumble, engine
sounds, the HUD (score boxes, icons, the clock, the bars, the winner text) and the ranking.
Variations whose world has no LAPS (0) score kills ("battle"); the others are races (laps,
finishing order, progress bars).

0x001c7f40 is tagged with this file; the functions from 0x001c7f20 to 0x001c9d30 are placed
here because of their "CTank_Players::" / "player.c" strings or because they work on this
layout.
*/

#include "tankedup_a.h"

#define TU_MAX_PLAYERS 4

/* a player record (0x48 bytes; the array is made for DAT_002bae94 players but most loops
   walk all four) */
struct TuPlayer {
	int human;                   /* +0x00 a person drives it (else the AI) */
	int done;                    /* +0x04 finished the race (the AI takes over) */
	int winner;                  /* +0x08 */
	int id;                      /* +0x0c the tank entity's id */
	void *input;                 /* +0x10 the input map */
	void *leftTrack;             /* +0x14 */
	void *leftTrackX;            /* +0x18 */
	void *rightTrack;            /* +0x1c */
	void *primary;               /* +0x20 */
	void *secondary;             /* +0x24 */
	int unknown28[6];            /* +0x28 */
	float bar;                   /* +0x40 the HUD score bar */
	int channel;                 /* +0x44 the engine sound channel */
};
OF_ASSERT_SIZE(struct TuPlayer, 0x48);

/* the player system (CTank_Game +0xc38, 0xa4 bytes) */
struct TuPlayers {
	const void *const *vtable;   /* +0x00 */
	struct TuPlayer *players;    /* +0x04 */
	void *entities;              /* +0x08 */
	struct TuTimer *timer;       /* +0x0c */
	void *camera;                /* +0x10 */
	void *world;                 /* +0x14 */
	void *sound;                 /* +0x18 */
	struct TuLight *lights;      /* +0x1c */
	int rank[TU_MAX_PLAYERS];    /* +0x20 1-4; 9 = not placed yet */
	int score[TU_MAX_PLAYERS];   /* +0x30 */
	int teamScore[TU_MAX_PLAYERS]; /* +0x40 */
	int nextRank;                /* +0x50 the place of the next tank to finish */
	int winnerText;              /* +0x54 the winner text is shown */
	int state;                   /* +0x58 the game state of the last state change */
	int gameState;               /* +0x5c the state of the last update */
	struct {
		int shellIcon;           /* +0x60 overlays per player */
		int text;                /* +0x64 */
		int winIcon;             /* +0x68 */
		int speedIcon;           /* +0x6c */
	} hud[TU_MAX_PLAYERS];
	int debugText;               /* +0xa0 the frame rate overlay ("TEMP") */
};
OF_ASSERT_OFFSET(struct TuPlayers, rank, 0x20);
OF_ASSERT_OFFSET(struct TuPlayers, hud, 0x60);
OF_ASSERT_SIZE(struct TuPlayers, 0xa4);

/* 0x001d2880 (tankedup_b)  CTank_Entities::SendDelayedMessage(kind, message, delay): the
   message is 6 words passed by value on the stack; tankedup_b declares them as 6 arguments:
   (0, entity id, 1, type, 0, unused). Type 4: a person stopped driving the tank, 5: a person
   drives it. */
#define TUB_Entities_SendDelayedMessage \
	((void (*)(void *, int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, \
		unsigned int, float))FUN_001d2880)

/* classlib_ui's rumble manager (0x002ed324) */
extern unsigned char DAT_002ed324[];
#define TU_RUMBLE ((int)DAT_002ed324)

/* core: the player table, 0x28 bytes per player */
extern int DAT_002baed4[];          /* the character of player i: [i * 10] */
extern int DAT_002baed8[];          /* player i is present (a pad): [i * 10] */
extern int DAT_002baee0[];          /* the pad port of player i: [i * 10] */
extern int DAT_002baee8[];          /* the minigame score of player i: [i * 10] */
extern int DAT_002baef4[];          /* the team of player i: [i * 10] */
extern int DAT_002baef8[];          /* (a second team word of player i): [i * 10] */
extern int DAT_002baec4[];          /* (core: indexed by team * 2 - that word) */
extern int DAT_002baebc;            /* teams are on */
extern int DAT_002bae94;            /* the number of players */
extern float DAT_002ba990;          /* frames per second (core) */
extern int DAT_00283dc0;            /* core: the simple tank controls option */
extern unsigned char DAT_00290718[]; /* classlib_fx: character colours, 0x60 bytes each */

/* the debug object's settings this file reads */
#define TU_DBG_CONTROLS TU_VAR(int, 0x00315150)    /* the tank control scheme 1-3 */
#define TU_DBG_HIDE_TEXT TU_VAR(int, 0x003151ac)   /* the frame rate text is hidden while 0 */
#define TU_DBG_BARS TU_VAR(int, 0x003150cc)        /* draw the HUD score bars */
#define TU_DBG_RUMBLE_AMP TU_VAR(int, 0x003151c8)  /* the hit rumble */
#define TU_DBG_RUMBLE_DUR TU_VAR(int, 0x003151cc)
#define TU_DBG_RUMBLE_ATT TU_VAR(int, 0x003151d0)
#define TU_DBG_RUMBLE_MODE TU_VAR(int, 0x003151d4)
#define TU_DBG_RUMBLE_LEFT TU_VAR(int, 0x003151d8)
#define TU_DBG_RUMBLE_RIGHT TU_VAR(int, 0x003151dc)

#define TU_HUD DAT_002ecfac

/* 0x00279fd8  vtable of CTank_Players (rebuilt from the XBE) */
const void *const vtable_00279fd8[1] = {
	(const void *)CTankPlayers_DeletingDtor,  /* +0x00 */
};

/* the record of the player whose tank is `id`, or -1 (the first four records) */
static int players_find(struct TuPlayers *self, int id)
{
	int i;

	for (i = 0; i < TU_MAX_PLAYERS; i++)
		if (self->players[i].id == id)
			return i;
	return -1;
}

static void players_send(struct TuPlayers *self, int id, int type)
{
	/* the last word is uninitialised in the original */
	TUB_Entities_SendDelayedMessage(self->entities, type, 0, (unsigned int)id, 1, (unsigned int)type,
		0, 0, 0.0f);
}

/* 0x001c7f20  TankedUp (gap) */
void *CTankPlayers_Ctor(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;

	players->vtable = vtable_00279fd8;
	players->entities = NULL;
	return self;
}

/* 0x001c7f30  TankedUp (gap) */
void CTankPlayers_Dtor(void *self)
{
	((struct TuPlayers *)self)->vtable = vtable_00279fd8;
}

/* 0x001c7f40  TankedUp/CTank_Players.cpp
Initialise: makes the player records, finds the tanks "player1".. and tells the humans'
tanks so (the setting "players" forces the first n players human), points the tracking
spots at them. Returns 1. */
int CTankPlayers_Initialise(void *self, void *entities, struct TuTimer *timer, void *world,
	void *camera, void *sound, void *lights)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	char name[64];
	float pos[4];
	char *text;
	unsigned int i;

	CLRumble_CreateMenu((int *)DAT_002ed324);
	players->world = world;
	players->entities = entities;
	players->timer = timer;
	players->sound = sound;
	players->lights = (struct TuLight *)lights;
	players->camera = camera;
	players->winnerText = 0;
	players->score[0] = 0;
	players->score[1] = 0;
	players->score[2] = 0;
	players->score[3] = 0;
	players->players = (struct TuPlayer *)bkHeapCalloc((uint)DAT_002bae94 * 0x48, 0, TU_FILE_PLAYERS,
		0x50, 0x2001);
	for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
		players->players[i].winner = 0;
		players->players[i].done = 0;
	}
	text = Settings_Get(DAT_002ba9a0, "players");
	i = 0;
	if (text != NULL) {
		if ((unsigned long)atol(text) != 0) {
			do {
				players->players[i].human = 1;
				i++;
			} while (i < (unsigned long)atol(text));
		}
		for (i = (unsigned long)atol(text); i < (unsigned int)DAT_002bae94; i++)
			players->players[i].human = 0;
	} else {
		if (DAT_002bae94 == 0)
			goto done;
		for (i = 0; i < (unsigned int)DAT_002bae94; i++)
			players->players[i].human = DAT_002baed8[i * 10];
	}
	for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
		_snprintf(name, 0x3f, "player%d", i + 1);
		players->players[i].id = CTankEntities_FindId(players->entities, name);
		if (players->players[i].id == 0)
			TU_DPRINTF("CTank_Players::Initialise - Could not find entity [%s]. Player %d has no "
				"tank!!\n", name, i + 1);
		else if (players->players[i].human != 0)
			players_send(players, players->players[i].id, 5);
	}
done:
	players->rank[0] = 9;
	players->rank[1] = 9;
	players->rank[2] = 9;
	players->rank[3] = 9;
	players->nextRank = 1;
	for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
		TUB_Entities_GetPosition(players->entities, players->players[i].id, pos);
		CTankLights_TrackSpot(players->lights, (int)i, pos);
	}
	return 1;
}

/* 0x001c81b0  TankedUp (gap)
Rumbles human player `player`'s pad during the game (state 4; in a race not after it
finished): kind 1 a hit (the debug object's settings), 2 a shot, 3 a big hit. The hit's
style is 4 for the debug rumble modes 1-5, else the player's number (as in the original). */
void CTankPlayers_Rumble(void *self, int player, int kind)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int amplitude, duration, attack, style;

	if (players->players[player].human == 0 || players->gameState != 4)
		return;
	if (CTankWorld_GetLaps(players->world) != 0 && players->rank[player] == 9)
		return;
	if (kind == 1) {
		amplitude = TU_DBG_RUMBLE_AMP;
		duration = TU_DBG_RUMBLE_DUR;
		attack = TU_DBG_RUMBLE_ATT;
		style = player;
		switch (TU_DBG_RUMBLE_MODE) {
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			style = 4;
			break;
		}
		if (TU_DBG_RUMBLE_LEFT != 0)
			CLRumble_PlayMotor(TU_RUMBLE, player, 0, 0x7fff, amplitude, duration, attack, 0, style);
		if (TU_DBG_RUMBLE_RIGHT != 0)
			CLRumble_PlayMotor(TU_RUMBLE, player, 1, 0x7fff, amplitude, duration, attack, 0, style);
		return;
	}
	if (kind == 2) {
		CLRumble_PlayMotor(TU_RUMBLE, player, 1, 0x7fff, 200, 0x46, 10, 0, 3);
		return;
	}
	if (kind == 3) {
		CLRumble_PlayMotor(TU_RUMBLE, player, 0, 0x7fff, 0x5dc, 0x1e, 0x28, 0, 3);
		CLRumble_PlayMotor(TU_RUMBLE, player, 1, 0x7fff, 1000, 10, 0x3c, 0, 4);
	}
}

/* 0x001c82e0  TankedUp (gap)
Whether the tanks `a` and `b` are on one team (a tank is on its own team). */
int CTankPlayers_SameTeam(void *self, int a, int b)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int i, j;

	if (a == b)
		return 1;
	if (DAT_002baebc != 0) {
		i = players_find(players, a);
		j = players_find(players, b);
		if (i != -1 && j != -1)
			return DAT_002baef4[i * 10] == DAT_002baef4[j * 10];
	}
	return 0;
}

/* 0x001c8370  TankedUp (gap)
Player `player`'s tank: its position (raised by 60 over its ground clearance) and rotation
(tankedup_b 0x001d1c30). */
void CTankPlayers_GetTankPose(void *self, int player, float *pos, float *rot)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	float velocity[4];
	float *posSlot = pos;
	float *rotSlot = rot;

	TUB_Entities_GetInfo(players->entities, players->players[player].id, pos, velocity, rot,
		(void *)&posSlot, (void *)&rotSlot);
}

/* 0x001c83b0  TankedUp (gap; a jump to 0x001ce350)
Whether player `player`'s tank is alive. */
int CTankPlayers_IsAlive(void *self, int player)
{
	struct TuPlayers *players = (struct TuPlayers *)self;

	return CTankEntities_IsAlive(players->entities, players->players[player].id);
}

/* 0x001c83d0  TankedUp (gap)
Whether player `player` came first. */
int CTankPlayers_IsFirst(void *self, int player)
{
	return ((struct TuPlayers *)self)->rank[player] == 1;
}

/* 0x001c83f0  TankedUp (gap)
Whether the tank `id` belongs to a winner. */
int CTankPlayers_IsWinner(void *self, int id)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int i = players_find(players, id);

	if (i == -1) {
		TU_DPRINTF("NOT PLAYER THEREFOR NOT WINNER\n");
		return 0;
	}
	if (players->players[i].winner != 0) {
		TU_DPRINTF("WINNER\n");
		return 1;
	}
	TU_DPRINTF("NOT WINNER\n");
	return 0;
}

/* 0x001c8460  TankedUp (gap)
Whether a person drives the tank `id`. */
int CTankPlayers_IsHuman(void *self, int id)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int i = players_find(players, id);

	return i == -1 ? 0 : players->players[i].human;
}

/* 0x001c8490  TankedUp (gap; a jump to 0x001ce410)
Whether player `player`'s tank can act. */
int CTankPlayers_CanAct(void *self, int player)
{
	struct TuPlayers *players = (struct TuPlayers *)self;

	return CTankEntities_CanAct(players->entities, players->players[player].id);
}

/* 0x001c84b0  TankedUp (gap)
Sets the engine sound of the tank `id` by its speed `value` (before the game ends). */
void CTankPlayers_SetEngineVolume(void *self, float value, int id)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int i;

	if (players->state > 4)
		return;
	for (i = 0; players->players[i].id != id; ) {
		i++;
		if (i >= 4)
			return;
	}
	bsSetChannelVolume(players->players[i].channel,
		(unsigned short)(0x32 - TU_FTOL(fabsf(value) * -100.0)));
}

/* the stick of a channel, -1 .. 1 */
static float players_stick(void *channel)
{
	return (float)(-TU_I32(channel, 0x14)) / (float)TU_I32(channel, 0x1c);
}

/* 0x001c8510  TankedUp/CTank_Players.cpp (inferred)
Drives the human tanks from their pads (three control schemes; the simple controls option
of core uses the tank controls); primary fire fires, secondary fire reverses. */
void CTankPlayers_Input(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	unsigned int i;

	for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
		struct TuPlayer *player = &players->players[i];
		float left, right, leftX;

		if (player->human == 0 || player->done != 0)
			continue;
		left = players_stick(player->leftTrack);
		right = players_stick(player->rightTrack);
		leftX = players_stick(player->leftTrackX);
		if (DAT_00283dc0 != 0 || TU_DBG_CONTROLS == 1)
			TUB_Entities_DriveTracks(players->entities, player->id, left, right);
		else if (TU_DBG_CONTROLS == 2)
			TUB_Entities_DriveStick(players->entities, player->id, leftX, left);
		else if (TU_DBG_CONTROLS == 3)
			TUB_Entities_DriveCamera(players->entities, player->id, leftX, left);
		if (TU_I32(players->players[i].primary, 0x14) != 0)
			TUB_Entities_FirePrimary(players->entities, players->players[i].id);
		if (TU_I32(players->players[i].secondary, 0x14) != 0)
			TUB_Entities_Reverse(players->entities, players->players[i].id);
	}
}

/* 0x001c8650  TankedUp (gap)
Draws the HUD (the debug frame-rate text, the score bars) and sets the render states. */
void CTankPlayers_Draw(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;

	TU_U16(players->debugText, 0x54) = (unsigned short)(TU_DBG_HIDE_TEXT == 0);
	if (TU_DBG_BARS != 0) {
		struct TuPlayer *p;

		COverlay_DrawAll(1);
		p = players->players;
		CLHud_DrawScoreBars(TU_HUD, p[0].bar, p[1].bar, p[2].bar, p[3].bar, 100.0f);
	}
	bdSetRenderState(4, 0, 0);
	bdSetProjectionMode(1);
	bdSetRenderState(7, 3, 0);
}

/* 0x001c86d0  TankedUp (gap)
Makes player `player`'s input map on its pad, plus the keyboard keys of that player. */
void CTankPlayers_CreatePlayerInput(void *self, int player)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	struct TuPlayer *p = &players->players[player];
	int port = DAT_002baee0[player * 10];

	p->input = biCreateMap(5);
	players->players[player].leftTrack = biCreateChannel((struct biMap *)players->players[player].input,
		"left track", 3, 0, 0, 0, 0);
	players->players[player].leftTrackX = biCreateChannel(
		(struct biMap *)players->players[player].input, "left track X", 3, 0, 0, 0, 0);
	players->players[player].rightTrack = biCreateChannel(
		(struct biMap *)players->players[player].input, "right track", 3, 0, 0, 0, 0);
	players->players[player].primary = biCreateChannel((struct biMap *)players->players[player].input,
		"primary fire", 1, 0, 0, 0, 0);
	players->players[player].secondary = biCreateChannel(
		(struct biMap *)players->players[player].input, "secondary fire", 1, 0, 0, 0, 0);
	p = &players->players[player];
	biBindInput((struct biChannel *)p->leftTrack, port | 0xbe, port | 0xbd);
	biBindInput((struct biChannel *)players->players[player].leftTrackX, port | 0xc0, port | 0xbf);
	biBindInput((struct biChannel *)players->players[player].leftTrack, port | 0xba, 0);
	biBindInput((struct biChannel *)players->players[player].leftTrackX, port | 0xb9, 0);
	biBindInput((struct biChannel *)players->players[player].rightTrack, port | 0xbc, 0);
	biBindInput((struct biChannel *)players->players[player].primary, port | 199, 0);
	biBindInput((struct biChannel *)players->players[player].primary, port | 0xc6, 0);
	biBindInput((struct biChannel *)players->players[player].secondary, port | 0xc5, 0);
	biBindInput((struct biChannel *)players->players[player].secondary, port | 0xc4, 0);
	p = &players->players[player];
	switch (player) {
	case 0:
		biBindInput((struct biChannel *)p->leftTrack, 0x45, 0x55);
		biBindInput((struct biChannel *)players->players[player].rightTrack, 0x57, 0x5b);
		biBindInput((struct biChannel *)players->players[player].primary, 0x66, 0);
		biBindInput((struct biChannel *)players->players[player].secondary, 0x6a, 0);
		break;
	case 1:
		biBindInput((struct biChannel *)p->leftTrack, 0x46, 0x4b);
		biBindInput((struct biChannel *)players->players[player].rightTrack, 0x52, 0x4c);
		biBindInput((struct biChannel *)players->players[player].primary, 0x4a, 0);
		biBindInput((struct biChannel *)players->players[player].secondary, 0x4e, 0);
		break;
	case 2:
		biBindInput((struct biChannel *)p->leftTrack, 99, 0x5f);
		biBindInput((struct biChannel *)players->players[player].rightTrack, 100, 0x60);
		biBindInput((struct biChannel *)players->players[player].primary, 0x54, 0);
		biBindInput((struct biChannel *)players->players[player].secondary, 0x61, 0);
		break;
	case 3:
		biBindInput((struct biChannel *)p->leftTrack, 0x87, 0x8a);
		biBindInput((struct biChannel *)players->players[player].rightTrack, 0x88, 0x8b);
		biBindInput((struct biChannel *)players->players[player].primary, 0x89, 0);
		biBindInput((struct biChannel *)players->players[player].secondary, 0x8f, 0);
		break;
	}
}

/* 0x001c8a20  TankedUp (gap)
Deletes player `player`'s input map. */
void CTankPlayers_ReleasePlayerInput(void *self, int player)
{
	struct TuPlayer *p = &((struct TuPlayers *)self)->players[player];

	if (p->leftTrack != NULL)
		biDeleteChannel((struct biChannel *)p->leftTrack);
	if (p->leftTrackX != NULL)
		biDeleteChannel((struct biChannel *)p->leftTrackX);
	if (p->rightTrack != NULL)
		biDeleteChannel((struct biChannel *)p->rightTrack);
	if (p->primary != NULL)
		biDeleteChannel((struct biChannel *)p->primary);
	if (p->secondary != NULL)
		biDeleteChannel((struct biChannel *)p->secondary);
	p->leftTrack = NULL;
	p->leftTrackX = NULL;
	p->rightTrack = NULL;
	p->primary = NULL;
	p->secondary = NULL;
	if (p->input != NULL)
		biDeleteMap((struct biMap *)p->input);
	p->input = NULL;
}

/* 0x001c8af0  TankedUp (gap)
Makes the input maps of the human players. */
void CTankPlayers_CreateInput(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	unsigned int i;

	for (i = 0; i < (unsigned int)DAT_002bae94; i++)
		if (players->players[i].human != 0)
			CTankPlayers_CreatePlayerInput(self, (int)i);
}

/* 0x001c8b30  TankedUp (gap)
Deletes the input maps of the human players. */
void CTankPlayers_ReleaseInput(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	unsigned int i;

	for (i = 0; i < (unsigned int)DAT_002bae94; i++)
		if (players->players[i].human != 0)
			CTankPlayers_ReleasePlayerInput(self, (int)i);
}

/* 0x001c8b70  TankedUp (gap)
Once a frame: a pad that went away hands its tank to the AI, a pad that came back takes
it again. */
void CTankPlayers_BeginFrame(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	unsigned int i;

	for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
		struct TuPlayer *p = &players->players[i];

		if (p->human != 0 && DAT_002baed8[i * 10] == 0) {
			p->human = 0;
			players_send(players, players->players[i].id, 4);
			CTankPlayers_ReleasePlayerInput(self, (int)i);
			TU_DPRINTF("player.c is swiching a player to AI\n");
		}
		p = &players->players[i];
		if (p->human == 0 && DAT_002baed8[i * 10] != 0) {
			p->human = 1;
			players_send(players, players->players[i].id, 5);
			CTankPlayers_CreatePlayerInput(self, (int)i);
		}
	}
}

/* the race ranking: progress of the four tanks (+1000 for a tank still racing), ranked by
tankedup_b 0x001d62d0 */
static void players_race_ranks(struct TuPlayers *players, int *out)
{
	int progress[TU_MAX_PLAYERS];
	int i;

	for (i = 0; i < TU_MAX_PLAYERS; i++) {
		progress[i] = CTankEntities_GetProgress(players->entities, players->players[i].id);
		if (players->rank[i] != 9)
			progress[i] = progress[i] + 1000;
	}
	TUB_RankScores(progress, out);
}

/* 0x001c8ca0  TankedUp (gap)
The place of the tank `id` (4 when it is not a player's): the finishing place in a battle,
the race position in a race. */
int CTankPlayers_GetRank(void *self, int id)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int ranks[TU_MAX_PLAYERS];
	int i;

	if (CTankWorld_GetLaps(players->world) == 0) {
		i = players_find(players, id);
		return i == -1 ? 4 : players->rank[i];
	}
	players_race_ranks(players, ranks);
	i = players_find(players, id);
	return i == -1 ? 4 : ranks[i];
}

/* 0x001c8d70  TankedUp (gap; no `this`)
Plays voice `kind` of player `player`'s character. */
void CTankPlayers_PlayVoice(int player, int kind, int unused, int volume)
{
	(void)unused;
	if (player >= 0 && player < 4)
		CLSamples_PlayVoice((char)DAT_002baed4[player * 10], kind, NULL, volume, 180.0f, 0, -1, 0);
}

/* 0x001c8db0  TankedUp (gap)
The deleting destructor (vtable 0x00279fd8 slot 0). */
void *CTankPlayers_DeletingDtor(void *self, unsigned int flags)
{
	((struct TuPlayers *)self)->vtable = vtable_00279fd8;
	if ((flags & 1) != 0)
		TU_DELETE(self);
	return self;
}

/* 0x001c8dd0  TankedUp (gap)
Free: the engine sounds, the input maps, the records, the winner text, the rumble. */
void CTankPlayers_Free(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	unsigned int i;

	players->entities = NULL;
	if (players->players != NULL) {
		for (i = 0; i < (unsigned int)DAT_002bae94; i++)
			if (players->players[i].channel != -1)
				bsReserveChannel(players->players[i].channel, 0);
		for (i = 0; i < (unsigned int)DAT_002bae94; i++)
			if (players->players[i].human != 0)
				CTankPlayers_ReleasePlayerInput(self, (int)i);
		TU_DELETE(players->players);
		players->players = NULL;
		if (players->winnerText != 0) {
			CLHud_DeleteWinnerText(TU_HUD);
			players->winnerText = 0;
		}
	}
	CLRumble_Shutdown((int *)DAT_002ed324);
}

/* 0x001c8e80  TankedUp (gap)
Rumbles the pad of the player whose tank is `id`. */
void CTankPlayers_RumbleId(void *self, int id, int kind)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int i;

	for (i = 0; i < TU_MAX_PLAYERS; i++)
		if (players->players[i].id == id)
			CTankPlayers_Rumble(self, i, kind);
}

/* 0x001c8ec0  TankedUp (gap)
A game state change. The end (5): a race places the tanks still racing by their position;
the scores become 5 - place; the winners go to the camera, the losers of a race moan, the
winner text shows, and every engine sound becomes the finish sound. */
void CTankPlayers_StateChange(void *self, int from, int to)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int ranks[TU_MAX_PLAYERS];
	float pos[4];
	int winners;
	int i;

	(void)from;
	if (to != 5) {
		players->state = to;
		return;
	}
	if (CTankWorld_GetLaps(players->world) != 0) {
		players_race_ranks(players, ranks);
		TU_DPRINTF("finshed ranking [%d,%d,%d,%d]\n", players->rank[0], players->rank[1],
			players->rank[2], players->rank[3]);
		TU_DPRINTF("racePos ranking [%d,%d,%d,%d]\n", ranks[0], ranks[1], ranks[2], ranks[3]);
		for (i = 0; i < TU_MAX_PLAYERS; i++)
			if (players->rank[i] == 9)
				players->rank[i] = ranks[i];
		TU_DPRINTF("    new ranking [%d,%d,%d,%d]\n", players->rank[0], players->rank[1],
			players->rank[2], players->rank[3]);
		for (i = 0; i < TU_MAX_PLAYERS; i++)
			DAT_002baee8[i * 10] = 5 - players->rank[i];
	}
	winners = 0;
	for (i = 0; i < TU_MAX_PLAYERS; i++) {
		if (players->rank[i] == 1) {
			TU_I32(players->camera, winners * 4) = players->players[i].id;
			winners++;
		}
		if (players->rank[i] == 4 && CTankWorld_GetLaps(players->world) != 0)
			CLSamples_PlayVoice((char)DAT_002baed4[i * 10], 0x10, NULL, 0xff, 180.0f, 0, -1, 0);
	}
	TU_I32(players->camera, 0x10) = winners;
	CLHud_CreateWinnerText(TU_HUD, players->score, 0);
	players->winnerText = 1;
	for (i = 0; i < TU_MAX_PLAYERS; i++) {
		TUB_Entities_GetPosition(players->entities, players->players[i].id, pos);
		bsStopChannel(players->players[i].channel);
		bsReserveChannel(players->players[i].channel, 0);
		players->players[i].channel = TUB_Sound_Play(players->sound, 0xd, pos, 0xff, 1, 0);
	}
	players->state = 5;
}

/* a zeroed overlay description (classlib_ui, 0x3b0 bytes) */
#define TU_DESC_WORDS 0xec

/* an icon in player `i`'s score box */
static int players_icon(int parent, const char *texture, float x, float y, float size)
{
	unsigned int desc[TU_DESC_WORDS];
	int overlay;

	memset(desc, 0, sizeof desc);
	desc[0x288 / 4] = 0;                         /* an image */
	TU_F32(desc, 0x250) = 90.0f;                 /* colour r, g, b, a */
	TU_F32(desc, 0x254) = 90.0f;
	TU_F32(desc, 0x258) = 90.0f;
	TU_F32(desc, 0x25c) = 255.0f;
	TU_F32(desc, 0x2b4) = x;                     /* position */
	TU_F32(desc, 0x2b8) = y;
	TU_F32(desc, 0x2bc) = 0.0f;
	TU_F32(desc, 0x308) = size;                  /* size */
	TU_F32(desc, 0x30c) = size;
	desc[1] = (unsigned int)bLoadResource(NULL, texture, 0, 0xdefa);
	desc[0x290 / 4] = 0x80;
	overlay = COverlay_CreateChild(parent, (undefined4 *)desc);
	TU_U16(overlay, 0x54) = 1;
	return overlay;
}

/* 0x001c9120  TankedUp (gap)
Makes the HUD: the score boxes, the clock (60 s), the scores (battle) or score labels
(race), the bars; per player a shell icon and a speed icon, and in a race a lap text in the
character's colour and a win icon; the debug frame-rate text. */
void CTankPlayers_CreateHud(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	unsigned int desc[TU_DESC_WORDS];
	int zeros[4];
	int i;

	zeros[0] = 0;
	zeros[1] = 0;
	zeros[2] = 0;
	zeros[3] = 0;
	CLHud_CreateScoreBoxes(TU_HUD);
	CLHud_CreateClock(TU_HUD, 60.0f);
	CLHud_SetClock(TU_HUD, 60.0f, 1);
	if (CTankWorld_GetLaps(players->world) == 0)
		CLHud_CreateScoreTexts(TU_HUD, zeros);
	else
		CLHud_CreateScoreLabels(TU_HUD, "", 1);
	CLHud_CreateBars(TU_HUD);
	CLHud_InitBarGeometry(TU_HUD);
	memset(desc, 0, sizeof desc);
	for (i = 0; i < DAT_002bae94; i++) {
		int parent = TU_I32(TU_HUD, 0xf0 + i * 4);

		players->hud[i].shellIcon = players_icon(parent, "icon\\icon-shell.bmp", 18.0f, 20.0f, 32.0f);
		players->hud[i].speedIcon = players_icon(TU_I32(TU_HUD, 0xf0 + i * 4), "icon\\icon-speed.bmp",
			18.0f, 20.0f, 32.0f);
		if (CTankWorld_GetLaps(players->world) != 0) {
			int character = DAT_002baed4[i * 10];
			int overlay;

			memset(desc, 0, sizeof desc);
			desc[0x288 / 4] = 2;                                   /* a text */
			desc[0x44 / 4] = (unsigned int)bLoadResource(NULL, "lcd-bold.ttf", 3, 0xdefa);
			TU_F32(desc, 0x250) = (float)DAT_00290718[character * 0x60];
			TU_F32(desc, 0x2b4) = 116.0f;
			TU_F32(desc, 0x254) = (float)DAT_00290718[character * 0x60 + 1];
			TU_F32(desc, 0x2b8) = 13.0f;
			TU_F32(desc, 0x2bc) = 0.0f;
			TU_F32(desc, 0x25c) = 75.0f;
			TU_F32(desc, 0x258) = (float)DAT_00290718[character * 0x60 + 2];
			TU_U16(desc, 0x24c) = 2;
			TU_F32(desc, 0x30c) = 200.0f;
			bkStringToWide((ushort *)TU_P(desc, 0x48), "");
			overlay = COverlay_CreateChild(TU_I32(TU_HUD, 0xf0 + i * 4), (undefined4 *)desc);
			players->hud[i].text = overlay;
			TU_U16(overlay, 0x54) = 0;
			TU_F32(players->hud[i].text, 0x2b8) = TU_F32(TU_HUD, 0x98 + i * 4) * 127.0f;
			players->hud[i].winIcon = players_icon(TU_I32(TU_HUD, 0xf0 + i * 4), "icon\\icon-win.bmp",
				17.0f, 20.0f, 25.0f);
		}
	}
	memset(desc, 0, sizeof desc);
	desc[0x288 / 4] = 2;
	TU_F32(desc, 0x2a4) = -130.0f;
	TU_F32(desc, 0x2a8) = 180.0f;
	TU_F32(desc, 0x2ac) = 0.0f;
	TU_F32(desc, 0x250) = 127.0f;
	TU_F32(desc, 0x254) = 127.0f;
	TU_F32(desc, 0x258) = 127.0f;
	TU_F32(desc, 0x25c) = 127.0f;
	desc[0x44 / 4] = (unsigned int)bLoadResource(NULL, "handgotl.ttf", 3, 0xdefa);
	desc[0x290 / 4] = 0;
	{
		static const ushort temp[] = { 'T', 'E', 'M', 'P', 0 };

		bkWideSprintf((ushort *)TU_P(desc, 0x48), temp);
	}
	players->debugText = COverlay_Create((int *)desc);
	TU_U16(players->debugText, 0x54) = 1;
}

/* 0x001c9600  TankedUp (gap)
Updates the HUD and the scores. In the game (state 4) the shell icon shows while the tank
has a shell and the speed icon while it has speed. Battle: the scores are the kills (with
the team mate's), the ranks follow the scores, the clock shows the time left. Race: the lap
fractions, the finishing order (the first place cheers; three finished tanks end the race:
*state = 5), the scores 5 - place, the progress bars. */
void CTankPlayers_UpdateHud(void *self, int *state)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	int i, j;

	if (players->state == 4) {
		for (i = 0; i < TU_MAX_PLAYERS; i++) {
			struct TuPlayer *p = &players->players[i];

			if (p->human == 0)
				TU_U16(players->hud[i].shellIcon, 0x54) = 1;
			else
				TU_U16(players->hud[i].shellIcon, 0x54) =
					(unsigned short)(CTankEntities_HasShell(players->entities, p->id) == 0);
			p = &players->players[i];
			if (p->human == 0)
				TU_U16(players->hud[i].speedIcon, 0x54) = 1;
			else if (CTankEntities_GetSpeedTime(players->entities, p->id) == 0.0f)
				TU_U16(players->hud[i].speedIcon, 0x54) = 1;
			else
				TU_U16(players->hud[i].speedIcon, 0x54) = 0;
		}
	}
	if (CTankWorld_GetLaps(players->world) == 0) {
		int kills, b, c, laps, finished;
		int low, high;
		float clock;

		for (i = 0; i < TU_MAX_PLAYERS; i++) {
			CTankEntities_GetStats(players->entities, players->players[i].id, &kills, &b, &c, &laps,
				&finished);
			players->score[i] = kills <= 0 ? 0 : kills;
		}
		for (i = 0; i < TU_MAX_PLAYERS; i++) {
			players->teamScore[i] = players->score[i];
			if (DAT_002baebc != 0)
				players->teamScore[i] = players->score[DAT_002baec4[DAT_002baef4[i * 10] * 2
					- DAT_002baef8[i * 10]]] + players->score[i];
		}
		for (i = 0; i < TU_MAX_PLAYERS; i++) {
			int rank = 1;

			for (j = 0; j < i; j++) {
				if (players->teamScore[j] > players->teamScore[i])
					rank++;
				if (players->teamScore[j] < players->teamScore[i])
					players->rank[j]++;
			}
			players->rank[i] = rank;
		}
		players->players[0].winner = players->rank[0] == 1;
		players->players[1].winner = players->rank[1] == 1;
		players->players[2].winner = players->rank[2] == 1;
		players->players[3].winner = players->rank[3] == 1;
		if (CTankWorld_GetLaps(players->world) == 0)
			CLHud_SetScores(TU_HUD, players->score);
		for (i = 0; i < TU_MAX_PLAYERS; i++)
			DAT_002baee8[i * 10] = players->score[i];
		high = players->score[0];
		low = players->score[0];
		if (high <= players->score[1])
			high = players->score[1];
		if (low > players->score[1])
			low = players->score[1];
		if (high <= players->score[2])
			high = players->score[2];
		if (low > players->score[2])
			low = players->score[2];
		if (high <= players->score[3])
			high = players->score[3];
		if (low > players->score[3])
			low = players->score[3];
		TU_F32(TU_HUD, 0x3c) = (float)low;
		TU_F32(TU_HUD, 0x40) = (float)high;
		CLHud_SetScoresInt(TU_HUD, players->score);
		CLHud_UpdateBars(TU_HUD);
		{
			float left = CTankCamera_GetShotTimeLeft(players->camera);

			switch (*state) {
			case 0:
			case 1:
			case 2:
			case 3:
				clock = 60.0f;
				break;
			case 4:
				clock = left;
				break;
			case 5:
			case 6:
			case 7:
				clock = 0.0f;
				break;
			default:
				return;
			}
		}
		CLHud_SetClock(TU_HUD, clock, 1);
		return;
	}
	{
		int progress[TU_MAX_PLAYERS];
		int finishedCount = 0;
		int best;

		for (i = 0; i < TU_MAX_PLAYERS; i++) {
			char text[64];
			int kills, b, c, laps, finished, value;

			CTankEntities_GetStats(players->entities, players->players[i].id, &kills, &b, &c, &laps,
				&finished);
			sprintf(text, "");
			bkStringToWide((ushort *)TU_P(players->hud[i].text, 0xa4), text);
			TU_F32(players->hud[i].text, 0x2b8) = TU_F32(TU_HUD, 0x98 + i * 4) * 127.0f;
			value = laps + 1;
			if (value <= 1)
				value = 1;
			CLHud_SetFraction(TU_HUD, i, value, CTankWorld_GetLaps(players->world));
			value = laps <= 0 ? 0 : laps;
			players->score[i] = value;
			players->teamScore[i] = value;
			if (finished != 0) {
				TU_U16(players->hud[i].shellIcon, 0x54) = 1;
				players->players[i].done = 1;
				finishedCount++;
				if (players->rank[i] == 9) {
					players->rank[i] = players->nextRank;
					if (players->nextRank == 1)
						CLSamples_PlayVoice((char)DAT_002baed4[i * 10], 9, NULL, 0xff, 180.0f, 0, -1, 0);
					players->nextRank++;
					TU_DPRINTF("rankings [%d,%d,%d,%d]\n", players->rank[0], players->rank[1],
						players->rank[2], players->rank[3]);
				}
			}
		}
		if (finishedCount >= 3)
			*state = 5;
		players->score[0] = 5 - players->rank[0];
		players->score[1] = 5 - players->rank[1];
		players->score[2] = 5 - players->rank[2];
		players->score[3] = 5 - players->rank[3];
		if (*state > 4)
			for (i = 0; i < TU_MAX_PLAYERS; i++)
				DAT_002baee8[i * 10] = players->score[i];
		for (i = 0; i < TU_MAX_PLAYERS; i++) {
			progress[i] = CTankEntities_GetProgress(players->entities, players->players[i].id);
			CLHud_SetBar(TU_HUD, i, (float)progress[i] * 0.00208333344f);
		}
		best = progress[0];
		if (best <= progress[1])
			best = progress[1];
		if (best <= progress[2])
			best = progress[2];
		if (best <= progress[3])
			best = progress[3];
		players->players[0].winner = progress[0] == best;
		players->players[1].winner = progress[1] == best;
		players->players[2].winner = progress[2] == best;
		players->players[3].winner = progress[3] == best;
		best = progress[0];
		if (progress[1] > best)
			best = progress[1];
		if (progress[2] > best)
			best = progress[2];
		if (progress[3] > best)
			best = progress[3];
		for (i = 0; i < TU_MAX_PLAYERS; i++)
			CTankEntities_SetLead(players->entities, players->players[i].id,
				(float)(best - progress[i]) * 0.00208333344f);
		CLHud_SetClock(TU_HUD, -1.0f, 1);
	}
}

/* 0x001c9c90  TankedUp (gap)
Starts the players: the HUD, and an engine sound (one of three at random) per tank. */
void CTankPlayers_Start(void *self)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	float pos[4];
	int i;

	CTankPlayers_CreateHud(self);
	for (i = 0; i < TU_MAX_PLAYERS; i++) {
		int kind = -1;

		TUB_Entities_GetPosition(players->entities, players->players[i].id, pos);
		switch (tu_rand() % 3) {
		case 0:
			kind = 10;
			break;
		case 1:
			kind = 0xb;
			break;
		case 2:
			kind = 0xc;
			break;
		}
		if (kind != -1)
			players->players[i].channel = TUB_Sound_Play(players->sound, kind, pos, 100, 1, 0);
		bsReserveChannel(players->players[i].channel, 1);
	}
}

/* 0x001c9d30  TankedUp (gap)
The players' update: the score bars, the debug frame-rate text, the HUD, the tracking
spots, the debris attractors (the four tank positions). In the intro (1) a fire button
skips to state 2; at the end (5, after 180 frames) the winners' fire buttons (anyone's
when no human won) go on to state 6. Adds the frame time to the HUD clock. Returns the
wished state. */
int CTankPlayers_Update(void *self, float step, int state)
{
	struct TuPlayers *players = (struct TuPlayers *)self;
	float positions[TU_MAX_PLAYERS][4];
	char text[256];
	unsigned int i;
	int anyone;

	players->gameState = state;
	for (i = 0; i < TU_MAX_PLAYERS; i++)
		players->players[i].bar = CTankEntities_GetScoreBar(players->entities, players->players[i].id);
	sprintf(text, "[frames last sec: %d, %d] [avg(theres): %.2f]", TU_I32(players->camera, 0x18),
		TU_I32(players->camera, 0x68), (double)DAT_002ba990);
	bkStringToWide((ushort *)TU_P(players->debugText, 0xa4), text);
	TU_U16(players->debugText, 0x54) = (unsigned short)(TU_DBG_HIDE_TEXT == 0);
	CTankPlayers_UpdateHud(self, &state);
	/* (the original leaves the slots of absent players uninitialised) */
	memset(positions, 0, sizeof positions);
	for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
		TUB_Entities_GetPosition(players->entities, players->players[i].id, positions[i]);
		CTankLights_TrackSpot(players->lights, (int)i, positions[i]);
	}
	DebrisColours_SetAll(positions[0], positions[1], positions[2], positions[3]);
	anyone = 1;
	if (state == 1) {
		if (DAT_002bae94 == 0)
			goto done;
		for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
			struct TuPlayer *p = &players->players[i];

			if (p->primary != NULL && TU_I32(p->primary, 0x14) != 0)
				state = 2;
			if (p->secondary != NULL && TU_I32(p->secondary, 0x14) != 0)
				state = 2;
		}
	}
	if (state == 5 && CTankCamera_GetShotTime(players->camera) > 180.0f && DAT_002bae94 != 0) {
		for (i = 0; i < (unsigned int)DAT_002bae94; i++)
			if (players->rank[i] == 1 && players->players[i].human != 0)
				anyone = 0;
		for (i = 0; i < (unsigned int)DAT_002bae94; i++) {
			if (players->rank[i] == 1 || anyone) {
				struct TuPlayer *p = &players->players[i];

				if (p->primary != NULL && TU_I32(p->primary, 0x14) != 0)
					state = 6;
				if (p->secondary != NULL && TU_I32(p->secondary, 0x14) != 0)
					state = 6;
			}
		}
	}
done:
	CLHud_AddSeconds(TU_HUD, step);
	return state;
}
