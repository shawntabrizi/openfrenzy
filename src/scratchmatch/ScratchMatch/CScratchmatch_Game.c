/*
ScratchMatch/CScratchmatch_Game.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Game.cpp
(module scratchmatch)

The rules of Scratch Match ("gxGameVariables"): the button sequence (data\sequence<handicap>_1.txt
of the variation package: a line count, then one line per beat with the buttons of the four
players), the game states (0 intro, 1 practice with the countdown, 2 the game, 3 the outro),
the speed levels of the survival variation, the ranking and the results.

A sequence entry is a button mask: bits 0-3 A, B, X, Y; 0x100 a bonus icon (any button).
*/

#include "scratchmatch.h"

static const char s_GameFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\CScratchmatch_Game.cpp";

/* 0x001bbf90  (gap; placed with the game rules): sorts the players into sm_Rank (insertion
sort: by score; in the survival game by lives, then score) and counts the players that share
first place (by score in both cases) */
void SMGame_RankPlayers(void)
{
	unsigned int k, j;
	int a, b, first;

	sm_Rank[0] = 0;
	sm_Rank[1] = 1;
	sm_Rank[2] = 2;
	sm_Rank[3] = 3;
	if (sm_GameVariation == 3) {
		for (k = 1; k < 4; k++) {
			for (j = k; j != 0; j--) {
				a = sm_Rank[j - 1];
				b = sm_Rank[j];
				if (sm_Players[a].lives < sm_Players[b].lives ||
				    (sm_Players[a].lives == sm_Players[b].lives && sm_Players[a].score < sm_Players[b].score)) {
					sm_Rank[j - 1] = b;
					sm_Rank[j] = a;
				}
			}
		}
	} else {
		for (k = 1; k < 4; k++) {
			for (j = k; j != 0; j--) {
				a = sm_Rank[j - 1];
				if (sm_Players[a].score < sm_Players[sm_Rank[j]].score) {
					sm_Rank[j - 1] = sm_Rank[j];
					sm_Rank[j] = a;
				}
			}
		}
	}
	first = sm_Players[sm_Rank[0]].score;
	sm_Leaders = 1;
	if (sm_Players[sm_Rank[1]].score == first)
		sm_Leaders = 2;
	if (sm_Players[sm_Rank[2]].score == first)
		sm_Leaders = sm_Leaders + 1;
	if (sm_Players[sm_Rank[3]].score == first)
		sm_Leaders = sm_Leaders + 1;
}

/* 0x001bc180  (gap; placed with the sequence it frees): frees the button sequence */
void SMGame_FreeSequence(void)
{
	bkHeapFree(sm_Sequence);
	sm_Sequence = NULL;
	FUN_001d5950();
}

/* 0x001bc1a0  (gap; a helper of the sequence loader): removes leading blanks (space, tab,
CR) and cuts the line at its CR; a blank line becomes "" */
void SMGame_TrimLine(char *line)
{
	unsigned int length, start, i;
	char *out;
	char c;

	length = (unsigned int)lstrlenA(line);
	start = 0;
	while (c = line[start], c == ' ' || c == '\t' || c == '\r') {
		start++;
		if (start == length) {
			lstrcpyA(line, "");
			return;
		}
	}
	i = start;
	out = line;
	if (length < start)
		return;
	do {
		if (line[i] == '\r') {
			line[i - start] = '\0';
			return;
		}
		*out++ = line[i];
		i++;
	} while (i <= length);
}

/* 0x001bc200  (gap; a helper of the sequence loader): the first number of a line */
int SMGame_ParseFirstNumber(const char *line)
{
	char word[64];
	char copy[256];
	int length, i;

	lstrcpyA(copy, line);
	SMGame_TrimLine(copy);
	length = lstrlenA(copy);
	for (i = 0; copy[i] != ' ' && copy[i] != '\t' && copy[i] != '\r' && i != length; i++)
		word[i] = copy[i];
	word[i] = '\0';
	return (int)atol(word);
}

/* 0x001bc270  ScratchMatch/CScratchmatch_Game.cpp: the buttons of `player` at sequence line
`position` */
unsigned int SMGame_GetSequenceEntry(int player, int position)
{
	unsigned int entry = sm_Sequence[player + position * 4];

	if (entry >= SM_SEQ_LIMIT)
		SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x42f, "ASSERTION FAILURE: un32Entry < SCBBUF_LIMIT");
	return entry;
}

