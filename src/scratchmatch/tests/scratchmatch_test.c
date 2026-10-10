/*
SCRATCHMATCH_TEST.C

scratchmatch module tests (src/scratchmatch/tests/run_tests.sh builds this as the main() of a
small guest image: the scratchmatch objects, the generated stubs and the platform layer). They
check what the game relies on and what a wrong decompilation would silently break:

- the name tables of the data block sit where the code indexes them by variation (a wrong
  base would load the wrong package, music or camera path);
- the sequence parser reads the file format of data\sequence<n>_1.txt (count line, four
  numbers per line, CR LF line ends, tabs);
- the ranking (scores; lives first in the survival game) and the count of shared winners,
  which decide the winner texts and the outro camera;
- the timing score (10 at the beat, 0 at the edge), the button history match, the icon list
  slots, the life lost on a missed icon, the bonus pattern sent to the other players, and the
  colour cycle of the spot lights.

The game build compiles every .c under src/scratchmatch: without OPENFRENZY_SCRATCHMATCH_TEST
(set by run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_SCRATCHMATCH_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scratchmatch.h"

/* ---------- the other modules' data that this image does not link */

float DAT_002855f0[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
float DAT_00285634[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
unsigned int DAT_00285630 = 1;
unsigned char DAT_00290718[0x60 * 8];
int DAT_00290960;
unsigned char DAT_002b9968[0x400];
int DAT_002b9c58;
unsigned char DAT_002ba750[0x58];
int DAT_002bae6c;
int DAT_002bae70;
int DAT_002bae98;
int DAT_002baed4[40];
void *DAT_002db808;
void *DAT_002ecc74;
unsigned char DAT_002ecd90[0x100];
int DAT_002ecdec;
void *DAT_002ecfac;
unsigned int DAT_002bc490[256];
unsigned int DAT_002bc898[256];
unsigned int DAT_002bce18[256];
unsigned int DAT_002bd640[256];

static int checks;
static int failures;

#define CHECK(cond, what) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, what); \
		} \
	} while (0)

/* moves timer `timer` back by `seconds` (as if it had been reset that long ago) */
static void age_timer(int timer, double seconds)
{
	sm_TimerStart(timer)->QuadPart -= (LONGLONG)(seconds * (double)sm_ClockFrequency->QuadPart);
}

/* a fake input channel: biChannel +0x14 is the value the game reads */
static unsigned char channels[4][8][0x40];

static void bind_fake_channels(void)
{
	int i, b;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		for (b = 0; b < 4; b++) {
			sm_Players[i].buttons[b] = (struct biChannel *)channels[i][b];
			sm_Players[i].dpad[b] = (struct biChannel *)channels[i][4 + b];
		}
	}
}

static void press(int player, int button, int down)
{
	SM_AT(channels[player][button], 0x14, int) = down;
}

