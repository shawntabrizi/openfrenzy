/*
Clusteroids/CClust_Player.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CClust_Player.cpp

The four players (records of 0x850 bytes from 0x002fcb20, see clusteroids_game.h): set-up
with their input map and models, the per-frame movement (stick or AI, thrust, drag,
boost, firing, turning, the arena wall), the end-of-round animations and positions, and
the rumble effects.

Tagged with CClust_Player.cpp: 0x000dee00 (allocation file name). 0x000df7f0-0x000dfc90
(gap) follow it and work on the same records; their file is inferred.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

#define CL_CHANNEL_ON(channel) CL_I32((channel), 0x14)

extern int DAT_002baee0[];          /* input port flags of player i: [i * 10] (core) */
extern int DAT_002baef4[];          /* the team of player i: [i * 10] (core) */

/* the actor names of the character animations (0x20 bytes each, 19 from 0x002972a8):
   idle, left, right, still, push_forward, nuetral_back, bump_l, bump_r, bump_f, bump_b,
   winstart, winloop, winend, losestart, loseloop, loseend, victory_start, victory_loop,
   victory_end; found in the character actor into +0x698 */
#define CL_ANIM_NAME(k) ((char *)CL_ADDR(0x002972a8) + (k) * 0x20)
#define CL_ANIM_COUNT 19

/* keyboard bindings of the four players (debug keyboard): fire, boost, x -/+, y -/+ */
static const unsigned int cl_keys[4][6] = {
	{ 0x69, 0x67, 0x4600009d, 0x4600009c, 0x4600009b, 0x4600009a },
	{ 0x68, 0x66, 0x46000048, 0x46000045, 0x46000057, 0x4600005b },
	{ 0x4c, 0x52, 0x46000050, 0x4600004e, 0x4600004f, 0x4600004d },
	{ 0x83, 0x82, 0x46000088, 0x46000086, 0x46000087, 0x4600008a },
};

