/*
CGAMEMODE_SCORES.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_Scores.cpp

openfrenzy core module: the score screen after a minigame. Points are given by rank
(0x00283b74: 6, 4, 2, 0; ties share), the winners dance in a ring, and the points fly as
orbs into each player's column. Vtable 0x002565d0. Decompiled from orig/default.xbe
(functional, not byte-matching).

The constructor (0x000284e0) and destructor (0x00028580) sit in the source-map gap before
this file; they set this class's vtable, so they are placed here.
*/

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "core_globals.h"
#include "core_extern.h"
#include "core_modes.h"

static const char scores_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_Scores.cpp";

/* a point orb, 0x20 bytes (array at +0xe54, 80 entries) */
struct ScoreOrb {
	float x, y;          /* +0x00 */
	float floor_x;       /* +0x08 x stops here (it falls to the left) */
	float target_y;      /* +0x0c */
	float velocity;      /* +0x10 */
	float y_speed;       /* +0x14 */
	unsigned char *actor;/* +0x18 */
	int player;          /* +0x1c */
};

/* CGameMode_Scores, 0xec8 bytes */
struct CGameMode_Scores {
	struct CMenuMode base;            /* +0x000 */
	float voice_delay;                /* +0x098 */
	int speed_up;                     /* +0x09c a player holds "Speed" */
	float speed_factor;               /* +0x0a0 */
	void *input_map;                  /* +0x0a4 */
	unsigned char *speed_buttons[4];  /* +0x0a8 */
	float timer;                      /* +0x0b8 */
	int screen;                       /* +0x0bc constructor argument */
	int phase;                        /* +0x0c0 0 voice, 1 orbs, 3 count, 4 done */
	int winner;                       /* +0x0c4 player or team, -1: draw */
	int shown;                        /* +0x0c8 players in the winners' ring */
	int voice;                        /* +0x0cc -1, -2: done */
	int voice2;                       /* +0x0d0 */
	int points[4];                    /* +0x0d4 this round */
	int totals[4];                    /* +0x0e4 */
	int ranks[4];                     /* +0x0f4 */
	int order[4];                     /* +0x104 */
	void *snd_move;                   /* +0x114 */
	void *snd_orb;                    /* +0x118 */
	void *title;                      /* +0x11c */
	uint32_t field_120;               /* +0x120 */
	unsigned char *name_text[4];      /* +0x124 */
	unsigned char *score_text[4];     /* +0x134 */
	void *const **popup;              /* +0x144 */
	unsigned char field_148[0xc];     /* +0x148 */
	int *anim_default;                /* +0x154 */
	unsigned char light_a[0x70];      /* +0x158 */
	unsigned char light_b[0x70];      /* +0x1c8 */
	unsigned char light_c[0x70];      /* +0x238 */
	void *models[4];                  /* +0x2a8 */
	unsigned char field_2b8[8];       /* +0x2b8 */
	unsigned char actors[4][0x170];   /* +0x2c0 */
	void *tiles[4];                   /* +0x880 */
	unsigned char field_890[0x5c0];   /* +0x890 */
	float ring_angle;                 /* +0xe50 */
	struct ScoreOrb *orbs;            /* +0xe54 */
	struct ScoreOrb *new_orbs[4][6];  /* +0xe58 */
	int orb_count;                    /* +0xeb8 */
	void *pkg_scores;                 /* +0xebc */
	void *pkg_charsel;                /* +0xec0 */
	void *font;                       /* +0xec4 */
};
_Static_assert(sizeof(struct CGameMode_Scores) == 0xec8, "CGameMode_Scores size");

/* points for 1st-4th, and the screen phase counter (core .data) */
#define g_RankPoints ((int *)CORE_ADDR(0x00283b74))
#define g_ScorePhase CORE_VAR(int, 0x00283b70)
#define g_ScoreOrder ((int *)CORE_ADDR(0x00283c54))   /* players by rank, 4 */
#define g_ScoreRadius ((float *)CORE_ADDR(0x00283c3c))  /* [n]: ring radius for n winners */
#define F(addr) CORE_VAR(float, addr)
#define g_ScoreFade CORE_VAR(char, 0x002ba6ac)
#define g_ScoreDance CORE_VAR(void **, 0x002ba6b4)

/* the 4 background vertices (0x002ba6b8, 0x24 bytes each) */
#define BG(off) CORE_VAR(uint32_t, 0x002ba6b8 + (off))

/* text box descriptor of classlib_ui (0x3b0 bytes) */
#define TB_I32(d, off) (*(int32_t *)((d) + (off)))
#define TB_F32(d, off) (*(float *)((d) + (off)))
#define TB_I16(d, off) (*(int16_t *)((d) + (off)))
#define TB_TEXT(d) ((unsigned short *)((d) + 0x48))

/* other modules */
extern int DAT_002b9718;                  /* core_modes_a: test map (random scores) */
extern unsigned char DAT_002b9968[];      /* core_modes_b: the score table object */
extern unsigned char DAT_002900c8[];      /* classlib_ui: characters, 0x108 bytes each */
extern const char *DAT_00290700[];        /* classlib_ui: character records, 0x18 pointers each */
extern void *DAT_002b95d0;                /* core_modes_a mode */
#define g_ScoreTable(round) ((int *)(DAT_002b9968 + 0x48) + (round) * 0x19)  /* 0x002b99b0 */
#define g_Round (*(int *)DAT_002b9968)

extern const void *const vtable_002565d0[14];

