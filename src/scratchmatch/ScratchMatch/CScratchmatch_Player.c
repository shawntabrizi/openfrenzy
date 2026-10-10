/*
ScratchMatch/CScratchmatch_Player.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Player.cpp
(module scratchmatch)

The four players of Scratch Match ("scPlayer[i]", struct SMPlayer at 0x00310890): their models
(a platform from the variation package, the full and the low-polygon character), their
input (stick, d-pad and the four buttons), their dances, the button history, hitting the
icons (scored by timing: "fHitAcceptanceRange") and the computer players.
*/

#include "scratchmatch.h"

static const char s_PlayerFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\CScratchmatch_Player.cpp";

/* a biChannel's current value */
#define SM_CHANNEL_DOWN(channel) (SM_AT(channel, 0x14, int) != 0)

/* the rating text of a timing score (1 perfect ... 5 poor) */
static int sm_Rating(unsigned int score)
{
	if (score == 10)
		return 1;
	if (score > 7)
		return 2;
	if (score > 5)
		return 3;
	return 5 - (3 < score);
}

/* 0x001beb60  (gap; frees what SMPlayer_InitAll made): deletes the instances and the input
maps (the actors themselves are freed with the packages) */
void SMPlayer_FreeAll(void)
{
	struct SMPlayer *player;
	int i;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		baDeleteActorInstance(&player->pod);
		baDeleteActorInstance(&player->inst);
		baDeleteActorInstance(player->lowInst);
		player->podActor = NULL;
		player->actor = NULL;
		player->lowActor = NULL;
	}
	for (i = 0; i < SM_NUM_PLAYERS; i++)
		biDeleteMap(sm_Players[i].input);
}

/* 0x001bebd0  (gap; placed with the players): runs the animations of both models */
void SMPlayer_UpdateAnimations(void)
{
	struct SMPlayer *player;
	struct BActorInstance *low;
	int i;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		baUpdateAnimations(&player->inst, player->inst.root, (float)(SMTimer_FrameTime() * 50.0f), NULL);
		low = player->lowInst;
		baUpdateAnimations(low, low->root, (float)(SMTimer_FrameTime() * 50.0f), NULL);
	}
}

/* 0x001bec40  (gap; placed with the players): 1 if any player holds one of the four buttons */
int SMPlayer_AnyButtonPressed(void)
{
	int i, b;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		for (b = 0; b < 4; b++) {
			if (SM_CHANNEL_DOWN(sm_Players[i].buttons[b]))
				return 1;
		}
	}
	return 0;
}

/* 0x001bec90  (gap; placed with the players): the winner's animation ("good_scratch") */
void SMPlayer_PlayWinAnimation(int index)
{
	struct SMPlayer *player = &sm_Players[index];
	struct BAnim *anim = (struct BAnim *)player->anims[5];

	baFlushAnimations(&player->inst, player->inst.root, 1, 0);
	baQueueAnimationBlendIn(&player->inst, player->inst.root, anim, 0x400, -1, 0.025f, 0, NULL, NULL);
	baQueueAnimation(&player->inst, player->inst.root, anim, 0x400, -1, 1, NULL, NULL);
}

/* 0x001bed00  ScratchMatch/CScratchmatch_Player.cpp: shifts the button history and stores the
buttons held now (bits 0-3 A B X Y, bits 4-7 d-pad up right down left) */
void SMPlayer_ReadButtons(int index)
{
	struct SMPlayer *player = &sm_Players[index];
	unsigned int mask;
	int k, b;

	for (k = sm_HistoryLength - 1; k > 0; k--)
		player->history[k] = player->history[k - 1];
	mask = 0;
	for (b = 0; b < 4; b++) {
		if (SM_CHANNEL_DOWN(player->buttons[b]))
			mask |= 1u << b;
	}
	for (b = 0; b < 4; b++) {
		if (SM_CHANNEL_DOWN(player->dpad[b]))
			mask |= 0x10u << b;
	}
	player->history[0] = mask;
}

