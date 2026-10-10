/*
CORE_TEST.C

openfrenzy core module tests (src/core/tests/run_tests.sh builds this as the main() of a
copy of the game image). They check what other modules and the game rely on:

- the core data blocks keep the Xbox layout, and other modules' Ghidra names alias into
  them (so DAT_002ba7a0 written by core_modes_* is the mode stack count core reads);
- .data starts with the XBE values (cheat table, preload list, rank points);
- the tweak constructor 0x001f2da0 sets the values the minigames read;
- the score screen gives rank points and shares them on ties, and team points;
- Schlick's gain curve, the board stage order rules, the tweak file parser.

Only pure core code is called: nothing here needs bKernel, the display or a disc.
The game build compiles every .c under src/core: without OPENFRENZY_CORE_TEST (set by
run_tests.sh) this file is empty.
*/

#ifdef OPENFRENZY_CORE_TEST

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core_globals.h"
#include "core_modes.h"
#include "core_tweak.h"

static int failures;
static int checks;

#define CHECK(cond, what) \
	do { \
		checks++; \
		if (!(cond)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, what); \
		} \
	} while (0)

/* Ghidra names of other modules, resolved by crt/core_data.c */
extern int DAT_002ba7a0;           /* the mode stack count */
extern int DAT_002baee0;           /* player 0 port; player k at +k * 0x28 */
extern float DAT_002ad19c;         /* a tweak variable */
extern char s_BGeffects_00283b18[];

static void test_layout(void)
{
	CHECK((uintptr_t)&DAT_002ba7a0 == (uintptr_t)&g_ModeStack.count,
		"DAT_002ba7a0 is CGameModeStack.count at 0x002ba750 + 0x50");
	CHECK((uintptr_t)&DAT_002baee0 + 2 * 0x28 == (uintptr_t)&g_PlayerPort(2),
		"players keep the 0x28 stride across the alias");
	CHECK((uintptr_t)s_BGeffects_00283b18 == (uintptr_t)g_RootModeName, "the root mode name buffer");
	CHECK(offsetof(struct CGameModeStack, first_drawn) == 0x54, "CGameModeStack.first_drawn offset");
	CHECK(sizeof(struct CoreCheat) == 0x4c, "cheat entry size");
	CHECK(sizeof(struct CMenuMode) == 0x98, "CMenuMode size");
}

static void test_initial_data(void)
{
	int i;

	CHECK(strcmp(g_RootModeName, "BGeffects") == 0, "root mode starts as BGeffects");
	CHECK(strcmp(g_Cheats[0].name, "Welsh Mode") == 0, "cheat 0 name");
	CHECK(strcmp(g_Cheats[CORE_CHEAT_DEBUG].name, "DEBUG") == 0, "cheat 6 is DEBUG");
	CHECK(strcmp(g_Cheats[3].name, "Turbo Mode") == 0 && g_Cheats[3].states == 2 && &g_Cheats[3].state == &g_TurboCheat,
		"Turbo Mode (cheat 3) is a toggle whose state is 0x00283e58");
	CHECK(g_Cheats[0].sequence[0] == 0xc8, "cheat sequences use button codes 0xc5-0xc8");
	CHECK(strcmp(g_PreloadPackages[0], "packages\\map.xbp") == 0, "first preloaded package");
	for (i = 0; strlen(g_PreloadPackages[i]) != 0; i++)
		;
	CHECK(i == 0x51, "81 packages before the empty string");
	CHECK(((int *)CORE_ADDR(0x00283b74))[0] == 6 && ((int *)CORE_ADDR(0x00283b74))[3] == 0,
		"rank points 6, 4, 2, 0");
	CHECK(g_FrameScale == 1.0f, "frame scale 1.0");
	CHECK(strcmp(g_LanguageFolders[0], "packages\\english") == 0
		&& strcmp(g_LanguageFolders[11], "packages\\japanese") == 0, "language folders");
}

static void test_tweak_constructor(void)
{
	FUN_001f2da0();
	CHECK(DAT_002ad19c == (float)(4.0 * (double)0.2f), "0x002ad19c = 0x00292c84 * 0.2");
	CHECK(CORE_VAR(int, 0x002ad138) == 90, "0x002ad138 = 90");
	CHECK(CORE_VAR(float, 0x002ad168) == 0.1f, "0x002ad168 = 0.1");
	CHECK(CORE_VAR(float, 0x002ad6c4) == (float)(4.0 * (double)0.05f), "0x002ad6c4 = 0x00292c84 * 0.05");
	CHECK(*(uint32_t *)CORE_ADDR(0x002ad120) == 0x40800000, "0x002ad120 copies 0x00292c84 (4.0)");
}

static void set_scores(int a, int b, int c, int d)
{
	g_PlayerScore(0) = a;
	g_PlayerScore(1) = b;
	g_PlayerScore(2) = c;
	g_PlayerScore(3) = d;
}