/* 0x000284e0  (Core, CGameMode_Scores.cpp) CGameMode_Scores::CGameMode_Scores(screen) */
struct CGameMode_Scores *CGameMode_Scores_Ctor(struct CGameMode_Scores *self, int screen)
{
	CMenuMode_Ctor(&self->base, "ScoreScreen");
	self->base.vtbl = vtable_002565d0;
	ext_ListInit(self->field_148);
	self->screen = screen;
	self->orb_count = 0;
	self->timer = 0.0f;
	self->phase = 0;
	self->orbs = NULL;
	self->base.flags |= 2;
	((void (*)(int))FUN_00071880)(3);
	*(int *)((char *)self + 0x60) = 1000000;
	return self;
}

/* 0x00028580  (Core, CGameMode_Scores.cpp) CGameMode_Scores::~CGameMode_Scores */
void CGameMode_Scores_Dtor(struct CGameMode_Scores *self)
{
	self->base.vtbl = vtable_002565d0;
	g_ScorePhase = 3;
	CORE_VAR(int, 0x002ba6b0) = 0;
	ext_Nothing(self->field_148);
	CMenuMode_Dtor(&self->base);
}

/* 0x000285f0  Core/CGameMode_Scores.cpp  update (vtable +0x14): "Speed" held by a player? */
void CGameMode_Scores_Update(struct CGameMode_Scores *self, int top)
{
	int i;

	self->speed_up = 0;
	for (i = 0; i < g_PlayerCount; i++)
	{
		if (g_PlayerPresent(i) != 0 && *(int *)(self->speed_buttons[i] + 0x14) != 0)
			self->speed_up = 1;
	}
	CMenuMode_Update(&self->base, top);
}

/* 0x00028650  Core/CGameMode_Scores.cpp  a button (vtable +0x34): A or B leaves when done */
int CGameMode_Scores_Button(struct CGameMode_Scores *self, int button, int player)
{
	(void)player;
	if (self->phase == 4 && (button == 4 || button == 5) && 30.0f <= self->timer)
	{
		ext_PlaySfx(9, 0x7f, 0, 0, 0);
		CGameModeStack_Pop(&g_ModeStack, 1);
		return 1;
	}
	return 0;
}

/* 0x000286b0  Core/CGameMode_Scores.cpp  draw the background, the winners and their shadows */
void CGameMode_Scores_DrawBackground(struct CGameMode_Scores *self)
{
	int i;
	uint32_t colour;

	ext_SetRenderMode(1);
	ext_SetRenderState(8, 0, 0);
	ext_SetRenderState(3, 1, 0);
	ext_SetRenderState(7, 3, 0);
	ext_SetRenderState(5, 1, 0);
	ext_SetRenderState(6, 1, 0);
	ext_SetAmbient(CORE_VAR(int, 0x00283b58), CORE_VAR(int, 0x00283b5c), CORE_VAR(int, 0x00283b60));
	ext_LightEnable(self->light_a, 1);
	ext_LightEnable(self->light_b, 1);
	ext_SetClipPlanes(50.0f, 2000.0f);
	ext_SetFov(F(0x00283c1c), 0.0f);
	ext_SetCamera((const float *)CORE_ADDR(0x00283c0c), (const float *)CORE_ADDR(0x00283bfc), 0.0f);
	ext_ApplyCamera();
	ext_FUN_000438b0();
	/* the shadows: the winners drawn flattened (scale y -1) */
	for (i = 0; i < 4 && self->models[i] != NULL; i++)
	{
		ext_SetRenderState(6, 1, 0);
		*(uint32_t *)(self->actors[i] + 0xd4) = 0xbf800000;  /* -1.0f */
		ext_ActorSetVisible(self->actors[i], 1);
		ext_ActorDraw(self->actors[i], 7);
	}
	/* the backdrop quad, coloured with the Extras background colour */
	memset(CORE_ADDR(0x002ba6b8), 0, 0x24 * 4);
	BG(0x04) = 0;
	colour = DAT_002bd640[CORE_VAR(int, 0x002844d4)] | DAT_002bc490[CORE_VAR(int, 0x002844d0)]
		| DAT_002bce18[CORE_VAR(int, 0x002844cc)];
	BG(0x18) = colour | DAT_002bc898[(0x002bcaf0 - 0x002bc898) / 4];
	BG(0x60) = colour | DAT_002bc898[(0x002bc988 - 0x002bc898) / 4];
	BG(0x00) = 0xc4160000;  /* -600 */
	BG(0x08) = 0xc37a0000;  /* -250 */
	BG(0x24) = 0x44160000;  /* 600 */
	BG(0x28) = 0;
	BG(0x2c) = 0xc37a0000;
	BG(0x48) = 0x44160000;
	BG(0x4c) = 0;
	BG(0x50) = 0x42c80000;  /* 100 */
	BG(0x6c) = 0xc4160000;
	BG(0x70) = 0;
	BG(0x74) = 0x42c80000;
	BG(0x3c) = BG(0x18);
	BG(0x84) = BG(0x60);
	ext_SetRenderState(6, 1, 0);
	ext_SetRenderState(3, 0, 0);
	ext_SetTexture(0, 0);
	ext_FUN_000438b0();
	ext_DrawPrimitives(5, CORE_ADDR(0x002ba6b8), 4);
	ext_FUN_000438b0();
	ext_SetRenderState(3, 1, 0);
	for (i = 0; i < 4 && self->models[i] != NULL; i++)
	{
		ext_SetRenderState(6, 1, 0);
		*(uint32_t *)(self->actors[i] + 0xd4) = 0x3f800000;  /* 1.0f */
		ext_ActorSetVisible(self->actors[i], 0);
		ext_ActorDraw(self->actors[i], 7);
	}
	ext_LightEnable(self->light_a, 0);
	ext_LightEnable(self->light_b, 0);
	ext_SetRenderState(3, 0, 0);
}