/* 0x001bee20  ScratchMatch/CScratchmatch_Player.cpp (bracketed): 1 if the buttons held over the
history (all of them together) are exactly `buttons` */
int SMPlayer_ButtonsMatch(int index, unsigned int buttons)
{
	unsigned int held;
	int k;

	if (buttons == 0)
		return 0;
	held = 0;
	for (k = 0; k < sm_HistoryLength; k++)
		held |= sm_Players[index].history[k];
	return buttons == held;
}

/* 0x001bee70  ScratchMatch/CScratchmatch_Player.cpp (bracketed): on each beat, for each living
player: a missed icon crosses out and breaks the combo (a missed bonus icon gives the others
10 points), a player without lives dies (survival), else the next sequence line becomes an
icon */
void SMPlayer_UpdateSequence(void)
{
	struct SMPlayer *player;
	unsigned int buttons;
	int i, j;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		if (player->dead == 1)
			continue;
		if (player->hit == 0) {
			if (player->prevButtons == SM_BONUS && player->unk3f4 != SM_BONUS) {
				player->prevButtons = player->unk3f4;
				for (j = 0; j < SM_NUM_PLAYERS; j++) {
					if (j != i) {
						sm_Players[j].score = sm_Players[j].score + 10;
						SMSprites_CrossPreviousIcon(i);
					}
				}
			} else if (player->prevButtons != 0 && SMSprites_IsBonusIcon((unsigned int)player->prevIcon) == 0) {
				SMSprites_CrossPreviousIcon(i);
				player->hits = 0;
				player->lifeLostTime = (float)SMTimer_Elapsed(SMT_BEAT);
			}
		}
		if (sm_GameVariation == 3 && player->lives == 0) {
			player->dead = 1;
			SMSprites_RemovePlayerIcons(i);
			player->deathTime = (float)SMTimer_Elapsed(SMT_BEAT);
			CLSamples_PlayCommentary(0x37, player->character, 0xff, 0, 0, NULL);
			sm_AliveCount = sm_AliveCount - 1;
		} else if (player->seqPos < sm_SequenceLines &&
			   SMTimer_Elapsed(SMT_BEAT) + sm_SequenceEndMargin <= (double)sm_GameLength - 4.0f) {
			buttons = SMGame_GetSequenceEntry(i, (int)player->seqPos);
			if (buttons != 0)
				SMSprites_AddIcon(i, (int)buttons, sm_IconLead + sm_BeatTime);
			player->seqPos = player->seqPos + 1;
		}
	}
}

