/*
core_modes_b module tests: one guest image (src/core_modes_b/tests/run_tests.sh builds it from
the game build's objects of src/core_modes_b, the generated weak stubs, the guest runtime and
the platform layer). Every other module's function is a stub here.

The game image globs every .c under src/core_modes_b, so this file compiles to nothing unless
OPENFRENZY_CORE_MODES_B_TEST is defined.

What each test protects (why it matters to the game):
- party ranking (CScoreTable_Rank): the places shown after every round and at the end decide
  who wins the party. The bonus ranking outweighs the round ranking, which outweighs the
  minigame score; equal values share a place and a shared place is skipped by the next one.
- round bookkeeping (AddRound, EndRound): the rows of each round fill in kind order, the bonus
  row (kind 3) stays out of the totals until the round ends.
- player order (cmb_sort_players): highest value first; equal values keep player order (a
  stable order, because the screens list the players by it).
- the tournament carousel (CGameMode_MiniGameSelect constructor): it lists exactly the
  available game variants, sorted by their localised name, with each variant's text id.
*/

#ifdef OPENFRENZY_CORE_MODES_B_TEST

#include "core_modes_b.h"

static int failures;
static int checks;

#define CHECK(condition, ...) \
	do { \
		checks++; \
		if (!(condition)) { \
			failures++; \
			fprintf(stderr, "FAIL %s:%d: %s: ", __FILE__, __LINE__, #condition); \
			fprintf(stderr, __VA_ARGS__); \
			fprintf(stderr, "\n"); \
		} \
	} while (0)

static void reset_scores(void)
{
	memset(&g_score_table, 0, sizeof g_score_table);
	memset(DAT_002baed4, 0, sizeof DAT_002baed4);
	DAT_002bae94 = 4;
	DAT_002bae98 = 0; /* a party, not a tournament */
}

/* ---------- party ranking */

static void test_rank_weights(void)
{
	reset_scores();
	/* player 2 has the best bonus; player 0 the best round; player 1 the best minigame score */
	g_score_table.bonus[0] = 1;
	g_score_table.bonus[1] = 2;
	g_score_table.bonus[2] = 9;
	g_score_table.bonus[3] = 3;
	g_score_table.rounds[0].sum[0] = 50;
	g_score_table.rounds[0].sum[1] = 10;
	g_score_table.rounds[0].sum[2] = 20;
	g_score_table.rounds[0].sum[3] = 30;
	DAT_002baed4[0][5] = 1;
	DAT_002baed4[1][5] = 7;
	DAT_002baed4[2][5] = 2;
	DAT_002baed4[3][5] = 3;
	CScoreTable_Rank(&g_score_table);
	/* points: p0 100+40+1, p1 200+10+4, p2 400+20+2, p3 300+30+3 */
	CHECK(g_score_table.place[2] == 0, "best bonus wins (place %d)", g_score_table.place[2]);
	CHECK(g_score_table.place[3] == 1, "second bonus is second (place %d)", g_score_table.place[3]);
	CHECK(g_score_table.place[1] == 2, "round and score do not beat bonus (place %d)", g_score_table.place[1]);
	CHECK(g_score_table.place[0] == 3, "best round alone is last (place %d)", g_score_table.place[0]);
	CHECK(g_score_table.by_place[0] == 2 && g_score_table.by_place[3] == 0,
		"by_place lists the players in place order (%d %d %d %d)", g_score_table.by_place[0],
		g_score_table.by_place[1], g_score_table.by_place[2], g_score_table.by_place[3]);
	CHECK(g_score_table.ties[0] == 1 && g_score_table.ties[1] == 1 && g_score_table.ties[2] == 1 &&
		g_score_table.ties[3] == 1, "no ties");
}

static void test_rank_ties(void)
{
	reset_scores();
	/* everybody equal except player 3, who has the best bonus */
	g_score_table.bonus[3] = 5;
	CScoreTable_Rank(&g_score_table);
	CHECK(g_score_table.place[3] == 0, "player 3 first (place %d)", g_score_table.place[3]);
	CHECK(g_score_table.place[0] == 1 && g_score_table.place[1] == 1 && g_score_table.place[2] == 1,
		"three players share second place (%d %d %d)", g_score_table.place[0],
		g_score_table.place[1], g_score_table.place[2]);
	CHECK(g_score_table.ties[1] == 3, "three ties on second place (%d)", g_score_table.ties[1]);
	CHECK(g_score_table.ties[2] == 0 && g_score_table.ties[3] == 0, "the shared place is skipped");
	CHECK(g_score_table.by_place[0] == 3 && g_score_table.by_place[1] == 0 &&
		g_score_table.by_place[2] == 1 && g_score_table.by_place[3] == 2,
		"tied players fill the following slots in player order (%d %d %d %d)",
		g_score_table.by_place[0], g_score_table.by_place[1], g_score_table.by_place[2],
		g_score_table.by_place[3]);
}

/* ---------- round bookkeeping */

static void test_rounds(void)
{
	int scores[4] = { 3, 1, 4, 1 };
	int bonus[4] = { 10, 0, 0, 20 };

	reset_scores();
	CScoreTable_AddRound(&g_score_table, scores);
	CHECK(g_score_table.rounds[0].scores[0][2] == 4, "kind 0 row filled");
	CHECK(g_score_table.rounds[0].sum[2] == 4 && g_score_table.totals[2] == 4, "sum and total grow");
	CHECK(g_score_table.rounds[0].kind == 1, "the next game of the round is kind 1");
	g_score_table.rounds[0].kind = 3;
	CScoreTable_AddRound(&g_score_table, bonus);
	CHECK(g_score_table.rounds[0].scores[3][3] == 20, "bonus row filled");
	CHECK(g_score_table.totals[3] == 1, "the bonus row stays out of the totals (%d)", g_score_table.totals[3]);
	CScoreTable_EndRound(&g_score_table);
	CHECK(g_score_table.bonus[3] == 20 && g_score_table.bonus[0] == 10, "the round end adds the bonus");
	CHECK(g_score_table.round == 1, "the next round starts");
	CScoreTable_Clear(&g_score_table);
	CHECK(g_score_table.round == 0 && g_score_table.totals[2] == 0 && g_score_table.bonus[3] == 0,
		"clear resets the party");
}

/* ---------- player order */

static void test_sort_players(void)
{
	int values[4] = { 5, 9, 5, 1 };
	int place[4];
	int order[4];

	DAT_002bae94 = 4;
	cmb_sort_players(values, place, order);
	CHECK(order[0] == 1 && order[1] == 0 && order[2] == 2 && order[3] == 3,
		"highest first, equal values in player order (%d %d %d %d)", order[0], order[1], order[2],
		order[3]);
	CHECK(place[1] == 0 && place[0] == 1 && place[2] == 2 && place[3] == 3, "places match the order");
}

/* ---------- the tournament carousel */

static unsigned short text_zeta[] = { 'Z', 'e', 't', 'a', 0 };
static unsigned short text_alpha[] = { 'A', 'l', 'p', 'h', 'a', 0 };
static unsigned short text_mid[] = { 'M', 'i', 'd', 0 };
static unsigned short text_empty[] = { 0 };
static unsigned int fake_texts[0x200 * 4];
static unsigned char fake_language[0x24];

static void test_carousel(void)
{
	unsigned char mode[0x200];
	int i;

	for (i = 0; i < 0x200; i++)
		fake_texts[i * 4] = (unsigned int)text_empty;
	/* game 1 variant 1 -> text 0xd1, game 2 variant 3 -> 0xd7, game 12 variant 4 -> 0x100 */
	fake_texts[0xd1 * 4] = (unsigned int)text_zeta;
	fake_texts[0xd7 * 4] = (unsigned int)text_alpha;
	fake_texts[0x100 * 4] = (unsigned int)text_mid;
	CMB_I32(fake_language, 0x20) = (int)fake_texts;
	DAT_002ecc74 = (int)fake_language;
	memset(DAT_002839f0, 0, sizeof DAT_002839f0);
	DAT_002839f0[1 * 4 + 1 - 4] = 1;
	DAT_002839f0[2 * 4 + 3 - 4] = -2;
	DAT_002839f0[12 * 4 + 4 - 4] = 3;
	memset(mode, 0, sizeof mode);
	CGameMode_MiniGameSelect_ctor((struct CGameMode_MiniGameSelect *)mode);
	CHECK(DAT_002ba4dc == 3, "three available variants (%d)", DAT_002ba4dc);
	CHECK(DAT_002ba238[0].game == 2 && DAT_002ba238[0].variant == 3 && DAT_002ba238[0].text == 0xd7,
		"Alpha first (%d %d %#x)", DAT_002ba238[0].game, DAT_002ba238[0].variant, DAT_002ba238[0].text);
	CHECK(DAT_002ba238[1].game == 12 && DAT_002ba238[1].variant == 4, "Mid second");
	CHECK(DAT_002ba238[2].game == 1 && DAT_002ba238[2].variant == 1, "Zeta last");
}

int main(void)
{
	test_rank_weights();
	test_rank_ties();
	test_rounds();
	test_sort_players();
	test_carousel();
	printf("core_modes_b tests: %d checks, %d failures\n", checks, failures);
	return failures == 0 ? 0 : 1;
}

#endif