/* 0x000dee00  Clusteroids/CClust_Player.cpp
Sets up player `player`: the input map ("Fire", "Boost", "p1 joy x/y" on its pad and keys),
the start position and heading, cleared state, the AI tuning, the ship and character
actors and the character's animations. */
void ClustPlayer_Init(int player)
{
	char *pl = CL_PLAYER(player);
	struct BInputMap *map;
	struct biChannel *channel;
	unsigned int port;
	int k;

	map = biCreateMap(8);
	CL_AT(pl, PL_IN_MAP, struct BInputMap *) = map;
	if (player >= 0 && player <= 3) {
		port = (unsigned int)DAT_002baee0[player * 10];
		channel = biCreateChannel((struct biMap *)map, "Fire", 1, 0, 0, 0, 0);
		CL_AT(pl, PL_IN_FIRE, struct biChannel *) = channel;
		biBindInput(channel, port | 199, 0);
		biBindInput(channel, port | 0xc6, 0);
		biBindInput(channel, cl_keys[player][0], 0);
		channel = biCreateChannel((struct biMap *)map, "Boost", 1, 0, 0, 0, 0);
		CL_AT(pl, PL_IN_BOOST, struct biChannel *) = channel;
		biBindInput(channel, port | 0xc5, 0);
		biBindInput(channel, cl_keys[player][1], 0);
		channel = biCreateChannel((struct biMap *)map, "p1 joy x", 3, 0, 0, 0, 0);
		CL_AT(pl, PL_IN_STICK_X, struct biChannel *) = channel;
		biBindInput(channel, port | 0xb9, 0);
		biBindInput(channel, cl_keys[player][2], cl_keys[player][3]);
		biBindInput(channel, port | 0xc0, port | 0xbf);
		channel = biCreateChannel((struct biMap *)map, "p1 joy y", 3, 0, 0, 0, 0);
		CL_AT(pl, PL_IN_STICK_Y, struct biChannel *) = channel;
		biBindInput(channel, port | 0xba, 0);
		biBindInput(channel, cl_keys[player][4], cl_keys[player][5]);
		biBindInput(CL_AT(pl, PL_IN_STICK_Y, struct biChannel *), port | 0xbe, port | 0xbd);
	}
	CL_F32(pl, PL_X) = CL_VAR(float, 0x002f7f40 + player * 8);
	CL_F32(pl, 0x30) = CL_VAR(float, 0x002f7f40 + player * 8);
	CL_I16(pl, 0x66c) = -1;
	CL_I32(pl, 0x67c) = -1;                 /* engine sound channel */
	CL_F32(pl, PL_Z) = CL_VAR(float, 0x002f7f44 + player * 8);
	CL_F32(pl, 0x34) = CL_VAR(float, 0x002f7f44 + player * 8);
	CL_I16(pl, PL_SCORE) = 0;
	CL_U32(pl, 0x8) = 0;
	CL_I16(pl, 0x4) = 0;
	CL_I16(pl, 0xc) = 0;
	CL_I16(pl, 0xe) = 0;
	CL_I16(pl, 0x10) = 0;
	CL_I16(pl, 0x12) = (short)player;
	CL_F32(pl, 0x5c) = 0.0f;                /* velocity x */
	CL_F32(pl, 0x60) = 0.0f;                /* velocity z */
	CL_I16(pl, 0x650) = 0;
	CL_U32(pl, 0x674) = 0;
	CL_U32(pl, 0x678) = 0;
	CL_F32(pl, 0x658) = 0.0f;               /* fire delay timer */
	CL_F32(pl, 0x648) = 0.0f;
	CL_F32(pl, 0x64c) = 5.0f;
	CL_I16(pl, 0x84c) = 0;
	CL_F32(pl, 0x664) = 0.0f;
	CL_I16(pl, 0x668) = 0;
	CL_I16(pl, 0x66a) = 0;
	CL_I16(pl, PL_OUT) = 0;
	CL_U32(pl, 0x654) = 0;
	CL_F32(pl, 0x65c) = 0.0f;               /* boost delay timer */
	CL_F32(pl, 0x680) = 0.0f;
	CL_F32(pl, 0x684) = 0.0f;               /* wall hit timer */
	CL_F32(pl, 0x48) = -1.0f;
	CL_F32(pl, 0x4c) = -1.0f;
	CL_F32(pl, 0x14) = 0.0f;
	CL_I16(pl, 0x808) = 0;                  /* AI buttons */
	CL_I16(pl, 0x80a) = 0;
	CL_I16(pl, 0x80e) = 0;
	CL_I16(pl, 0x810) = 0;
	CL_U32(pl, 0x848) = 0;
	CL_I16(pl, 0x80c) = 0;
	switch (player) {
	case 0:
		CL_F32(pl, 0x634) = 3.1415925f;     /* heading */
		CL_F32(pl, 0x638) = 3.1415925f;     /* wanted heading */
		break;
	case 1:
		CL_F32(pl, 0x634) = 1.5707963f;
		CL_F32(pl, 0x638) = 1.5707963f;
		break;
	case 2:
		CL_F32(pl, 0x634) = 4.712389f;
		CL_F32(pl, 0x638) = 4.712389f;
		break;
	case 3:
		CL_F32(pl, 0x634) = 0.0f;
		CL_F32(pl, 0x638) = 0.0f;
		break;
	}
	/* AI tuning (the first value is written twice; the second ignores human players) */
	CL_F32(pl, 0x824) = (DAT_002baed8[player * 10] != 0 ? CL_VAR(float, 0x002f7d74)
		: CL_VAR(float, 0x002f7d78)) * 32768.0f;
	CL_U32(pl, 0x828) = CL_VAR(unsigned int, 0x002f7d7c);
	CL_F32(pl, 0x824) = CL_VAR(float, 0x002f7d78) * 32768.0f;
	CL_U32(pl, 0x82c) = CL_VAR(unsigned int, 0x002f7d80);

	baInitActorInstance((struct BActorInstance *)(pl + PL_SHIP), CL_VAR(struct BActor *, 0x002f8f70 + player * 4),
		0, CL_FILE_PLAYER, 0xe7);
	baPrepActorInstance((struct BActorInstance *)(pl + PL_SHIP), 1);
	baInitActorInstance((struct BActorInstance *)(pl + PL_SHIP2), CL_VAR(struct BActor *, 0x002f8f80 + player * 4),
		0, CL_FILE_PLAYER, 0xeb);
	baPrepActorInstance((struct BActorInstance *)(pl + PL_SHIP2), 1);
	baInitActorInstance((struct BActorInstance *)(pl + PL_CHAR), CL_VAR(struct BActor *, 0x002f8f90 + player * 4),
		10, CL_FILE_PLAYER, 0xef);
	baPrepActorInstance((struct BActorInstance *)(pl + PL_CHAR), 1);
	baInitActorInstance((struct BActorInstance *)(pl + PL_CHAR2), CL_VAR(struct BActor *, 0x002f8fa0 + player * 4),
		10, CL_FILE_PLAYER, 0xf3);
	baPrepActorInstance((struct BActorInstance *)(pl + PL_CHAR2), 1);
	CL_F32(pl, PL_SHIP + 0xd8) = 0.2f;
	CL_F32(pl, PL_SHIP + 0xd4) = 0.2f;
	CL_F32(pl, PL_SHIP + 0xd0) = 0.2f;
	CL_F32(pl, PL_SHIP2 + 0xd8) = 0.2f;
	CL_F32(pl, PL_SHIP2 + 0xd4) = 0.2f;
	CL_F32(pl, PL_SHIP2 + 0xd0) = 0.2f;
	CL_F32(pl, PL_CHAR + 0xd8) = 0.2f;
	CL_F32(pl, PL_CHAR + 0xd4) = 0.2f;
	CL_F32(pl, PL_CHAR + 0xd0) = 0.2f;
	CL_F32(pl, PL_CHAR2 + 0xd8) = 0.2f;
	CL_F32(pl, PL_CHAR2 + 0xd4) = 0.2f;
	CL_F32(pl, PL_CHAR2 + 0xd0) = 0.2f;
	baSetInstanceAnimations(CL_VAR(struct BActor *, 0x002f8f90 + player * 4),
		(struct BActorInstance *)(pl + PL_CHAR2));
	for (k = 0; k < CL_ANIM_COUNT; k++)
		CL_AT(pl, PL_ANIMS + k * 4, struct BActorSubObject *) =
			baFindSubObject((struct BActorInstance *)(pl + PL_CHAR), CL_ANIM_NAME(k));
	baFlushAnimations(pl + PL_CHAR, CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *), 1, 0);
	baQueueAnimation((struct BActorInstance *)(pl + PL_CHAR), CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *),
		CL_AT(pl, PL_ANIMS, struct BAnim *), 0x400, -1, 1, NULL, NULL);
	baFlushAnimations(pl + PL_CHAR2, CL_AT(pl, PL_CHAR2 + 0x134, struct BActorNode *), 1, 0);
	baQueueAnimation((struct BActorInstance *)(pl + PL_CHAR2), CL_AT(pl, PL_CHAR2 + 0x134, struct BActorNode *),
		CL_AT(pl, PL_ANIMS, struct BAnim *), 0x400, -1, 1, NULL, NULL);
	CL_VAR(int, 0x002fa1e4) = 0;
}

