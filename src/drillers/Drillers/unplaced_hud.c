/*
Drillers/unplaced_hud.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The HUD extras object (one static object at 0x002aab08, vtable 0x0026cb7c): the floating
"behind by" text above a rider, the round clock and fps counter, and the results table
with the score count-up. Layout as these functions use it:
  +0x008  struct bdTexture *[4]  the players' portrait textures (results table)
  +0x014  int [4][4]             round scores: player p, round r (1..3) at +0x14 + p*0x10
                                 + r*4; the total of player p at +0x24 + p*0x10
  +0x058  int [4][4]             the shown values (count up to +0x18 + p*0x10 + c*4)
  +0x098  int                    frames since the results opened (table after 45)
  +0x09c  int                    the shown values are initialised
  +0x0a0  int                    the winner text is still to be made (last round)
  +0x2e8  int                    the darken fade (down by 8 a frame)
These gap functions lie between CDrillers_Resources.cpp (0x0011e1e0) and
CDrillers_Particle.cpp (0x00123550); no assert names their file. The table entry
0x001214f0 is a chunk of 0x00120d00 (Drillers/drillers_chunks.c).
*/

#include "drillers.h"

#define HUD(off, type) DR_AT(self, off, type)

extern void *DAT_002ecc74; /* classlib_ui: the text table; +0x20 entries of 0x10 bytes */
extern int DAT_002baea8;   /* a minigame was chosen (front end) */
extern long long DAT_002cd7a0; /* bkernel: timer ticks per second */

#define DR_FONT_TEXT DR_VAR(struct bdFont *, 0x002aa984u)
#define DR_FONT_FPS DR_VAR(struct bdFont *, 0x002aa988u)
#define DR_SAMPLE_TICK DR_VAR(struct BSample *, 0x002aa9d0u)
#define DR_TUNE_FPS DR_VAR(float, 0x002aae24u)

/* the game object's clock (game +0x28 .. +0x7c) */
#define DR_CLOCK_START DR_VAR(unsigned long long, 0x002ffa18u) /* game +0x28 */
#define DR_CLOCK_NOW DR_VAR(unsigned long long, 0x002ffa20u)   /* game +0x30 */
#define DR_CLOCK_LEFT DR_VAR(long long, 0x002ffa28u)           /* game +0x38, 1/100 s */
#define DR_FRAME_LAST DR_VAR(unsigned long long, 0x002ffa60u)  /* game +0x70 */
#define DR_FRAME_NOW DR_VAR(unsigned long long, 0x002ffa68u)   /* game +0x78 */
#define DR_ROUND DR_VAR(int, 0x002ffa58u)                      /* game +0x68 */
#define DR_HUD_SCORES DR_PTR(int, 0x002ffa30u)                 /* game +0x40 */

/* the results table quad (4 bdVertex) and its frame counter */
#define DR_TABLE_VTX DR_PTR(bdVertex, 0x00302038u)
#define DR_TABLE_FRAME DR_VAR(int, 0x003020c8u)

/* the round score of player p (player record +0x700) */
#define DR_ROUND_SCORE(p) DR_PF(p, 0x700, int)

/* colour channel tables (classlib): value << shift of each 0..255 */
#define DR_RED(v) DAT_002bce18[v]
#define DR_GREEN(v) DAT_002bc490[v]
#define DR_BLUE(v) DAT_002bd640[v]
#define DR_ALPHA(v) DAT_002bc898[v]

/* 0x0026cb80  L"%d %S" */
static const unsigned short dr_fmt_behind[] = {'%', 'd', ' ', '%', 'S', 0};

/* 0x0026cb7c  vtable of the HUD extras object (rebuilt from the XBE) */
const void *const vtable_0026cb7c[1] = {
	(const void *)DrHud_DeletingDtor, /* +0x00 */
};

/* 0x00120870  Drillers (gap; file not known; not in the function table)
The text "<behind> <text 0x1b1>" above player's bike, at height 500, turned to face the
camera and scaled 6; centred, grey. */
void FUN_00120870(void *self, int player, int behind)
{
	float t[16], r[16], s[16], tr[16], world[16];
	unsigned short text[0x50];
	float width, height;
	char *bike;

	(void)self;
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 1, 0);
	bdSetRenderState(5, 0, 0);
	bike = DR_PF(player, 0x4c, char *);
	bmMatrixTranslation(t, DR_F32(bike, 0xc0), 500.0f, DR_F32(bike, 0xc8));
	bmMatrixRotationY(r, 3.1415927f);
	bmMatrixScaling(s, 6.0f, 6.0f, 6.0f);
	bmMatrixMultiplyCopy(tr, t, r);
	bmMatrixMultiplyCopy(world, tr, s);
	bdSetWorldMatrix(world);
	bkWideSprintf(text, dr_fmt_behind, behind,
		DR_AT(DR_PTRF(DAT_002ecc74, 0x20), 0x1b10, unsigned short *));
	bdGetStringSize(DR_FONT_TEXT, text, bkWideStrlen(text), &width, &height);
	bdPrintFontLite(DR_FONT_TEXT, text, bkWideStrlen(text), width * -0.5f, height * 0.5f,
		0x7f, 0x7f, 0x7f, 0xff, NULL, NULL);
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(5, 1, 0);
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);
}