/* 0x00028950  Core/CGameMode_Scores.cpp  scalar deleting destructor (vtable [0]) */
struct CGameMode_Scores *CGameMode_Scores_Destroy(struct CGameMode_Scores *self, int flags)
{
	CGameMode_Scores_Dtor(self);
	if (flags & 1)
		ext_Delete(self);
	return self;
}

/* 0x00028970  Core/CGameMode_Scores.cpp  popped (vtable +0x0c): free orbs, actors, packages */
void CGameMode_Scores_Popped(struct CGameMode_Scores *self)
{
	int i;

	for (self->orb_count--; self->orb_count >= 0; self->orb_count--)
		ext_ActorFree(self->orbs[self->orb_count].actor);
	if (self->orbs != NULL)
	{
		ext_Delete(self->orbs);
		self->orbs = NULL;
	}
	for (i = 0; i < 4; i++)
	{
		g_PlayerScore(i) = 0;
		if (self->models[i] != NULL)
			ext_ActorFree(self->actors[i]);
	}
	if (self->pkg_scores != NULL)
		((void (*)(void *))FUN_0004db40)(self->pkg_scores);
	if (self->pkg_charsel != NULL)
		((void (*)(void *))FUN_0004db40)(self->pkg_charsel);
	if (self->popup != NULL)
		((void (*)(void *, int))(*self->popup)[0])(self->popup, 1);
	ext_TextFreeAll();
	if (self->input_map != NULL)
		ext_InputMapFree(self->input_map);
	if (g_ScoreDance != NULL)
	{
		ext_Delete(*g_ScoreDance);
		ext_Delete(g_ScoreDance);
		g_ScoreDance = NULL;
	}
	ext_LightFree(self->light_a);
	ext_LightFree(self->light_b);
	ext_LightFree(self->light_c);
}

/* 0x00028ab0  Core/CGameMode_Scores.cpp  this round's points from the minigame scores
(g_PlayerScore): POINTS[player]; *WINNER = winning player or team, -1 on a draw. FINAL:
1 point to each winner only. Also sorts g_ScoreOrder by score. */
void CGameMode_Scores_ComputePoints(int *winner, int *points, int final)
{
	int score[4] = { 0, 0, 0, 0 };
	int best, i, j, sum, n, swapped, count;

	if (DAT_002b9718 != 0)
	{
		/* test map: random scores 0-4 (the libc rand, inline) */
		for (i = 0; i < g_PlayerCount; i++)
		{
			DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
			g_PlayerScore(i) = (int)((DAT_00285630 >> 16) & 0x7fff) % 5;
		}
	}
	if (g_TeamMode != 0)
	{
		int w, l;

		*winner = -1;
		for (i = 0; i < 2; i++)
			score[i] += g_PlayerScore(g_TeamPlayer(i, 0)) + g_PlayerScore(g_TeamPlayer(i, 1));
		if (score[0] == score[1])
		{
			int value = (final == 0) * 2 + 1;

			points[g_TeamPlayer(0, 0)] = value;
			points[g_TeamPlayer(1, 0)] = value;
			points[g_TeamPlayer(0, 1)] = value;
			points[g_TeamPlayer(1, 1)] = value;
			g_ScoreOrder[0] = 0;
			g_ScoreOrder[1] = 1;
			g_ScoreOrder[2] = 2;
			g_ScoreOrder[3] = 3;
			return;
		}
		w = score[0] < score[1] ? 1 : 0;
		l = 1 - w;
		*winner = w;
		points[g_TeamPlayer(w, 0)] = final != 0 ? 1 : g_RankPoints[0];
		points[g_TeamPlayer(l, 0)] = final != 0 ? 0 : g_RankPoints[3];
		g_ScoreOrder[0] = g_TeamPlayer(w, 0);
		g_ScoreOrder[2] = g_TeamPlayer(l, 0);
		points[g_TeamPlayer(w, 1)] = final != 0 ? 1 : g_RankPoints[0];
		points[g_TeamPlayer(l, 1)] = final != 0 ? 0 : g_RankPoints[3];
		g_ScoreOrder[1] = g_TeamPlayer(w, 1);
		g_ScoreOrder[3] = g_TeamPlayer(l, 1);
		return;
	}
	*winner = -1;
	best = -1;
	count = g_PlayerCount;
	for (i = 0; i < count; i++)
	{
		score[i] = g_PlayerScore(i);
		g_ScoreOrder[i] = i;
		if (score[i] > best)
		{
			*winner = i;
			count = g_PlayerCount;
			best = score[i];
		}
	}
	for (i = 0; i < count; i++)
	{
		if (i != *winner && score[i] == best)
		{
			*winner = -1;
			count = g_PlayerCount;
			break;
		}
	}
	do
	{
		swapped = 0;
		for (i = 0; i < count - 1; i++)
		{
			int a = g_ScoreOrder[i], b = g_ScoreOrder[i + 1];

			if (score[a] < score[b])
			{
				swapped = 1;
				g_ScoreOrder[i] = b;
				g_ScoreOrder[i + 1] = a;
			}
		}
	} while (swapped);
	if (final != 0)
	{
		for (i = 0; i < 4; i++)
			points[i] = g_PlayerScore(i) == best;
		return;
	}
	/* equal scores share the points of their ranks */
	for (i = 0; i < 4; i = j)
	{
		sum = 0;
		n = 0;
		j = i;
		do
		{
			sum += g_RankPoints[j];
			j++;
			n++;
		} while (j < 4 && g_PlayerScore(g_ScoreOrder[j]) == g_PlayerScore(g_ScoreOrder[j - 1]));
		for (; i < j; i++)
			points[g_ScoreOrder[i]] = sum / n;
	}
}

