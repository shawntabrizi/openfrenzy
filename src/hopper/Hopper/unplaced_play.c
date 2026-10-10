/*
Hopper/unplaced_play.c  <-  gap functions 0x0014b100-0x0014c870 (no source path in the XBE)
(module hopper)

The play object (0x002b0038, vtable 0x0026f038): the clock, the HUD, the end of the round
(the winner pose and camera, then back to the board game), an fps/polygon counter
and a debug profile of the frame's parts.

The object (0x340 bytes):
  +0xa8 byte[4][4] the players' colours (r at +0xa8 + player, g +0xac, b +0xb0)
  +0xb8 int64 now   +0xc0 int64 the round's start (moved on while paused)
  +0xc8 int64 this frame   +0xd0 int64 the last frame
  +0xd8 int64 the play time left (hundredths; 6000 at the start; hop_TimeLeft)
  +0xe0 frames since the round ended   +0xe4 the end flash was made
  +0xe8 the screen darkening at the end (to 0x109)
  +0xf0 int64[16] profile section starts   +0x170 int64[16] their ends
  +0x1f0 char[16][20] section names        +0x330   +0x334 a darkening fading in
  +0x338 the number of winners
*/

#include "hopper.h"

extern long long DAT_002cd7a0;         /* bkernel: timer ticks per second */
extern int DAT_002bae98;               /* core: the game type (2: tournament) */
extern int DAT_002baea8;               /* core: variation 4 ends with every player out */
void DbgBreakPoint(void);              /* xkernel (kernel export 0x0003097d): logs and continues */

#define PLAY_I(self, off) HOP_AI(self, off)

static unsigned long long hop_Get64(void *self, int off)
{
	return ((unsigned long long)HOP_AU(self, off + 4) << 32) | HOP_AU(self, off);
}

static void hop_Set64(void *self, int off, unsigned long long v)
{
	HOP_AU(self, off) = (unsigned int)v;
	HOP_AU(self, off + 4) = (unsigned int)(v >> 32);
}

static int hop_WideLength(const char *s)
{
	return (int)strlen(s);
}

/* 0x0014b100  (gap): the play's start: 60 seconds, the players' colours, empty profile
names */
void HopPlay_Start(void *self)
{
	unsigned char *colour;
	int k;

	PLAY_I(self, 0xe0) = 0;
	PLAY_I(self, 0xd8) = 6000;
	PLAY_I(self, 0xdc) = 0;
	for (k = 0; k < 4; k++) {
		colour = DAT_00290718 + DAT_002baed4[k][0] * 0x60;
		HOP_AB(self, 0xa8 + k) = colour[0];
		HOP_AB(self, 0xac + k) = colour[1];
		HOP_AB(self, 0xb0 + k) = colour[2];
	}
	for (k = 0; k < 0x10; k++)
		((char *)self + 0x1f0 + k * 0x14)[0] = '\0';    /* sprintf(name, "") */
	PLAY_I(self, 0x330) = 0;
}

/* 0x0014b270  (gap): at the end of the round: did a human winner (winner[k] set) press
jump? When no human won, did any human press jump? */
int HopPlay_WinnerPressed(void *self, int *winner)
{
	int humanWinners = 0;
	int k;

	(void)self;
	for (k = 0; k < 4; k++) {
		if (DAT_002baed4[k][1] != 0 && HOP_AI(HOP_AP(hop_Players[k], 0x68), 0x14) != 0 && winner[k] != 0)
			return 1;
	}
	for (k = 0; k < 4; k++) {
		if (DAT_002baed4[k][1] != 0 && winner[k] != 0)
			humanWinners++;
	}
	if (humanWinners == 0) {
		for (k = 0; k < 4; k++) {
			if (DAT_002baed4[k][1] != 0 && HOP_AI(HOP_AP(hop_Players[k], 0x68), 0x14) != 0)
				return 1;
		}
	}
	return 0;
}

