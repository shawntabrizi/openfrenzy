/*
Rowing/unplaced_hud.c  <-  Rowing gap 0x001aff40-0x001b10a0 (no file tag)

The placings and HUD object (game +0x8c, 0xf8 bytes): it ranks the players every frame by
the variant's measure (+0x104 of each player) and feeds the shared HUD (DAT_002ecfac,
classlib_ui CLHud_*) with the clock, the bars and the scores (the player records'
score, DAT_002baed4[i * 10 + 5]).

The object (fields at original offsets): +0x00 the players by place [4], +0x10 the variant
- 1, +0x14 the font, +0x18 the score labels [4] (variant 2), +0x28 the gate dots [4][8],
+0xc8 the token scores [4] (variant 3), +0xd8 the clock scores [4], +0xe8 the times in
tenths [4] (variant 2).
"Variant N" is the game's variant N (1-4).
*/

#include "rowing_a.h"
#include "rowing_a_ext.h"

#define HD_I32(off) RA_I32(self, off)
#define HD_PTR(off) RA_PTRF(self, off)

#define g_RowGameFlags RW_VAR(unsigned int, 0x0030f0b0)
#define g_GameState RW_VAR(int, 0x0030f0b4)
#define g_RowWorld RW_VAR(void *, 0x0030f12c)
#define g_RowPlayer(i) RW_VAR(void *, 0x0030f490 + (i) * 4)
#define g_TimeLeft RW_VAR(float, 0x0030f0e4)  /* game +0x34: the time left in ms */
#define g_ShowClock RW_VAR(int, 0x002b5488)
#define HUD ((int)DAT_002ecfac)
#define HUD_F32(off) RA_F32((void *)(uintptr_t)DAT_002ecfac, off)
#define PLAYER_SCORE(i) DAT_002baed4[(i) * 10 + 5]

extern int DAT_002baebc;               /* core: team game */
extern unsigned char DAT_00290718[];   /* classlib_ui: the character colours, 0x60 each */

/* the clock in seconds, or 0 when the clock is off */
static float ra_ClockSeconds(void)
{
	return g_ShowClock != 0 ? g_TimeLeft * 0.00100000005f : 0.0f;
}

static float ra_Clamp01(float v)
{
	if (v < 0.0f)
		return 0.0f;
	if (v > 1.0f)
		return 1.0f;
	return v;
}

/* 0x001aff40  Rowing (gap)
The constructor: all zero, the variant VARIANT. */
void *FUN_001aff40(void *self, int variant)
{
	memset(self, 0, 0xf8);
	HD_I32(0x10) = variant;
	return self;
}

/* 0x001aff60  Rowing (gap)
Ranks the players. The measure (+0x104): variant 1 laps and checkpoints (+1000 - 100 x
place once finished; at least 12); variant 2 minus the time with penalties / 1000, minus
0.1 when not finished and per gate not passed; variant 3 the tokens; variant 4 the time
afloat (or the sinking time), also kept as the score (+0x3a0). Sorts them (+0x00 the order,
+0xe0 and +0xe4 the place) and in variant 1 gives equal places within 0.01. */
void FUN_001aff60(void *self)
{
	float s[4], t, f;
	int order[4], i, j, n, k;
	void *p;

	for (i = 0; i < DAT_002bae94; i++) {
		p = g_RowPlayer(i);
		/* the original leaves s[i] unset for players in the results states (2, 3) and
		   reads stack garbage; the port keeps their last measure */
		s[i] = RA_F32(p, 0x104);
		switch (HD_I32(0x10)) {
		case 0:
			if (RA_I32(p, 0x380) == 2 || RA_I32(p, 0x380) == 3)
				break;
			k = RA_I32(p, 0xf4) * RA_I32(g_RowWorld, 0x144) + RA_I32(p, 0xf8);
			if (RA_I32(p, 0xe8) != 0)
				k += (10 - RA_I32(p, 0xe8)) * 100;
			s[i] = (float)k + RA_F32(p, 0xfc);
			if (s[i] < 12.0f)
				s[i] = 12.0f;
			RA_F32(p, 0x104) = s[i];
			break;
		case 1:
			if (RA_I32(p, 0x380) == 2 || RA_I32(p, 0x380) == 3)
				break;
			t = RA_F32(p, 0x128) * -0.00100000005f;
			s[i] = t;
			if (RA_I32(p, 0xe8) == 0)
				s[i] = t - 0.100000001f;
			for (k = 0; k < RA_I32(g_RowWorld, 0x19c); k++)
				if (RA_U8(p, 0x110 + k) == 0)
					s[i] -= 0.100000001f;
			break;
		case 2:
			s[i] = (float)RA_I32(p, 0x3a0);
			break;
		case 3:
			if (RA_I32(p, 0x108) == 0) {
				f = ra_Clamp01((RA_F32(p, 0x2bc) - -1860.0f) * 0.000529100536f);
				s[i] = (float)RA_I32(p, 0x39c) + f * 1000.0f;
			} else
				s[i] = (float)RA_I32(p, 0x108);
			RA_I32(p, 0x3a0) = (int)s[i];
			break;
		}
		RA_F32(g_RowPlayer(i), 0x104) = s[i];
	}
	n = DAT_002bae94;
	for (i = 0; i < n; i++)
		order[i] = i;
	/* the original reads uninitialised order[] entries below in variant 1 with fewer than
	   four players; the port fills them with their index */
	for (i = n; i < 4; i++)
		order[i] = i;
	for (i = 0; i < n; i++) {
		for (j = i + 1; j < n; j++) {
			if (s[j] > s[i]) {
				t = s[j];
				s[j] = s[i];
				s[i] = t;
				k = order[j];
				order[j] = order[i];
				order[i] = k;
			}
		}
	}
	for (i = 0; i < DAT_002bae94; i++) {
		HD_I32(i * 4) = order[i];
		RA_I32(g_RowPlayer(order[i]), 0xe0) = i;
		RA_I32(g_RowPlayer(order[i]), 0xe4) = i;
	}
	if ((g_RowGameFlags & 0xc) == 0) {
		for (i = 0; i < 3; i++)
			if (RA_F32(g_RowPlayer(order[i]), 0x104) - RA_F32(g_RowPlayer(order[i + 1]), 0x104) < 0.00999999978f)
				RA_I32(g_RowPlayer(order[i + 1]), 0xe4) = RA_I32(g_RowPlayer(order[i]), 0xe4);
	}
}