/* 0x001bf000  ScratchMatch/CScratchmatch_Player.cpp: sets up the players from the player
records: input maps and bindings, colours, lives, the three models, the animations, and the
place on the stage (facing the centre, dropping in from 400 above) */
void SMPlayer_InitAll(void)
{
	char name[256];
	struct SMPlayer *player;
	struct biMap *map;
	const unsigned char *rgb;
	unsigned int port, r, g, b;
	int *record;
	float x, z;
	int i, k;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		record = sm_PlayerRecord(i);
		memset(player, 0, sizeof *player);
		map = (struct biMap *)biCreateMap(0xb);
		player->input = map;
		player->leftRight = biCreateChannel(map, "left/right", 3, 0, 0, 0, 0);
		player->upDown = biCreateChannel(player->input, "up/down", 3, 0, 0, 0, 0);
		player->buttons[0] = biCreateChannel(player->input, "button1", 1, 1, 0, 0, 0);
		player->buttons[1] = biCreateChannel(player->input, "button2", 1, 1, 0, 0, 0);
		player->buttons[2] = biCreateChannel(player->input, "button3", 1, 1, 0, 0, 0);
		player->buttons[3] = biCreateChannel(player->input, "button4", 1, 1, 0, 0, 0);
		player->dpad[0] = biCreateChannel(player->input, "digup", 1, 1, 0, 0, 0);
		player->dpad[1] = biCreateChannel(player->input, "digright", 1, 1, 0, 0, 0);
		player->dpad[2] = biCreateChannel(player->input, "digdown", 1, 1, 0, 0, 0);
		player->dpad[3] = biCreateChannel(player->input, "digleft", 1, 1, 0, 0, 0);
		port = (unsigned int)record[3];
		biBindInput(player->leftRight, port | 0xb9, 0);
		biBindInput(player->upDown, port | 0xba, 0);
		biBindInput(player->leftRight, port | 0xbb, 0);
		biBindInput(player->upDown, port | 0xbc, 0);
		biBindInput(player->dpad[0], port | 0xbd, 0);
		biBindInput(player->dpad[1], port | 0xc0, 0);
		biBindInput(player->dpad[2], port | 0xbe, 0);
		biBindInput(player->dpad[3], port | 0xbf, 0);
		biBindInput(player->buttons[0], port | 0xc5, 0);
		biBindInput(player->buttons[1], port | 0xc6, 0);
		biBindInput(player->buttons[2], port | 199, 0);
		biBindInput(player->buttons[3], port | 200, 0);
		player->score = 0;
		player->unk448 = 0;
		player->dance = 0;
		player->cpu = record[1] != 1;
		player->character = record[0];
		rgb = &DAT_00290718[record[0] * 0x60];
		player->colour = ((unsigned int)rgb[0] << 16) | ((unsigned int)rgb[1] << 8) | rgb[2];
		r = (unsigned int)rgb[0] * 2;
		if (r > 0xff)
			r = 0xff;
		g = (unsigned int)rgb[1] * 2;
		if (g > 0xff)
			g = 0xff;
		b = (unsigned int)rgb[2] * 2;
		if (b > 0xff)
			b = 0xff;
		player->visible = 1;
		player->lifeLostTime = -100.0f;
		player->pod.scale[0] = 1.0f;
		player->pod.scale[1] = 1.0f;
		player->pod.scale[2] = 1.0f;
		player->lives = sm_StartLives;
		player->brightColour = (r << 16) | (g << 8) | b;
		player->hit = 0;
		player->dead = 0;
		player->unk3d8 = 0;
		player->seqPos = 0;
		player->hits = 0;
		player->icon = SM_NO_ICON;
		player->prevIcon = SM_NO_ICON;
		player->unk3f4 = SM_NO_ICON;
		bkPrintf("Loading player %ld models...\n", i);
		sprintf(name, "actors\\%s", sm_CharacterModels + record[0] * 0x180);
		player->podActor = (struct BActor *)bLoadResource(sm_VarPackage, name, 1, 0xdefa);
		if (player->podActor == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Player.cpp", 0xe4, "POINTER ASSERTION FAILURE: scPlayer[i].actorPtr");
		sprintf(name, "models\\%s", sm_CharacterModels + 0x80 + record[0] * 0x180);
		player->actor = (struct BActor *)bLoadResource(sm_Package, name, 1, 0xdefa);
		if (player->actor == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Player.cpp", 0xe8, "POINTER ASSERTION FAILURE: scPlayer[i].pl_actorPtr");
		sprintf(name, "models\\%s", sm_CharacterModels + 0x100 + record[0] * 0x180);
		player->lowActor = (struct BActor *)bLoadResource(sm_Package, name, 1, 0xdefa);
		if (player->lowActor == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Player.cpp", 0xec, "POINTER ASSERTION FAILURE: scPlayer[i].pLowPolyActor");
		bkPrintf("Loaded player %ld models\n", i);
		baInitActorInstance(&player->inst, player->actor, 4, s_PlayerFile, 0xf2);
		player->lowInst = baInitActorInstance(NULL, player->lowActor, 4, s_PlayerFile, 0xf4);
		baSetInstanceAnimations(player->actor, player->lowInst);
		for (k = 0; k < 7; k++)
			player->anims[k] = baFindSubObject(&player->inst, sm_AnimNames[k]);
		baFlushAnimations(&player->inst, player->inst.root, 1, 0);
		baQueueAnimation(&player->inst, player->inst.root, (struct BAnim *)player->anims[0], 0x400, -1, 1, NULL, NULL);
		baFlushAnimations(player->lowInst, player->lowInst->root, 1, 0);
		baQueueAnimation(player->lowInst, player->lowInst->root, (struct BAnim *)player->anims[0], 0x400, -1, 1,
			NULL, NULL);
		baInitActorInstance(&player->pod, player->podActor, 4, s_PlayerFile, 0x108);
		x = sm_PlayerSpots[i * 2];
		player->pos[0] = x;
		player->spin = 0.0f;
		z = sm_PlayerSpots[i * 2 + 1];
		player->unk45c = 0.0f;
		player->pos[2] = z;
		player->drop = 400.0f;
		player->deathTime = 0.0f;
		player->heading = (float)atan2((double)x / (double)z, 1.0);
	}
	sm_Players[2].cpu = 0;
	sm_Players[3].cpu = 0;
}

/* 0x001bf5e0  ScratchMatch/CScratchmatch_Player.cpp (bracketed): the players bob on the stage
(each a quarter beat apart), minus their drop height */
void SMPlayer_UpdateIntroDrop(void)
{
	struct SMPlayer *player;
	double t;
	unsigned int i;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		t = SMTimer_Elapsed(SMT_BEAT);
		player->pos[0] = sm_PlayerSpots[i * 2];
		player->pos[2] = sm_PlayerSpots[i * 2 + 1];
		player->pos[1] = (float)((sin(sm_ufloat(i) * 1.57079637f + t * 4.0f) * 10.0f + sm_StageHeight) - player->drop);
	}
}

/* 0x001bf660  ScratchMatch/CScratchmatch_Player.cpp (bracketed): every 3.5 s each player starts
a random one of the four scratch dances (both models) */
void SMPlayer_UpdateDances(void)
{
	struct SMPlayer *player;
	struct BActorInstance *low;
	struct BAnim *anim;
	unsigned int dance;
	int i;

	if (SMTimer_Elapsed(SMT_DANCE) > SM_F(0x0030ffe8)) {
		SMTimer_Reset(SMT_DANCE);
		for (i = 0; i < SM_NUM_PLAYERS; i++) {
			player = &sm_Players[i];
			dance = sm_rand() & 3;
			player->dance = dance;
			anim = (struct BAnim *)player->anims[dance];
			baFlushAnimations(&player->inst, player->inst.root, 1, 0);
			baQueueAnimationBlendIn(&player->inst, player->inst.root, anim, 0x400, -1, 0.025f, 0, NULL, NULL);
			baQueueAnimation(&player->inst, player->inst.root, anim, 0x400, -1, 1, NULL, NULL);
			low = player->lowInst;
			baFlushAnimations(low, low->root, 1, 0);
			low = player->lowInst;
			baQueueAnimationBlendIn(low, low->root, anim, 0x400, -1, 0.025f, 0, NULL, NULL);
			low = player->lowInst;
			baQueueAnimation(low, low->root, anim, 0x400, -1, 1, NULL, NULL);
		}
	}
}

/* 0x001bf7b0  ScratchMatch/CScratchmatch_Player.cpp: decides for each computer player whether
it hits its current icon, and its timing: the chance (per mille) comes from the handicap and
the number of buttons; a bonus icon in the "Pattern" variation is always hit. The original
returns a leftover value that nobody uses; it is kept. An unknown handicap leaves the chance
as it was (the original then used whatever ECX held; here it starts at 0). */
unsigned int SMPlayer_DecideAIHits(void)
{
	struct SMPlayer *player;
	unsigned int result, buttons, random, count;
	int chance = 0;
	int i, bit;

	result = 0;
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		result = (unsigned int)player->cpu;
		if (result != 1)
			continue;
		if (DAT_002bae70 == 0)
			chance = 500;
		else if (DAT_002bae70 == 1)
			chance = 750;
		else if (DAT_002bae70 == 2)
			chance = 900;
		else
			SM_ASSERT_FAIL("CScratchmatch_Player.cpp", 0x5e6, "ASSERTION FAILURE: FALSE");
		buttons = SMSprites_GetIconButtons(player->icon);
		count = 0;
		for (bit = 0; bit < 8; bit++) {
			if ((buttons & (1u << bit)) != 0)
				count++;
		}
		count &= 0xff;
		if (count == 1)
			chance = chance + 150;
		else if (count == 3)
			chance = chance - 200;
		else if (count == 4)
			chance = chance - 300;
		if (chance > 1000)
			chance = 1000;
		else if (chance < 0)
			chance = 0;
		if (sm_GameVariation == 2 && buttons == SM_BONUS)
			chance = 1000;
		random = sm_rand();
		result = random / 1000;
		if ((int)(random % 1000) < chance) {
			float base = (float)(2 - DAT_002bae70) * 0.100000001f;
			unsigned int spread = sm_rand();

			result = sm_rand() & 1;
			player->aiWillHit = 1;
			player->aiTiming = base - (float)(int)(spread % 1000) * sm_HitRange * 0.000250000012f;
			if (result == 0)
				player->aiTiming = -player->aiTiming;
		} else {
			player->aiWillHit = 0;
		}
	}
	return result;
}