/* both character actors of player `player` play animation `k` (looping) */
static void cl_play_character_anim(int player, int k)
{
	char *pl = CL_PLAYER(player);
	struct BAnim *anim = CL_AT(pl, PL_ANIMS + k * 4, struct BAnim *);

	baFlushAnimations(pl + PL_CHAR, CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *), 1, 0);
	baFlushAnimations(pl + PL_CHAR2, CL_AT(pl, PL_CHAR2 + 0x134, struct BActorNode *), 1, 0);
	baQueueAnimation((struct BActorInstance *)(pl + PL_CHAR), CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *),
		anim, 0x400, -1, 1, NULL, NULL);
	baQueueAnimation((struct BActorInstance *)(pl + PL_CHAR2), CL_AT(pl, PL_CHAR2 + 0x134, struct BActorNode *),
		anim, 0x400, -1, 1, NULL, NULL);
}

/* 0x000df7f0  Clusteroids (gap; CClust_Player.cpp inferred)
The end of the round: the winner's characters play animation 13 ("losestart" in the
name table), the others animation 10 ("winstart"); in a draw (team games) all play 13.
Without a single winner, every player with the highest score counts as a winner. */
void ClustPlayers_Celebrate(void)
{
	char *pl;
	int winner;
	int i;

	if (DAT_002baebc == 0) {
		winner = ClustGame_GetWinner();
		if (winner != -1) {
			for (i = 0; i < 4; i++)
				cl_play_character_anim(i, i == winner ? 13 : 10);
			return;
		}
		winner = ClustGame_GetHighestScore();
		for (i = 0; i < 4; i++)
			cl_play_character_anim(i, winner == CL_I16(CL_PLAYER(i), PL_SCORE) ? 13 : 10);
		return;
	}
	if (CL_VAR(short, 0x002f7f7e) == -1) {
		for (i = 0; i < 4; i++) {
			pl = CL_PLAYER(i);
			baFlushAnimations(pl + PL_CHAR, CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *), 1, 0);
			baQueueAnimation((struct BActorInstance *)(pl + PL_CHAR),
				CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *),
				CL_AT(pl, PL_ANIMS + 13 * 4, struct BAnim *), 0x400, -1, 1, NULL, NULL);
			baFlushAnimations(pl + PL_CHAR2, CL_AT(pl, PL_CHAR2 + 0x134, struct BActorNode *), 1, 0);
			baQueueAnimation((struct BActorInstance *)(pl + PL_CHAR2),
				CL_AT(pl, PL_CHAR2 + 0x134, struct BActorNode *),
				CL_AT(pl, PL_ANIMS + 13 * 4, struct BAnim *), 0x400, -1, 1, NULL, NULL);
		}
		return;
	}
	for (i = 0; i < 4; i++)
		cl_play_character_anim(i, DAT_002baef4[i * 10] == (int)CL_VAR(short, 0x002f7f7e) ? 13 : 10);
}