/* 0x001b03c0  Rowing (gap)
Variant 1's HUD: the clock, each racer's bar (laps done beyond the first, out of 3) and
score (5 - place, or 10 - rank once finished). SELF is not read. */
void FUN_001b03c0(void *self)
{
	int i, n;
	void *p;

	(void)self;
	CLHud_SetClock(HUD, ra_ClockSeconds(), 1);
	for (i = 0; i < DAT_002bae94; i++) {
		p = g_RowPlayer(i);
		if (RA_I32(p, 0x380) == 2 || RA_I32(p, 0x380) == 3)
			continue;
		n = RA_I32(g_RowWorld, 0x144);
		HUD_F32(0x4c + i * 4) = ra_Clamp01((RA_F32(p, 0x104) - (float)n) / (float)(n * 3));
		p = g_RowPlayer(i);
		if (RA_I32(p, 0xe8) == 0)
			PLAYER_SCORE(i) = 5 - RA_I32(p, 0xe4);
		else
			PLAYER_SCORE(i) = 10 - RA_I32(p, 0xec);
	}
	CLHud_UpdateBars(HUD);
}

/* 0x001b04e0  Rowing (gap)
Variant 2's HUD: each player's projected time (+0x12c: the time so far over the fraction
of the course done, 0.01-1; the final time once finished) sets its bar (the best time full,
x0.8, scaled by the fraction) and score (the tenths of the time relative to the worst). */
void FUN_001b04e0(void *self)
{
	float frac[4], best = 1.0e8f, worst = 0.0f, v, f;
	int i, n, base;
	void *p;

	for (i = 0; i < DAT_002bae94; i++) {
		p = g_RowPlayer(i);
		if (RA_I32(p, 0xe8) != 0) {
			f = 1.0f;
		} else {
			n = RA_I32(g_RowWorld, 0x144);
			f = (((float)(RA_I32(p, 0xf4) * n + RA_I32(p, 0xf8)) + RA_F32(p, 0xfc)) - (float)n) / (float)n;
			if (!(f > 0.0f))
				f = 0.00999999978f;
			else if (f > 1.0f)
				f = 1.0f;
		}
		frac[i] = f;
		if (RA_I32(p, 0xe8) != 0)
			RA_F32(p, 0x12c) = RA_F32(p, 0x128);
		else
			RA_F32(p, 0x12c) = RA_F32(p, 0x128) / f;
		p = g_RowPlayer(i);
		if (RA_F32(p, 0x12c) < best)
			best = RA_F32(p, 0x12c);
		if (worst < RA_F32(p, 0x12c))
			worst = RA_F32(p, 0x12c);
	}
	if (DAT_002bae94 > 0 && worst == best)
		worst += 1.0f;
	if (worst < best + best)
		worst = best + best;
	if (worst < 90.0f)
		worst = 90.0f;
	for (i = 0; i < DAT_002bae94; i++) {
		v = (RA_F32(g_RowPlayer(i), 0x12c) - worst) / (best - worst) * 0.800000012f;
		if (v < 0.0f)
			v = 0.0f;
		HUD_F32(0x4c + i * 4) = v * frac[i];
	}
	base = 500 - (int)(worst * -0.00999999978f);
	for (i = 0; i < DAT_002bae94; i++) {
		p = g_RowPlayer(i);
		if (RA_I32(p, 0x380) == 2 || RA_I32(p, 0x380) == 3)
			continue;
		HD_I32(0xe8 + i * 4) = (int)(RA_F32(p, 0x128) * 0.00999999978f);
		if (RA_I32(p, 0xe8) != 0)
			n = (int)(RA_F32(p, 0x128) * 0.00999999978f);
		else
			n = base - (int)(RA_F32(p, 0x12c) * -0.00999999978f);
		PLAYER_SCORE(i) = base * 2 - n;
	}
	CLHud_SetClock(HUD, ra_ClockSeconds(), 1);
	CLHud_UpdateBars(HUD);
}