static void test_rank_points(void)
{
	int points[4], winner;

	g_PlayerCount = 4;
	g_TeamMode = 0;
	set_scores(5, 3, 3, 1);
	CGameMode_Scores_ComputePoints(&winner, points, 0);
	CHECK(winner == 0, "highest score wins");
	CHECK(points[0] == 6 && points[1] == 3 && points[2] == 3 && points[3] == 0,
		"2nd and 3rd tie: they share (4 + 2) / 2");
	set_scores(2, 7, 7, 0);
	CGameMode_Scores_ComputePoints(&winner, points, 0);
	CHECK(winner == -1, "a tie for first: no winner");
	CHECK(points[1] == 5 && points[2] == 5 && points[0] == 2 && points[3] == 0, "tied winners share 6 + 4");
	set_scores(1, 9, 4, 9);
	CGameMode_Scores_ComputePoints(&winner, points, 1);
	CHECK(points[1] == 1 && points[3] == 1 && points[0] == 0 && points[2] == 0,
		"final round: one point to every best score");
	/* teams: players 0+2 against 1+3 */
	g_TeamMode = 1;
	g_TeamPlayer(0, 0) = 0;
	g_TeamPlayer(0, 1) = 2;
	g_TeamPlayer(1, 0) = 1;
	g_TeamPlayer(1, 1) = 3;
	set_scores(1, 5, 1, 0);
	CGameMode_Scores_ComputePoints(&winner, points, 0);
	CHECK(winner == 1, "team 1 (5) beats team 0 (2)");
	CHECK(points[1] == 6 && points[3] == 6 && points[0] == 0 && points[2] == 0, "winning team gets 1st place points");
	set_scores(2, 1, 0, 1);
	CGameMode_Scores_ComputePoints(&winner, points, 0);
	CHECK(winner == -1 && points[0] == 3 && points[1] == 3, "a team draw gives everyone 3");
	g_TeamMode = 0;
}

static void test_gain(void)
{
	CHECK(fabsf(Core_Gain(0.5f, 0.25f) - 0.25f) < 1e-6f, "gain 0.5 is the identity");
	CHECK(fabsf(Core_Gain(0.8f, 0.5f) - 0.5f) < 1e-6f, "gain passes through the middle");
	CHECK(Core_Gain(0.8f, 0.25f) < 0.25f && Core_Gain(0.8f, 0.75f) > 0.75f, "gain > 0.5 steepens the middle");
}

static void test_stage_order(void)
{
	const uint8_t *follows = (const uint8_t *)CORE_ADDR(0x002844c4);
	int *stages = (int *)CORE_ADDR(0x002844ac);
	int run, i;

	CORE_VAR(int, 0x002ba638) = 0;   /* no forced route */
	for (run = 0; run < 20; run++)
	{
		unsigned int seen = 0;

		Core_ChooseStages(6);
		for (i = 0; i < 6; i++)
		{
			CHECK(stages[i] >= 0 && stages[i] < 6, "a stage is 0-5");
			CHECK((seen & (1u << stages[i])) == 0, "no stage twice");
			seen |= 1u << stages[i];
			if (i > 0)
				CHECK((follows[stages[i - 1]] >> stages[i]) & 1, "each stage may follow the one before");
		}
	}
	CORE_VAR(int, 0x002ba638) = 1;   /* forced: the Extras "Stage n" values */
	((int *)CORE_ADDR(0x00283b00))[0] = 4;
	((int *)CORE_ADDR(0x00283b00))[1] = 2;
	Core_ChooseStages(2);
	CHECK(stages[0] == 4 && stages[1] == 2, "a forced route is copied");
	CORE_VAR(int, 0x002ba638) = 0;
}

static void test_tweak_file(void)
{
	static char file[] = "[Real_FPS_Extras] 1\n  [Music_Vol_Extras]\t77\r\n[Fog_Extras] 2.5\n";
	struct CTweakMenu menu;
	int fps = 0, volume = 0, missing = 5;
	float fog = 0.0f;
	int size = (int)strlen(file);

	memset(&menu, 0, sizeof(menu));
	strcpy(menu.name, "Extras");
	CORE_VAR(char *, 0x002bae4c) = file;
	CORE_VAR(int, 0x00284498) = 1;
	CTweakMenu_LoadValue(&menu, "Real FPS", &size, NULL, NULL, &fps);
	CTweakMenu_LoadValue(&menu, "Music Vol", &size, NULL, &volume, NULL);
	CTweakMenu_LoadValue(&menu, "Fog", &size, &fog, NULL, NULL);
	CTweakMenu_LoadValue(&menu, "Sound Vol", &size, NULL, &missing, NULL);
	CHECK(fps == 1, "bool item read; spaces in names become _");
	CHECK(volume == 77, "int item read after tabs and CR");
	CHECK(fog == 2.5f, "float item read");
	CHECK(missing == 5, "a missing item keeps its value");
	CHECK(CORE_VAR(char *, 0x002bae4c) == file, "the cursor is restored");
	CORE_VAR(char *, 0x002bae4c) = NULL;
}

int main(void)
{
	test_layout();
	test_initial_data();
	test_tweak_constructor();
	test_rank_points();
	test_gain();
	test_stage_order();
	test_tweak_file();
	fprintf(stderr, "core tests: %d checks, %d failed\n", checks, failures);
	return failures != 0;
}

#endif