/* 0x000dfb30  Clusteroids (gap; CClust_Player.cpp inferred)
Lines the ships up for the winner display (x = -150 + 100 * player, z = -400, heading 0),
with their pod chains behind them. */
void ClustPlayers_LineUp(void)
{
	char *pl;
	char *pod;
	float x;
	int i;

	for (i = 0; i < 4; i++) {
		pl = CL_PLAYER(i);
		pod = CL_PTR(pl, 0x674);
		x = (float)(i * 100) - 150.0f;
		CL_F32(pl, PL_Z) = -400.0f;
		CL_F32(pl, 0x34) = -400.0f;
		CL_F32(pl, 0x3c) = -400.0f;
		CL_F32(pl, 0x634) = 0.0f;
		CL_F32(pl, PL_X) = x;
		CL_F32(pl, 0x30) = x;
		CL_F32(pl, 0x38) = x;
		if (pod == NULL)
			continue;
		CL_F32(pod, 0x10) = x;
		CL_F32(pod, 0x14) = CL_F32(pl, PL_Z) + 65.0f;
		while (CL_PTR(pod, 0) != NULL) {
			CL_U32(CL_PTR(pod, 0), 0x10) = CL_U32(pod, 0x10);
			CL_F32(CL_PTR(pod, 0), 0x14) = CL_F32(pod, 0x14) + 60.0f;
			pod = CL_PTR(pod, 0);
		}
	}
}