/* 0x001b0790  Rowing (gap)
Variant 3's HUD: the clock, each player's tokens as bar (x0.1; the HUD's +0x40 the highest)
and score. */
void FUN_001b0790(void *self)
{
	int i;
	float v;

	CLHud_SetClock(HUD, g_TimeLeft * 0.00100000005f, 1);
	for (i = 0; i < DAT_002bae94; i++) {
		HUD_F32(0x4c + i * 4) = (float)RA_I32(g_RowPlayer(i), 0x3a0) * 0.100000001f;
		v = (float)RA_I32(g_RowPlayer(i), 0x3a0) * 0.100000001f;
		if (v > HUD_F32(0x40))
			HUD_F32(0x40) = v;
		HD_I32(0xc8 + i * 4) = RA_I32(g_RowPlayer(i), 0x3a0);
		PLAYER_SCORE(i) = RA_I32(g_RowPlayer(i), 0x3a0);
	}
	CLHud_SetScores(HUD, (int *)RA_P(self, 0xc8));
	CLHud_UpdateBars(HUD);
}

/* 0x001b0880  Rowing (gap)
Variant 4's HUD: each boat's bar is how far down the course it is (from z -1860 over 1890);
its score its sinking time, or player 0's frame count + 500 (+300 when out or sunk); the
clock; a hook (0x001d5950, empty) in the last 10 ms of the race. */
void FUN_001b0880(void *self)
{
	float v;
	int i, score;
	void *p;

	for (i = 0; i < DAT_002bae94; i++) {
		p = g_RowPlayer(i);
		if (RA_U8(p, 0x34a) != 0) {
			v = 0.0f;
		} else {
			v = (RA_F32(p, 0x2bc) - -1860.0f) * 0.000529100536f;
			if (!(v > 0.0f))
				v = 0.0f;
			else if (v > 1.0f)
				v = 1.0f;
		}
		HUD_F32(0x4c + i * 4) = v;
		HD_I32(0xc8 + i * 4) = 0;
		p = g_RowPlayer(i);
		score = RA_I32(p, 0x108);
		if (score == 0) {
			if (RA_U8(p, 0x37c) != 0 && RA_U8(p, 0x349) == 0)
				score = RA_I32(g_RowPlayer(0), 0x39c) + 500;
			else
				score = RA_I32(g_RowPlayer(0), 0x39c) + 300;
		}
		PLAYER_SCORE(i) = score;
	}
	CLHud_SetClock(HUD, ra_ClockSeconds(), 1);
	CLHud_UpdateBars(HUD);
	if (g_ShowClock != 0 && g_TimeLeft < 10.0f && g_GameState == 5)
		FUN_001d5950();
}

/* a HUD element descriptor for COverlay_CreateChild (0x3b0 bytes) */
#define DESC_U32(off) RA_U32(desc, off)
#define DESC_F32(off) RA_F32(desc, off)