/* 0x001bf990  ScratchMatch/CScratchmatch_Player.cpp: the score of a hit from its timing: 10 at
the beat, 0 at the edge of the acceptance range */
int SMPlayer_TimingScore(float timing)
{
	float distance = fabsf(timing);

	if (!(distance <= sm_HitRange))
		SM_ASSERT_FAIL("CScratchmatch_Player.cpp", 0x630,
			"ASSERTION FAILURE: fAbsTiming <= gxGameVariables.fHitAcceptanceRange");
	return SM_FTOL((sm_HitRange - distance) / sm_HitRange * 10.5f);
}

/* 0x001bf9f0  (gap; placed with the players): a computer player reaches its icon. A normal
icon is hit if the player decided so and its timing has come (the score as for a human). A
bonus icon makes up a random pattern of 1-4 buttons (more with a higher handicap), shuffled,
and sends it to the other players. */
void SMPlayer_AIHit(int index, unsigned int buttons, float timing)
{
	struct SMPlayer *player = &sm_Players[index];
	unsigned int score, pick, k, r, bitK, bitR;
	int rating;

	if (buttons != SM_BONUS) {
		if (!(player->aiTiming < timing))
			return;
		if (player->aiWillHit != 1)
			return;
		player->hit = 1;
		player->hitButtons = SMSprites_GetPlayerIconButtons(index);
		SMSprites_RemoveIcon((unsigned int)player->icon);
		player->hits = player->hits + 1;
		SMSprites_StartPulse(index);
		if (sm_GameVariation == 3) {
			player->score = player->score + 1;
			return;
		}
		score = (unsigned int)SMPlayer_TimingScore(timing);
		rating = 3;
		if (score == 10) {
			rating = 1;
			if (rand() % 20 == 4)
				SMSound_PlayVoice(4, index);
		} else if (score > 7) {
			rating = 2;
		} else if (score <= 5) {
			rating = 5 - (3 < score);
		}
		SMSprites_AddText(index, rating, 0);
		if (sm_GameVariation != 3 && (player->hits & 0xf) == 0) {
			SMSprites_AddText(index, 6, player->hits);
			score = score + player->hits;
		}
		if (score == 0)
			score = 1;
		player->score = player->score + (int)score;
		return;
	}
	if (player->hit != 0)
		return;
	if (!(player->aiTiming < timing))
		return;
	if (player->aiWillHit != 1)
		return;
	pick = sm_rand() % 0x46 + (unsigned int)DAT_002bae70 * 0xf;
	if (pick > 100)
		buttons = 0xf;
	else if (pick < 0x46)
		buttons = 1;
	else if (pick < 0x55)
		buttons = 3;
	else if (pick < 0x5f)
		buttons = 7;
	else
		buttons = 0xf;
	/* swap bit k with a random bit, for each of the four bits */
	for (k = 0; k < 4; k++) {
		r = Core_Rand() & 3;
		bitR = (buttons >> r) & 1;
		bitK = (buttons >> k) & 1;
		buttons = (buttons & ~(1u << k)) | (bitR << k);
		buttons = (buttons & ~(1u << r)) | (bitK << r);
	}
	SMGame_SetOthersNextEntry((unsigned int)index, buttons);
	SMSprites_SetPlayerIconButtons(index, (int)buttons);
	SMSprites_StartPulse(index);
	player->hit = 1;
}