/* rumble of player `player` with checks as in the original ("Rumble Error") */
static void cl_rumble_check(int player)
{
	if (player < 0 || 3 < player)
		bkPrintf("Rumble Error\n");
}

/* 0x000dfbe0  Clusteroids (gap; CClust_Player.cpp inferred)
Rumble effect `effect` on player's pad: 0 wall hit, 1 hit by a bullet, 2 boost. */
void ClustPlayer_Rumble(int player, int effect)
{
	cl_rumble_check(player);
	if (effect == 0) {
		CLRumble_PlayMotor((int)DAT_002ed324, player, 0, 0x7fff, 200, 0x46, 10, 0, 0);
	} else if (effect == 1) {
		CLRumble_PlayMotor((int)DAT_002ed324, player, 0, 0x7fff, 0x5dc, 0x1e, 0x28, 0, 3);
		CLRumble_PlayMotor((int)DAT_002ed324, player, 1, 0x7fff, 1000, 10, 0x3c, 0, 4);
	} else if (effect == 2) {
		CLRumble_PlayMotor((int)DAT_002ed324, player, 1, 0x7fff, 0x2ee, -1, 0x3c, 0, 0);
	}
}

/* 0x000dfc90  Clusteroids (gap; CClust_Player.cpp inferred)
One frame of player `player`: the stick (human) or the variant's AI gives the thrust
direction; the engine sound follows the thrust; thrust, movement (0.625 per frame),
drag; boost (a kick of 80 opposite the heading, then a delay); timers; firing; turning
towards the thrust direction at most at the turn rate; the arena wall pushes back with
sparks, a sound and rumble. */
void ClustPlayer_Update(int player)
{
	char *pl = CL_PLAYER(player);
	float stickX;
	float stickY;
	float timer;
	float delta;
	float turn;
	float step;
	float limit;
	float dir[4];
	float pos[4];
	float wall;
	double angle;
	int volume;
	int pressed;
	int i;

	CL_U32(pl, 0x38) = CL_U32(pl, PL_X);
	CL_U32(pl, 0x3c) = CL_U32(pl, PL_Z);
	if (CL_I16(pl, PL_OUT) != 0)
		return;
	if (DAT_002baed8[player * 10] == 0) {
		switch (g_ClVariant) {
		case 1:
			ClustAI_Tails(player, &stickX, &stickY);
			break;
		case 2:
			ClustAI_PodHog(player, &stickX, &stickY);
			break;
		case 3:
			ClustAI_Mines(player, &stickX, &stickY);
			break;
		case 4:
			ClustAI_Collector(player, &stickX, &stickY);
			break;
		}
	} else {
		stickX = (float)CL_CHANNEL_ON(CL_PTR(pl, PL_IN_STICK_X));
		stickY = -(float)CL_CHANNEL_ON(CL_PTR(pl, PL_IN_STICK_Y));
	}
	CL_F32(pl, PL_CHAR + 0xd8) = 0.2f;
	CL_F32(pl, PL_CHAR + 0xd4) = 0.2f;
	CL_F32(pl, PL_CHAR + 0xd0) = 0.2f;

	/* engine sound: volume 32-100 by the thrust */
	if (CL_I32(pl, 0x67c) == -1)
		CL_I32(pl, 0x67c) = ClustSound_Play(0x13, 100);
	volume = (short)CL_FTOL(sqrt((double)(stickX * stickX + stickY * stickY)) /
		(double)CL_I32(CL_PTR(pl, PL_IN_STICK_X), 0x1c) * 68.0 + 32.0);
	if ((short)volume > 0x80)
		volume = 0x80;
	bsSetChannelVolume(CL_I32(pl, 0x67c), (unsigned short)(short)volume);

	/* thrust */
	if (stickX != 0.0f || stickY != 0.0f) {
		CL_F32(pl, 0x638) = CLMath_WrapAngle(FUN_001334f0(stickX, -stickY));
		CL_F32(pl, 0x5c) = CL_VAR(float, 0x002f7d0c) * stickX * 3.0517578e-05f * g_ClFrameTime +
			CL_F32(pl, 0x5c);
		CL_F32(pl, 0x60) = CL_VAR(float, 0x002f7d0c) * stickY * 3.0517578e-05f * g_ClFrameTime +
			CL_F32(pl, 0x60);
	}
	CL_F32(pl, PL_X) = CL_F32(pl, 0x5c) * 0.625f * g_ClFrameTime + CL_F32(pl, PL_X);
	CL_F32(pl, PL_Z) = CL_F32(pl, 0x60) * 0.625f * g_ClFrameTime + CL_F32(pl, PL_Z);
	CL_F32(pl, 0x5c) = CL_F32(pl, 0x5c) - CL_VAR(float, 0x002f7d08) * CL_F32(pl, 0x5c) * g_ClFrameTime;
	CL_F32(pl, 0x60) = CL_F32(pl, 0x60) - CL_VAR(float, 0x002f7d08) * CL_F32(pl, 0x60) * g_ClFrameTime;

	/* boost */
	if (CL_F32(pl, 0x65c) > 0.0f) {
		timer = CL_F32(pl, 0x65c) - g_ClFrameTime;
		CL_F32(pl, 0x65c) = timer;
		if (timer < 0.0f)
			CL_F32(pl, 0x65c) = 0.0f;
	} else {
		if (DAT_002baed8[player * 10] == 0) {
			if (CL_I16(pl, 0x80e) == 0)
				goto timers;
		} else {
			if (CL_CHANNEL_ON(CL_PTR(pl, PL_IN_BOOST)) == 0)
				goto timers;
			cl_rumble_check(player);
			CLRumble_PlayMotor((int)DAT_002ed324, player, 1, 0x7fff, 0x2ee, -1, 0x3c, 0, 0);
		}
		if (CL_F32(pl, 0x660) == 0.0f && CL_VAR(int, 0x002f7dbc) != 0) {
			angle = (double)CL_F32(pl, 0x634) + (double)1.5707963f;
			CL_U32(pl, 0x65c) = CL_VAR(unsigned int, 0x002f7d1c);
			CL_F32(pl, 0x5c) = (float)((double)CL_F32(pl, 0x5c) - cos(angle) * 80.0);
			CL_F32(pl, 0x60) = (float)((double)CL_F32(pl, 0x60) - sin(angle) * 80.0);
			ClustSound_Play((int)Core_Rand() % 3 + 0xf, 100);
		}
	}
timers:
	if (CL_F32(pl, 0x658) > 0.0f) {
		timer = CL_F32(pl, 0x658) - g_ClFrameTime;
		CL_F32(pl, 0x658) = timer;
		if (timer < 0.0f)
			CL_F32(pl, 0x658) = 0.0f;
	}
	if (CL_F32(pl, 0x664) > 0.0f) {
		timer = CL_F32(pl, 0x664) - g_ClFrameTime;
		CL_F32(pl, 0x664) = timer;
		if (timer < 0.0f)
			CL_F32(pl, 0x664) = 0.0f;
	}
	if (CL_F32(pl, 0x14) > 0.0f) {
		timer = CL_F32(pl, 0x14) - g_ClFrameTime;
		CL_F32(pl, 0x14) = timer;
		if (timer < 0.0f)
			CL_F32(pl, 0x14) = 0.0f;
	}

	/* fire */
	if (DAT_002baed8[player * 10] == 0)
		pressed = CL_I16(pl, 0x80c) != 0;
	else
		pressed = CL_CHANNEL_ON(CL_PTR(pl, PL_IN_FIRE)) != 0;
	if (pressed && CL_F32(pl, 0x658) == 0.0f) {
		CL_U32(pl, 0x658) = CL_VAR(unsigned int, 0x002f7d18);
		ClustBullets_Fire((short)player);
		ClustSound_Play(cl_rand() % 2 + 10, 0x96);
	}

	/* turn */
	delta = CLMath_AngleDelta(CL_F32(pl, 0x638), CL_F32(pl, 0x634));
	turn = delta;
	if (delta < 0.0f) {
		limit = -CL_F32(pl, 0x82c);
		if (delta < limit)
			turn = limit;
	} else if (CL_F32(pl, 0x82c) < delta) {
		turn = CL_F32(pl, 0x82c);
	}
	step = g_ClFrameTime * turn;
	if (fabsf(step) < fabsf(delta))
		CL_F32(pl, 0x634) = step + CL_F32(pl, 0x634);
	else
		CL_U32(pl, 0x634) = CL_U32(pl, 0x638);

	/* the arena wall */
	if (fabsf(sqrtf(CL_F32(pl, PL_X) * CL_F32(pl, PL_X) + CL_F32(pl, PL_Z) * CL_F32(pl, PL_Z))) <=
			CL_VAR(float, 0x002f7f68) - 35.0f) {
		CL_F32(pl, 0x684) = 0.0f;
	} else {
		dir[0] = CL_F32(pl, PL_X);
		dir[1] = 0.0f;
		dir[2] = CL_F32(pl, PL_Z);
		dir[3] = 0.0f;
		CLVec3_Normalize(dir);
		wall = CL_VAR(float, 0x002f7f68) - 36.0f;
		pos[0] = dir[0] * wall;
		CL_F32(pl, PL_X) = pos[0];
		pos[2] = wall * dir[2];
		CL_F32(pl, PL_Z) = pos[2];
		i = cl_rand();
		CL_F32(pl, 0x5c) = CL_F32(pl, 0x5c) * 0.9f;
		CL_F32(pl, 0x60) = CL_F32(pl, 0x60) * 0.9f;
		if ((double)((float)i * 3.0518509e-05f) > 0.5) {
			pos[1] = 0.0f;
			pos[3] = 0.0f;
			Debris_AddRing((struct DebrisType *)CL_ADDR(0x00296c20), 1, pos, 3.2f, 1.8f, 0.8f, 0, 0xffffffff);
		}
		if (CL_F32(pl, 0x684) == 0.0f) {
			ClustSound_Play(0x16, 100);
			CL_F32(pl, 0x684) = 100.0f;
		}
		i = cl_rand();
		if ((double)((float)i * 3.0518509e-05f) > 0.75 || CL_F32(pl, 0x684) == 0.0f) {
			cl_rumble_check(player);
			CLRumble_PlayMotor((int)DAT_002ed324, player, 0, 0x7fff, 200, 0x46, 10, 0, 0);
		}
	}
	timer = CL_F32(pl, 0x680) - g_ClFrameTime;
	CL_F32(pl, 0x680) = timer;
	if (timer <= 0.0f)
		CL_F32(pl, 0x680) = 0.0f;
	if (CL_F32(pl, 0x648) != 0.0f) {
		timer = CL_F32(pl, 0x648) - CL_F32(pl, 0x64c);
		CL_F32(pl, 0x648) = timer;
		if (timer < 0.0f)
			CL_F32(pl, 0x648) = 0.0f;
	}
	/* an empty HUD method (shared by identical-code folding with 0x000b0770) */
	for (i = 0; i < 10; i++)
		AI_HoldEnd((void *)DAT_002ecfac, player, i);
	if (DAT_002baed8[player * 10] == 0)
		CL_I16(CL_VAR(int, 0x002f7a4c + player * 4), 0x54) = 1;
	else if (CL_F32(pl, 0x65c) == 0.0f)
		CL_I16(CL_VAR(int, 0x002f7a4c + player * 4), 0x54) = 0;
	else
		CL_I16(CL_VAR(int, 0x002f7a4c + player * 4), 0x54) = 1;
	baUpdateAnimations((struct BActorInstance *)(pl + PL_CHAR), CL_AT(pl, PL_CHAR + 0x134, struct BActorNode *),
		g_ClFrameTime, NULL);
}