/* 0x001b09f0  Rowing (gap)
Creates variant 2's HUD elements: per player a score label in the character's colour and
eight gate dots ("tex_special\panel_circle.bmp", 14 apart from x 9). */
void FUN_001b09f0(void *self)
{
	unsigned int desc[0xec];
	void *dot;
	float x;
	int i, k, character;

	memset(desc, 0, sizeof(desc));
	HD_PTR(0x14) = bLoadResource(NULL, "handgotb.ttf", 3, 0xdefa);
	dot = bLoadResource(NULL, "tex_special\\panel_circle.bmp", 0, 0xdefa);
	for (i = 0; i < 4; i++) {
		memset(desc, 0, sizeof(desc));
		RA_PTRF(desc, 0x44) = HD_PTR(0x14);
		character = DAT_002baed4[i * 10];
		DESC_F32(0x250) = (float)DAT_00290718[character * 0x60];
		DESC_F32(0x254) = (float)DAT_00290718[character * 0x60 + 1];
		DESC_U32(0x288) = 2;
		DESC_F32(0x258) = (float)DAT_00290718[character * 0x60 + 2];
		DESC_F32(0x2b4) = 125.0f;
		DESC_F32(0x2b8) = 17.0f;
		DESC_F32(0x25c) = 75.0f;
		RA_U16(desc, 0x24c) = 2;
		DESC_F32(0x30c) = 200.0f;
		bkStringToWide((unsigned short *)RA_P(desc, 0x48), "");
		DESC_U32(0x294) |= 2;
		DESC_U32(0x290) = 0x8000;
		DESC_F32(0x320) = 1.0f;
		DESC_F32(0x318) = 0.699999988f;
		HD_I32(0x18 + i * 4) = COverlay_CreateChild(RA_I32((void *)(uintptr_t)DAT_002ecfac, 0xf0 + i * 4),
			(undefined4 *)desc);
		x = 9.0f;
		for (k = 0; k < 8; k++) {
			memset(desc, 0, sizeof(desc));
			DESC_F32(0x2b4) = x;
			DESC_U32(0x288) = 0;
			RA_PTRF(desc, 0x4) = dot;
			DESC_F32(0x250) = 32.0f;
			DESC_F32(0x254) = 32.0f;
			DESC_F32(0x258) = 32.0f;
			DESC_F32(0x25c) = 32.0f;
			DESC_F32(0x2b8) = 46.0f;
			DESC_U32(0x290) = 0x80;
			DESC_F32(0x308) = 16.0f;
			DESC_F32(0x30c) = 14.0f;
			HD_I32(0x28 + (i * 8 + k) * 4) = COverlay_CreateChild(
				RA_I32((void *)(uintptr_t)DAT_002ecfac, 0xf0 + i * 4), (undefined4 *)desc);
			x += 14.0f;
		}
	}
}

/* 0x001b0c40  Rowing (gap)
Updates variant 2's HUD elements: each label shows the time in seconds ("%4.1f") with the
player's HUD fade (HUD +0x98) as alpha; each gate dot is grey (not reached), green
(passed) or red (missed), faded the same way. */
void FUN_001b0c40(void *self)
{
	char text[64];
	int c[4], i, k;
	float fade;
	void *label, *dot;

	c[0] = c[1] = c[2] = c[3] = 0; /* the original starts from stack garbage (marks are 0-2) */
	for (i = 0; i < DAT_002bae94; i++) {
		fade = HUD_F32(0x98 + i * 4);
		label = (void *)(uintptr_t)HD_I32(0x18 + i * 4);
		RA_F32(label, 0x2b8) = fade * 127.0f;
		sprintf(text, "%4.1f", (double)((float)HD_I32(0xe8 + i * 4) * 0.100000001f));
		bkStringToWide((unsigned short *)RA_P(label, 0xa4), text);
		for (k = 0; k < 8; k++) {
			switch ((signed char)RA_U8(g_RowPlayer(i), 0x110 + k)) {
			case 0:
				c[0] = 0x40;
				c[1] = 0x40;
				c[2] = 0x40;
				c[3] = 0x7f;
				break;
			case 1:
				c[0] = 0;
				c[1] = 0x7f;
				c[2] = 0;
				c[3] = 0x7f;
				break;
			case 2:
				c[0] = 0x7f;
				c[1] = 0;
				c[2] = 0;
				c[3] = 0x7f;
				break;
			}
			fade = HUD_F32(0x98 + i * 4);
			c[0] = (int)((float)c[0] * fade);
			c[1] = (int)((float)c[1] * fade);
			c[2] = (int)((float)c[2] * fade);
			dot = (void *)(uintptr_t)HD_I32(0x28 + (i * 8 + k) * 4);
			RA_F32(dot, 0x2ac) = (float)c[0];
			RA_F32(dot, 0x2b0) = (float)c[1];
			RA_F32(dot, 0x2b4) = (float)c[2];
			RA_F32(dot, 0x2b8) = (float)c[3];
		}
	}
}

