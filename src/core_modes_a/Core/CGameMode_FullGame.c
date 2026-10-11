/*
Core/CGameMode_FullGame.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_FullGame.cpp

The full game (party/tournament): a state machine that, each time it comes
back to the top of the mode stack, pushes the next screen (board map, round
scores, game statistics, game winner) or starts the next minigame. The
constructor (0x00018800) is unplaced in the table; it is this class's.
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

/* CGameMode_FullGame (vtable 0x00254fd8) */
struct CGameMode_FullGame {
	struct CGameModeMenu menu;      /* +0x00 */
	int unk98;                      /* +0x98 */
	int state;                      /* +0x9c */
	int finished;                   /* +0xa0 */
};
OF_ASSERT_OFFSET(struct CGameMode_FullGame, finished, 0xa0);

/* core_modes_b's score table (CScoreTable, 0x310 bytes): round, then 7 rounds of 25 ints
   (kind, scores[4][4], sum[4], extra[4]), totals, bonus, place, by_place, ties */
extern int DAT_002b9968[0xc4];
#define CMA_SCORE_ROUND DAT_002b9968[0]
#define CMA_SCORE_KIND(r) DAT_002b9968[1 + (r) * 25]
#define CMA_SCORE_SUM(r, p) DAT_002b9968[18 + (r) * 25 + (p)]
#define CMA_SCORE_BONUS(p) DAT_002b9968[0xb4 + (p)]
#define CMA_SCORE_TIES DAT_002b9968[0xc0]

extern int DAT_002ba568;            /* core: the mode after a tournament game */
extern int DAT_002ba978;            /* core: game 9 (the tie breaker) */
extern int DAT_002bae74;            /* core: the game size chosen */
extern int DAT_002baea8;            /* core: replay the same game */
extern int DAT_002844a0[];          /* core: rounds per game size */
extern int DAT_002b9718;            /* the game is a tournament */
extern unsigned short DAT_00283c7c[]; /* core: 6 entries of 0x20 bytes, counted pairs */

/* constructors of other modes (thiscall) */
#define CMA_CGameMode_Scores_ctor(mem, kind) ((void *(*)(void *, int))FUN_000284e0)((mem), (kind))
#define CMA_CGameMode_Map_ctor(mem, kind) ((void *(*)(void *, int))FUN_0001ea60)((mem), (kind))

static const char s_FullGame_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_FullGame.cpp";

/* 0x00254fd8 CGameMode_FullGame */
const void *const vtable_00254fd8[] = {
	(const void *)FUN_0001d1e0,     /* core_modes_b: deleting destructor of menu modes */
	(const void *)FUN_001d5950,
	(const void *)CGameMode_FullGame_Resume,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_FullGame_Pushed,
	(const void *)FUN_0002fe60,
	(const void *)FUN_00030000,
	(const void *)FUN_00030310,
	(const void *)CGameMode_FullGame_PostDraw,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_FullGame_HandleButton,
};

/* start the current game and variation (DAT_002baeac, DAT_002baeb0) */
static void CMA_StartCurrentGame(void)
{
	void *game = DAT_002ba954[DAT_002baeac];

	CMA_VFN(game, 0x0c, int (*)(void *, int))(game, DAT_002baeb0 + 1);
}

/* 0x00018800  Core (unplaced; CGameMode_FullGame) */
struct CGameMode_FullGame *CGameMode_FullGame_ctor(struct CGameMode_FullGame *self)
{
	FUN_00030090((struct CMenuMode *)self, "FullGame");
	self->menu.base.refCount = self->menu.base.refCount + 1;
	self->menu.base.vtbl = vtable_00254fd8;
	self->finished = 0;
	return self;
}

/* 0x00018830  Core/CGameMode_FullGame.cpp: pushed: clear core's counted tables */
void CGameMode_FullGame_Pushed(struct CGameMode_FullGame *self)
{
	unsigned short *entry;

	self->state = 2;
	for (entry = DAT_00283c7c; entry < DAT_00283c7c + 6 * 16; entry += 16) {
		int i;

		entry[1] = 0;
		for (i = 0; i < (int)entry[0]; i++)
			*(unsigned int *)(entry + 4 + i * 4) = 0;
	}
	FUN_0004eed0(0);
}

/* 0x00018880  Core/CGameMode_FullGame.cpp: no buttons (also GameWinner's) */
int CGameMode_FullGame_HandleButton(struct CGameMode *self, int button, int player)
{
	(void)self;
	(void)button;
	(void)player;
	return 0;
}

/* 0x00018890  Core/CGameMode_FullGame.cpp (vtable +0x20): back to the start when finished */
void CGameMode_FullGame_PostDraw(struct CGameMode_FullGame *self, int active)
{
	if (active != 0 && self->finished != 0)
		FUN_0002c3f0();
}

/* 0x000188b0  Core/CGameMode_FullGame.cpp: resume: the next step of the game (with 0x00018d90,
   a part of it) */