static void test_tables(void)
{
	CHECK(strcmp(sm_VariationPackages[1], "packages\\scratch1") == 0, "variation 1 loads packages\\scratch1");
	CHECK(strcmp(sm_VariationPackages[3], "packages\\scratch3") == 0, "variation 3 loads packages\\scratch3");
	CHECK(strcmp(sm_MusicNames[1], "wmas\\scratch_match_v1.wma") == 0, "variation 1 plays v1");
	CHECK(strcmp(sm_MusicNames[2], "wmas\\scratch_match_v3.wma") == 0, "variation 2 plays the v3 music (as shipped)");
	CHECK(strcmp(sm_CameraFiles[1], "cams\\cam1.lom") == 0 && strcmp(sm_CameraFiles[3], "cams\\cam3.lom") == 0,
		"the intro camera path of each variation");
	CHECK(sm_Leader == 0xffff, "the camera's leader starts as none (shares the word before the camera table)");
	CHECK(strcmp(sm_SampleNames[0], "samples\\sfx\\crowdambient.wav") == 0, "sample 0 is the crowd");
	CHECK(strcmp(sm_SampleNames[9], "samples\\sfx\\click.wav") == 0, "sample 9 is the click");
	CHECK(strcmp(sm_AnimNames[0], "scratch_1") == 0 && strcmp(sm_AnimNames[5], "good_scratch") == 0,
		"animation 5 is the winner's");
	CHECK(strcmp(sm_SpriteNames[SMSPR_BUTTONFLASH], "sprites\\new\\buttonflash.bmp") == 0,
		"the last of the 27 HUD sprites");
	CHECK(strcmp(sm_SpriteNames[SMSPR_LIFEOFF], "sprites\\new\\lifeoff.bmp") == 0, "the HUD sprite enum");
	CHECK(strcmp(sm_CharacterModels, "pod_grn.obe") == 0 && strcmp(sm_CharacterModels + 0x80, "scratch_dub.obe") == 0 &&
		strcmp(sm_CharacterModels + 0x100, "dubopt.obe") == 0, "character 0's three models");
	CHECK(sm_PlayerSpots[0] == -670.0f && sm_PlayerSpots[1] == 120.0f && sm_PlayerSpots[7] == 120.0f,
		"the players' spots on the stage");
	CHECK(sm_Levels[0] == 19.7f && sm_Levels[1] == 0.48f && sm_Levels[9] == 360.0f,
		"the survival speed levels (start time, beat period)");
	CHECK(strcmp(SM_A(0x002b5ce8), "models\\hovver_light") == 0 && strcmp(SM_A(0x002b5d2c), "spin") == 0,
		"the hovering light's model and animation");
	CHECK(strcmp((const char *)SM_P(0x002b5e34), "actors\\lightpod.obe") == 0, "the light pod model");
	CHECK(SM_P(0x002b5e88) == (void *)SM_A(0x002b5e40), "the smoke sprite's name pointer points into the block");
	CHECK(sm_DrawActors == 1 && sm_DrawExtras == 1, "the draw tweaks start on");
}

static void test_sequence_parsing(void)
{
	char line[256];
	int values[4];

	CHECK(SMGame_ParseFirstNumber("  212\r") == 212, "the line count line");
	CHECK(SMGame_ParseFirstNumber("\t7 9\r") == 7, "the first number of a line");
	strcpy(line, "   \r");
	SMGame_TrimLine(line);
	CHECK(line[0] == '\0', "a blank line becomes empty");
	strcpy(line, "\t 1 2\r");
	SMGame_TrimLine(line);
	CHECK(strcmp(line, "1 2") == 0, "leading blanks and the CR go");
	memset(values, 0xff, sizeof values);
	SMGame_ParseLine(values, "1 2\t4 256");
	CHECK(values[0] == 1 && values[1] == 2 && values[2] == 4 && values[3] == 256,
		"a sequence line: one button mask per player (256 is the bonus)");
	SMGame_ParseLine(values, "8 0 0 0");
	CHECK(values[0] == 8 && values[1] == 0 && values[3] == 0, "0 is no icon for that player");
}

static void set_player(int i, int score, int lives)
{
	sm_Players[i].score = score;
	sm_Players[i].lives = lives;
}

static void test_ranking(void)
{
	sm_GameVariation = 1;
	set_player(0, 10, 0);
	set_player(1, 40, 0);
	set_player(2, 20, 0);
	set_player(3, 40, 0);
	SMGame_RankPlayers();
	CHECK(sm_Players[sm_Rank[0]].score == 40 && sm_Players[sm_Rank[3]].score == 10, "ranked by score");
	CHECK(sm_Leaders == 2, "two players share first place");
	sm_GameVariation = 3;
	set_player(0, 99, 1);
	set_player(1, 5, 3);
	set_player(2, 7, 3);
	set_player(3, 0, 0);
	SMGame_RankPlayers();
	CHECK(sm_Rank[0] == 2 && sm_Rank[1] == 1 && sm_Rank[2] == 0 && sm_Rank[3] == 3,
		"survival: lives first, then score");
	CHECK(sm_Leaders == 1, "the winners are counted by score in every variation");
	sm_GameVariation = 1;
}