/* 0x00120a70  Drillers (gap; file not known)
The frame clock: the frame rate; in variants 2 and 4 the round clock runs only while the
game plays (state 5, not paused): the time left is 90 s (variant 2) or 60 s less the
time played, in 1/100 s, not below 0 (90 s fixed while a minigame was chosen in variant
2). The darken fade; the "fps: %d" counter when the tuning value is set. */
void DrHud_UpdateClock(void *self)
{
	char buf[40];
	unsigned short wide[40];
	unsigned long long now;
	long long freq;
	float fps;

	DR_FRAME_NOW = bkTimerRead();
	fps = bkTimerRate((long long)bkTimerDelta(DR_FRAME_LAST, DR_FRAME_NOW));
	if (g_DrVariant == 2 || g_DrVariant == 4) {
		freq = DAT_002cd7a0;
		now = bkTimerRead();
		DR_CLOCK_NOW = now;
		if (DAT_002ecdec != 0 || g_DrState != 5)
			DR_CLOCK_START += bkTimerDelta(DR_FRAME_LAST, now);
		if (g_DrVariant == 2) {
			if (DAT_002baea8 != 0) {
				DR_CLOCK_LEFT = 9000;
				goto done;
			}
			DR_CLOCK_LEFT = (long long)(9000.0f -
				(double)(long long)bkTimerDelta(DR_CLOCK_START, DR_CLOCK_NOW) /
					((double)freq * 0.01f));
		} else {
			DR_CLOCK_LEFT = (long long)(6000.0f -
				(double)(long long)bkTimerDelta(DR_CLOCK_START, DR_CLOCK_NOW) /
					((double)freq * 0.01f));
		}
		if (DR_CLOCK_LEFT <= 0)
			DR_CLOCK_LEFT = 0;
	}
done:
	DR_FRAME_LAST = DR_FRAME_NOW;
	if (DAT_002ecdec == 0 && 0 < HUD(0x2e8, int)) {
		HUD(0x2e8, int) -= 8;
		CLScreen_DrawDarken(HUD(0x2e8, int), 1);
	}
	if (DR_TUNE_FPS != 0.0f) {
		bdSetProjectionMode(0);
		bdSetRenderState(1, 1, 0);
		sprintf(buf, "fps: %d", DR_FTOL(fps));
		bkStringToWide(wide, buf);
		bdPrintFontLite(DR_FONT_FPS, wide, (int)strlen(buf), -300.0f, 200.0f, 0x7f, 0x7f, 0x7f,
			0x7f, NULL, NULL);
		bdSetRenderState(1, 5, 0);
		bdSetProjectionMode(1);
		bdSetTexture(0, NULL);
	}
}

/* 0x00120ce0  Drillers (gap; file not known; vtable 0x0026cb7c +0x00) */
void *DrHud_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026cb7c;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* the results table's portrait column in round order: the total of player p */
#define DR_TOTAL(p) HUD(0x24 + (p) * 0x10, int)