/* 0x001b0e00  Rowing (gap)
Variant 1's lap counters: laps done out of 3 (4 once finished). */
void FUN_001b0e00(void *self)
{
	int i;

	(void)self;
	for (i = 0; i < DAT_002bae94; i++) {
		if (RA_I32(g_RowPlayer(i), 0xe8) != 0)
			CLHud_SetFraction(HUD, i, 4, 3);
		else
			CLHud_SetFraction(HUD, i, RA_I32(g_RowPlayer(i), 0xf4), 3);
	}
}

/* 0x001b0e50  Rowing (gap)
Marks the winners (+0x3b8): the players of the higher-scoring team (both on a draw), or the
players with the best score. */
void FUN_001b0e50(void *self)
{
	int team[2] = {0, 0}, win, best, i;

	(void)self;
	if (DAT_002baebc != 0) {
		win = -1;
		if (DAT_002bae94 > 0) {
			for (i = 0; i < DAT_002bae94; i++)
				team[DAT_002baed4[i * 10 + 8]] += PLAYER_SCORE(i);
			if (team[0] != team[1])
				win = team[1] > team[0];
		}
		for (i = 0; i < DAT_002bae94; i++)
			RA_I32(g_RowPlayer(i), 0x3b8) = (win == -1 || DAT_002baed4[i * 10 + 8] == win) ? 1 : 0;
		return;
	}
	best = PLAYER_SCORE(0);
	for (i = 1; i < DAT_002bae94; i++)
		if (PLAYER_SCORE(i) > best)
			best = PLAYER_SCORE(i);
	for (i = 0; i < DAT_002bae94; i++)
		RA_I32(g_RowPlayer(i), 0x3b8) = PLAYER_SCORE(i) == best ? 1 : 0;
}

/* 0x001b0f70  Rowing (gap)
Sets the HUD up for the variant: score boxes, bars, clock scores, the clock (.data
0x002b548c-0x002b5498 by variant: 90, 90, 60, 90 s) and its countdown; variant 1 adds empty
labels, variant 2 its gate elements, variant 3 the score texts. */
void FUN_001b0f70(void *self)
{
	CLHud_CreateScoreBoxes(HUD);
	CLHud_CreateBars(HUD);
	HD_I32(0xe4) = 0;
	HD_I32(0xe0) = 0;
	HD_I32(0xdc) = 0;
	HD_I32(0xd8) = 0;
	CLHud_CopyScores(HUD, (undefined4 *)RA_P(self, 0xd8));
	HUD_F32(0x40) = 1.0f;
	RA_I32((void *)(uintptr_t)DAT_002ecfac, 0x3c) = 0;
	/* CLHud_StopCountdown (0x00078930) is called with 10.0 pushed; it reads no argument */
	switch (HD_I32(0x10)) {
	case 0:
		CLHud_CreateClock(HUD, RW_VAR(float, 0x002b548c));
		CLHud_StopCountdown(HUD);
		CLHud_CreateScoreLabels(HUD, "", 1);
		break;
	case 1:
		CLHud_CreateClock(HUD, RW_VAR(float, 0x002b5490));
		CLHud_StopCountdown(HUD);
		FUN_001b09f0(self);
		break;
	case 2:
		CLHud_CreateScoreTexts(HUD, (int *)RA_P(self, 0xc8));
		CLHud_CreateClock(HUD, RW_VAR(float, 0x002b5494));
		CLHud_StopCountdown(HUD);
		break;
	default:
		CLHud_CreateClock(HUD, RW_VAR(float, 0x002b5498));
		CLHud_StopCountdown(HUD);
		break;
	}
	HUD_F32(0xb4) = 100.0f;
	HUD_F32(0xb0) = 100.0f;
	HUD_F32(0xac) = 100.0f;
	HUD_F32(0xa8) = 100.0f;
}

/* 0x001b10a0  Rowing (gap)
The frame: ranks the players and updates the variant's HUD. DT is not read. */
void FUN_001b10a0(void *self, float dt)
{
	(void)dt;
	FUN_001aff60(self);
	switch (HD_I32(0x10)) {
	case 0:
		FUN_001b03c0(self);
		FUN_001b0e00(self);
		break;
	case 1:
		FUN_001b04e0(self);
		FUN_001b0c40(self);
		break;
	case 2:
		FUN_001b0790(self);
		break;
	default:
		FUN_001b0880(self);
		break;
	}
}