/* 0x00028df0  Core/CGameMode_Scores.cpp  the title and each player's name and total */
void CGameMode_Scores_CreateTexts(struct CGameMode_Scores *self)
{
	unsigned char box[0x3b0];
	int k, row;
	unsigned char *table = *(unsigned char **)((unsigned char *)DAT_002ecc74 + 0x20);

	memset(box, 0, sizeof(box));
	TB_I32(box, 0x44) = (int32_t)(uintptr_t)self->font;
	TB_F32(box, 0x250) = (float)(CORE_VAR(int, 0x002844fc) / 2);
	TB_F32(box, 0x254) = (float)(CORE_VAR(int, 0x00284500) / 2);
	TB_F32(box, 0x258) = (float)(CORE_VAR(int, 0x00284504) / 2);
	TB_I32(box, 0x288) = 2;
	TB_F32(box, 0x25c) = (float)(CORE_VAR(int, 0x00284508) / 2);
	TB_I16(box, 0x24c) = 2;
	TB_I32(box, 0x2a4) = 0x437a0000;  /* 250 */
	TB_F32(box, 0x2a8) = 180.0f;
	TB_I32(box, 0x2ac) = 0;
	msvc_swprintf(TB_TEXT(box), u"%s - %s %s",
		*(unsigned short **)(table + (CORE_VAR(int, 0x002baeb4) + 0xcb) * 0x10),
		*(unsigned short **)(table + 0x1070),
		*(unsigned short **)(table + (self->screen + 0x118) * 0x10));
	self->title = ext_TextCreate(box);
	for (row = 0, k = 0; k < 0xf0; k += 0x3c, row++)
	{
		int player = self->order[row];
		const char *portrait;

		if (g_PlayerCharacter(player) == -1)
			g_PlayerCharacter(player) = row;
		/* the portrait */
		memset(box, 0, sizeof(box));
		TB_I32(box, 0x288) = 0;
		portrait = DAT_00290700[g_PlayerCharacter(player) * 0x18];
		TB_I32(box, 0x04) = (int32_t)(uintptr_t)ext_FindResource(0, portrait, 0, 0xdefa);
		TB_F32(box, 0x0c) = 120.0f - (float)k;
		TB_F32(box, 0x250) = 128.0f;
		TB_F32(box, 0x254) = 128.0f;
		TB_F32(box, 0x258) = 128.0f;
		TB_F32(box, 0x25c) = 0.0f;
		TB_I32(box, 0x2a4) = 0xc37a0000;  /* -250 */
		TB_F32(box, 0x08) = -250.0f;
		TB_I32(box, 0x308) = 0x42c00000;  /* 96 */
		TB_I32(box, 0x30c) = 0x42c00000;
		TB_I32(box, 0x260) = 8;
		TB_I32(box, 0x268) = 0;
		TB_I32(box, 0x264) = 0x7f;
		TB_I32(box, 0x28c) = 5;
		TB_I32(box, 0x284) = CORE_VAR(int, 0x00283c40);
		TB_F32(box, 0x2a8) = TB_F32(box, 0x0c);
		self->name_text[player] = ext_TextCreate(box);
		/* the total */
		memset(box, 0, sizeof(box));
		TB_F32(box, 0x0c) = 90.0f - (float)row * 60.0f;
		TB_I32(box, 0x288) = 2;
		TB_I16(box, 0x24c) = 2;
		TB_I32(box, 0x44) = (int32_t)(uintptr_t)self->font;
		TB_F32(box, 0x250) = (float)(CORE_VAR(int, 0x002844fc) / 2);
		TB_F32(box, 0x254) = (float)(CORE_VAR(int, 0x00284500) / 2);
		TB_I16(box, 0x24e) = 1;
		TB_F32(box, 0x258) = (float)(CORE_VAR(int, 0x00284504) / 2);
		TB_I32(box, 0x2a4) = 0xc3160000;  /* -150 */
		TB_I32(box, 0x2ac) = 0;
		TB_F32(box, 0x08) = -150.0f;
		TB_F32(box, 0x25c) = 0.0f;
		TB_I32(box, 0x260) = 8;
		TB_I32(box, 0x284) = CORE_VAR(int, 0x00283c40);
		TB_I32(box, 0x268) = 0;
		TB_I32(box, 0x264) = 0x7f;
		TB_I32(box, 0x28c) = 5;
		TB_F32(box, 0x2a8) = TB_F32(box, 0x0c);
		msvc_swprintf(TB_TEXT(box), u"%d", self->totals[player]);
		self->score_text[player] = ext_TextCreate(box);
	}
}

/* place the N winners in a ring of radius g_ScoreRadius[n] turned by ANGLE (x87: in double) */
static void CGameMode_Scores_PlaceRing(struct CGameMode_Scores *self, double offset)
{
	int i;

	for (i = 0; i < self->shown; i++)
	{
		unsigned char *actor = self->actors[i];
		double angle = (double)6.2831855f / (double)self->shown * (double)i + offset;
		float radius = g_ScoreRadius[self->shown];

		*(float *)(actor + 0xc4) = 0.0f;
		*(float *)(actor + 0xcc) = 1.0f;
		*(float *)(actor + 0xc0) = (float)(cos(angle) * (double)radius);
		*(float *)(actor + 0xc8) = (float)(sin(angle) * (double)radius);
		ext_MatrixRotateY((float *)(actor + 0xe0), (float)(angle + (double)1.5707963f));
	}
}