void CGameMode_FullGame_Resume(struct CGameMode_FullGame *self)
{
	void *memory;
	void *mode;
	int scores[4];
	int i;

	DAT_00292c50 = 0x18;
	DAT_00292c4c = 0x18;
	DAT_00284408 = 0;
	DAT_00292c78 = 0x240;
	FUN_00054690(DAT_002bae6c);
	DAT_002baed0[0].unk00 = 0;
	switch (self->state) {
	case 0:
		if (DAT_002baea8 != 0) {
			CMA_StartCurrentGame();
			return;
		}
		FUN_0004eed0(1);
		memory = CMA_New(0xed0, s_FullGame_cpp, 0xcd, 0x1001);
		mode = memory == 0 ? 0 : CMA_CGameMode_Scores_ctor(memory, CMA_SCORE_KIND(CMA_SCORE_ROUND));
		CMA_ModeStack_Push(mode, 1);
		self->state = 2;
		return;
	case 1:
	case 7:
		if (DAT_002baea8 != 0) {
			CMA_StartCurrentGame();
			return;
		}
		FUN_0004eed0(1);
		if (CMA_SCORE_ROUND == DAT_002844a0[DAT_002bae74]) {
			/* the last round: clear the round scores, rank */
			for (i = 0; i < 4; i++) {
				CMA_FIELD(int, &DAT_002baed0[i], 0x18) = 0;
				CMA_SCORE_SUM(CMA_SCORE_ROUND, i) = 0;
			}
			FUN_0001bd70((struct CScoreTable *)DAT_002b9968);
			if (CMA_SCORE_TIES > 1) {
				/* a tie: the tie breaker game */
				self->state = 6;
				DAT_002baed0[0].unk00 = 1;
				DAT_002baeac = 9;
				DAT_002baeb0 = 4;
				CMA_VFN(DAT_002ba978, 0x0c, int (*)(void *, int))((void *)DAT_002ba978, 5);
				return;
			}
			self->state = 3;
			memory = CMA_New(0x1f0, s_FullGame_cpp, 0xae, 0x1001);
			mode = memory == 0 ? 0 : CGameMode_GameWinner_ctor(memory);
		} else {
			self->state = 4;
			memory = CMA_New(0x5e8, s_FullGame_cpp, 0xb5, 0x1001);
			mode = memory == 0 ? 0 : CMA_CGameMode_Map_ctor(memory, 4);
		}
		CMA_ModeStack_Push(mode, 1);
		return;
	case 2: {
		int kind;

		if (CMA_SCORE_KIND(CMA_SCORE_ROUND) == 0 && CMA_SCORE_ROUND == 0)
			kind = 1;
		else
			kind = (CMA_SCORE_KIND(CMA_SCORE_ROUND) == 3) + 2;
		self->state = 4;
		memory = CMA_New(0x5e8, s_FullGame_cpp, 0xc2, 0x1001);
		mode = memory == 0 ? 0 : CMA_CGameMode_Map_ctor(memory, kind);
		CMA_ModeStack_Push(mode, 1);
		return;
	}
	case 3:
		self->finished = 1;
		return;
	case 4:
		if (DAT_002baeac != 0)
			(&DAT_002ba564)[DAT_002baeb0 + DAT_002baeac * 4] =
				(&DAT_002ba564)[DAT_002baeb0 + DAT_002baeac * 4] + 1;
		if (DAT_002b9718 == 0) {
			if (DAT_002baebc == 0) {
				self->state = CMA_SCORE_KIND(CMA_SCORE_ROUND) >= 3;
				CMA_StartCurrentGame();
				return;
			}
			self->state = 5;
			CMA_ModeStack_Push((void *)DAT_002ba568, 1);
			return;
		}
		if (DAT_002baebc != 0) {
			self->state = 5;
			CMA_ModeStack_Push((void *)DAT_002ba568, 1);
			return;
		}
		self->state = 2;
		if (CMA_SCORE_KIND(CMA_SCORE_ROUND) < 3) {
			memory = CMA_New(0xed0, s_FullGame_cpp, 0x46, 0x1001);
			mode = memory == 0 ? 0 : CMA_CGameMode_Scores_ctor(memory, CMA_SCORE_KIND(CMA_SCORE_ROUND));
			CMA_ModeStack_Push(mode, 1);
			return;
		}
		/* a bonus round: random scores */
		for (i = 0; i < 4; i++)
			scores[i] = ((int)Core_Rand() % 10) * 100;
		FUN_0001c0f0((struct CScoreTable *)DAT_002b9968, scores);
		FUN_0001c1b0((struct CScoreTable *)DAT_002b9968);
		self->state = 1;
		memory = CMA_New(0x13c, s_FullGame_cpp, 0x51, 0x1001);
		mode = memory == 0 ? 0 : CGameMode_GameStats_ctorShow(memory, 1);
		CMA_ModeStack_Push(mode, 1);
		return;
	case 5:
		if (DAT_002b9718 == 0) {
			self->state = 0;
			CMA_StartCurrentGame();
			return;
		}
		self->state = 2;
		memory = CMA_New(0xed0, s_FullGame_cpp, 0x6e, 0x1001);
		mode = memory == 0 ? 0 : CMA_CGameMode_Scores_ctor(memory, CMA_SCORE_KIND(CMA_SCORE_ROUND));
		CMA_ModeStack_Push(mode, 1);
		return;
	case 6: {
		/* after the tie breaker: the player with the best score gets a bonus point. When no
		   score is above 0 the original reads an uninitialised index; here it is player 0. */
		int best = 0;
		int winner = 0;

		self->state = 3;
		if (CMA_FIELD(int, &DAT_002baed0[0], 0x18) > 0) {
			winner = 0;
			best = CMA_FIELD(int, &DAT_002baed0[0], 0x18);
		}
		for (i = 1; i < 4; i++) {
			if (CMA_FIELD(int, &DAT_002baed0[i], 0x18) > best) {
				winner = i;
				best = CMA_FIELD(int, &DAT_002baed0[i], 0x18);
			}
		}
		CMA_SCORE_BONUS(winner) = CMA_SCORE_BONUS(winner) + 1;
		memory = CMA_New(0x1f0, s_FullGame_cpp, 0x82, 0x1001);
		mode = memory == 0 ? 0 : CGameMode_GameWinner_ctor(memory);
		CMA_ModeStack_Push(mode, 1);
		return;
	}
	default:
		return;
	}
}