static void test_timing(void)
{
	sm_HitRange = 0.25f;
	CHECK(SMPlayer_TimingScore(0.0f) == 10, "on the beat scores 10");
	CHECK(SMPlayer_TimingScore(0.125f) == 5 && SMPlayer_TimingScore(-0.125f) == 5,
		"half the range scores 5, early or late");
	CHECK(SMPlayer_TimingScore(0.25f) == 0, "the edge of the range scores 0");
}

static void test_buttons(void)
{
	bind_fake_channels();
	sm_HistoryLength = 4;
	memset(sm_Players[0].history, 0, sizeof sm_Players[0].history);
	press(0, 0, 1);
	SMPlayer_ReadButtons(0);
	press(0, 0, 0);
	press(0, 2, 1);
	SMPlayer_ReadButtons(0);
	press(0, 2, 0);
	CHECK(sm_Players[0].history[0] == 4 && sm_Players[0].history[1] == 1, "the history keeps the newest first");
	CHECK(SMPlayer_ButtonsMatch(0, 5) == 1, "buttons pressed within the history count together");
	CHECK(SMPlayer_ButtonsMatch(0, 1) == 0, "an extra button is a miss");
	press(0, 4 + 2, 1);
	SMPlayer_ReadButtons(0);
	press(0, 4 + 2, 0);
	CHECK(sm_Players[0].history[0] == 0x40, "d-pad down is bit 6");
	CHECK(SMPlayer_AnyButtonPressed() == 0, "nothing held");
	press(3, 1, 1);
	CHECK(SMPlayer_AnyButtonPressed() == 1, "any player's button");
	press(3, 1, 0);
}

static void test_icons(void)
{
	float now;
	int i;

	SMTimer_InitAll();
	age_timer(SMT_BEAT, 10.0);
	sm_GameVariation = 3;
	sm_LaneSpacing = 140.0f;
	sm_IconStartY = -140;
	sm_HitLineY = 105.0f;
	sm_BeatPeriod = 0.5f;
	for (i = 0; i < SM_NUM_ICONS; i++) {
		sm_Icons[i].buttons = 0;
		sm_Icons[i].player = -1;
	}
	sm_FirstFreeIcon = 0;
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		sm_Players[i].icon = SM_NO_ICON;
		sm_Players[i].prevIcon = SM_NO_ICON;
		sm_Players[i].hit = 0;
		sm_Players[i].lives = 3;
		sm_Players[i].seqPos = 5;
	}
	now = (float)SMTimer_Elapsed(SMT_BEAT);
	SMSprites_AddIcon(1, 2, now + 0.1f);
	SMSprites_AddIcon(2, 8, now + 5.0f);
	CHECK(sm_FirstFreeIcon == 2 && sm_Icons[0].player == 1 && sm_Icons[0].seqPos == 5,
		"icons take the first free slots and remember the sequence line");
	CHECK(sm_Icons[0].x == 140.0f - 210.0f, "an icon is in its player's lane");
	SMSprites_RemoveIcon(0);
	CHECK(sm_FirstFreeIcon == 0, "a freed slot is reused first");
	SMSprites_AddIcon(1, 2, now + 0.1f);
	SMSprites_UpdatePlayerIcons();
	CHECK(sm_Players[1].icon == 0 && sm_Players[2].icon == SM_NO_ICON,
		"the current icon is the one within half a beat of the hit line");
	CHECK(SMSprites_GetPlayerIconButtons(1) == 2 && SMSprites_GetPlayerIconSequence(1) == 5, "its buttons and line");
	sm_Icons[0].hitTime = now - 5.0f;
	sm_Icons[0].y = 200.0f;
	SMSprites_UpdatePlayerIcons();
	CHECK(sm_Players[1].icon == SM_NO_ICON && sm_Players[1].prevIcon == 0 && sm_Players[1].prevButtons == 2,
		"the icon passes: it becomes the previous one");
	CHECK(sm_Players[1].lives == 2, "an icon that was not hit costs a life");
	sm_Icons[1].hitTime = now;
	SMSprites_UpdatePlayerIcons();
	CHECK(sm_Players[2].icon == 1 && sm_Players[2].hit == 0, "a new current icon starts unhit");
	sm_Players[2].hit = 1;
	sm_Icons[1].hitTime = now - 5.0f;
	SMSprites_UpdatePlayerIcons();
	CHECK(sm_Players[2].lives == 3, "a hit icon costs nothing");
	sm_GameVariation = 1;
}

