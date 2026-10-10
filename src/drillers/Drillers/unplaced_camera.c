/*
Drillers/unplaced_camera.c  <-  C:\BlitzParty\Game\Src\Drillers\ (file not known)

The camera object (one static object at 0x002aa830, vtable 0x0026c568) and the game's state
machine that it drives: the intro camera (state 1), the countdown (3), play (5), the end of
a round or the game (6) and Rubble Alliance's drop to the next layer (7). These gap
functions lie between CDrillers_Lom.cpp (0x0011b470) and CDrillers_Resources.cpp
(0x0011e1e0); no assert names their file.

The object, fields at original offsets (several are read as globals elsewhere):
  +0x04 eye (0x002aa834)   +0x10 target (0x002aa840)   +0x1c heading (0x002aa84c)
  +0x20 distance (0x002aa850)   +0x24 field of view   +0x28
  +0x48 lift (0x002aa878)   +0x54 drop counter (float)   +0x5c fade-out (0x002aa88c)
  +0x6c frames at the end
*/

#include "drillers.h"

#define CAM(off, type) DR_AT(self, off, type)

/* the game object's fields */
#define DR_GAME_FRAMES DR_VAR(int, 0x002ffa04u)       /* game +0x14 */
#define DR_GAME_COUNTER DR_VAR(int, 0x002ffa0cu)      /* game +0x1c */
#define DR_ROUND DR_VAR(int, 0x002ffa58u)             /* game +0x68 */
#define DR_FROZEN DR_VAR(signed char, 0x002ffa74u)    /* game +0x84 */
#define DR_SINGLE DR_VAR(int, 0x002ffaa0u)            /* game +0xb0 */
#define DR_SINGLE_PLAYER DR_VAR(int, 0x002ffaa4u)     /* game +0xb4 */
#define DR_OVER_COUNT DR_VAR(int, 0x002ffaa8u)        /* game +0xb8 */
#define DR_SCORES DR_PTR(int, 0x002ffa30u)            /* game +0x40 */
/* the map size */
#define DR_CELL DR_VAR(int, 0x002ff9e0u)
#define DR_HALF_Z DR_VAR(int, 0x002ff9e4u)
#define DR_HALF_X DR_VAR(int, 0x002ff9e8u)
/* the camera globals (fields of this object) */
#define DR_EYE DR_PTR(float, 0x002aa834u)
#define DR_TARGET DR_PTR(float, 0x002aa840u)
#define DR_HEADING DR_VAR(float, 0x002aa84cu)
#define DR_DIST DR_VAR(float, 0x002aa850u)
/* the round angles of Rubble Alliance (this round, the last) */
#define DR_ROUND_ANGLE DR_VAR(float, 0x002a55e0u)
#define DR_ROUND_ANGLE_OLD DR_VAR(float, 0x002a55e4u)
/* the spin of Erodeo's arena */
#define DR_ARENA_SPIN DR_VAR(float, 0x002a5618u)
/* the winner the camera shows */
#define DR_SHOWN DR_VAR(int, 0x0030202cu)
/* the effects object's flash */
#define DR_FLASH DR_VAR(int, 0x00301498u)
/* the pads object's pointer (its +0x14: a button) */
#define DR_PAD0 DR_VAR(char *, 0x002aab00u)
/* the round totals of the HUD extras object */
#define DR_HUD2_ROW(i) DR_PTR(int, 0x002aab20u + (i) * 0x10)
#define DR_HUD2_TOTAL(i) DR_VAR(int, 0x002aab6cu + (i) * 0x10)
/* the engine sound channels */
#define DR_ENGINE_CHANNEL(i) DR_VAR(int, 0x002aaa04u + (i) * 4)
/* the demo camera of bumpercars_b */
extern void *DAT_002f26c4;
/* attract mode (core) */
extern int DAT_002baeb8;
/* core's state (2: in a board game) */
extern int DAT_002bae98;
/* the font of the resources object */
#define DR_FONT DR_VAR(struct bdFont *, 0x002aa984u)

#define DR_DEG 0.017453292f
#define DR_K12 0.083333336f   /* 1/12 */

/* 0x0026c568  vtable of the camera object (rebuilt from the XBE) */
const void *const vtable_0026c568[1] = {
	(const void *)DrCamera_DeletingDtor, /* +0x00 */
};