/* 0x0014b340  (gap): starts timing profile section `index` named `name` */
void HopPlay_ProfileBegin(void *self, int index, const char *name)
{
	hop_Set64(self, 0xf0 + index * 8, bkTimerRead());
	sprintf((char *)self + 0x1f0 + index * 0x14, "%s", name);
}

/* 0x0014b380  (gap): ends timing profile section `index` */
void HopPlay_ProfileEnd(void *self, int index)
{
	hop_Set64(self, 0x170 + index * 8, bkTimerRead());
}

/* 0x0014b3a0  (gap): the profile overlay (tweak "draw timers"): per section its name, its
share of section 0 ("all") as a percentage and a bar 300 wide */
void HopPlay_DrawProfile(void *self)
{
	unsigned short wide[20];
	char text[12];
	float *bar = (float *)HOP_ADDR(0x003044b8);
	float total, frac, w, y;
	unsigned int argb;
	char *name;
	int pct, i;

	if (HOP_F(0x002ae980) == 0.0f)
		return;
	if (bkTimerDelta(hop_Get64(self, 0xf0), hop_Get64(self, 0x170)) == 0)
		return;
	bdSetTexture(0, NULL);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 0, 0);
	bdSetRenderState(7, 1, 0);
	bdSetRenderState(3, 0, 0);
	bdSetProjectionMode(0);
	total = (float)(long long)bkTimerDelta(hop_Get64(self, 0xf0), hop_Get64(self, 0x170));
	for (i = 0; i < 0x10; i++) {
		name = (char *)self + 0x1f0 + i * 0x14;
		frac = (float)(long long)bkTimerDelta(hop_Get64(self, 0xf0 + i * 8), hop_Get64(self, 0x170 + i * 8)) / total;
		pct = HOP_FTOL(100.0f * frac);
		w = frac * 300.0f;
		bdSetWorldIdentity();
		bkStringToWide(wide, name);
		y = 105.0f - (float)i * 15.0f;
		bdPrintFontLite(HOP_G(0x002ae998, struct bdFont *), wide, hop_WideLength(name), -260.0f, y, 0x40, 0x7f, 0x40,
			0xdc, NULL, NULL);
		if (pct != 0) {
			sprintf(text, "%d %%", pct);
			bkStringToWide(wide, text);
			bdPrintFontLite(HOP_G(0x002ae998, struct bdFont *), wide, hop_WideLength(text), w - 140.0f, y, 0x40, 0x7f,
				0x40, 0xdc, NULL, NULL);
		}
		bdSetWorldIdentity();
		bdSetTexture(0, NULL);
		argb = HOP_ARGB(220, 127, 200, 127);
		bar[1] = 100.0f - (float)i * 15.0f;
		bar[9] = w - 150.0f;
		bar[0] = -150.0f;
		bar[10] = bar[1];
		bar[2] = 0.0f;
		bar[11] = 0.0f;
		bar[0x12] = -150.0f;
		bar[0x14] = 0.0f;
		bar[0x1d] = 0.0f;
		bar[0x13] = 92.0f - (float)i * 15.0f;
		bar[0x1c] = bar[0x13];
		bar[0x1b] = bar[9];
		HOP_AU(bar, 0x18) = argb;
		HOP_AU(bar, 0x3c) = argb;
		HOP_AU(bar, 0x60) = argb;
		HOP_AU(bar, 0x84) = argb;
		bdDrawPrimitiveFVF(4, bar, 4, 0x152);
	}
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetRenderState(7, 2, 0);
	bdSetRenderState(3, 1, 0);
	bdSetProjectionMode(1);
}