/* 0x001bc2b0  ScratchMatch/CScratchmatch_Game.cpp (bracketed): copies the results into the
player records (survival: 5000 points a life left, else the score) and counts the players
sharing the best result */
void SMGame_StoreResults(void)
{
	int i, best;

	if (sm_GameVariation == 3) {
		for (i = 0; i < SM_NUM_PLAYERS; i++)
			sm_PlayerRecord(i)[5] = sm_Players[i].lives * 5000;
		for (i = 0; i < SM_NUM_PLAYERS; i++) {
			if (sm_PlayerRecord(i)[5] == 0)
				sm_PlayerRecord(i)[5] = sm_Players[i].score;
		}
		for (i = 0; i < SM_NUM_PLAYERS; i++)
			bkPrintf("Player %ld gets %ld points and has %ld lives left\n", i, sm_PlayerRecord(i)[5],
				sm_Players[i].lives);
	} else {
		for (i = 0; i < SM_NUM_PLAYERS; i++)
			sm_PlayerRecord(i)[5] = sm_Players[i].score;
	}
	best = 0;
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		if (best < sm_PlayerRecord(i)[5])
			best = sm_PlayerRecord(i)[5];
	}
	sm_Leaders = sm_PlayerRecord(0)[5] == best;
	for (i = 1; i < SM_NUM_PLAYERS; i++) {
		if (sm_PlayerRecord(i)[5] == best)
			sm_Leaders = sm_Leaders + 1;
	}
}

/* 0x001bc420  ScratchMatch/CScratchmatch_Game.cpp: puts `buttons` into the sequence of the
other players, 8 lines after the current icon of `player` (the "pattern" a player sends) */
void SMGame_SetOthersNextEntry(unsigned int player, unsigned int buttons)
{
	unsigned int position, i;

	position = SMSprites_GetPlayerIconSequence((int)player);
	if (position + 8 > 0xfffffeu)
		SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x494,
			"ASSERTION FAILURE: un32SequencePosition < 0x00FFFFFF");
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		if (i != player)
			sm_Sequence[i + (position + 8) * 4] = buttons;
	}
}

/* 0x001bc480  ScratchMatch/CScratchmatch_Game.cpp: the next game state: intro -> practice
(the bars), practice -> game, game -> outro (results, winners dance, the winner text) */
void SMGame_NextState(void)
{
	int scores[4];
	unsigned int i;

	if (sm_State == 0) {
		SMCamera_DestroyIntro();
		sm_State = 1;
		for (i = 0; i < SM_NUM_PLAYERS; i++) {
			sm_Players[i].drop = 0.0f;
			sm_Players[i].spin = 0.0f;
		}
		SMLights_CheckSpotLights();
		CLHud_StartBars((float *)DAT_002ecfac, 280.0f);
		SMSound_ResumeMusicIfPaused();
		SMTimer_Reset(SMT_STATE);
		bkPrintf("MH: After Intro\n");
		return;
	}
	if (sm_State == 1) {
		sm_State = 2;
		SMPlayer_DecideAIHits();
		sm_Wander = 1;
		SMSprites_UpdatePlayerIcons();
		return;
	}
	if (sm_State == 2) {
		sm_State = 3;
		SMLights_DisableSpotLights();
		SMGame_RankPlayers();
		SMGame_StoreResults();
		for (i = 0; i < sm_Leaders; i++)
			SMPlayer_PlayWinAnimation(sm_Rank[i]);
		for (i = 0; i < SM_NUM_PLAYERS; i++)
			scores[i] = sm_GameVariation == 3 ? sm_PlayerRecord(i)[5] : sm_Players[i].score;
		CLHud_CreateWinnerText(SM_HUD, scores, 0);
		for (i = 0; i < SM_NUM_PLAYERS; i++)
			sm_Players[i].dead = 0;
		SMTimer_Reset(SMT_GAME);
		sm_CamCloseUp = 0;
		sm_Wander = 0;
		for (i = 0; i < SM_NUM_PLAYERS; i++) {
			sm_Players[i].spin = 0.0f;
			sm_Players[i].drop = 0.0f;
		}
		return;
	}
	SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x166, "ASSERTION FAILURE: FALSE");
}