/* 0x001bfc40  (gap; placed with the players): reads the buttons and checks hits. In the intro
a button (after 1 s) skips the intro camera; in the outro (after 2 s) a button of a human
winner (or of anybody, when all winners are computer players) ends it. A human hits an icon
when the buttons held over the history are exactly the icon's within the acceptance range; on
a bonus icon whatever is held becomes the pattern sent to the others. */
void SMPlayer_UpdateHits(void)
{
	struct SMPlayer *player;
	unsigned int buttons, held, score, k;
	float now, timing;
	int i, b, allCpu, rating;

	now = (float)SMTimer_Elapsed(SMT_BEAT);
	if (sm_State == 0) {
		if (SMPlayer_AnyButtonPressed() != 1)
			goto players;
		if (SMTimer_Elapsed(SMT_GAME) > 1.0f) {
			sm_IntroSkipped = 1;
			bsSetSfxVolume((DAT_002bae6c * 0xff) / 0xff);
			return;
		}
	}
	if (sm_State == 3 && SMTimer_Elapsed(SMT_GAME) > 2.0f) {
		allCpu = 1;
		if (sm_Leaders != 0) {
			for (k = 0; k < sm_Leaders; k++) {
				if (sm_Players[sm_Rank[k]].cpu == 0)
					allCpu = 0;
			}
			if (!allCpu) {
				for (k = 0; k < sm_Leaders; k++) {
					for (b = 0; b < 4; b++) {
						if (SM_CHANNEL_DOWN(sm_Players[sm_Rank[k]].buttons[b])) {
							sm_OutroHold = 1;
							return;
						}
					}
				}
				goto players;
			}
		}
		for (k = 0; k < 4; k++) {
			for (b = 0; b < 4; b++) {
				if (SM_CHANNEL_DOWN(sm_Players[sm_Rank[k]].buttons[b])) {
					sm_OutroHold = 1;
					return;
				}
			}
		}
	}
players:
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		SMPlayer_ReadButtons(i);
		buttons = SMSprites_GetIconButtons(player->icon);
		timing = (float)(now - (double)SMSprites_GetIconHitTime(player->icon));
		if (buttons == SM_BONUS) {
			if (player->hit != 0 || !(fabsf(timing) < sm_HitRange))
				continue;
			if (player->cpu == 1) {
				SMPlayer_AIHit(i, SM_BONUS, timing);
				continue;
			}
			held = 0;
			if (sm_HistoryLength != 0) {
				for (b = 0; b < sm_HistoryLength; b++)
					held |= player->history[b];
				if (held != 0) {
					SMGame_SetOthersNextEntry((unsigned int)i, held);
					SMSprites_SetPlayerIconButtons(i, (int)held);
					SMSprites_StartPulse(i);
					player->hit = 1;
				}
			}
			continue;
		}
		if (buttons == 0 || player->hit != 0 || !(fabsf(timing) < sm_HitRange))
			continue;
		if (player->cpu == 1) {
			SMPlayer_AIHit(i, buttons, timing);
			continue;
		}
		if (SMPlayer_ButtonsMatch(i, buttons) != 1)
			continue;
		player->hit = 1;
		for (b = 0; b < sm_HistoryLength; b++)
			player->history[b] = 0;
		SMSprites_StartPulse(i);
		player->hitButtons = SMSprites_GetPlayerIconButtons(i);
		SMSprites_RemoveIcon((unsigned int)player->icon);
		player->hits = player->hits + 1;
		if (sm_GameVariation == 3) {
			player->score = player->score + 1;
			continue;
		}
		score = (unsigned int)SMPlayer_TimingScore(timing);
		rating = sm_Rating(score);
		if (score == 10 && rand() % 20 == 4)
			SMSound_PlayVoice(4, i);
		SMSprites_AddText(i, rating, 0);
		if ((player->hits & 0xf) == 0) {
			SMSprites_AddText(i, 6, player->hits);
			score = score + player->hits;
		}
		if (score == 0)
			score = 1;
		player->score = player->score + (int)score;
	}
}