/* 0x00029190  Core/CGameMode_Scores.cpp  the round's winners: models, ring, victory dance */
void CGameMode_Scores_CreateWinners(struct CGameMode_Scores *self)
{
	int best = 0, row, i;

	for (i = 0; i < 4; i++)
	{
		if (best < self->points[i])
			best = self->points[i];
	}
	self->shown = 0;
	for (row = 0; row < 4; row++)
	{
		int player = self->order[row];

		if (self->points[player] != best)
			continue;
		self->models[self->shown] = ext_FindResource(0,
			(const char *)(DAT_002900c8 + g_PlayerCharacter(player) * 0x108), 1, 0);
		if (self->models[self->shown] != NULL)
		{
			unsigned char *actor = self->actors[self->shown];

			ext_ActorCreate(actor, self->models[self->shown], 6, scores_cpp, 0x27f);
			if (g_ScoreDance == NULL)
				ext_ActorSetMaterial(actor, 0, 1, NULL);
			else
				ext_ActorSetMaterial(actor, 0, 7, g_ScoreDance);
			ext_ActorSetMaterial(actor, 1, 0, NULL);
		}
		self->shown++;
	}
	if (self->shown == 1)
	{
		/* (the original writes here, not into the first actor) */
		*(uint32_t *)((char *)self + 0x940) = 0;
		*(uint32_t *)((char *)self + 0x944) = 0;
		*(uint32_t *)((char *)self + 0x948) = 0;
		*(uint32_t *)((char *)self + 0x94c) = 0x3f800000;
	}
	else
		CGameMode_Scores_PlaceRing(self, 0.0);
	for (i = 0; i < self->shown; i++)
	{
		unsigned char *actor = self->actors[i];
		void *anim;

		if (self->models[i] == NULL)
			continue;
		anim = ext_ActorFindAnim(actor, "victory");
		if (anim != NULL)
			ext_ActorPlay(actor, *(void **)(actor + 0x134), anim, 0x400, -1, 0, 0, 0);
		anim = ext_ActorFindAnim(actor, "victory_idle");
		if (anim != NULL)
			ext_ActorPlayBlend(actor, *(void **)(actor + 0x134), anim, 0x400, -1, 0.01f, 1, 0, 0);
		ext_ActorUpdate(actor, *(void **)(actor + 0x134), 0.0f, 0);
		ext_ActorSetVisible(actor, 1);
	}
}