/* 0x0011c620  Drillers (gap; file not known)
Prints a space (red) with the resources font: at the screen centre's x and y 80, and in a
640 x 480 box (a no-op text, kept as in the original). */
void DrCamera_PrintBlank(void)
{
	unsigned int format[8];
	unsigned short wide[10];
	float width;
	float height;

	format[0] = 0;
	format[1] = 0;
	format[2] = dr_fbits(640.0f);
	format[3] = dr_fbits(480.0f);
	format[4] = 0;
	format[5] = 1;
	format[6] = 0;
	format[7] = 0;
	bkStringToWide(wide, " ");
	bdGetStringSize(DR_FONT, wide, 1, &width, &height);
	bdPrintFontLite(DR_FONT, wide, 1, width * -0.5f, 80.0f, 0x7f, 0, 0, 0x7f, NULL, NULL);
	bdPrintFont(DR_FONT, wide, 1, (struct bdPrintFormat *)format, 0x7f, 0, 0, 0x7f, NULL, NULL);
}

/* 0x0011c6f0  Drillers (gap; file not known)
Has a human winner (flags[i]) pressed the button? When no winner is human, any human
counts. */
int DrCamera_WinnerPressed(void *self, int *flags)
{
	int n = 0;
	int i;

	(void)self;
	for (i = 0; i < 4; i++) {
		if (DAT_002baed8[i * 10] != 0 && DR_I32(DR_PF(i, 0x28, char *), 0x14) != 0 &&
				flags[i] != 0)
			return 1;
	}
	if (DAT_002baed8[0] != 0 && flags[0] != 0)
		n = 1;
	if (DAT_002baf00 != 0 && flags[1] != 0)
		n++;
	if (DAT_002baf28 != 0 && flags[2] != 0)
		n++;
	if (DAT_002baf50 != 0 && flags[3] != 0)
		n++;
	if (n == 0) {
		for (i = 0; i < 4; i++) {
			if (DAT_002baed8[i * 10] != 0 && DR_I32(DR_PF(i, 0x28, char *), 0x14) != 0)
				return 1;
		}
	}
	return 0;
}