/* 0x001bc640  ScratchMatch/CScratchmatch_Game.cpp (bracketed): state 0, the intro: the players
drop onto the stage spinning, the intro camera flies; then the practice starts */
void SMGame_UpdateIntro(void)
{
	struct SMPlayer *player;
	double value;
	int i;

	SMPlayer_UpdateHits();
	SMPlayer_UpdateIntroDrop();
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		value = player->drop - SMTimer_FrameTime() * 80.0;
		player->drop = (float)value;
		if (value < 0.0)
			player->drop = 0.0f;
		value = SMTimer_FrameTime() * 2.5132742302607625 + player->spin;
		player->spin = (float)value;
		if (value > 12.566371f)
			player->spin = 12.566371f;
	}
	if (SMCamera_UpdateIntro() == 1)
		SMGame_NextState();
}

/* 0x001bc6d0  ScratchMatch/CScratchmatch_Game.cpp (bracketed): state 1, the practice: the beat
starts after sm_PracticeStart seconds and the icons scroll, until the HUD countdown ends */
void SMGame_UpdatePractice(void)
{
	SMCamera_Wander();
	if (SMTimer_Elapsed(SMT_STATE) > sm_PracticeStart && sm_BeatStarted == 0) {
		SMTimer_Reset(SMT_BEAT);
		sm_BeatTime = 0.0f;
		sm_BeatStarted = 1;
	} else if (sm_BeatStarted != 1) {
		goto countdown;
	}
	SMSprites_Update();
	SMSprites_UpdatePlayerIcons();
	if (SMTimer_Elapsed(SMT_BEAT) > (double)sm_BeatPeriod * 0.5f + sm_BeatTime) {
		SMPlayer_UpdateSequence();
		SMScenic_Play(sm_Speakers);
		sm_BeatTime = sm_BeatPeriod + sm_BeatTime;
	}
countdown:
	if (CLHud_UpdateCountdown(SM_HUD) == 1)
		SMGame_NextState();
}

/* 0x001bc790  ScratchMatch/CScratchmatch_Game.cpp (bracketed): state 2, the game: hits, icons
and a beat every sm_BeatPeriod seconds; players who lost their last life spin and sink; the
survival game speeds up by level and ends with one player left; else the game ends after
sm_GameLength seconds */
void SMGame_UpdatePlay(void)
{
	struct SMPlayer *player;
	double value;
	int i;

	SMCamera_Wander();
	if (sm_BeatStarted == 0) {
		if (SMTimer_Elapsed(SMT_STATE) <= sm_PracticeStart)
			goto check;
		SMTimer_Reset(SMT_BEAT);
		sm_BeatTime = 0.0f;
		sm_BeatStarted = 1;
	} else {
check:
		if (sm_BeatStarted != 1)
			goto players;
	}
	SMPlayer_UpdateHits();
	SMSprites_Update();
	SMSprites_UpdatePlayerIcons();
	if (SMTimer_Elapsed(SMT_BEAT) > (double)sm_BeatPeriod * 0.5f + sm_BeatTime) {
		SMPlayer_DecideAIHits();
		SMPlayer_UpdateSequence();
		SMScenic_Play(sm_Speakers);
		if (sm_GameVariation == 3 && sm_AliveCount < 2)
			sm_LastSurvivor = 1;
		sm_BeatTime = sm_BeatPeriod + sm_BeatTime;
	}
players:
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		if (player->dead == 1 && player->visible == 1) {
			value = player->spin - SMTimer_FrameTime() * 2.5132742302607625;
			player->spin = (float)value;
			if (value < 0.0)
				player->spin = 6.28318548f;
			value = SMTimer_FrameTime() * 80.0 + player->drop;
			player->drop = (float)value;
			if (value > 400.0f)
				player->visible = 0;
		}
	}
	if (sm_GameVariation == 3 && sm_Levels[sm_Level * 3] < sm_IconLead + SMTimer_Elapsed(SMT_STATE)) {
		sm_BeatPeriod = sm_Levels[sm_Level * 3 + 1];
		if (sm_Level < 3)
			sm_Level = sm_Level + 1;
	}
	if (SMTimer_Elapsed(SMT_BEAT) + sm_SequenceEndMargin <= sm_GameLength && sm_LastSurvivor != 1)
		return;
	SMGame_NextState();
}