static void test_bonus_pattern(void)
{
	unsigned int sequence[4 * 32];
	unsigned int pattern, bits;
	int i;

	memset(sequence, 0, sizeof sequence);
	sm_Sequence = sequence;
	DAT_002bae70 = 0;
	for (i = 0; i < SM_NUM_ICONS; i++)
		sm_Icons[i].buttons = 0;
	sm_Icons[3].buttons = SM_BONUS;
	sm_Icons[3].seqPos = 4;
	sm_Players[0].icon = 3;
	sm_Players[0].hit = 0;
	sm_Players[0].aiWillHit = 1;
	sm_Players[0].aiTiming = -1.0f;
	SMPlayer_AIHit(0, SM_BONUS, 0.0f);
	pattern = sm_Icons[3].buttons;
	for (bits = 0, i = 0; i < 4; i++)
		bits += (pattern >> i) & 1;
	CHECK(sm_Players[0].hit == 1, "the bonus icon is taken");
	CHECK(pattern != 0 && pattern <= 0xf && bits >= 1, "the bonus icon becomes a pattern of A/B/X/Y");
	CHECK(sequence[(4 + 8) * 4 + 1] == pattern && sequence[(4 + 8) * 4 + 3] == pattern &&
		sequence[(4 + 8) * 4 + 0] == 0, "the pattern goes to the other players, 8 lines later");
	sm_Sequence = NULL;
}

static void test_light_colours(void)
{
	SMTimer_InitAll();
	sm_LightColours = 0x51;
	SMLights_CycleColours();
	CHECK(sm_LightColours == 0x51, "no change before half a second");
	age_timer(SMT_LIGHTS, 1.0);
	SMLights_CycleColours();
	CHECK(sm_LightColours == 0x144, "the colour bits rotate left by 2");
	sm_LightColours = 0xc00001;
	age_timer(SMT_LIGHTS, 1.0);
	SMLights_CycleColours();
	CHECK(sm_LightColours == 0x000007, "within 24 bits");
}

static void test_texts(void)
{
	int i;

	sm_GameVariation = 1;
	for (i = 0; i < SM_NUM_TEXTS; i++)
		sm_Texts[i].type = 0;
	sm_FirstFreeText = 0;
	sm_Players[2].brightColour = 0x123456;
	SMSprites_AddText(2, 6, 16);
	SMSprites_AddText(0, 1, 0);
	CHECK(sm_Texts[0].type == 6 && sm_Texts[0].value == 16 && sm_Texts[0].rgba[0] == 0x12 &&
		sm_Texts[0].rgba[2] == 0x56, "a combo text in the player's colour");
	CHECK(sm_Texts[1].duration == 0.5f && sm_FirstFreeText == 2, "a rating text lasts half a second");
	sm_FrameTime = 0.75;
	SMSprites_UpdateTexts();
	CHECK(sm_Texts[1].type == 0 && sm_Texts[0].type == 6 && sm_FirstFreeText == 1,
		"expired texts free their slot");
	sm_GameVariation = 3;
	SMSprites_AddText(0, 1, 0);
	CHECK(sm_Texts[1].type == 0, "no texts in the survival game");
	sm_GameVariation = 1;
}

int main(void)
{
	test_tables();
	test_sequence_parsing();
	test_ranking();
	test_timing();
	test_buttons();
	test_icons();
	test_bonus_pattern();
	test_light_colours();
	test_texts();
	fprintf(stderr, "scratchmatch tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
