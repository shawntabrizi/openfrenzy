/*
UNPLACED.C  <-  C:\BlitzParty\Game\Src\Core\ (file not known)

Gap functions of Core between the tagged files of this module: the score table of a party
or tournament (0x0001bd30-0x0001c200, used by the main menu and the game-winner screens) and
small constructors/destructors of the game modes.
*/

#include "core_modes_b.h"

/* 0x0001bd30  (unplaced): CScoreTable::Clear */
void CScoreTable_Clear(struct CScoreTable *self)
{
	/* the first 0x2e0 bytes: round, rounds[], totals[], bonus[] */
	memset(self, 0, offsetof(struct CScoreTable, place));
}

/* 0x0001bd10 (inside the gap before 0x0001bd30): qsort comparison of the original,
   "a < b". The original qsort swaps on a positive result, which sorts descending; musl's
   qsort needs a consistent comparison, so this one returns the descending order's sign. The
   sorted arrays hold ints, so the result is the same. */
static int cmb_compare_descending(const void *a, const void *b)
{
	int left = *(const int *)a;
	int right = *(const int *)b;

	return left < right ? 1 : (left > right ? -1 : 0);
}

/* value of player `p` for the first ranking of CScoreTable_Rank */
static int cmb_rank_value(struct CScoreTable *self, int player, int *valid)
{
	if (DAT_002bae98 == 1) {
		*valid = DAT_002ba55c != 0;
		return DAT_002ba55c != 0 ? CMB_AT(DAT_002ba55c, 0x1b0 + player * 4, int) : 0;
	}
	*valid = 1;
	return self->bonus[player];
}

/* 0x0001bd70  (unplaced): CScoreTable::Rank. Points: 400/300/200/100 for the place in the
   first ranking (tournament values or the bonus), 40/30/20/10 for the place in this round's
   sums, 4/3/2/1 for the place in the per-player game score; then the places from the
   points, with ties. */
int CScoreTable_Rank(struct CScoreTable *self)
{
	int points[4];
	int sorted_points[4];
	int sorted_score[4];
	int sorted_first[4];
	int sorted_round[4];
	int player;
	int rank;
	int weight;
	int valid;
	int slot;

	points[0] = 0;
	points[1] = 0;
	points[2] = 0;
	points[3] = 0;
	for (player = 0; player < 4; player++) {
		if (DAT_002bae98 == 1) {
			sorted_first[player] = DAT_002ba55c == 0 ? 0 : CMB_AT(DAT_002ba55c, 0x1b0 + player * 4, int);
			sorted_round[player] = 0;
		} else {
			sorted_first[player] = self->bonus[player];
			sorted_round[player] = self->rounds[self->round].sum[player];
		}
		sorted_score[player] = DAT_002baed4[player][5];
	}
	qsort(sorted_first, DAT_002bae94, 4, cmb_compare_descending);
	qsort(sorted_round, DAT_002bae94, 4, cmb_compare_descending);
	qsort(sorted_score, DAT_002bae94, 4, cmb_compare_descending);

	for (rank = 0, weight = 400; weight > 0; rank++, weight -= 100) {
		if (rank != 0 && sorted_first[rank] == sorted_first[rank - 1])
			continue;
		for (player = 0; player < 4; player++) {
			int value = cmb_rank_value(self, player, &valid);

			if (!valid)
				break;
			if (value == sorted_first[rank])
				points[player] += weight;
		}
	}
	for (rank = 0, weight = 40; weight > 0; rank++, weight -= 10) {
		if (rank != 0 && sorted_round[rank] == sorted_round[rank - 1])
			continue;
		for (player = 0; player < 4; player++) {
			if (self->rounds[self->round].sum[player] == sorted_round[rank])
				points[player] += weight;
		}
	}
	for (rank = 0; rank < 4; rank++) {
		if (rank != 0 && sorted_score[rank] == sorted_score[rank - 1])
			continue;
		for (player = 0; player < 4; player++) {
			if (DAT_002baed4[player][5] == sorted_score[rank])
				points[player] += 4 - rank;
		}
	}

	for (rank = 0; rank < 4; rank++) {
		sorted_points[rank] = points[rank];
		self->by_place[rank] = -1;
		self->ties[rank] = 0;
	}
	qsort(sorted_points, DAT_002bae94, 4, cmb_compare_descending);
	for (rank = 0; rank < 4; rank++) {
		if (rank != 0 && sorted_points[rank] == sorted_points[rank - 1])
			continue;
		for (player = 0; player < 4; player++) {
			if (sorted_points[rank] != points[player])
				continue;
			self->place[player] = rank;
			self->ties[rank]++;
			for (slot = rank; self->by_place[slot] != -1; slot++)
				;
			self->by_place[slot] = player;
		}
	}
	return 0;
}

/* 0x0001c0f0  (unplaced): CScoreTable::AddRound(scores of the players) */
void CScoreTable_AddRound(struct CScoreTable *self, int *scores)
{
	struct CScoreRound *round;
	int player;

	if (DAT_002bae98 != 0)
		CScoreTable_Clear(self);
	round = &self->rounds[self->round];
	for (player = 0; player < DAT_002bae94; player++) {
		round->scores[round->kind][player] = scores[player];
		if (round->kind != 3) {
			round->sum[player] += scores[player];
			self->totals[player] += scores[player];
		}
	}
	if (DAT_002bae98 == 0)
		round->kind++;
	CScoreTable_Rank(self);
}

/* 0x0001c1b0  (unplaced): CScoreTable::EndRound, adds the bonus row and starts the next round */
void CScoreTable_EndRound(struct CScoreTable *self)
{
	struct CScoreRound *round = &self->rounds[self->round];

	self->bonus[0] += round->scores[3][0];
	self->bonus[1] += round->scores[3][1];
	self->bonus[2] += round->scores[3][2];
	self->bonus[3] += round->scores[3][3];
	self->round++;
	CScoreTable_Rank(self);
}

/* 0x0001c200  (unplaced): order the players by value, highest first: order[k] is the player
   in place k, place_of[player] its place (bubble sort, equal values keep player order) */
void cmb_sort_players(int *values, int *place_of, int *order)
{
	int index[4];
	int i;
	int swapped;
	int temp;

	for (i = 0; i < DAT_002bae94; i++)
		index[i] = i;
	do {
		swapped = 0;
		if (DAT_002bae94 - 1 < 1)
			break;
		for (i = 0; i < DAT_002bae94 - 1; i++) {
			temp = index[i];
			if (values[temp] < values[index[i + 1]]) {
				swapped = 1;
				index[i] = index[i + 1];
				index[i + 1] = temp;
			}
		}
	} while (swapped);
	for (i = 0; i < DAT_002bae94; i++) {
		order[i] = index[i];
		place_of[index[i]] = i;
	}
}

/* 0x0001ddb0  (unplaced): constructor of a small object whose member at +8 is cleared by
   core FUN_0002f570 (no caller found) */
void *FUN_0001ddb0(void *self)
{
	((void (*)(void *))FUN_0002f570)((char *)self + 8);
	return self;
}