/* 0x0011c7c0  Drillers (gap; file not known; vtable 0x0026c568 +0x00) */
void *DrCamera_DeletingDtor(void *self, unsigned int flags)
{
	*(const void *const **)self = vtable_0026c568;
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* the default view of the arena: variants 1, 2 from the back (eye 0, eyeY, eyeZ, target y
   targetY, distance dist), variants 3, 4 from -25 degrees at distance d3 */
static void dr_camera_home(float eyeY, float eyeZ, float targetY, float dist, float y3,
	float dist3, float r3)
{
	DR_TARGET[0] = 0.0f;
	DR_TARGET[2] = 0.0f;
	if (g_DrVariant < 3) {
		DR_EYE[0] = 0.0f;
		DR_DIST = dist;
		DR_TARGET[1] = targetY;
		DR_EYE[1] = eyeY;
		DR_EYE[2] = eyeZ;
	} else {
		DR_TARGET[1] = 0.0f;
		DR_DIST = dist3;
		DR_EYE[1] = y3;
		DR_EYE[0] = (float)(sin(-0.4363323152065277) * r3);
		DR_EYE[2] = (float)(cos(-0.4363323152065277) * r3);
	}
}

/* the camera on player `p` at the end (eye at 1000 behind the target, looking from its
   heading); `k` the easing of x and z, `ky` of y, `ty` the target height */
static void dr_camera_follow(void *self, int p, float ty, float ky)
{
	char *body = DR_PF(p, 0x4c, char *);
	float a;

	CAM(0x10, float) = (DR_F32(body, 0xc0) - CAM(0x10, float)) * 0.125f + CAM(0x10, float);
	CAM(0x14, float) = (ty - CAM(0x14, float)) * ky + CAM(0x14, float);
	CAM(0x18, float) = (DR_F32(body, 0xc8) - CAM(0x18, float)) * 0.125f + CAM(0x18, float);
	a = atan2f(-CAM(0x10, float), -CAM(0x18, float));
	CAM(0x4, float) = ((sinf(a) * 1000.0f + CAM(0x10, float)) - CAM(0x4, float)) +
		CAM(0x4, float);
	CAM(0x8, float) = (700.0f - CAM(0x8, float)) + CAM(0x8, float);
	CAM(0xc, float) = ((cosf(a) * 1000.0f + CAM(0x18, float)) - CAM(0xc, float)) +
		CAM(0xc, float);
}

/* the scores: the HUD, its bars and the winner's bar (the best of the four) */
static void dr_camera_show_scores(void)
{
	int best = 0;
	int i;

	for (i = 0; i < 4; i++) {
		if (best < DR_SCORES[i])
			best = DR_SCORES[i];
	}
	CLHud_SetScoresInt(DAT_002ecfac, DR_SCORES);
	DR_I32(DAT_002ecfac, 0x3c) = 0;
	DR_F32(DAT_002ecfac, 0x40) = (float)best;
	CLHud_UpdateBars(DAT_002ecfac);
	CLHud_SetScores(DAT_002ecfac, DR_SCORES);
}

/* back to the board: the scores to core, pop the mode; returns 1 */
static int dr_camera_leave(void)
{
	DAT_002baee8[0] = DR_SCORES[0];
	DAT_002baf10 = DR_SCORES[1];
	DAT_002baf38 = DR_SCORES[2];
	DAT_002baf60 = DR_SCORES[3];
	CGameModeStack_Pop((struct CGameModeStack *)DAT_002ba750, 1);
	return 1;
}

/* the start of the countdown's bars, state 3 */
static int dr_camera_to_countdown(void)
{
	CLHud_StartBars((float *)(uintptr_t)DAT_002ecfac, 280.0f);
	return 0;
}

/* 0x0011c7e0  Drillers (gap; file not known)
The camera and the game state, once a frame. Returns 1 when the game is over and the mode
was popped, else 0.
- In attract mode (core 0x002baeb8) the countdown view; while frozen nothing moves.
- 1: the intro camera; it ends (or a human presses on round 1) into the countdown.
- 3: the countdown; at its end play starts: the engines on four channels, the centre 3x3
  cells drilled; the camera eases to the arena view.
- 5: play: a single human is followed; variants 3, 4 circle; variants 1, 2 frame all the
  drillers in play.
- 6: the end: the winners (best score or round points) are shown in turn, 240 frames
  each; variants 1, 3 rounds 1, 2: the next round starts after 120 frames once a winner
  presses (or 240); otherwise the screen fades out (+0x5c) and the game goes back to the
  board (or a new game in the game's own menu).
- 7: Rubble Alliance's drop to the next layer (60 frames), then the countdown. */
int DrCamera_Update(void *self)
{
	float pts[4][4];
	int list[4];
	int flags[4];
	unsigned long long now;
	float sumX;
	float sumZ;
	float inv;
	float minX;
	float maxX;
	float minZ;
	float maxZ;
	float fx;
	float fz;
	float fov;
	float a;
	float d;
	float d2;
	float off1;
	float off2;
	char *body;
	int variant;
	int found;
	int count;
	int frames;
	int best;
	int bestRound;
	int sel;
	int flagA;
	int flagB;
	int n;
	int i;
	int k;
	int r;
	int v;

	for (i = 0; i < 4; i++) {
		flags[i] = 0;
		list[i] = -1;
	}
	if (DAT_002f26c4 != NULL && g_DrState == 5) {
		BCarFreeCamera_Update((struct BCarFreeCamera *)DAT_002f26c4, 1.0f);
		return 0;
	}
	if (DAT_002baeb8 != 0 && g_DrState != 5 && g_DrState != 6) {
		g_DrState = 3;
		CAM(0x24, float) = dr_bitsf(0x3f9c61aau);
		bdSetFieldOfView(dr_bitsf(0x3f9c61aau), dr_bitsf(0x3f6a927fu));
		dr_camera_home(1500.0f, 2500.0f, -500.0f, 2500.0f, 500.0f, 2000.0f, 2000.0f);
	}
	if (DR_FROZEN != 0)
		return 0;
	DR_GAME_FRAMES++;
	DrCamera_PrintBlank();
	switch (g_DrState) {
	case 1:
		bdSetNearFar(1.0f, 140000.0f);
		DR_ARENA_SPIN = 0.0f;
		r = CLIntroCam_Update(1.0f, 0);
		variant = g_DrVariant;
		for (i = 0; i < 4; i++) {
			if (DAT_002baed8[i * 10] == 0 || DR_I32(DR_PAD0, 0x14) == 0)
				continue;
			r = 1;
			if (variant == 1 && 1 < DR_ROUND)
				continue;
			bdSetFieldOfView(dr_bitsf(0x3f9c61aau), dr_bitsf(0x3f6a927fu));
			dr_camera_home(1500.0f, 1500.0f, 0.0f, 1500.0f, 1000.0f, 1000.0f, 1000.0f);
			variant = g_DrVariant;
		}
		CAM(0x48, float) = 0.0f;
		if (r == 0)
			return 0;
		if (r == -1) {
			bkPrintf("***CAMERA ERROR***\n");
			return 0;
		}
		CLIntroCam_Stop();
		g_DrState = 3;
		DR_FLASH = 0xff;
		bdSetNearFar(1.0f, 75000.0f);
		CLIntroCam_GetFov(&CAM(0x24, float), CLIntroCam_GetTime());
		now = bkTimerRead();
		DR_VAR(unsigned long long, 0x002ffa18u) = now;
		DR_VAR(unsigned long long, 0x002ffa20u) = now;
		DR_VAR(unsigned long long, 0x002ffa60u) = now;
		DR_GAME_FRAMES = 0;
		g_DrState = 3;
		DR_GAME_COUNTER = 0;
		return dr_camera_to_countdown();

	case 3:
		CAM(0x28, int) = 0;
		if (CLHud_UpdateCountdown(DAT_002ecfac) != 0) {
			g_DrState = 5;
			bdSetNearFar(300.0f, 75000.0f);
			DR_SHOWN = -1;
			DR_GAME_COUNTER = 0;
			DR_I32(DAT_002ecfac, 0x88) = 1;
			for (i = 0; i < 4; i++) {
				body = DR_PF(i, 0x50, char *);
				baFlushAnimations(body, DR_AT(body, 0x134, struct BActorNode *), 1, 0);
			}
			if (g_DrVariant == 3)
				bsPlaySample(DR_VAR(struct BSample *, 0x002aa9ccu), 0x96, 0, 0, 0);
			for (i = 0; i < 4; i++) {
				for (k = 0; k < bsGetNumChannels(); k++) {
					if (bsGetChannelStatus(k) == 0) {
						DR_ENGINE_CHANNEL(i) = k;
						break;
					}
				}
				bsPlaySampleOnChannel(DR_ENGINE_CHANNEL(i),
					DR_VAR(struct BSample *, 0x002aa9b8u), g_DrVariant < 3 ? 100 : 0x82, 0, 0, 0);
				bsReserveChannel(DR_ENGINE_CHANNEL(i), 1);
			}
			if (g_DrVariant < 3) {
				for (i = 0; i < 3; i++) {
					for (k = 0; k < 3; k++) {
						DrCell *cell = &g_DrMap->columns[DR_HALF_X + i - 1][DR_HALF_Z + k - 1];

						if (g_DrVariant == 1)
							cell->height = 0x3ff;
						else if (g_DrVariant == 2)
							cell->height = 0x401;
					}
				}
			}
		}
		if (g_DrVariant == 1 && 1 < DR_ROUND)
			return 0;
		fov = (1.2217305f - CAM(0x24, float)) * DR_K12 + CAM(0x24, float);
		CAM(0x24, float) = fov;
		bdSetFieldOfView(fov, fov * 3.0f * 0.25f);
		DR_TARGET[0] = -DR_TARGET[0] * DR_K12 + DR_TARGET[0];
		DR_TARGET[2] = -DR_TARGET[2] * DR_K12 + DR_TARGET[2];
		if (g_DrVariant < 3) {
			DR_DIST = 2500.0f;
			DR_TARGET[1] = -DR_TARGET[1] * DR_K12 + DR_TARGET[1];
			DR_EYE[1] = (1500.0f - DR_EYE[1]) * DR_K12 + DR_EYE[1];
			DR_EYE[0] = -DR_EYE[0] * DR_K12 + DR_EYE[0];
			DR_EYE[2] = (2500.0f - DR_EYE[2]) * DR_K12 + DR_EYE[2];
			return 0;
		}
		DR_DIST = 2000.0f;
		DR_TARGET[1] = -DR_TARGET[1] * 0.0625f + DR_TARGET[1];
		DR_EYE[1] = (500.0f - DR_EYE[1]) * 0.0625f + DR_EYE[1];
		DR_EYE[0] = ((float)(sin(-0.4363323152065277) * 2000.0f) - DR_EYE[0]) * 0.0625f +
			DR_EYE[0];
		DR_EYE[2] = ((float)(cos(-0.4363323152065277) * 2000.0f) - DR_EYE[2]) * 0.0625f +
			DR_EYE[2];
		return 0;

	case 5:
		if (g_DrVariant == 2 && DR_ARENA_SPIN < 25.0f)
			DR_ARENA_SPIN = DR_ARENA_SPIN + 0.2f;
		if (DR_SINGLE != 0) {
			k = DR_SINGLE_PLAYER;
			body = DR_PF(k, 0x50, char *);
			CAM(0x4, float) = DR_F32(body, 0xc0);
			CAM(0x8, float) = DR_F32(body, 0xc4) + 300.0f;
			CAM(0xc, float) = DR_F32(body, 0xc8);
			if (g_DrVariant == 1)
				CAM(0x8, float) = CAM(0x8, float) + 320.0f;
			a = g_DrVariant < 3 ? DR_PF(k, 0x14, float) : DR_PF(k, 0xc, float);
			CAM(0x10, float) = sinf(a * DR_DEG) * 1000.0f + CAM(0x4, float);
			CAM(0x14, float) = CAM(0x8, float) - 200.0f;
			CAM(0x18, float) = cosf(a * DR_DEG) * 1000.0f + CAM(0xc, float);
			return 0;
		}
		if (2 < g_DrVariant) {
			DR_DIST = (5000.0f - DR_DIST) * 0.0625f + DR_DIST;
			v = g_DrVariant == 3 ? DR_GAME_COUNTER : -DR_GAME_COUNTER;
			DR_HEADING = (float)v * 0.125f;
			DR_TARGET[1] = (-1000.0f - DR_TARGET[1]) * 0.0625f + DR_TARGET[1];
			a = (DR_HEADING - 25.0f) * DR_DEG;
			DR_EYE[0] = sinf(a) * DR_DIST;
			DR_EYE[1] = (2500.0f - DR_EYE[1]) * 0.125f + DR_EYE[1];
			DR_EYE[2] = cosf(a) * DR_DIST;
			return 0;
		}
		/* frame the drillers in play */
		n = 0;
		sumX = 0.0f;
		sumZ = 0.0f;
		for (i = 0; i < 4; i++) {
			if (DR_PF(i, 0x20, signed char) != 0) {
				sumX = sumX + DR_PF(i, 0x4, float);
				sumZ = DR_PF(i, 0x8, float) + sumZ;
				n++;
			}
		}
		if (n == 0)
			return 0;
		inv = 1.0f / (float)n;
		DR_TARGET[0] = ((sumX * inv * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL -
			DR_TARGET[0]) * 0.16666667f + DR_TARGET[0];
		DR_TARGET[2] = ((inv * sumZ * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL -
			DR_TARGET[2]) * 0.16666667f + DR_TARGET[2];
		k = 0;
		for (i = 0; i < 4; i++) {
			if (DR_PF(i, 0x20, signed char) != 0) {
				pts[k][0] = (DR_PF(i, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) *
					(float)DR_CELL;
				pts[k][1] = (DR_PF(i, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) *
					(float)DR_CELL;
				k++;
			}
		}
		minX = maxX = pts[0][0];
		minZ = maxZ = pts[0][1];
		for (i = 0; i < k; i++) {
			if (pts[i][0] < minX)
				minX = pts[i][0];
			if (pts[i][0] > maxX)
				maxX = pts[i][0];
			if (pts[i][1] < minZ)
				minZ = pts[i][1];
			if (maxZ < pts[i][1])
				maxZ = pts[i][1];
		}
		d = (float)((((double)maxX - minX) * 0.5) / tan(0.6108652353286743) * 3.4f);
		d2 = (float)((((double)maxZ - minZ) * 0.5) / tan(0.45378559827804565) * 3.4f);
		if (d < d2)
			d = d2;
		if (d < 3000.0f)
			d = 3000.0f;
		else if (d > 4000.0f)
			d = 4000.0f;
		if (g_DrVariant == 2)
			d = 4000.0f;
		DR_EYE[0] = DR_TARGET[0];
		DR_EYE[1] = ((float)(sin(0.7853981852531433) * d) - DR_EYE[1]) * 0.25f + DR_EYE[1];
		DR_EYE[2] = ((float)(cos(0.7853981852531433) * d) - DR_EYE[2]) * 0.25f + DR_EYE[2];
		return 0;

	case 6:
		if (DAT_002baeb8 != 0) {
			if (CAM(0x5c, int) == 0) {
				CAM(0x5c, int) = 1;
				CLHud_DeleteWinnerText(DAT_002ecfac);
			}
			if (CAM(0x5c, int) < 0x109)
				return 0;
			if (CGameModeStack_Find((struct CGameModeStack *)DAT_002ba750, "Meta") == NULL &&
					DAT_002bae98 == 2)
				return 0;
			return dr_camera_leave();
		}
		if (DR_OVER_COUNT == 1) {
			DR_FLASH = 0xff;
			return 0;
		}
		if (0 < DR_OVER_COUNT)
			return 0;
		if (g_DrVariant == 2 && DR_ARENA_SPIN > 0.0f)
			DR_ARENA_SPIN = DR_ARENA_SPIN - 0.25f;
		frames = CAM(0x6c, int) + 1;
		CAM(0x6c, int) = frames;
		variant = g_DrVariant;
		if (variant == 2 || variant == 4) {
			flagA = 0;
			flagB = 1;
		} else {
			flagB = DR_ROUND == 3;
			flagA = 1;
		}
		bestRound = DR_ROUND == 3 && 0x37 < DR_VAR(int, 0x002aa89cu);
		count = 0;
		best = 0;
		for (i = 0; i < 4; i++) {
			if (!bestRound && (variant == 1 || variant == 3))
				v = DR_PF(i, 0x700, int);
			else
				v = DR_SCORES[i];
			if (i == 0) {
				if (0 < v)
					best = v;
			} else if (best < v) {
				best = v;
			}
		}
		for (i = 0; i < 4; i++) {
			if (!bestRound && (variant == 1 || variant == 3))
				v = DR_PF(i, 0x700, int);
			else
				v = DR_SCORES[i];
			if (v == best && (DR_PF(i, 0x20, signed char) == 1 || bestRound)) {
				list[count++] = i;
				flags[i] = 1;
			}
		}
		sel = -1;
		if (0 <= frames)
			sel = list[0];
		if (0xf0 <= frames)
			sel = list[1];
		if (0x1e0 <= frames)
			sel = list[2];
		if (0x2d0 <= frames)
			sel = list[3];
		if (sel != -1)
			DR_SHOWN = sel;
		else
			sel = DR_SHOWN == -1 ? 0 : DR_SHOWN;
		if (flagA == 0) {
			/* variants 2 and 4 */
			body = DR_PF(sel, 0x4c, char *);
			CAM(0x10, float) = (DR_F32(body, 0xc0) - CAM(0x10, float)) * 0.125f +
				CAM(0x10, float);
			if (DR_PF(sel, 0x20, signed char) == 1) {
				CAM(0x14, float) = (200.0f - CAM(0x14, float)) * 0.125f + CAM(0x14, float);
				CAM(0x18, float) = (DR_F32(body, 0xc8) - CAM(0x18, float)) * 0.125f +
					CAM(0x18, float);
				a = atan2f(-CAM(0x10, float), -CAM(0x18, float));
				CAM(0x4, float) = ((sinf(a) * 1000.0f + CAM(0x10, float)) - CAM(0x4, float)) +
					CAM(0x4, float);
				CAM(0x8, float) = (700.0f - CAM(0x8, float)) + CAM(0x8, float);
				CAM(0xc, float) = ((cosf(a) * 1000.0f + CAM(0x18, float)) - CAM(0xc, float)) +
					CAM(0xc, float);
			} else {
				CAM(0x14, float) = -CAM(0x14, float) * 0.125f + CAM(0x14, float);
				CAM(0x18, float) = (DR_F32(body, 0xc8) - CAM(0x18, float)) * 0.125f +
					CAM(0x18, float);
				CAM(0x4, float) = -CAM(0x4, float) * DR_K12 + CAM(0x4, float);
				CAM(0x8, float) = (DR_DIST * 0.5f - CAM(0x8, float)) * DR_K12 + CAM(0x8, float);
				CAM(0xc, float) = (DR_DIST * 0.5f - CAM(0xc, float)) * DR_K12 + CAM(0xc, float);
			}
		} else if (flagB) {
			/* variants 1 and 3, round 3: the winner bobs */
			DR_PF(sel, 0x5c, float) = fabsf(sinf((float)(DR_GAME_FRAMES * 8) * DR_DEG) * 150.0f);
			CAM(0x48, float) = 0.0f;
			body = DR_PF(sel, 0x4c, char *);
			if (DR_PF(sel, 0x20, signed char) == 1) {
				CAM(0x10, float) = (DR_F32(body, 0xc0) - CAM(0x10, float)) * 0.125f +
					CAM(0x10, float);
				CAM(0x14, float) = ((g_DrVariant == 3 ? 200.0f : 700.0f) - CAM(0x14, float)) *
					DR_K12 + CAM(0x14, float);
				CAM(0x18, float) = (DR_F32(body, 0xc8) - CAM(0x18, float)) * 0.125f +
					CAM(0x18, float);
				a = atan2f(-CAM(0x10, float), -CAM(0x18, float));
				CAM(0x4, float) = ((sinf(a) * 1000.0f + CAM(0x10, float)) - CAM(0x4, float)) *
					0.125f + CAM(0x4, float);
				CAM(0x8, float) = (700.0f - CAM(0x8, float)) + CAM(0x8, float);
				CAM(0xc, float) = ((cosf(a) * 1000.0f + CAM(0x18, float)) - CAM(0xc, float)) *
					0.125f + CAM(0xc, float);
			} else {
				CAM(0x4, float) = -CAM(0x4, float) * 0.125f + CAM(0x4, float);
				CAM(0x8, float) = (700.0f - CAM(0x8, float)) * 0.125f + CAM(0x8, float);
				CAM(0xc, float) = (DR_DIST * 0.5f - CAM(0xc, float)) * 0.125f + CAM(0xc, float);
				CAM(0x10, float) = (DR_F32(body, 0xc0) - CAM(0x10, float)) * DR_K12 +
					CAM(0x10, float);
				CAM(0x14, float) = ((g_DrVariant < 3 ? 400.0f : 200.0f) - CAM(0x14, float)) *
					DR_K12 + CAM(0x14, float);
				CAM(0x18, float) = (DR_F32(body, 0xc8) - CAM(0x18, float)) * DR_K12 +
					CAM(0x18, float);
			}
		} else {
			/* variants 1 and 3, rounds 1 and 2: the round's winner bobs; then the next
			   round */
			fx = (DR_PF(sel, 0x4, float) * 0.0009765625f - (float)DR_HALF_X) * (float)DR_CELL;
			fz = (DR_PF(sel, 0x8, float) * 0.0009765625f - (float)DR_HALF_Z) * (float)DR_CELL;
			d = fabsf(sinf((float)(DR_GAME_FRAMES * 8) * DR_DEG) * 150.0f);
			DR_PF(sel, 0x5c, float) = d;
			if (g_DrVariant == 1) {
				if (d <= 11.0f && DR_PF(sel, 0x20, signed char) == 1)
					baAddExtraItem(g_DrFx->ring, (double)fx, 0.0, (double)fz, 0x10, 0x7f, 0x10,
						0.0);
				if (g_DrVariant == 1 && DR_PF(sel, 0x5c, float) < 16.0f &&
						DR_PF(sel, 0x20, signed char) == 1) {
					off1 = (float)(((int)(Core_Rand() & 3) - 2) * DR_CELL);
					off2 = (float)(((int)(Core_Rand() & 3) - 2) * DR_CELL);
					for (k = 0; k < 8; k++)
						baAddExtraItem(g_DrFx->worm, 2, (double)(off1 + fx), 0.0,
							(double)(off2 + fz));
				}
			}
			DR_PF(sel, 0xc, float) = (float)(DR_GAME_FRAMES * 6);
			if (DR_PF(sel, 0x20, signed char) == 1)
				dr_camera_follow(self, sel, g_DrVariant == 3 ? 300.0f : 500.0f, DR_K12);
			frames = CAM(0x6c, int);
			if (0x78 < frames && (DrCamera_WinnerPressed(self, flags) != 0 || 0xf0 < frames)) {
				for (i = 0; i < 4; i++)
					DR_SCORES[i] = DR_HUD2_TOTAL(i);
				dr_camera_show_scores();
				if (g_DrVariant < 3)
					DrMap_Free(g_DrMap);
				CDrillersGame_NewRound(g_DrGame);
				CAM(0x54, int) = 0;
				if (g_DrVariant == 1) {
					a = DrAi_AngleDiff(DR_PTR(void, 0x002aadf8u), DR_ROUND_ANGLE,
						DR_ROUND_ANGLE_OLD) * DR_DEG;
					DR_EYE[0] = sinf(a) * DR_DIST;
					DR_EYE[2] = cosf(a) * DR_DIST;
				}
				for (i = 0; i < 4; i++) {
					struct BActorSubObject *seg;

					body = DR_PF(i, 0x50, char *);
					baFlushAnimations(body, DR_AT(body, 0x134, struct BActorNode *), 1, 0);
					seg = baFindSubObject((struct BActorInstance *)body, "idle");
					if (seg != NULL)
						baQueueAnimation((struct BActorInstance *)body,
							DR_AT(body, 0x134, struct BActorNode *), (struct BAnim *)seg, 0x400,
							-1, 1, NULL, NULL);
					else
						bkPrintf("IDLE SEG NOT FOUND\n");
				}
			}
			goto fade;
		}
		/* the end of the game (variants 2, 4; round 3 of 1, 3) */
		frames = CAM(0x6c, int);
		if (0x78 < frames && CAM(0x5c, int) == 0 &&
				(DrCamera_WinnerPressed(self, flags) != 0 ||
				(count * 0xf0 < frames && CLSamples_CommentaryIdle() != 0))) {
			CLHud_DeleteWinnerText(DAT_002ecfac);
			CAM(0x5c, int) = 1;
		}
fade:
		if (CAM(0x5c, int) < 0x109)
			return 0;
		CAM(0x5c, int) = 0;
		CAM(0x6c, int) = 0;
		if (CGameModeStack_Find((struct CGameModeStack *)DAT_002ba750, "Meta") != NULL ||
				DAT_002bae98 != 2)
			return dr_camera_leave();
		/* a new game (the minigame's own menu) */
		if (g_DrVariant < 3) {
			DrMap_Free(g_DrMap);
			baClearExtraSystem(g_DrFx->worm);
			for (i = 0; i < 4; i++) {
				DR_HUD2_ROW(i)[0] = 0;
				DR_HUD2_ROW(i)[1] = 0;
				DR_HUD2_ROW(i)[2] = 0;
				DR_HUD2_ROW(i)[3] = 0;
			}
		}
		for (i = 0; i < 4; i++)
			DR_SCORES[i] = 0;
		CLHud_SetScoresInt(DAT_002ecfac, DR_SCORES);
		DR_I32(DAT_002ecfac, 0x40) = 0;
		DR_I32(DAT_002ecfac, 0x3c) = 0;
		CLHud_UpdateBars(DAT_002ecfac);
		CLHud_SetScores(DAT_002ecfac, DR_SCORES);
		CDrillersGame_NewRound(g_DrGame);
		CAM(0x54, int) = 0;
		return 0;

	case 7:
		d = CAM(0x54, float) + 1.0f;
		CAM(0x54, float) = d;
		if (d < 40.0f)
			CAM(0x48, float) = (20.0f - 0.5f * d) * 80.0f;
		else
			CAM(0x48, float) = 0.0f;
		CAM(0x20, float) = 2000.0f;
		CAM(0x10, float) = -CAM(0x10, float) * 0.0625f + CAM(0x10, float);
		CAM(0x14, float) = -CAM(0x14, float) * 0.0625f + CAM(0x14, float);
		CAM(0x18, float) = -CAM(0x18, float) * 0.0625f + CAM(0x18, float);
		if (d < 40.0f)
			a = DrAi_AngleDiff(DR_PTR(void, 0x002aadf8u), DR_ROUND_ANGLE, DR_ROUND_ANGLE_OLD) *
				0.025f * (40.0f - CAM(0x54, float));
		else
			a = 0.0f;
		a = a * DR_DEG;
		DR_EYE[0] = (sinf(a) * DR_DIST - DR_EYE[0]) * 0.125f + DR_EYE[0];
		DR_EYE[1] = ((CAM(0x20, float) - 1000.0f) - DR_EYE[1]) * 0.0625f + DR_EYE[1];
		DR_EYE[2] = (cosf(a) * DR_DIST - DR_EYE[2]) * 0.125f + DR_EYE[2];
		if (dr_fbits(CAM(0x54, float)) == 0x42700000u) {
			g_DrState = 3;
			return dr_camera_to_countdown();
		}
		return 0;
	}
	return 0;
}