/* 0x00120d00  Drillers (gap; file not known; not in the function table; its chunk
0x00121150 holds the table entry 0x001214f0)
The round results: the players' round scores and totals, sorted by total (lowest first).
After 45 frames: the table quad and the portraits (the original sets up the vertices and
textures but draws nothing), then every 16th frame each shown value counts one up to its
score, with a white flash on the score bars, the bars' scale and a tick sound; the bars
fade back to twice the player's colour. In the last round the winner text is made once. */
void FUN_00120d00(void *self)
{
	int order[4];
	int i, k, swapped, changed, col, best, value;
	int *shown;
	int *player;
	char *bar;
	char buf[40];
	unsigned short wide[40];
	unsigned int colour;
	float x, y;
	int winners[4];
	unsigned char *rgb;

	if (HUD(0x9c, int) == 0 && DR_ROUND <= 1) {
		for (k = 0; k < 16; k++)
			HUD(0x58 + k * 4, int) = HUD(0x18 + k * 4, int);
		HUD(0x9c, int) = 1;
		DR_TABLE_FRAME = 0;
	}
	HUD(0x14 + DR_ROUND * 4, int) = DR_ROUND_SCORE(0);
	HUD(0x24, int) = HUD(0x20, int) + HUD(0x1c, int) + HUD(0x18, int);
	HUD(0x24 + DR_ROUND * 4, int) = DR_ROUND_SCORE(1);
	HUD(0x34, int) = HUD(0x30, int) + HUD(0x2c, int) + HUD(0x28, int);
	HUD(0x34 + DR_ROUND * 4, int) = DR_ROUND_SCORE(2);
	HUD(0x44, int) = HUD(0x40, int) + HUD(0x3c, int) + HUD(0x38, int);
	HUD(0x44 + DR_ROUND * 4, int) = DR_ROUND_SCORE(3);
	HUD(0x54, int) = HUD(0x50, int) + HUD(0x4c, int) + HUD(0x48, int);

	order[0] = 0;
	order[1] = 1;
	order[2] = 2;
	order[3] = 3;
	do {
		swapped = 0;
		for (k = 0; k < 3; k++) {
			if (DR_TOTAL(order[k + 1]) < DR_TOTAL(order[k])) {
				i = order[k];
				order[k] = order[k + 1];
				order[k + 1] = i;
				swapped = 1;
			}
		}
	} while (swapped);

	if (DAT_002ecdec == 0)
		HUD(0x98, int)++;
	if (HUD(0x98, int) < 0x2d)
		return;

	bdSetTexture(0, NULL);
	bdSetRenderState(0xc, 0, 0);
	bdSetProjectionMode(0);
	bdSetRenderState(6, 1, 0);
	bdSetRenderState(10, 0, 0);
	bdSetRenderState(7, 1, 0);
	bdSetWorldIdentity();
	bdSetTexture(0, NULL);

	/* the round marker: x from the round number */
	colour = DR_RED(156) | DR_GREEN(177) | DR_ALPHA(60) | DR_BLUE(200);
	DR_TABLE_VTX[0].color = colour;
	DR_TABLE_VTX[1].color = colour;
	DR_TABLE_VTX[3].color = colour;
	x = (float)(DR_ROUND * 0x32 - 0x32);
	DR_TABLE_VTX[2].color = colour;
	DR_TABLE_VTX[0].y = 192.0f;
	DR_TABLE_VTX[0].x = x - 70.0f;
	DR_TABLE_VTX[0].z = 0.0f;
	DR_TABLE_VTX[1].y = 192.0f;
	DR_TABLE_VTX[1].z = 0.0f;
	DR_TABLE_VTX[2].y = 17.0f;
	DR_TABLE_VTX[2].z = 0.0f;
	DR_TABLE_VTX[1].x = x - 45.0f;
	DR_TABLE_VTX[3].y = 17.0f;
	DR_TABLE_VTX[3].z = 0.0f;
	DR_TABLE_VTX[2].x = x - 70.0f;
	DR_TABLE_VTX[3].x = x - 45.0f;
	bdSetRenderState(6, 0, 0);
	bdSetRenderState(10, 0, 0);
	DR_TABLE_VTX[0].u = 0.0f;
	DR_TABLE_VTX[0].v = 1.0f;
	DR_TABLE_VTX[1].u = 1.0f;
	DR_TABLE_VTX[1].v = 1.0f;
	DR_TABLE_VTX[2].u = 0.0f;
	DR_TABLE_VTX[2].v = 0.0f;
	DR_TABLE_VTX[3].u = 1.0f;
	DR_TABLE_VTX[3].v = 0.0f;

	/* the portraits, one row of 40 per place */
	for (i = 0; i < 4; i++) {
		y = (float)i * 40.0f - 8.0f;
		colour = DR_ALPHA(64) | DR_BLUE(127) | DR_GREEN(127) | DR_RED(127);
		DR_TABLE_VTX[0].x = -132.0f;
		DR_TABLE_VTX[0].z = 0.0f;
		DR_TABLE_VTX[1].x = -68.0f;
		DR_TABLE_VTX[0].y = y - 2.0f;
		DR_TABLE_VTX[1].z = 0.0f;
		DR_TABLE_VTX[1].y = y - 2.0f;
		DR_TABLE_VTX[2].x = -132.0f;
		DR_TABLE_VTX[2].z = 0.0f;
		DR_TABLE_VTX[3].x = -68.0f;
		DR_TABLE_VTX[3].z = 0.0f;
		DR_TABLE_VTX[0].color = colour;
		DR_TABLE_VTX[2].y = y + 62.0f;
		DR_TABLE_VTX[1].color = colour;
		DR_TABLE_VTX[3].y = y + 62.0f;
		DR_TABLE_VTX[2].color = colour;
		DR_TABLE_VTX[3].color = colour;
		bdSetTexture(0, HUD(0x8 + order[i] * 4, struct bdTexture *));
	}
	bdSetRenderState(1, 1, 0);
	if (DAT_002ecdec == 0)
		DR_TABLE_FRAME++;

	/* the count-up: column col (rounds 1..3, then the total) of each player */
	changed = 0;
	for (col = 0; col < 4; col++) {
		shown = &HUD(0x58 + col * 4, int);
		player = DAT_002baed4;
		for (k = 0; k < 4; k++, shown += 4, player += 10) {
			if (DAT_002ecdec == 0 && (DR_TABLE_FRAME & 0xf) == 0 && *shown < shown[-0x10]) {
				*shown += 1;
				for (i = 0; i < 4; i++) {
					bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + i * 4);
					DR_F32(bar, 0x2ac) = 255.0f;
					DR_F32(bar, 0x2b0) = 255.0f;
					DR_F32(bar, 0x2b4) = 255.0f;
				}
				DR_HUD_SCORES[k] = *shown;
				changed = 1;
				best = 0;
				if (0 < DR_HUD_SCORES[0])
					best = DR_HUD_SCORES[0];
				if (best < DR_HUD_SCORES[1])
					best = DR_HUD_SCORES[1];
				if (best < DR_HUD_SCORES[2])
					best = DR_HUD_SCORES[2];
				if (best < DR_HUD_SCORES[3])
					best = DR_HUD_SCORES[3];
				CLHud_SetScoresInt(DAT_002ecfac, DR_HUD_SCORES);
				DR_I32((uintptr_t)DAT_002ecfac, 0x3c) = 0;
				DR_F32((uintptr_t)DAT_002ecfac, 0x40) = (float)best;
				CLHud_UpdateBars(DAT_002ecfac);
				CLHud_SetScores(DAT_002ecfac, DR_HUD_SCORES);
			}
			if (col < 3)
				value = HUD(0x58 + (col + order[k] * 4) * 4, int);
			else
				value = HUD(0x64 + order[k] * 0x10, int);
			sprintf(buf, "%d", value);
			bkStringToWide(wide, buf);

			/* the bar fades back to twice the player's colour */
			rgb = &DAT_00290718[*player * 0x60];
			bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + k * 4);
			if ((float)(rgb[0] << 1) < DR_F32(bar, 0x2ac))
				DR_F32(bar, 0x2ac) -= 8.0f;
			bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + k * 4);
			if ((float)(rgb[1] << 1) < DR_F32(bar, 0x2b0))
				DR_F32(bar, 0x2b0) -= 8.0f;
			bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + k * 4);
			if ((float)(rgb[2] << 1) < DR_F32(bar, 0x2b4))
				DR_F32(bar, 0x2b4) -= 8.0f;
			bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + k * 4);
			if (DR_F32(bar, 0x2ac) < (float)(rgb[0] * 2))
				DR_F32(bar, 0x2ac) = (float)rgb[0] + (float)rgb[0];
			bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + k * 4);
			if (DR_F32(bar, 0x2b0) < (float)(rgb[1] * 2))
				DR_F32(bar, 0x2b0) = (float)rgb[1] + (float)rgb[1];
			bar = DR_PTRF((uintptr_t)DAT_002ecfac, 0x78 + k * 4);
			if (DR_F32(bar, 0x2b4) < (float)(rgb[2] * 2))
				DR_F32(bar, 0x2b4) = (float)rgb[2] + (float)rgb[2];
		}
	}
	if (changed)
		bsPlaySample(DR_SAMPLE_TICK, 0xf0, 0, 0, 0);
	if (DR_ROUND == 3 && HUD(0xa0, int) != 0) {
		winners[0] = DAT_002baee8[0];
		winners[1] = DAT_002baee8[10];
		winners[2] = DAT_002baee8[20];
		winners[3] = DAT_002baee8[30];
		CLHud_CreateWinnerText(DAT_002ecfac, winners, 0);
		HUD(0xa0, int) = 0;
	}
	bdSetProjectionMode(1);
	bdSetTexture(0, NULL);
	bdSetWorldIdentity();
	bdSetRenderState(1, 5, 0);
	bdSetRenderState(7, 2, 0);
}