/* 0x001bc960  ScratchMatch/CScratchmatch_Game.cpp (bracketed): the four numbers of a sequence
line. After each number the rest of the line moves to the front; the CR test of that move
reads the caller's line, not the copy (as in the original). */
void SMGame_ParseLine(int *out, const char *line)
{
	char word[64];
	char copy[256];
	unsigned int length, i, k;
	int n;
	char c;

	lstrcpynA(copy, line, 0x100);
	for (n = 0; n < 4; n++) {
		SMGame_TrimLine(copy);
		length = (unsigned int)lstrlenA(copy);
		for (i = 0; c = copy[i], c != ' ' && c != '\t' && c != '\r' && c != '\n' && i != length; i++)
			word[i] = c;
		word[i] = '\0';
		if (i <= length) {
			for (k = i; k <= length; k++) {
				if (line[k] == '\r')
					break;
				copy[k - i] = copy[k];
			}
		}
		out[n] = (int)atol(word);
	}
}

/* 0x001bca30  ScratchMatch/CScratchmatch_Game.cpp: loads the button sequence of the handicap
(data\sequence<handicap + 1>_1.txt): the first line holds the line count (below 50000), then
one line of four numbers per beat; empty lines are skipped. The loaded file is not freed. */
void SMGame_LoadSequence(void)
{
	char line[256];
	char name[64];
	int values[4];
	const char *data;
	unsigned int lines, count, i, k;
	unsigned int *entry;

	bkPrintf("Loading game sequence data...\n");
	wsprintfA(name, "data\\sequence%d_%d.txt", DAT_002bae70 + 1, 1);
	data = (const char *)bkLoadFile(sm_VarPackage, name, NULL, NULL, NULL, 0);
	if (data == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x35a, "POINTER ASSERTION FAILURE: pbData");
	i = 0;
	do {
		k = i;
		if (data[k] == '\r') {
			line[k] = '\0';
			break;
		}
		line[k] = data[k];
		i = k + 1;
	} while (k + 1 < 0x100);
	lines = (unsigned int)SMGame_ParseFirstNumber(line);
	if (lines > 49999)
		SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x371, "ASSERTION FAILURE: un32NumLines < 50000");
	data = data + k + 2;
	sm_Sequence = (unsigned int *)bkHeapAllocEx(lines << 4, s_GameFile, 0x377, 0x2001, bkGetHeapContext(), 0x80);
	if (sm_Sequence == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x37b, "POINTER ASSERTION FAILURE: mpun32Sequence");
	memset(sm_Sequence, 0, (size_t)(lines & 0xfffffffu) * 16);
	entry = sm_Sequence;
	count = 0;
	if (lines != 0) {
		do {
			i = 0;
			do {
				k = i;
				if (data[k] == '\r') {
					line[k] = '\0';
					break;
				}
				line[k] = data[k];
				i = k + 1;
			} while (k + 1 < 0x100);
			if (lstrlenA(line) != 0) {
				SMGame_ParseLine(values, line);
				entry[0] = (unsigned int)values[0];
				entry[1] = (unsigned int)values[1];
				entry[2] = (unsigned int)values[2];
				count++;
				entry[3] = (unsigned int)values[3];
				entry += 4;
			}
			data = data + k + 2;
		} while (count < lines);
	}
	sm_SequenceLines = lines;
	bkPrintf("Loaded game sequence data\n");
}