/* 0x0014b6c0  (gap): scalar deleting destructor */
void *HopPlay_Destroy(void *self, byte flags)
{
	*(const void *const **)self = vtable_0026f038;
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x0026f038  the play object's vtable */
const void *const vtable_0026f038[1] = {
	(const void *)FUN_0014b6c0, /* [0] scalar deleting destructor */
};

/* 0x0014b6e0  (gap): the HUD per frame: the clock waits until the play starts; the scores,
the clock (variations 1 and 2: the time left), the score bars' range, the projectors,
the overlays and the RefBot */
void HopPlay_UpdateHud(void *self)
{
	int hi = 0, lo = 0;
	int *score = HOP_IV(0x002aff24);
	int k;

	(void)self;
	if (hop_Play != 5) {
		hop_Set64(HOP_ADDR(0x002b0038), 0xb8, bkTimerRead());
		hop_Set64(HOP_ADDR(0x002b0038), 0xc0, hop_Get64(HOP_ADDR(0x002b0038), 0xb8));
	}
	CLHud_SetScores(DAT_002ecfac, score);
	if (DAT_002ecdec == 0)
		CLHud_SetClock(DAT_002ecfac, hop_Variation < 3 ? (float)HOP_I(0x002b0110) * 0.01f : -1.0f, 1);
	if (0 < score[0])
		hi = score[0];
	if (score[0] < 0)
		lo = score[0];
	for (k = 1; k < 4; k++) {
		if (hi < score[k])
			hi = score[k];
		if (score[k] < lo)
			lo = score[k];
	}
	CLHud_SetScoresInt(DAT_002ecfac, score);
	HOP_AF((void *)(uintptr_t)DAT_002ecfac, 0x40) = (float)hi;
	HOP_AF((void *)(uintptr_t)DAT_002ecfac, 0x3c) = (float)lo;
	if (DAT_002ecdec == 0) {
		CLHud_UpdateBars(DAT_002ecfac);
		if (DAT_002ecdec == 0)
			CProjector_UpdateAll();
	}
	COverlay_UpdateAll();
	bdSaveRenderStates((unsigned int *)HOP_ADDR(0x002aea48));
	if (1 < (signed char)hop_Play)
		RefBot_Update();
	bdRestoreRenderStates((unsigned int *)HOP_ADDR(0x002aea48));
}

/* the camera eye and target (camera +0x04, +0x14) */
#define hop_Eye HOP_FV(0x002aebdc)
#define hop_Target HOP_FV(0x002aebec)

/* 0x0014b830  (gap): the end of the round, per frame. The winners are the players with the
best score (variation 3: place). They pose in turn (state 12, variation 4 several winners:
13) while the camera looks at them; after 120 frames a human winner's jump (or a comment
ending) darkens the screen, then the scores go to core (a tournament with no board game
restarts the round instead) and the mode is popped. Each variation sets its winner camera;
variation 4 keeps its rules running. */
void HopPlay_Winner(void *self)
{
	int winners[4];
	int isWinner[4];
	unsigned int *flags;
	void *ai;
	void *actor;
	void *player;
	int best = 0, val, k, t, st;
	unsigned int winner = 0;
	float a, r, s, ex, ez, tx, tz;
	int *slot;

	PLAY_I(self, 0x338) = 0;
	for (k = 0; k < 4; k++) {
		isWinner[k] = 0;
		winners[k] = -1;
	}
	for (k = 0; k < 4; k++) {
		if (hop_Variation == 1) {
			ai = DAT_002f289c[k];
			if (ai != NULL && HOP_AP(ai, 0x30) != NULL) {
				if (HOP_AP(HOP_AP(ai, 0x30), 0x18c) != NULL)
					HOP_AI(HOP_AP(HOP_AP(ai, 0x30), 0x18c), 0x1ac) = 0;
				flags = &HOP_AU(HOP_AP(DAT_002f289c[k], 0x30), 0x220);
				*flags &= 0xffffffeeu;
			}
		}
		val = hop_Variation == 3 ? HOP_IV(0x002aff34)[k] : HOP_IV(0x002aff24)[k];
		if (best < val)
			best = val;
	}
	for (k = 0; k < 4; k++) {
		val = hop_Variation == 3 ? HOP_IV(0x002aff34)[k] : HOP_IV(0x002aff24)[k];
		if (val == best) {
			winners[PLAY_I(self, 0x338)] = k;
			PLAY_I(self, 0x338)++;
			isWinner[k] = 1;
		}
	}
	if (PLAY_I(self, 0x338) == 0)
		DbgBreakPoint();
	if (DAT_002ecdec == 0)
		PLAY_I(self, 0xe0)++;
	if (PLAY_I(self, 0xe4) == 0) {
		baAddExtraItem(HOP_G(0x002afff8, struct BExtraSystem *), 24.0);
		PLAY_I(self, 0xe4) = 1;
		PLAY_I(self, 0xe0) = 0;
	}
	if (PLAY_I(self, 0xe8) != 0) {
		if (DAT_002ecdec == 0)
			PLAY_I(self, 0xe8) += 0x20;
		if (0x108 < PLAY_I(self, 0xe8))
			PLAY_I(self, 0xe8) = 0x109;
		CLScreen_DrawDarken(PLAY_I(self, 0xe8), 1);
	}
	t = PLAY_I(self, 0xe0);
	if (0x78 < t && PLAY_I(self, 0xe8) == 0 &&
		(HopPlay_WinnerPressed(self, isWinner) != 0 ||
			(PLAY_I(self, 0x338) * 0xf0 < t && CLSamples_CommentaryIdle() != 0))) {
		CLHud_DeleteWinnerText(DAT_002ecfac);
		PLAY_I(self, 0xe8) = 1;
	}
	if (0x108 < PLAY_I(self, 0xe8)) {
		PLAY_I(self, 0xe8) = 0;
		PLAY_I(self, 0xe0) = 0;
		if (CGameModeStack_Find((struct CGameModeStack *)DAT_002ba750, "Meta") == NULL && DAT_002bae98 == 2) {
			HopGame_Restart(hop_Game);
			return;
		}
		for (k = 0; k < 4; k++)
			DAT_002baed4[k][5] = hop_Variation == 3 ? HOP_IV(0x002aff34)[k] : HOP_IV(0x002aff24)[k];
		CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
		HOP_I(0x003027b8) = 1;
		return;
	}
	if (HOP_B(0x003027b0) != 0) {
		for (k = 0; k < 4; k++)
			HOP_VFN(hop_Players[k], 0x40, void (*)(void *, int, int))(hop_Players[k], 0, -1);
	}
	for (k = 0; k < 4; k++) {
		if (k * 0xf0 <= PLAY_I(self, 0xe0))
			winner = (unsigned int)winners[k];
		if (1 < PLAY_I(self, 0x338))
			winner = (unsigned int)winners[0];
		if (k * 0xf0 == PLAY_I(self, 0xe0) && winners[k] != -1) {
			player = hop_Players[winners[k]];
			st = HOP_AS(player, 0x36);
			if (hop_Variation == 4 && st != 0xd) {
				if (1 < PLAY_I(self, 0x338))
					AI_SetState(player, 0xd);
				else if (st != 0xc)
					AI_SetState(player, 0xc);
			} else if (st != 0xc) {
				AI_SetState(player, 0xc);
			}
		}
	}
	if (winner == 0xffffffffu)
		winner = HOP_U(0x002b0030) == 0xffffffffu ? 0 : HOP_U(0x002b0030);
	else
		HOP_U(0x002b0030) = winner;
	switch (hop_Variation) {
	case 1:
		HOP_U(0x002af084) = 0x3fc90fdbu;    /* the propellers' angles (degrees) */
		HOP_F(0x002af088) = 0.0f;
		for (k = 0; k < 4; k++) {
			a = ((float)k * 90.0f + 45.0f) * HOP_DEG;
			actor = hop_PlayerActor(k);
			HOP_AF(actor, 0xd0) = sinf(a) * 600.0f;
			HOP_AF(hop_PlayerActor(k), 0xd8) = cosf(a) * 600.0f;
			HOP_AU(hop_PlayerActor(k), 0xd4) = 0xc306c831u;
			HOP_AF(hop_PlayerActor(k), 0x1d8) = a;
			hop_ActorSnap(hop_PlayerActor(k));
			HOP_AI(hop_PlayerActor(k), 0x19c) = 0;
			HOP_AI(hop_PlayerActor(k), 0x198) = 0;
			HOP_AI(hop_PlayerActor(k), 0x194) = 0;
		}
		player = hop_Players[winner];
		hop_Target[0] = HOP_AF(HOP_AP(player, 0x30), 0xd0);
		hop_Target[1] = HOP_AF(HOP_AP(player, 0x30), 0xd4) + 100.0f;
		hop_Target[2] = HOP_AF(HOP_AP(player, 0x30), 0xd8);
		hop_Eye[1] = 150.0f;
		a = ((float)(int)winner * 90.0f + 45.0f) * HOP_DEG;
		hop_Eye[0] = sinf(a) * 1000.0f;
		hop_Eye[2] = cosf(a) * 1000.0f;
		if (HOP_AS(player, 0x36) != 0xc && hop_RoundOver == 0)
			AI_SetState(player, 0xc);
		break;
	case 2:
		for (k = 0; k < 4; k++) {
			if (HOP_AF(hop_PlayerActor(k), 0xd4) < 0.0f) {
				a = ((float)k * 90.0f + 45.0f) * HOP_DEG;
				HOP_AF(hop_PlayerActor(k), 0xd0) = sinf(a) * 100.0f;
				HOP_AF(hop_PlayerActor(k), 0xd8) = cosf(a) * 100.0f;
				HOP_AF(hop_PlayerActor(k), 0xd4) = 0.0f;
				HopAntics_PlaceActor(hop_Antics, hop_PlayerActor(k));
			}
		}
		for (k = 0; k < 27; k++)
			HOP_AF(hop_Platforms[k], 0x1d8) = 0.0f;
		Platforms_UpdateAll();
		actor = HOP_AP(hop_Players[winner], 0x30);
		hop_Target[0] = HOP_AF(actor, 0xd0);
		hop_Target[1] = HOP_AF(actor, 0xd4) + 100.0f;
		hop_Target[2] = HOP_AF(actor, 0xd8);
		hop_Eye[0] = HOP_AF(actor, 0xd0) + 500.0f;
		hop_Eye[1] = HOP_AF(actor, 0xd4) + 300.0f;
		hop_Eye[2] = HOP_AF(actor, 0xd8);
		HOP_AU(actor, 0x1d8) = 0x3fc90fdbu;
		break;
	case 3:
		HOP_AF(hop_Platforms[0], 0x1d8) = 0.0f;
		HOP_AF(hop_Platforms[1], 0x1d8) = 0.0f;
		HOP_AF(hop_Platforms[2], 0x1d8) = 0.0f;
		Platforms_UpdateAll();
		memset(HOP_ADDR(0x002af11c), 0, 5 * 4);
		for (k = 0; k < 4; k++) {
			a = ((float)k * 20.0f + (k < 2 ? 210.0f : 90.0f)) * HOP_DEG;
			HOP_AF(hop_PlayerActor(k), 0xd0) = sinf(a) * 1300.0f;
			HOP_AF(hop_PlayerActor(k), 0xd8) = cosf(a) * 1300.0f;
			HOP_AF(hop_PlayerActor(k), 0xd4) = -269.0f;
		}
		actor = HOP_AP(hop_Players[winner], 0x30);
		hop_Target[0] = HOP_AF(actor, 0xd0);
		hop_Target[1] = HOP_AF(actor, 0xd4) + 100.0f;
		hop_Target[2] = HOP_AF(actor, 0xd8);
		hop_Eye[0] = HOP_AF(actor, 0xd0);
		hop_Eye[1] = HOP_AF(actor, 0xd4) + 500.0f;
		hop_Eye[2] = HOP_AF(actor, 0xd8) - 700.0f;
		HOP_AU(actor, 0x1d8) = 0x40490fdbu;
		break;
	case 4:
		slot = winners;
		for (k = 0; k < 4; k++) {
			if (isWinner[k] != 0) {
				a = (hop_CamAngle - 75.0f) * HOP_DEG;
				HOP_AU(hop_PlayerActor(k), 0x220) |= 1;
				actor = hop_PlayerActor(k);
				*slot++ = k;
				r = (float)k * 140.0f + 800.0f;
				HOP_AF(actor, 0xd0) = -(sinf(a) * r);
				HOP_AF(hop_PlayerActor(k), 0xd4) = -hop_Sink;
				HOP_AF(hop_PlayerActor(k), 0xd8) = -(cosf(a) * r);
				hop_ActorSnap(hop_PlayerActor(k));
				HOP_AF(hop_PlayerActor(k), 0x1d8) = hop_CamAngle * HOP_DEG;
			}
		}
		for (k = 0; k < 4; k++) {
			st = HOP_AS(hop_Players[k], 0x36);
			if (st != 0xc && st != 0xd && st != 0)
				AI_SetState(hop_Players[k], 0xc);
		}
		a = (hop_CamAngle - 75.0f) * HOP_DEG;
		r = (float)(int)winner * 120.0f + 800.0f;
		tx = sinf(a) * r;
		hop_Eye[1] = 200.0f - hop_Sink;
		tz = cosf(a) * r;
		a = (hop_CamAngle - 100.0f) * HOP_DEG;
		ex = sinf(a) * -1000.0f;
		ez = cosf(a) * -1000.0f;
		hop_Target[1] = hop_Eye[1];
		if (1 < PLAY_I(self, 0x338)) {
			s = (float)(HOP_IV(0x002aff04)[winner] / 2);
			a = ((hop_CamAngle - (125.0f - HOP_AF(hop_Players[winner], 0x1180))) + s) * HOP_DEG;
			r = (float)HOP_IV(0x002ae8cc)[winner] * 140.0f + (float)(HOP_IV(0x002aff04)[winner] * 3) + 800.0f;
			tx = sinf(a) * r;
			hop_Target[1] = -hop_Sink * 1.1f;
			tz = cosf(a) * r;
			a = ((hop_CamAngle - (155.0f - HOP_AF(hop_Players[winner], 0x1180))) + s) * HOP_DEG;
			ex = -(sinf(a) * r) * 1.5f;
			hop_Eye[1] = -hop_Sink * 1.5f;
			ez = -(cosf(a) * r) * 1.5f;
		}
		hop_Target[2] = -tz;
		hop_Target[0] = -tx;
		hop_Eye[2] = ez;
		hop_Eye[0] = ex;
		hop_RoundOver = 1;
		HopAntics_Update(hop_Antics);
		goto view;
	default:
		goto view;
	}
	hop_RoundOver = 1;
	if (DAT_002ecdec == 0) {
		AI_TickAll();
		Actor_UpdateAll();
		Actor_UpdateAnimations();
	}
view:
	bdSetCameraLookAt(hop_Eye, hop_Target, 0.0f);
	bdUpdateView();
}

/* 0x0014c4a0  (gap): the clock. The fps and polygon counters (tweak "fps"); the time left
(6000 hundredths less the time since the start, which moves on while paused). The round
ends when the time is up (variations 1, 2), when three players are out of lives
(variation 3: the places go to core), or when three (all, if 0x002baea8) players are off
the arm (variation 4): the winner text appears and the winner sequence starts. */
void HopPlay_Clock(void *self)
{
	unsigned short wide[40];
	char text[40];
	unsigned long long now, prev;
	long long elapsed, t;
	float fps;
	int n, k;
	int *scores = NULL;

	bdSetProjectionMode(0);
	bdSetWorldIdentity();
	now = bkTimerRead();
	hop_Set64(self, 0xc8, now);
	fps = bkTimerRate((long long)bkTimerDelta(hop_Get64(self, 0xd0), now));
	if (HOP_F(0x002ae968) != 0.0f) {
		bdPushRenderState(3, 0, 0);
		sprintf(text, "fps:%03d", HOP_FTOL(fps));
		bkStringToWide(wide, text);
		bdPrintFontLite(HOP_G(0x002ae994, struct bdFont *), wide, 7, 80.0f, 200.0f, 0x40, 0x40, 0x40, 0x7f, NULL, NULL);
		sprintf(text, "polys:%d", HOP_I(0x003027c0));
		bkStringToWide(wide, text);
		bdPrintFontLite(HOP_G(0x002ae994, struct bdFont *), wide, hop_WideLength(text), 40.0f, 170.0f, 0x40, 0x40, 0x40,
			0x7f, NULL, NULL);
		bdPopRenderState(3);
	}
	hop_Set64(self, 0xb8, bkTimerRead());
	if (DAT_002ecdec != 0) {
		prev = bkTimerDelta(hop_Get64(self, 0xd0), hop_Get64(self, 0xb8));
		hop_Set64(self, 0xc0, hop_Get64(self, 0xc0) + prev);
	}
	elapsed = (long long)bkTimerDelta(hop_Get64(self, 0xc0), hop_Get64(self, 0xb8));
	t = elapsed / (DAT_002cd7a0 / 100);
	hop_Set64(self, 0xd8, (unsigned long long)(6000 - t));
	hop_Set64(self, 0xd0, hop_Get64(self, 0xc8));
	if (hop_Variation < 3 && (long long)hop_Get64(self, 0xd8) <= 0) {
		if (HOP_B(0x003027b0) != 0)
			CLHud_CreateWinnerText(DAT_002ecfac, HOP_IV(0x002aff24), 0);
		hop_Set64(self, 0xd8, 0);
	} else if (hop_Variation == 3) {
		n = 0;
		for (k = 0; k < 4; k++) {
			if (HOP_IV(0x002aff24)[k] < 1)
				n++;
		}
		if (n < 3)
			goto done;
		DAT_002baed4[0][5] = HOP_IV(0x002aff34)[0];
		DAT_002baed4[3][5] = HOP_IV(0x002aff34)[3];
		DAT_002baed4[1][5] = HOP_IV(0x002aff34)[1];
		DAT_002baed4[2][5] = HOP_IV(0x002aff34)[2];
		if (HOP_B(0x003027b0) != 0) {
			scores = HOP_IV(0x002aff34);
			CLHud_CreateWinnerText(DAT_002ecfac, scores, 0);
		}
	} else {
		if (hop_Variation != 4)
			goto done;
		n = 0;
		for (k = 0; k < 4; k++) {
			if (HOP_IV(0x002aff54)[k] != 0)
				n++;
		}
		CLHud_SetScores(DAT_002ecfac, HOP_IV(0x002aff24));
		if (DAT_002baea8 == 0) {
			if (n < 3)
				goto done;
		} else if (n != 4) {
			goto done;
		}
		if (HOP_B(0x003027b0) != 0)
			CLHud_CreateWinnerText(DAT_002ecfac, HOP_IV(0x002aff24), 0);
	}
	HopPlay_Winner(self);
	HOP_B(0x003027b0) = 0;
done:
	bdSetProjectionMode(1);
	bdSetWorldIdentity();
}

/* 0x0014c800  (gap): draws the overlays after the frame: during the intro and countdown
the view again (near 1, far 100000), the overlays (not with the debug free camera), the
clock, and the darkening fade-in */
void HopPlay_DrawOverlay(void *self)
{
	if (0 < (signed char)hop_Play && (signed char)hop_Play < 5) {
		bdSetNearFar(1.0f, 100000.0f);
		HopCamera_Apply(HOP_ADDR(0x002aebd8));
	}
	if (hop_Play == 2)
		FUN_001d5950();
	if (DAT_002ba56c == 0)
		COverlay_DrawAll(1);
	HopPlay_Clock(self);
	if (DAT_002ecdec == 0 && 0 < PLAY_I(self, 0x334)) {
		PLAY_I(self, 0x334) -= 8;
		CLScreen_DrawDarken(PLAY_I(self, 0x334), 1);
	}
}