/* 0x00029440  Core/CGameMode_Scores.cpp  draw (vtable +0x1c) */
void CGameMode_Scores_Draw(struct CGameMode_Scores *self, int top)
{
	static const float eye[4] = { 0.0f, 0.0f, -320.0f, 1.0f };
	static const float at[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int i;

	if (top == 0)
		return;
	ext_SetRenderState(8, 0, 0);
	if (g_ScoreFade != 0)
		ext_FadeTo(0xff, 0);
	CGameMode_Scores_DrawBackground(self);
	ext_FUN_00073900(1);
	if (self->orb_count != 0)
	{
		ext_SetCamera(eye, at, 0.0f);
		ext_SetClipPlanes(100.0f, 1000.0f);
		ext_SetFov(F(0x00283c64), 0.0f);
		ext_ApplyCamera();
		ext_SetAmbient(0, 0, 0);
		ext_LightEnable(self->light_c, 1);
		ext_SetRenderState(8, 0, 0);
		ext_SetRenderState(7, 3, 0);
		ext_SetRenderState(5, 1, 0);
		ext_SetRenderState(3, 1, 0);
		ext_SetRenderState(6, 0, 0);
		for (i = 0; i < self->orb_count; i++)
		{
			ext_ActorSetVisible(self->orbs[i].actor, 1);
			ext_ActorDraw(self->orbs[i].actor, 1);
		}
		ext_SetRenderState(3, 0, 0);
		ext_SetRenderState(4, 1, 0);
		ext_SetRenderState(7, 1, 0);
		ext_SetRenderState(5, 0, 0);
		ext_SetRenderState(6, 1, 0);
		ext_SetRenderState(10, 1, 0);
		for (i = 0; i < self->orb_count; i++)
			ext_ActorDraw(self->orbs[i].actor, 4);
		ext_SetRenderState(4, 0, 0);
		ext_LightEnable(self->light_c, 0);
	}
	if (self->popup != NULL)
		((void (*)(void *))(*self->popup)[3])(self->popup);
}

/* 0x00029640  Core/CGameMode_Scores.cpp  packages loaded (vtable +0x2c): build the screen */
void CGameMode_Scores_Loaded(struct CGameMode_Scores *self)
{
	char name[0x20];
	int i;

	if (self->pkg_charsel != NULL)
	{
		ext_PackagePrepare(self->pkg_charsel, 1, 0xdefa, 0);
		ext_PackagePrepare(self->pkg_charsel, 2, 0xdefa, 0);
	}
	if (self->pkg_scores != NULL)
		ext_PackagePrepare(self->pkg_scores, 7, 0xdefa, 0);
	if (self->pkg_scores == NULL && self->pkg_charsel == NULL)
		DbgBreakPoint();
	self->base.state = 0;
	CGameMode_Scores_CreateTexts(self);
	CGameMode_Scores_CreateWinners(self);
	for (i = 0; i < 4; i++)
	{
		sprintf(name, "colour_tile0%d_00.obe", g_PlayerCharacter(i));
		self->tiles[i] = ext_FindResource(0, name, 1, 0);
		if (self->tiles[i] == NULL)
			DbgBreakPoint();
	}
	self->snd_move = ext_FindResource(0, "moveplscore.wav", 2, 0xdefa);
	self->snd_orb = ext_FindResource(0, "orbchink.wav", 2, 0xdefa);
	self->voice = -1;
	if (g_TeamMode == 0)
	{
		/* a "close race" voice for the last player */
		if ((int)((double)g_PlayerScore(g_ScoreOrder[3]) * 1.5) < g_PlayerScore(g_ScoreOrder[2])
			&& (double)g_PlayerScore(g_ScoreOrder[0]) <= (double)g_PlayerScore(g_ScoreOrder[1]) * 2.0)
			self->voice = ext_PlayVoice(g_PlayerCharacter(g_ScoreOrder[3]) + 0x25, -1, 0xff, 0, 0, 0);
	}
	ext_FUN_000724b0(0x400000, 0, 0xff, -20, 20, 0);
}

/* 0x00029800  Core/CGameMode_Scores.cpp  a new orb of PLAYER at (X, Y) */
struct ScoreOrb *CGameMode_Scores_AddOrb(struct CGameMode_Scores *self, int player, float x, float y)
{
	struct ScoreOrb *orb = &self->orbs[self->orb_count];
	unsigned char *actor;
	int *anim;

	self->orb_count++;
	orb->floor_x = x;
	orb->y = y;
	orb->x = x;
	orb->target_y = y;
	orb->player = player;
	actor = ext_ActorCreate(NULL, self->tiles[player], 1, scores_cpp, 0x520);
	orb->actor = actor;
	*(float *)(actor + 0xd0) = 0.2f;
	*(float *)(actor + 0xd4) = 0.2f;
	*(float *)(actor + 0xd8) = 0.2f;
	*(float *)(actor + 0xdc) = 1.0f;
	self->anim_default = ext_ActorFindAnim(orb->actor, "default");
	if (self->anim_default == NULL)
		DbgBreakPoint();
	DAT_00285630 = DAT_00285630 * 0x343fd + 0x269ec3;
	anim = self->anim_default;
	ext_ActorPlay(orb->actor, *(void **)(orb->actor + 0x134), anim, 0,
		(int)((DAT_00285630 >> 16) & 0x7fff) % (anim[2] - anim[1]) + anim[1], 1, 0, 0);
	return orb;
}

/* 0x00029900  Core/CGameMode_Scores.cpp  pushed (vtable +0x10) */
void CGameMode_Scores_Pushed(struct CGameMode_Scores *self)
{
	int i;

	g_PadsPrimary = 1;
	self->voice_delay = 0.0f;
	ext_FUN_00071690();
	ext_FUN_00072720(self->field_148);
	if (DAT_002b95d0 != NULL)
		*((unsigned char *)DAT_002b95d0 + 0x58) = 1;
	self->speed_factor = 5.0f;
	self->timer = 0.0f;
	self->ring_angle = 0.0f;
	self->voice2 = -1;
	self->voice = -1;
	self->phase = 0;
	self->popup = NULL;
	self->orbs = ext_Alloc(0xa00, 0, scores_cpp, 0xcd, 0x2001);
	self->orb_count = 0;
	self->input_map = ext_InputMapCreate(g_PlayerCount);
	for (i = 0; i < g_PlayerCount; i++)
	{
		uint32_t port = (uint32_t)g_PlayerPort(i);

		self->speed_buttons[i] = ext_InputMapAdd(self->input_map, "Speed", 1, 0, 0, 0, 0);
		ext_InputBind(self->speed_buttons[i], port | 0xc5, 0);
	}
	if (ext_FindResource(0, (const char *)(DAT_002900c8 + g_PlayerCharacter(0) * 0x108), 1, 0) != NULL)
		self->pkg_charsel = NULL;
	else
		self->pkg_charsel = ext_LoadPackageAsync("Packages\\CharacterSel", 0);
	self->pkg_scores = ext_LoadPackageAsync("Packages\\ScoreScreen", 0);
	self->base.state = 2;
	ext_LightInit(self->light_a, 0);
	ext_LightSpot(self->light_a, (const float *)CORE_ADDR(0x00283b98), (const float *)CORE_ADDR(0x00283ba8),
		F(0x00283b94), F(0x00283b90));
	ext_LightColour(self->light_a, CORE_VAR(int, 0x00283b84), CORE_VAR(int, 0x00283b88), CORE_VAR(int, 0x00283b8c));
	ext_LightInit(self->light_b, 0);
	ext_LightSpot(self->light_b, (const float *)CORE_ADDR(0x00283bcc), (const float *)CORE_ADDR(0x00283bdc),
		F(0x00283bc8), F(0x00283bc4));
	ext_LightColour(self->light_b, CORE_VAR(int, 0x00283bb8), CORE_VAR(int, 0x00283bbc), CORE_VAR(int, 0x00283bc0));
	ext_LightInit(self->light_c, 4);
	ext_LightDirection(self->light_c, (const float *)CORE_ADDR(0x00283bec));
	g_ClearRed = CORE_VAR(int, 0x002844cc);
	g_ClearGreen = CORE_VAR(int, 0x002844d0);
	g_ClearBlue = CORE_VAR(int, 0x002844d4);
	g_ClearAlpha = CORE_VAR(int, 0x002844d8);
	self->font = ext_FindResource(0, "handgotb.ttf", 3, 0xdefa);
	self->models[0] = NULL;
	self->models[1] = NULL;
	self->models[2] = NULL;
	self->models[3] = NULL;
	g_FlagBAE9C = 3;
	self->totals[0] = g_ScoreTable(g_Round)[0];
	g_ScoreFade = 1;
	self->totals[1] = g_ScoreTable(g_Round)[1];
	self->totals[2] = g_ScoreTable(g_Round)[2];
	self->totals[3] = g_ScoreTable(g_Round)[3];
	((void (*)(int *, int *, int *))FUN_0001c200)(g_ScoreTable(g_Round), self->ranks, self->order);
	CGameMode_Scores_ComputePoints(&self->winner, self->points, 0);
	((void (*)(void *, int *))FUN_0001c0f0)(DAT_002b9968, self->points);
}

/* 0x00029c10  Core/CGameMode_Scores.cpp  process (vtable +0x18): voices, then the orbs fly
(phase 1), are counted into the totals (phase 3), and the ranks move (phase 4) */
void CGameMode_Scores_Process(struct CGameMode_Scores *self, int top)
{
	int i;

	if (DAT_002ecdec != 0 || top == 0)
		return;
	g_ScoreFade = 0;
	if (self->base.state != 0)
	{
		((void (*)(void *))self->base.vtbl[9])(self);
		return;
	}
	if (self->speed_up != 0)
		DAT_0028fda8 = DAT_0028fda8 * self->speed_factor;
	self->timer = DAT_0028fda8 + self->timer;
	if (g_TeamMode == 0 && g_ScorePhase <= 0
		&& g_PlayerScore(g_ScoreOrder[0]) > (int)((double)g_PlayerScore(g_ScoreOrder[1]) * 2.0)
		&& self->voice == -2 && (self->voice2 == -1 || !ext_SoundPlaying(self->voice2)))
		self->voice = ext_PlayVoice(g_PlayerCharacter(g_ScoreOrder[0]) + 0x1f, -1, 0xff, 0, 0, 0);
	if (self->voice_delay != 0.0f)
	{
		self->voice_delay = self->voice_delay - DAT_0028fda8;
		if (self->voice_delay <= 0.0f)
		{
			ext_PlayCharacterVoice((unsigned char)g_PlayerCharacter(g_TeamPlayer(self->winner, 1)),
				4, 0, 0xff, 0, 1, 0);
			self->voice_delay = 0.0f;
		}
	}
	switch (self->phase)
	{
	case 0:
		if (self->voice < 0 || !ext_SoundPlaying(self->voice))
		{
			if (self->voice != -2)
			{
				self->voice = -2;
				if (self->winner != -1)
				{
					if (g_TeamMode != 0)
					{
						self->voice2 = ext_PlayCharacterVoice(
							(unsigned char)g_PlayerCharacter(g_TeamPlayer(self->winner, 0)), 4, 0, 0xff, 0, 1, 0);
						self->voice_delay = 60.0f;
					}
					else
						self->voice2 = ext_PlayCharacterVoice(
							(unsigned char)g_PlayerCharacter(self->winner), 9, 0, 0xff, 0, -1, 0);
				}
			}
		}
		if (self->voice != -2)
			break;
		self->voice = -2;
		if (self->voice2 >= 0)
		{
			if (!ext_SoundPlaying(self->voice2))
				self->voice2 = -1;
		}
		else if (self->timer >= 100.0f)
		{
			self->phase = 1;
			self->timer = 0.0f;
		}
		break;
	case 1:
		if (g_ScorePhase < 0)
		{
			self->phase = 3;
			self->timer = -F(0x00283c20);
			g_ScorePhase = 3;
			break;
		}
		if (self->timer >= 30.0f)
		{
			int row = g_ScorePhase;
			int player = self->order[row];
			int missing = g_ScoreTable(g_Round)[player] - self->totals[player];
			float y = (float)(90.0 - (double)row * 60.0);

			for (i = 0; i < self->totals[player]; i++)
				CGameMode_Scores_AddOrb(self, player,
					(float)((double)i * (double)F(0x00283c3c) + (double)F(0x00283c34)), y);
			for (i = 0; i < missing; i++)
				self->new_orbs[player][i] = CGameMode_Scores_AddOrb(self, player,
					(float)((double)F(0x00283c38) - (double)i * (double)F(0x00283c3c)), y);
			ext_PlaySfx(0xb, 0x80, 32000, 0, 0);
			*(int *)(self->name_text[player] + 0x2f4) = 1;
			*(int *)(self->score_text[player] + 0x2f4) = 1;
			g_ScorePhase--;
			self->timer = 0.0f;
		}
		break;
	case 3:
		if (self->timer >= 15.0f)
		{
			int moved = 0;

			for (g_ScorePhase = 0; g_ScorePhase < 4; g_ScorePhase++)
			{
				int player = self->order[g_ScorePhase];
				int total = self->totals[player];
				int target = g_ScoreTable(g_Round)[player];

				*(uint32_t *)(self->score_text[player] + 0x2b8) = 0x42fe0000;  /* 127.0f */
				if (total != target)
				{
					self->new_orbs[player][target - total - 1]->floor_x =
						(float)((double)total * (double)F(0x00283c3c) + (double)F(0x00283c34));
					self->totals[player] = total + 1;
					msvc_swprintf((unsigned short *)(self->score_text[player] + 0xa4), u"%d", total + 1);
					moved++;
				}
			}
			if (moved == 0)
			{
				int old_ranks[4];
				int changed = 0;
				void *memory;

				memcpy(old_ranks, self->ranks, sizeof(old_ranks));
				((void (*)(int *, int *, int *))FUN_0001c200)(g_ScoreTable(g_Round), self->ranks, self->order);
				for (i = 0; i < 4; i++)
				{
					int rank = self->ranks[i];

					if (old_ranks[i] != rank)
					{
						int distance = old_ranks[i] - rank;
						float speed, y, y2;
						struct ScoreOrb *orb;
						int k;

						if (distance < 0)
							distance = -distance;
						speed = (float)((double)distance * (double)F(0x00283c40));
						changed = 1;
						y = (float)(120.0 - (double)(rank * 0x3c));
						y2 = (float)(90.0 - (double)(rank * 0x3c));
						*(float *)(self->name_text[i] + 0x68) = y;
						*(int *)(self->name_text[i] + 0x2f0) = 1;
						*(float *)(self->name_text[i] + 0x2e0) = speed;
						*(float *)(self->score_text[i] + 0x68) = y2;
						*(int *)(self->score_text[i] + 0x2f0) = 1;
						*(float *)(self->score_text[i] + 0x2e0) = speed;
						for (k = 0, orb = self->orbs; k < self->orb_count; k++, orb++)
						{
							if (orb->player == i)
							{
								orb->target_y = y2;
								orb->y_speed = speed;
							}
						}
					}
				}
				if (changed)
					ext_PlaySample(self->snd_move, 0xff, 0, 0, 0);
				self->phase = 4;
				memory = ext_New(0x90, scores_cpp, 0x3c6, 0x1001);
				self->popup = memory ? ((void *(*)(void *, int, int, void *, void *))FUN_00011c50)(memory,
					-260, -180,
					*(void **)(*(unsigned char **)((unsigned char *)DAT_002ecc74 + 0x20) + 0x10d0),
					self->font) : NULL;
				*(uint32_t *)((unsigned char *)self->popup + 0x7c) = 0x44020000;  /* 520.0f */
				*(uint32_t *)((unsigned char *)self->popup + 0x8c) |= 0x30;
				*(float *)((unsigned char *)self->popup + 0x34) = 0.75f;
				((void (*)(void *, float))FUN_000114b0)(self->popup, 0.0f);
				((void (*)(void *, float))(*self->popup)[4])(self->popup, 30.0f);
			}
			self->timer = 0.0f;
		}
		break;
	default:
		break;
	}

	ext_TextUpdateAll();
	for (i = 0; i < self->orb_count; i++)
	{
		struct ScoreOrb *orb = &self->orbs[i];
		unsigned char *actor = orb->actor;
		double stretch;
		float step, d;

		ext_ActorUpdate(actor, *(void **)(actor + 0x134), DAT_0028fda8 * 60.0f, 0);
		*(float *)(actor + 0xc0) = orb->x;
		*(float *)(actor + 0xc4) = orb->y;
		stretch = fabs((double)orb->velocity) * (double)F(0x00283c2c) + 1.0;
		*(float *)(actor + 0xd0) = (float)((double)F(0x00283c30) * stretch);
		*(float *)(actor + 0xd4) = (float)((double)F(0x00283c30) / stretch);
		orb->velocity = (float)((double)F(0x00283c24) * (double)DAT_0028fda8 + (double)orb->velocity);
		orb->x = (float)((double)orb->velocity * (double)DAT_0028fda8 + (double)orb->x);
		if (orb->x < orb->floor_x)
		{
			/* bounce */
			float bounce = (float)((double)F(0x00283c28) * (double)orb->velocity);

			orb->x = orb->floor_x;
			orb->velocity = bounce;
			if (bounce < 0.5f)
				orb->velocity = 0.0f;
			else
			{
				double strength = fabs((double)bounce);
				int volume = (int)(strength * 40.0);

				if (volume > 0x80)
					volume = 0x80;
				ext_PlaySample(self->snd_orb, volume, 21000 - (int)(strength * 1600.0), 0, 0);
			}
		}
		/* slide toward the target row */
		step = DAT_0028fda8 * orb->y_speed;
		d = orb->y - orb->target_y;
		if (d < -step)
			orb->y = step + orb->y;
		else if (d <= step)
			orb->y = orb->target_y;
		else
			orb->y = orb->y - step;
	}
	if (self->popup != NULL)
		((void (*)(void *))(*self->popup)[2])(self->popup);
	if (self->speed_up != 0)
		DAT_0028fda8 = DAT_0028fda8 / self->speed_factor;
	for (i = 0; i < 4; i++)
	{
		if (self->models[i] != NULL)
			ext_ActorUpdate(self->actors[i], *(void **)(self->actors[i] + 0x134), DAT_0028fda8, 0);
	}
	if (self->shown > 1)
	{
		double angle;

		CGameMode_Scores_PlaceRing(self, (double)self->ring_angle);
		angle = (double)F(0x00283c50) * (double)DAT_0028fda8 + (double)self->ring_angle;
		self->ring_angle = (float)angle;
		if (angle > (double)6.2831855f)
			self->ring_angle = (float)(angle - (double)6.2831855f);
		else if (angle < 0.0)
			self->ring_angle = (float)(angle + (double)6.2831855f);
	}
}

/* 0x002565d0: the CGameMode_Scores vtable */
const void *const vtable_002565d0[14] = {
	(const void *)CGameMode_Scores_Destroy,  /* [0] */
	(const void *)FUN_001d5950,               /* [1] suspend: nothing */
	(const void *)FUN_001d5950,               /* [2] resume: nothing */
	(const void *)CGameMode_Scores_Popped,    /* [3] */
	(const void *)CGameMode_Scores_Pushed,    /* [4] */
	(const void *)CGameMode_Scores_Update,    /* [5] */
	(const void *)CGameMode_Scores_Process,   /* [6] */
	(const void *)CGameMode_Scores_Draw,      /* [7] */
	(const void *)FUN_001ec260,               /* [8] */
	(const void *)FUN_00012930,               /* [9] while loading */
	(const void *)FUN_000128e0,               /* [10] */
	(const void *)CGameMode_Scores_Loaded,    /* [11] */
	(const void *)FUN_001d5950,               /* [12] nothing */
	(const void *)CGameMode_Scores_Button,    /* [13] a button pressed or repeated */
};