/* 0x001bcc30  ScratchMatch/CScratchmatch_Game.cpp: the game variables: timers, the arena,
the HUD sprites, lives, the beat, the game length of the variation, the screen size, and the
button sequence */
void SMGame_Init(void)
{
	memset(SM_A(0x00310008), 0, 0x1a * 4);
	sm_GameVariation = sm_Variation;
	sm_State = 0;
	sm_Paused = 0;
	sm_OutroHold = 0;
	SM_I(0x00310054) = 0;
	sm_Finished = 0;
	SMTimer_InitAll();
	SMRender_Init();
	SMSprites_Init();
	SMTimer_Reset(SMT_GAME);
	SMTimer_Reset(SMT_DANCE);
	sm_StartLives = DAT_002bae70 * -2 + 8;
	sm_AliveCount = 4;
	sm_HistoryLength = 4;
	SM_F(0x0030ffc0) = (float)-SM_I(0x0030ffb0) * 0.5f;
	SM_F(0x00310010) = 0.0f;
	sm_BeatStarted = 0;
	sm_Level = 0;
	SM_F(0x0030ffc8) = (float)SM_I(0x0030ffb0) * 0.25f;
	sm_Started = 0;
	sm_LastSurvivor = 0;
	SM_F(0x0030ffcc) = 4.0f;
	SM_F(0x0030ffbc) = 4.0f;
	sm_BeatPeriod = 0.5f;
	SM_F(0x0030ffd0) = 4.0f;
	SM_F(0x0030ffd4) = 0.0f;
	SM_F(0x0030ffd8) = 5.0f;
	SM_F(0x0030ffc4) = 100.0f;
	SM_F(0x0030ffdc) = 16380.0f;
	SM_F(0x0030ffe4) = 4.0f;
	SM_F(0x0030ffe8) = 3.5f;          /* seconds between dance changes */
	sm_LeaderPeriod = 4.0f;
	sm_HitRange = 0.25f;
	sm_SequenceEndMargin = 4.0f;
	sm_PracticeStart = 6.1f;
	SM_U(0x0030ffb8) = 0x3feef9dbu;   /* sm_IconLead, 1.867 s */
	if (sm_GameVariation == 1 || sm_GameVariation == 2)
		sm_GameLength = 72.0f;
	else if (sm_GameVariation == 3)
		sm_GameLength = 63.0f;
	else
		SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0xdc, "ASSERTION FAILURE: FALSE");
	bdGetScreenInfo(&sm_HalfWidth, &sm_HalfHeight, NULL, NULL);
	sm_HalfWidth = sm_HalfWidth / 2;
	sm_HalfHeight = sm_HalfHeight / 2;
	SM_U(0x003100a8) = sm_rand() & 0xf;
	sm_ShowFramerate = 0;
	CScoreTable_Rank((struct CScoreTable *)DAT_002b9968);
	SM_F(0x00310010) = 0.0f;
	sm_IntroSkipped = 0;
	SM_I(0x00310040) = DAT_002b9c58;
	SMGame_LoadSequence();
}

/* 0x001bce70  ScratchMatch/CScratchmatch_Game.cpp: restarts the game (a debug path: it
asserts first) */
void SMGame_Restart(void)
{
	int scores[4];

	bkPrintf("Don't do this\n");
	SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x180, "ASSERTION FAILURE: FALSE");
	SMCamera_Init();
	SMPlayer_InitAll();
	SMGame_Init();
	scores[0] = 0;
	scores[1] = 0;
	scores[2] = 0;
	sm_State = 0;
	scores[3] = 0;
	CLHud_SetScores(SM_HUD, scores);
	CLHud_ResetCountdown(SM_HUD);
	if (sm_WinnerText == 1) {
		CLHud_DeleteWinnerText(SM_HUD);
		sm_WinnerText = 0;
	}
}

/* 0x001bcf00  ScratchMatch/CScratchmatch_Game.cpp: state 3, the outro: the camera visits the
winners; when they have all been shown and the commentary is quiet (or a winner presses a
button) the screen fades out, then the game mode is popped */
void SMGame_UpdateOutro(void)
{
	double shown;

	SMCamera_FollowLeader();
	SMPlayer_UpdateHits();
	if (sm_FadeState == 2) {
		SMSound_StopSFX(0);
		SMSound_StopMusic();
		SMCamera_RestartIntro();
		CLHud_DeleteWinnerText(SM_HUD);
		if (DAT_002bae98 == 2) {
			SM_ASSERT_FAIL("CScratchmatch_Game.cpp", 0x32f, "ASSERTION FAILURE: FALSE");
			/* port: the original calls SMGame_Restart here (start state 2: a game started
			from the command line, OPENFRENZY_GAME, loops its round). That debug path is
			broken in the original too ("Don't do this"): SMPlayer_InitAll and SMRender_Init
			load the actors and sets a second time from the packages in memory, so
			bFixupNodeDefs adds the base to pointers that are already fixed up, and
			bkAddResource links the resources twice (fault in bFixupNodeDefs <- baLoadActor).
			The port ends the mode instead, as after a board round. */
			sm_Finished = 1;
			return;
		}
		sm_Finished = 1;
		return;
	}
	shown = (double)sm_ufloat(sm_Leaders) * sm_LeaderPeriod;
	if ((shown > SMTimer_Elapsed(SMT_GAME) + 1.0 || CLSamples_CommentaryIdle() != 1) && sm_OutroHold != 1)
		return;
	if (sm_FadeState != 1) {
		sm_FadeState = 1;
		SMTimer_Reset(SMT_FADE);
	}
}
