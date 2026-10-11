/*
ClassLib/Hud.c  (classlib_ui)
Original: C:\BlitzParty\Game\Src\ClassLib\Hud.cpp (tag at 0x00078cb0).

The in-game HUD shared by the minigames: per-player score boxes and timers, the
player markers over the characters (position 1-4, AI rings, "press start" prompts),
the countdown, the winner text and the score bars. Functions 0x00078870-0x0007d520
are placed here (0x00078cb0 is tagged; the others work on the HUD object, inferred).

The HUD object (400 bytes, pointer at 0x002ecfac) is accessed by offset:
  0x00-0x14  score bar layout      0x1c, 0x20  fonts (mslcd, handgotb)
  0x28  per-player score text overlays (4)   0x38  winner text overlay
  0x4c  per-player scores (4)       0x74  clock seconds
  0x78  per-player bar overlays (4) 0x88  dirty flag
  0xa8  per-player marker height (4)          0xb8  per-player marker timers (4)
  0xc8  markers enabled             0xcc  countdown state
  0xdc  "press start" overlay       0x150  per-player "out" overlays (4)
  0x160 per-player join prompts (4) 0x170  per-player position markers (4)
  0x180 per-player name markers (4)
*/

#include "classlib_ui.h"

#define OV_F(o, off) (*(float *)((o) + (off)))
#define OV_I(o, off) (*(int *)((o) + (off)))
#define OV_U(o, off) (*(unsigned int *)((o) + (off)))
#define OV_S(o, off) (*(short *)((o) + (off)))

/* ---------- globals (classlib_ui_data.h) */

#define clu_time_scale CLU_VAR(float, 0x0028fda8)
#define clu_characters CLU_ARR(const struct CLCharacterInfo, 0x002906f8)
/* 0x00290be0  life time of a score pop-up (frames) */
#define clu_popup_life CLU_VAR(float, 0x00290be0)
/* 0x00290be4  per-player marker height offsets (4 floats, 50.0) */
#define clu_marker_y CLU_ARR(float, 0x00290be4)
/* 0x00290bf8  score bar scale */
#define clu_bar_scale CLU_VAR(float, 0x00290bf8)
/* 0x00290bfc  "press start" pulse period (3.0) */
#define clu_pulse_period CLU_VAR(float, 0x00290bfc)
/* 0x00290c00  position marker height offset (50.0) */
#define clu_position_marker_y CLU_VAR(float, 0x00290c00)
/* 0x002ecc74  language text table */
#define clu_language CLU_VAR(int, 0x002ecc74)
/* 0x002ecdec  frames since a controller was unplugged */
#define clu_unplugged_time CLU_VAR(int, 0x002ecdec)
/* 0x002ecf08-0x002ecf14  position marker textures pos1-pos4.bmp */
#define clu_position_textures CLU_ARR(int, 0x002ecf08)
/* 0x002ecf90  per-player marker x offsets (4 floats) */
#define clu_marker_x CLU_ARR(float, 0x002ecf90)
/* 0x002ecfa8  font handgotl.ttf */
#define clu_font_light CLU_VAR(int, 0x002ecfa8)
/* 0x002ecfac  the HUD object */
#define clu_hud CLU_VAR(int, 0x002ecfac)
/* 0x002ecfb8 / 0x002ecfbc  AI player ring textures */
#define clu_ai_ring CLU_VAR(int, 0x002ecfb8)
#define clu_ai_ring_nogoal CLU_VAR(int, 0x002ecfbc)
/* 0x002ecfc0  per-player position marker show counters (4) */
#define clu_position_marker_count CLU_ARR(int, 0x002ecfc0)
/* 0x002ecfd0  HUD clock (frames) */
#define clu_hud_clock CLU_VAR(float, 0x002ecfd0)

/* the language text at byte offset `off` of the text table (UTF-16) */
#define CL_TEXT(off) (*(unsigned short **)(*(int *)(clu_language + 0x20) + (off)))

/* globals of other modules (tentative definitions in classlib_ui_extern.c) */
extern int DAT_002bae94;       /* core: player count */
extern int DAT_002baed0;       /* core: game over */
extern int DAT_002baed4[40];   /* core: player table, character at +0 (10 ints per player) */
extern int DAT_002baed8[40];   /* core: player table +4: player present */
extern int DAT_002baeb8;       /* core: game state (results shown) */
extern int DAT_002b9c48[4];    /* core: per-player flag shown on the join prompt */
extern float DAT_002855f0[16]; /* engine: identity matrix */
extern float DAT_002ed350[16]; /* per-player world positions (x, y, z, w), bumpercars data */

/* ---------- creation and small setters */

/* 0x00078870  ClassLib/Hud.cpp (inferred)
HUD constructor: clears the object and loads the fonts. */
int *CLHud_Construct(int *self)
{
	memset(self, 0, 100 * 4);
	self[7] = (int)FUN_0004bb30(0, "mslcd.ttf", 3, 0xdefa);
	clu_font_light = (int)FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa);
	self[0xe] = 0;
	self[0x10] = 0;
	self[8] = (int)FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
	self[0xf] = 0x4e6e6b28; /* 1e9f */
	return self;
}

/* 0x000788e0  ClassLib/Hud.cpp (inferred)
Shows text `text` (index 0x194 + text of the text table) on player `player`'s name
marker for 120 frames. */
void CLHud_ShowMarkerText(int self, int player, int text)
{
	OV_U(OV_I(self, 0x180 + player * 4), 0x2a4) = *(unsigned int *)((text + 0x194) * 0x10 + *(int *)(clu_language + 0x20));
	OV_I(OV_I(self, 0x180 + player * 4), 0xa0) = clu_font_light;
	OV_F(self, 0xb8 + player * 4) = 120.0f;
}

/* 0x00078930  ClassLib/Hud.cpp (inferred) */
void CLHud_StopCountdown(int self)
{
	OV_I(self, 0xcc) = 0;
}

/* 0x00078940  ClassLib/Hud.cpp (inferred) */
void CLHud_ResetCountdown(int self)
{
	OV_I(self, 0x90) = OV_I(self, 0x18);
	OV_I(self, 0xcc) = 0;
}

/* 0x00078960  ClassLib/Hud.cpp (inferred)
Writes every player's score (scores[player]) as "%d" into its score text. */
void CLHud_SetScores(int self, int *scores)
{
	char text[64];
	int player;

	for (player = 0; player < DAT_002bae94; player++) {
		sprintf(text, "%d", scores[player]);
		if (OV_I(self, 0x28 + player * 4) != 0)
			FUN_0004a330((ushort *)(OV_I(self, 0x28 + player * 4) + 0xa4), (byte *)text);
	}
}

/* 0x000789c0  ClassLib/Hud.cpp (inferred)
Shows "value/max" for a player, or the player's "out" overlay when value > max. */
void CLHud_SetFraction(int self, int player, int value, int max)
{
	char text[64];
	int overlay;

	overlay = OV_I(self, 0x28 + player * 4);
	if (overlay == 0)
		return;
	if (value < 0)
		value = 0;
	if (max < value) {
		OV_S(overlay, 0x54) = 1;
		overlay = OV_I(self, 0x150 + player * 4);
		if (overlay != 0)
			OV_S(overlay, 0x54) = 0;
		return;
	}
	OV_S(overlay, 0x54) = 0;
	overlay = OV_I(self, 0x150 + player * 4);
	if (overlay != 0)
		OV_S(overlay, 0x54) = 1;
	sprintf(text, "%d/%d", value, max);
	FUN_0004a330((ushort *)(OV_I(self, 0x28 + player * 4) + 0xa4), (byte *)text);
}

/* 0x00078a50  ClassLib/Hud.cpp (inferred)
Sets the target width of a player's score bar. */
void CLHud_SetBar(int self, int player, float value)
{
	OV_F(OV_I(self, 0x78 + player * 4), 0x36c) = clu_bar_scale * value;
}

/* 0x00078a70  ClassLib/Hud.cpp (inferred; thiscall)
Starts the score bar animation over `width` (and shows the name markers). */
void CLHud_StartBars(float *self, float width)
{
	if (self[0x32] != 0.0f) {
		if (DAT_002baed8[0] != 0)
			*(short *)(*(int *)&self[0x60] + 0x54) = 0; /* the slot holds a pointer */
		if (DAT_002baed8[10] != 0)
			*(short *)(*(int *)&self[0x61] + 0x54) = 0; /* the slot holds a pointer */
		if (DAT_002baed8[20] != 0)
			*(short *)(*(int *)&self[0x62] + 0x54) = 0; /* the slot holds a pointer */
		if (DAT_002baed8[30] != 0)
			*(short *)(*(int *)&self[99] + 0x54) = 0; /* the slot holds a pointer */
	}
	self[2] = 0.0f;
	self[4] = 0.0f;
	self[5] = 0.0f;
	width = width * 0.25f;
	self[0] = width;
	self[1] = width;
	self[3] = width * 0.75f;
	FUN_00084d80();
}

/* 0x00078af0  ClassLib/Hud.cpp (inferred)
Makes an overlay a score pop-up: it grows from 0 to 10 times its size and fades out. */
void CLHud_MakePopup(int overlay)
{
	OV_F(overlay, 0x3dc) = clu_popup_life;
	OV_U(overlay, 0x2f4) = OV_U(overlay, 0x2f4) | 1;
	OV_I(overlay, 0x2c4) = 1;
	OV_U(overlay, 0x2f8) = OV_U(overlay, 0x2f8) | 1;
	OV_F(overlay, 0x378) = 0.02f;
	OV_F(overlay, 0x380) = 0.02f;
	OV_F(overlay, 0x388) = 0.02f;
	OV_I(overlay, 0x2bc) = -10;
	OV_I(overlay, 0x2c0) = 0x100;
	OV_F(overlay, 0x2b8) = 175.0f;
	OV_F(overlay, 0x38c) = 0.0f;
	OV_F(overlay, 0x390) = 10.0f;
	OV_F(overlay, 0x394) = 0.0f;
	OV_F(overlay, 0x398) = 10.0f;
	OV_F(overlay, 0x39c) = 0.0f;
	OV_F(overlay, 0x3a0) = 10.0f;
}

/* 0x00078b90  ClassLib/Hud.cpp (inferred)
Fades an overlay in (alpha +25 per frame up to 256). */
void CLHud_FadeIn(int overlay)
{
	OV_U(overlay, 0x2f4) = OV_U(overlay, 0x2f4) | 1;
	OV_I(overlay, 0x2bc) = 0x19;
	OV_I(overlay, 0x2c0) = 0x100;
	OV_I(overlay, 0x2c4) = 1;
}

/* 0x00078bd0  ClassLib/Hud.cpp (inferred; thiscall)
Deletes the winner text. */
void CLHud_DeleteWinnerText(int self)
{
	if (OV_I(self, 0x38) != 0)
		COverlay_Delete(OV_I(self, 0x38));
	OV_I(self, 0x38) = 0;
}

/* 0x00078bf0  ClassLib/Hud.cpp (inferred)
Copies the players' scores (raw 32-bit values). */
void CLHud_CopyScores(int self, undefined4 *scores)
{
	int player;

	for (player = 0; player < DAT_002bae94; player++)
		OV_U(self, 0x4c + player * 4) = scores[player];
}

/* 0x00078c20  ClassLib/Hud.cpp (inferred)
Sets the players' scores from integers. */
void CLHud_SetScoresInt(int self, int *scores)
{
	int player;

	for (player = 0; player < DAT_002bae94; player++)
		OV_F(self, 0x4c + player * 4) = (float)scores[player];
}

/* 0x00078c50  ClassLib/Hud.cpp (inferred)
Shows player `player`'s position marker for 7 frames. */
void CLHud_ShowPositionMarker(int player)
{
	clu_position_marker_count[player] = 7;
}

/* 0x00078c70  ClassLib/Hud.cpp (inferred; thiscall)
Adds seconds to the clock and wraps it below 60. */
void CLHud_AddSeconds(int self, float seconds)
{
	double t;

	OV_F(self, 0x74) = seconds + OV_F(self, 0x74);
	if (OV_F(self, 0x74) > 60.0f) {
		t = (double)OV_F(self, 0x74);
		do {
			t = t - (double)60.0f;
		} while (t > (double)60.0f);
		OV_F(self, 0x74) = (float)t;
	}
}

/* 0x00078cb0  ClassLib/Hud.cpp
Creates the HUD and loads its marker textures. No parameters: the original's only caller
(core 0x000282d4) pushes nothing, and its "push ecx" before bkHeapAlloc reserves a stack
slot (the call pops 0x10 bytes: 4 arguments). */
void CLHud_Create(void)
{
	int memory;
	int i;

	memory = (int)((void *(*)(unsigned int, const char *, int, unsigned int))FUN_0004c480)(
		400, "C:\\BlitzParty\\Game\\Src\\ClassLib\\Hud.cpp", 0x3f, 0x1001);
	if (memory == 0)
		clu_hud = 0;
	else
		clu_hud = (int)CLHud_Construct((int *)memory);
	clu_ai_ring = (int)FUN_0004bb30(0, "ai_player_ring.bmp", 0, 0xdefa);
	clu_ai_ring_nogoal = (int)FUN_0004bb30(0, "ai_player_ringnogoal.bmp", 0, 0xdefa);
	for (i = 0; i < 4; i++) {
		OV_F(clu_hud, 0xa8 + i * 4) = 275.0f;
		clu_marker_y[i] = 50.0f;
		clu_marker_x[i] = 0.0f;
	}
	clu_position_textures[0] = (int)FUN_0004bb30(0, "pos1.bmp", 0, 0xdefa);
	clu_position_textures[1] = (int)FUN_0004bb30(0, "pos2.bmp", 0, 0xdefa);
	clu_position_textures[2] = (int)FUN_0004bb30(0, "pos3.bmp", 0, 0xdefa);
	clu_position_textures[3] = (int)FUN_0004bb30(0, "pos4.bmp", 0, 0xdefa);
	OV_I(clu_hud, 200) = 1;
}

/* ---------- per-frame */

/* 0x00078e00  ClassLib/Hud.cpp (inferred; thiscall)
Updates the "press start" prompts and the per-player join prompts. */
void CLHud_UpdatePrompts(int self)
{
	int player;
	int prompt;
	double pulse;

	if (clu_unplugged_time == 0)
		clu_hud_clock = clu_time_scale + clu_hud_clock;
	if (OV_I(self, 0xdc) != 0) {
		pulse = sin((double)clu_hud_clock / ((double)clu_pulse_period + (double)clu_pulse_period));
		OV_F(OV_I(self, 0xdc), 0x2b8) = (float)((pulse + (double)1.0f) * (double)70.0f);
	}
	for (player = 0; player < DAT_002bae94; player++) {
		prompt = OV_I(self, 0x160 + player * 4);
		if (prompt == 0) {
			OV_F(self, 0x98 + player * 4) = 1.0f;
		} else if (DAT_002baed0 == 0) {
			if (DAT_002baed8[player * 10] == 0) {
				if (OV_I(self, 8) != 5 && DAT_002baeb8 == 0) {
					OV_F(self, 0x98 + player * 4) = 0.0f;
					pulse = sin((double)clu_hud_clock / ((double)clu_pulse_period + (double)clu_pulse_period));
					OV_F(prompt, 0x2b8) = (float)((pulse + (double)1.0f) * (double)70.0f);
					OV_S(prompt, 0x54) = 0;
					if (OV_I(self, 0x28 + player * 4) != 0)
						OV_F(OV_I(self, 0x28 + player * 4), 0x2b8) = 0.0f;
					continue;
				}
				if (OV_I(self, 0x38) != 0)
					goto hide;
				OV_S(OV_I(self, 0xe0 + player * 4), 0x54) = (short)DAT_002baeb8;
			} else {
			hide:
				OV_S(OV_I(self, 0xe0 + player * 4), 0x54) = 1;
			}
			OV_S(prompt, 0x54) = 1;
			OV_F(self, 0x98 + player * 4) = 1.0f;
			if (DAT_002baed0 == 0) {
				if (OV_I(self, 0xf0 + player * 4) != 0) {
					OV_S(OV_I(self, 0xf0 + player * 4), 0x54) = (short)DAT_002baeb8;
					OV_F(OV_I(self, 0xf0 + player * 4), 0x2b8) = 127.0f;
				}
				if (OV_I(self, 0x28 + player * 4) != 0)
					OV_F(OV_I(self, 0x28 + player * 4), 0x2b8) = 127.0f;
			}
		} else {
			OV_S(OV_I(self, 0x28 + player * 4), 0x54) = 1;
			OV_S(OV_I(self, 0x100 + player * 4), 0x54) = 1;
			OV_S(OV_I(self, 0xe0 + player * 4), 0x54) = 1;
			if (DAT_002b9c48[player] == 0)
				OV_S(OV_I(self, 0xf0 + player * 4), 0x54) = 0;
			else
				OV_S(OV_I(self, 0xf0 + player * 4), 0x54) = 1;
		}
	}
}

/* 0x00078fb0  ClassLib/Hud.cpp (inferred; thiscall)
Places the per-player markers over the characters (projected from their world
positions) and counts their timers down. */
void CLHud_UpdateMarkers(int self)
{
	float in[4];
	float out[4];
	float *timer;
	int marker;
	int player;
	const unsigned char *rgb;

	for (player = 0; player < 4; player++) {
		timer = (float *)(self + 0xb8 + player * 4);
		marker = OV_I(self, 0x180 + player * 4);
		if (DAT_002baed8[player * 10] == 0) {
			*timer = 0.0f;
			if (marker != 0)
				OV_S(marker, 0x54) = 1;
		}
		if (!(0 < clu_position_marker_count[player] ||
		      (marker != 0 && (OV_S(marker, 0x54) == 0 || *timer != 0.0f))))
			continue;
		if (clu_unplugged_time == 0 && 0.0f < *timer && OV_I(self, 200) != 0) {
			if (DAT_002baed8[player * 10] != 0)
				OV_S(marker, 0x54) = 0;
			*timer = *timer - clu_time_scale;
			if (*timer <= 0.0f) {
				*timer = 0.0f;
				OV_S(marker, 0x54) = 1;
				OV_U(marker, 0x2a4) = *(unsigned int *)(*(int *)(clu_language + 0x20) + 0x1900 + player * 0x10);
				OV_I(marker, 0xa0) = OV_I(self, 0x20);
			}
		}
		FUN_00043930((undefined4 *)DAT_002855f0);
		in[1] = DAT_002ed350[player * 4 + 1] + OV_F(self, 0xa8 + player * 4);
		in[0] = DAT_002ed350[player * 4];
		in[2] = DAT_002ed350[player * 4 + 2];
		in[3] = 1.0f;
		FUN_00043b70(1);
		FUN_00043c10(out, in, 1);
		rgb = (const unsigned char *)&clu_characters[DAT_002baed4[player * 10]].colour;
		if (clu_position_marker_count[player] < 1) {
			OV_F(marker, 0x300) = out[3] > 0.0f ? (float)((double)out[0] + (double)clu_marker_x[player]) : -10000.0f;
			OV_F(marker, 0x304) = (float)((double)out[1] + (double)clu_marker_y[player]);
			OV_F(marker, 0x308) = out[2];
			if ((OV_U(marker, 0x2f4) & 1) == 0) {
				OV_U(marker, 0x2f4) = OV_U(marker, 0x2f4) | 1;
				OV_F(marker, 0x2b8) = 127.0f;
				OV_F(marker, 0x2ac) = (float)rgb[0];
				OV_F(marker, 0x2b0) = (float)rgb[1];
				OV_F(marker, 0x2b4) = (float)rgb[2];
			}
		} else {
			marker = OV_I(self, 0x170 + player * 4);
			OV_F(marker, 0x300) = out[3] > 0.0f ? out[0] : -10000.0f;
			OV_F(marker, 0x304) = (float)((double)clu_position_marker_y + (double)out[1]);
			OV_F(marker, 0x308) = out[2];
			OV_S(OV_I(self, 0x170 + player * 4), 0x54) = 0;
			OV_S(OV_I(self, 0x180 + player * 4), 0x54) = 1;
			*timer = 0.0f;
			if ((OV_U(marker, 0x2f4) & 1) == 0) {
				OV_U(marker, 0x2f4) = OV_U(marker, 0x2f4) | 1;
				OV_F(marker, 0x2b8) = 127.0f;
				OV_F(marker, 0x2ac) = (float)rgb[0];
				OV_F(marker, 0x2b0) = (float)rgb[1];
				OV_F(marker, 0x2b4) = (float)rgb[2];
				clu_position_marker_count[player] = clu_position_marker_count[player] + -1;
				if (clu_position_marker_count[player] == 0)
					OV_S(OV_I(self, 0x170 + player * 4), 0x54) = 1;
			}
		}
	}
}

/* ---------- clock and score boxes */

/* overlay descriptor fields (the 0x3b0-byte descriptor of COverlay_Create*) */
#define DESC_I(d, off) (*(int *)((unsigned char *)(d) + (off)))
#define DESC_U(d, off) (*(unsigned int *)((unsigned char *)(d) + (off)))
#define DESC_F(d, off) (*(float *)((unsigned char *)(d) + (off)))
#define DESC_S(d, off) (*(short *)((unsigned char *)(d) + (off)))

/* 0x00290c04  colour fade rate of the clock in the last seconds */
#define clu_clock_flash_rate CLU_VAR(int, 0x00290c04)
/* 0x00290c98 / 0x00290c9c  score box spacing and width on the results screen */
#define clu_box_gap CLU_VAR(float, 0x00290c98)
#define clu_box_width CLU_VAR(float, 0x00290c9c)
/* 0x002ecf3c  versus animation state */
#define clu_versus_state CLU_VAR(int, 0x002ecf3c)

extern int DAT_002baea8;     /* core: quit to the board allowed */
extern int DAT_002baebc;     /* core: team game */
extern int DAT_002ba6a4;     /* core */
extern int DAT_002b9c68;     /* core: number of score boxes on the results screen */

/* 0x000792f0  ClassLib/Hud.cpp (inferred; thiscall)
Shows the clock "m:ss" (or "-:--" for -1). With `sound`, the last ten seconds flash
and tick and the commentator says "almost over" once. */
void CLHud_SetClock(int self, float seconds, int sound)
{
	char text[64];
	float shown;
	int whole;
	int previous;

	if (sound != 0)
		CLHud_UpdatePrompts(self);
	if (seconds == -1.0f) {
		sprintf(text, "-:--");
		if (OV_I(self, 0xd0) != 0)
			FUN_0004a330((ushort *)(OV_I(self, 0xd0) + 0xa4), (byte *)text);
		return;
	}
	shown = seconds + 0.999f;
	OV_U(self, 0x94) = OV_U(self, 0x90);
	OV_F(self, 0x90) = seconds;
	whole = (int)((double)seconds + (double)0.999f) % 0x3c;
	sprintf(text, "%d:%02d", (int)((double)shown * (double)0.016666668f), whole);
	if (OV_I(self, 0xd0) != 0)
		FUN_0004a330((ushort *)(OV_I(self, 0xd0) + 0xa4), (byte *)text);
	if (sound == 0 || !(shown < 11.0f))
		return;
	previous = (int)((double)OV_F(self, 0x94) + (double)0.999f) % 0x3c;
	if (whole != previous) {
		if (OV_I(self, 0xd0) != 0) {
			OV_F(OV_I(self, 0xd0), 0x2b0) = 0.0f;
			OV_F(OV_I(self, 0xd0), 0x2b4) = 0.0f;
			OV_U(OV_I(self, 0xd0), 0x2f4) = OV_U(OV_I(self, 0xd0), 0x2f4) | 1;
			OV_I(OV_I(self, 0xd0), 0x2bc) = clu_clock_flash_rate;
		}
		CLSamples_PlayGeneric(7, 0x7f, 0, 0, NULL);
	}
	if (OV_I(self, 0xcc) == 0) {
		CLSamples_PlayCommentary(0x1b, -1, 0xff, 0, 0, NULL);
		OV_I(self, 0xcc) = 1;
	}
}

/* 0x00079490  ClassLib/Hud.cpp (inferred; thiscall)
Creates the clock (seconds `seconds`) and, when the game may be quit or the results
are shown, the "press start" prompt. */
void CLHud_CreateClock(int self, float seconds)
{
	undefined4 desc[0xec];
	char text[68];
	int clock;

	OV_F(self, 0x18) = seconds;
	OV_F(self, 0x90) = seconds;
	memset(desc, 0, sizeof(desc));
	DESC_I(desc, 0x2ac) = 0;
	DESC_F(desc, 0x258) = 0.0f;
	DESC_I(desc, 0x264) = 0x7f;
	DESC_I(desc, 0x268) = 0x7f;
	DESC_I(desc, 0x278) = 0x7f;
	DESC_I(desc, 0x26c) = 0x7f;
	OV_I(self, 0x38) = 0;
	OV_I(self, 8) = 0;
	DESC_I(desc, 0x288) = 2;
	DESC_I(desc, 0x28c) = 1;
	DESC_I(desc, 0x298) = 8;
	DESC_F(desc, 0x2a4) = -260.0f;
	DESC_F(desc, 0x2a8) = 174.0f;
	DESC_F(desc, 0x250) = 127.0f;
	DESC_F(desc, 0x254) = 70.0f;
	DESC_F(desc, 0x25c) = 127.0f;
	DESC_I(desc, 0x44) = OV_I(self, 0x1c);
	DESC_I(desc, 0x274) = 0;
	DESC_I(desc, 0x270) = 0x46;
	DESC_I(desc, 0x280) = 0;
	DESC_I(desc, 0x27c) = 0;
	sprintf(text, "%2.0f", (double)seconds);
	FUN_0004a330((ushort *)((unsigned char *)desc + 0x48), (byte *)text);
	DESC_U(desc, 0x290) = 0x8000;
	clock = COverlay_Create((int *)desc);
	OV_I(self, 0xd0) = clock;
	if (DAT_002baeb8 != 0)
		OV_S(clock, 0x54) = 1;
	CLHud_SetClock(self, OV_F(self, 0x90), 0);
	OV_I(self, 0xcc) = 0;
	if ((DAT_002baea8 != 0 || DAT_002baeb8 != 0) && DAT_002ba6a4 == 0) {
		DESC_I(desc, 0x44) = OV_I(self, 0x20);
		DESC_F(desc, 0x2a4) = 0.0f;
		DESC_F(desc, 0x2a8) = 194.0f;
		DESC_I(desc, 0x2ac) = 0;
		DESC_F(desc, 0x250) = 127.0f;
		DESC_F(desc, 0x254) = 127.0f;
		DESC_F(desc, 0x258) = 127.0f;
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_S(desc, 0x24c) = 1;
		if (DAT_002baea8 == 0)
			DESC_U(desc, 0x248) = *(unsigned int *)(*(int *)(clu_language + 0x20) + 0x1b00);
		else
			DESC_U(desc, 0x248) = *(unsigned int *)(*(int *)(clu_language + 0x20) + 0x1260);
		DESC_U(desc, 0x298) = DESC_U(desc, 0x298) | 4;
		OV_I(self, 0xdc) = COverlay_Create((int *)desc);
		return;
	}
	OV_I(self, 0xdc) = 0;
}

/* sets the scale animation of a descriptor: scale x, y and rotation start at 20 and
shrink by 0.6 per frame to 1 */
static void CLHud_DescZoomIn(undefined4 *desc)
{
	DESC_F(desc, 0x31c) = -0.6f;
	DESC_F(desc, 0x324) = -0.6f;
	DESC_F(desc, 0x32c) = -0.6f;
	DESC_F(desc, 0x318) = 20.0f;
	DESC_F(desc, 0x320) = 20.0f;
	DESC_F(desc, 0x328) = 20.0f;
	DESC_F(desc, 0x334) = 20.0f;
	DESC_F(desc, 0x33c) = 20.0f;
	DESC_F(desc, 0x344) = 20.0f;
	DESC_F(desc, 0x330) = 1.0f;
	DESC_F(desc, 0x338) = 1.0f;
	DESC_F(desc, 0x340) = 1.0f;
}

/* sets the colour r, g, b of a descriptor from a character's colour */
static void CLHud_DescCharacterColour(undefined4 *desc, int character)
{
	const unsigned char *rgb = (const unsigned char *)&clu_characters[character].colour;

	DESC_F(desc, 0x250) = (float)rgb[0];
	DESC_F(desc, 0x254) = (float)rgb[1];
	DESC_F(desc, 0x258) = (float)rgb[2];
}

/* 0x000796c0  ClassLib/Hud.cpp (inferred; thiscall)
Creates the score boxes: "vs" for team games, per player a box (portrait on the
results screen), score line, CPU label, rank digits and name, and the per-player
name markers and position markers. */
void CLHud_CreateScoreBoxes(int self)
{
	undefined4 desc[0xec];
	char name[36];
	const unsigned char *rgb;
	int *rank_slot;
	int box;
	int child;
	int player;
	int slot;
	int results_index;
	unsigned int box_y;
	int k;
	int text;

	memset(desc, 0, sizeof(desc));
	OV_I(self, 0xd0) = 0;
	box_y = 0xc3070000; /* -135.0f */
	if (DAT_002baebc == 0) {
		OV_I(self, 0xd8) = 0;
		OV_I(self, 0xd4) = 0;
	} else {
		clu_versus_state = 0;
		DESC_I(desc, 0x288) = 0;
		DESC_F(desc, 0x2a4) = 0.0f;
		DESC_F(desc, 0x2a8) = -170.0f;
		DESC_I(desc, 0x2ac) = 0;
		DESC_F(desc, 0x250) = 127.0f;
		DESC_F(desc, 0x254) = 127.0f;
		DESC_F(desc, 0x258) = 127.0f;
		DESC_F(desc, 0x25c) = 0.0f;
		DESC_F(desc, 0x308) = 64.0f;
		DESC_F(desc, 0x30c) = 64.0f;
		DESC_I(desc, 0x28c) = 3;
		DESC_I(desc, 0x260) = 2;
		DESC_I(desc, 0x268) = 0;
		DESC_I(desc, 0x264) = 0x7f;
		DESC_I(desc, 0x37c) = 0;
		CLHud_DescZoomIn(desc);
		DESC_I(desc, 4) = (int)FUN_0004bb30(0, "vs.bmp", 0, 0xdefa);
		OV_I(self, 0xd4) = COverlay_Create((int *)desc);
		OV_S(OV_I(self, 0xd4), 0x54) = 1;
		DESC_S(desc, 0x24c) = 1;
		DESC_S(desc, 0x24e) = 1;
		DESC_I(desc, 0x288) = 2;
		DESC_U(desc, 0x290) = 0x8000;
		DESC_F(desc, 0x2b4) = -1.0f;
		DESC_F(desc, 0x2b8) = -7.0f;
		DESC_I(desc, 0x2bc) = 0;
		DESC_F(desc, 0x250) = 127.0f;
		DESC_F(desc, 0x254) = 127.0f;
		DESC_F(desc, 0x258) = 127.0f;
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_F(desc, 0x308) = 4.0f;
		DESC_F(desc, 0x30c) = 18.0f;
		DESC_I(desc, 0x28c) = 3;
		DESC_I(desc, 0x260) = 2;
		DESC_I(desc, 0x268) = 0;
		DESC_I(desc, 0x264) = 0x7f;
		DESC_I(desc, 0x37c) = 0;
		CLHud_DescZoomIn(desc);
		DESC_I(desc, 0x44) = clu_font_light;
		DESC_F(desc, 0x348) = 0.33f;
		DESC_U(desc, 0x248) = *(unsigned int *)(*(int *)(clu_language + 0x20) + 0x18c0);
		OV_I(self, 0xd8) = COverlay_CreateChild(OV_I(self, 0xd4), desc);
	}

	results_index = 0;
	rank_slot = (int *)(self + 0x110);
	for (player = 0; player < DAT_002bae94; player++) {
		memset(desc, 0, sizeof(desc));
		OV_I(self, 0x28 + player * 4) = 0;
		OV_I(self, 0x150 + player * 4) = 0;
		DESC_I(desc, 0x288) = 0;
		if (DAT_002baed0 != 0) {
			DESC_F(desc, 8) = (float)(((double)clu_box_gap + (double)clu_box_gap + (double)clu_box_width) *
							  (double)results_index -
						  ((double)(DAT_002b9c68 - 1) * (double)clu_box_gap +
						   (double)DAT_002b9c68 * (double)clu_box_width * (double)0.5f));
			if (DAT_002b9c48[player] == 0)
				results_index = results_index + 1;
			box_y = 0xc2f00000; /* -120.0f */
		} else {
			if (DAT_002baebc != 0)
				slot = DAT_002baed4[player * 10 + 9] + DAT_002baed4[player * 10 + 8] * 2;
			else
				slot = player;
			DESC_F(desc, 8) = (float)((double)slot * (double)136.0f - (double)270.0f);
		}
		DESC_F(desc, 0x2a4) = DESC_F(desc, 8);
		DESC_U(desc, 0x2a8) = box_y;
		DESC_U(desc, 0xc) = box_y;
		DESC_I(desc, 0x28c) = DESC_I(desc, 0x28c) | 4;
		DESC_I(desc, 0x2ac) = 0;
		DESC_I(desc, 0x10) = 0;
		if (DAT_002baebc != 0)
			DESC_I(desc, 0x28c) = 4;
		DESC_U(desc, 0x290) = 0x8000;
		rgb = (const unsigned char *)&clu_characters[DAT_002baed4[player * 10]].colour;
		if (DAT_002baed0 != 0) {
			DESC_I(desc, 4) = (int)FUN_0004bb30(0, (char *)clu_characters[DAT_002baed4[player * 10]].portrait, 0, 0xdefa);
			DESC_F(desc, 0x250) = 127.0f;
			DESC_F(desc, 0x254) = 127.0f;
			DESC_F(desc, 0x258) = 127.0f;
		} else {
			DESC_I(desc, 4) = (int)FUN_0004bb30(0, "score4.tga", 0, 0xdefa);
			DESC_F(desc, 0x250) = (float)(rgb[0] / 3);
			DESC_F(desc, 0x254) = (float)(rgb[1] / 3);
			DESC_F(desc, 0x258) = (float)(rgb[2] / 3);
		}
		DESC_F(desc, 0x25c) = 127.0f;
		box = COverlay_Create((int *)desc);
		OV_I(self, 0xf0 + player * 4) = box;
		OV_S(box, 0x54) = (short)DAT_002baeb8;

		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_I(desc, 4) = (int)FUN_0004bb30(0, "scoreline.tga", 0, 0xdefa);
		OV_I(self, 0x100 + player * 4) = COverlay_CreateChild(box, desc);

		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_U(desc, 0x290) = 0x8000;
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_I(desc, 4) = (int)FUN_0004bb30(0, "cpu.tga", 0, 0xdefa);
		child = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
		OV_I(self, 0xe0 + player * 4) = child;
		OV_S(child, 0x54) = (short)DAT_002baed8[player * 10];

		DESC_U(desc, 0x290) = 0;
		for (k = 0; k < 4; k++) {
			CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
			DESC_F(desc, 0x25c) = 127.0f;
			sprintf(name, "score%d.tga", k + 1);
			DESC_I(desc, 4) = (int)FUN_0004bb30(0, name, 0, 0xdefa);
			child = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
			*rank_slot = child;
			rank_slot++;
			OV_S(child, 0x54) = 1;
		}

		memset(desc, 0, sizeof(desc));
		DESC_I(desc, 0x44) = clu_font_light;
		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_I(desc, 0x288) = 2;
		DESC_F(desc, 0x2b4) = 95.0f;
		DESC_F(desc, 0x2b8) = 25.0f;
		DESC_F(desc, 0x25c) = 0.0f;
		DESC_S(desc, 0x24c) = 1;
		DESC_I(desc, 0x30c) = 0;
		DESC_F(desc, 0x40) = -6.0f;
		DESC_U(desc, 0x248) = *(unsigned int *)(*(int *)(clu_language + 0x20) + 0xca0);
		DESC_U(desc, 0x290) = 0x8000;
		OV_I(self, 0x160 + player * 4) = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
	}

	for (player = 0, text = 0x1900; text < 0x1940; player++, text += 0x10) {
		memset(desc, 0, sizeof(desc));
		DESC_I(desc, 0x268) = 0;
		DESC_I(desc, 0x44) = OV_I(self, 0x20);
		DESC_I(desc, 0x288) = 2;
		DESC_I(desc, 0x28c) = 1;
		DESC_I(desc, 0x260) = -3;
		DESC_I(desc, 0x264) = 0x7f;
		DESC_S(desc, 0x24c) = 1;
		DESC_U(desc, 0x248) = *(unsigned int *)((DAT_002baed4[player * 10] + 0x143) * 0x10 +
							*(int *)(clu_language + 0x20));
		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_F(desc, 0x40) = -10.0f;
		DESC_U(desc, 0x290) = 0x8000;
		child = COverlay_Create((int *)desc);
		OV_I(self, 0x170 + player * 4) = child;
		OV_S(child, 0x54) = 1;
		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_U(desc, 0x248) = *(unsigned int *)(text + *(int *)(clu_language + 0x20));
		DESC_U(desc, 0x290) = 0x8000;
		child = COverlay_Create((int *)desc);
		OV_I(self, 0x180 + player * 4) = child;
		OV_S(child, 0x54) = 1;
		OV_U(OV_I(self, 0x180 + player * 4), 0x2f4) = OV_U(OV_I(self, 0x180 + player * 4), 0x2f4) | 4;
		OV_F(self, 0xb8 + player * 4) = 0.0f;
	}
}

/* 0x002ecfa0  score bar state */
#define clu_bar_state CLU_VAR(int, 0x002ecfa0)

/* 0x0007a080  ClassLib/Hud.cpp (inferred; thiscall)
Creates the per-player score bars (thin bars in twice the character colour). */
void CLHud_CreateBars(int self)
{
	undefined4 desc[0xec];
	const unsigned char *rgb;
	int player;
	int slot;
	int bar;

	for (player = 0; player < DAT_002bae94; player++) {
		OV_I(self, 0x4c + player * 4) = 0;
		slot = player;
		if (DAT_002baebc != 0)
			slot = DAT_002baed4[player * 10 + 9] + DAT_002baed4[player * 10 + 8] * 2;
		memset(desc, 0, sizeof(desc));
		rgb = (const unsigned char *)&clu_characters[DAT_002baed4[player * 10]].colour;
		DESC_F(desc, 0x250) = (float)((unsigned int)rgb[0] << 1);
		DESC_F(desc, 0x254) = (float)((unsigned int)rgb[1] << 1);
		DESC_F(desc, 0x258) = (float)((unsigned int)rgb[2] << 1);
		DESC_I(desc, 0x288) = 0;
		DESC_F(desc, 0x2a8) = (float)(slot * -6 + 0xaa);
		DESC_F(desc, 0xc) = DESC_F(desc, 0x2a8);
		DESC_F(desc, 0x25c) = 255.0f;
		DESC_F(desc, 0x2a4) = -180.0f;
		DESC_I(desc, 4) = 0;
		DESC_F(desc, 0x308) = 1.0f;
		DESC_F(desc, 0x30c) = 5.0f;
		DESC_F(desc, 8) = -180.0f;
		DESC_F(desc, 0x284) = 4.0f;
		DESC_F(desc, 0x314) = 4.0f;
		DESC_U(desc, 0x290) = 0x8000;
		bar = COverlay_Create((int *)desc);
		OV_I(self, 0x78 + player * 4) = bar;
		OV_S(bar, 0x54) = (short)DAT_002baeb8;
		if (DAT_002baed0 != 0)
			OV_S(OV_I(self, 0x78 + player * 4), 0x54) = 1;
	}
	clu_bar_state = 5;
}

/* 0x0007a200  ClassLib/Hud.cpp (inferred; thiscall)
Creates the score texts ("%d" of scores[player]) in the score boxes. */
void CLHud_CreateScoreTexts(int self, int *scores)
{
	undefined4 desc[0xec];
	char text[64];
	int player;

	for (player = 0; player < DAT_002bae94; player++) {
		memset(desc, 0, sizeof(desc));
		DESC_I(desc, 0x44) = OV_I(self, 0x20);
		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_I(desc, 0x288) = 2;
		DESC_S(desc, 0x24c) = 2;
		DESC_U(desc, 0x290) = 0x8000;
		DESC_F(desc, 0x2b4) = 126.0f;
		DESC_F(desc, 0x2b8) = 20.0f;
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_F(desc, 0x30c) = 200.0f;
		sprintf(text, "%d", scores[player]);
		FUN_0004a330((ushort *)((unsigned char *)desc + 0x48), (byte *)text);
		OV_I(self, 0x28 + player * 4) = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
	}
}

/* 0x0007a360  ClassLib/Hud.cpp (inferred; thiscall)
Creates the same text `text` in every score box and, with `flag`, a hidden
chequered flag (cheqflag.bmp) for each player. */
void CLHud_CreateScoreLabels(int self, char *text, int flag)
{
	undefined4 desc[0xec];
	int player;
	int texture;
	int overlay;

	for (player = 0; player < DAT_002bae94; player++) {
		memset(desc, 0, sizeof(desc));
		DESC_I(desc, 0x44) = OV_I(self, 0x20);
		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_I(desc, 0x288) = 2;
		DESC_U(desc, 0x290) = 0x8000;
		DESC_F(desc, 0x2b4) = 126.0f;
		DESC_F(desc, 0x2b8) = 20.0f;
		DESC_F(desc, 0x25c) = 127.0f;
		DESC_S(desc, 0x24c) = 2;
		DESC_F(desc, 0x30c) = 200.0f;
		FUN_0004a330((ushort *)((unsigned char *)desc + 0x48), (byte *)text);
		OV_I(self, 0x28 + player * 4) = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
		if (flag != 0 && (texture = (int)FUN_0004bb30(0, "cheqflag.bmp", 0, 0xdefa)) != 0) {
			memset(desc, 0, sizeof(desc));
			DESC_I(desc, 0x288) = 0;
			DESC_F(desc, 0x250) = 127.0f;
			DESC_F(desc, 0x254) = 127.0f;
			DESC_F(desc, 0x258) = 127.0f;
			DESC_F(desc, 0x25c) = 127.0f;
			DESC_F(desc, 0x2b4) = 126.0f;
			DESC_F(desc, 0x2b8) = 20.0f;
			DESC_S(desc, 0x24c) = 2;
			DESC_U(desc, 0x290) = 0x8000;
			DESC_I(desc, 4) = texture;
			overlay = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
			OV_I(self, 0x150 + player * 4) = overlay;
			OV_S(overlay, 0x54) = 1;
		}
	}
}

/* 0x0007cc00  ClassLib/Hud.cpp (inferred)
Frees the HUD. */
void CLHud_Destroy(void)
{
	clu_ai_ring = 0;
	clu_ai_ring_nogoal = 0;
	if (clu_hud != 0)
		FUN_0004c110((void *)clu_hud);
	clu_hud = 0;
}

/* ---------- countdown */

/* 0x00290d14  colour ramp of the countdown models (16 ints, CRumble.cpp's .data) */
#define clu_trace_ramp CLU_ARR(int, 0x00290d14)
#define CL_RAMP(addr) CLU_VAR(int, addr)

/* 0x0007a550  ClassLib/Hud.cpp (inferred)
Shows countdown model `step` (1 "3", 2 "2", 3 "1", 4 "go"; 0 nothing) and sets the
colour ramp of its trace effect. Returns the overlay (0 for step 0). */
int CLHud_ShowCountdown(int step)
{
	undefined4 desc[0xec];
	float origin[4];
	const char *name;
	int overlay;

	memset(desc, 0, sizeof(desc));
	switch (step) {
	case 0:
		return 0;
	case 1:
		name = "trace_3.obe";
		break;
	case 2:
		name = "trace_2.obe";
		break;
	case 3:
		name = "trace_1.obe";
		break;
	case 4:
		name = "trace_go.obe";
		break;
	default:
		name = NULL;
		break;
	}
	if (name != NULL)
		sprintf((char *)desc + 0x2c4, "%s", name);
	DESC_I(desc, 0x288) = 1;
	DESC_F(desc, 0x2a4) = 0.0f;
	DESC_F(desc, 0x2a8) = 0.0f;
	DESC_F(desc, 0x2ac) = 50.0f;
	DESC_I(desc, 0x1c) = 0;
	DESC_I(desc, 0x20) = 0;
	DESC_I(desc, 0x24) = 0;
	DESC_I(desc, 0x2c) = 0;
	DESC_I(desc, 0x30) = 0;
	DESC_I(desc, 0x34) = 0;
	DESC_F(desc, 0x318) = 0.1f;
	DESC_F(desc, 0x320) = 0.1f;
	DESC_F(desc, 0x328) = 0.1f;
	DESC_I(desc, 0x28c) = 3;
	DESC_I(desc, 0x298) = 4;
	DESC_F(desc, 0x25c) = 0.0f;
	DESC_F(desc, 0x250) = 127.0f;
	DESC_F(desc, 0x254) = 127.0f;
	DESC_F(desc, 0x258) = 127.0f;
	overlay = COverlay_Create((int *)desc);
	OV_S(overlay, 0x54) = 0;
	switch (step) {
	case 1:
		CL_RAMP(0x002ecf64) = CL_RAMP(0x00290d18);
		CL_RAMP(0x002ecf60) = CL_RAMP(0x00290d14);
		CL_RAMP(0x002ecf6c) = CL_RAMP(0x00290d20);
		CL_RAMP(0x002ecf68) = CL_RAMP(0x00290d1c);
		CL_RAMP(0x002ecf84) = CL_RAMP(0x00290d28);
		CL_RAMP(0x002ecf80) = CL_RAMP(0x00290d24);
		CL_RAMP(0x002ecf8c) = CL_RAMP(0x00290d30);
		CL_RAMP(0x002ecf88) = CL_RAMP(0x00290d2c);
		CL_RAMP(0x002ecf30) = CL_RAMP(0x00290d38);
		CL_RAMP(0x002ecf2c) = CL_RAMP(0x00290d34);
		CL_RAMP(0x002ecf38) = CL_RAMP(0x00290d40);
		CL_RAMP(0x002ecf34) = CL_RAMP(0x00290d3c);
		CL_RAMP(0x002ecf1c) = CL_RAMP(0x00290d48);
		CL_RAMP(0x002ecf18) = CL_RAMP(0x00290d44);
		CL_RAMP(0x002ecf24) = CL_RAMP(0x00290d50);
		CL_RAMP(0x002ecf20) = CL_RAMP(0x00290d4c);
		CL_RAMP(0x00290d14) = 100;
		CL_RAMP(0x00290d24) = 100;
		CL_RAMP(0x00290d28) = 0;
		CL_RAMP(0x00290d34) = 100;
		CL_RAMP(0x00290d38) = 0;
		CL_RAMP(0x00290d40) = 0x5a;
		CL_RAMP(0x00290d44) = 0x1e;
		CL_RAMP(0x00290d48) = 0;
		break;
	case 2:
		CL_RAMP(0x00290d14) = 100;
		CL_RAMP(0x00290d28) = 0x36;
		CL_RAMP(0x00290d24) = 100;
		CL_RAMP(0x00290d34) = 100;
		CL_RAMP(0x00290d38) = 0x2c;
		CL_RAMP(0x00290d44) = 0x32;
		CL_RAMP(0x00290d48) = 0x18;
		CL_RAMP(0x00290d40) = 100;
		break;
	case 3:
		CL_RAMP(0x00290d14) = 100;
		CL_RAMP(0x00290d24) = 100;
		CL_RAMP(0x00290d28) = 100;
		CL_RAMP(0x00290d34) = 100;
		CL_RAMP(0x00290d38) = 100;
		CL_RAMP(0x00290d40) = 100;
		CL_RAMP(0x00290d44) = 0x28;
		CL_RAMP(0x00290d48) = 0x28;
		break;
	case 4:
		CL_RAMP(0x00290d28) = 0x56;
		CL_RAMP(0x00290d14) = 0;
		CL_RAMP(0x00290d24) = 0;
		CL_RAMP(0x00290d34) = 0;
		CL_RAMP(0x00290d38) = 0x56;
		CL_RAMP(0x00290d44) = 0;
		CL_RAMP(0x00290d48) = 0x28;
		CL_RAMP(0x00290d40) = 100;
		break;
	default:
		goto trace;
	}
	CL_RAMP(0x00290d30) = 100;
	CL_RAMP(0x00290d20) = 100;
	CL_RAMP(0x00290d1c) = 0;
	CL_RAMP(0x00290d2c) = 0;
	CL_RAMP(0x00290d3c) = 0;
	CL_RAMP(0x00290d4c) = 0;
	CL_RAMP(0x00290d50) = 0;
	CL_RAMP(0x00290d18) = CL_RAMP(0x00290d28);
trace:
	origin[0] = 240.0f;
	origin[1] = 60.0f;
	origin[2] = 600.0f;
	origin[3] = 1.0f;
	FUN_000846c0(OV_I(overlay, 0x74), (struct CProjRayType *)clu_trace_ramp, (float *)origin);
	return overlay;
}

/* 0x002ecf3c  versus animation state (also above) */
extern int DAT_002baec4;     /* core: the two teams' first players */
extern int DAT_002baec8;

/* restores the countdown colour ramp saved by CLHud_ShowCountdown */
static void CLHud_RestoreRamp(void)
{
	CL_RAMP(0x00290d14) = CL_RAMP(0x002ecf60);
	CL_RAMP(0x00290d18) = CL_RAMP(0x002ecf64);
	CL_RAMP(0x00290d1c) = CL_RAMP(0x002ecf68);
	CL_RAMP(0x00290d20) = CL_RAMP(0x002ecf6c);
	CL_RAMP(0x00290d24) = CL_RAMP(0x002ecf80);
	CL_RAMP(0x00290d28) = CL_RAMP(0x002ecf84);
	CL_RAMP(0x00290d2c) = CL_RAMP(0x002ecf88);
	CL_RAMP(0x00290d30) = CL_RAMP(0x002ecf8c);
	CL_RAMP(0x00290d34) = CL_RAMP(0x002ecf2c);
	CL_RAMP(0x00290d38) = CL_RAMP(0x002ecf30);
	CL_RAMP(0x00290d3c) = CL_RAMP(0x002ecf34);
	CL_RAMP(0x00290d40) = CL_RAMP(0x002ecf38);
	CL_RAMP(0x00290d44) = CL_RAMP(0x002ecf18);
	CL_RAMP(0x00290d48) = CL_RAMP(0x002ecf1c);
	CL_RAMP(0x00290d4c) = CL_RAMP(0x002ecf20);
	CL_RAMP(0x00290d50) = CL_RAMP(0x002ecf24);
}

/* 0x0007a900  ClassLib/Hud.cpp (inferred; thiscall)
Runs the start countdown (states 0-4 show "3", "2", "1", "go" with the commentator;
5 is playing). Returns 1 when the game may run. */
int CLHud_UpdateCountdown(int self)
{
	double t;
	int box_a, box_b;

	if (DAT_002baeb8 != 0)
		return 1;
	if (clu_unplugged_time != 0)
		return 0;
	if (OV_I(self, 8) == 5) {
		FUN_00084e20();
		return 1;
	}
	t = (double)OV_F(self, 4) - (double)clu_time_scale;
	OV_F(self, 4) = (float)t;
	if (t < (double)OV_F(self, 0xc) && (double)clu_time_scale + t > (double)OV_F(self, 0xc)) {
		if (OV_I(self, 8) != 0)
			CLHud_FadeIn(OV_I(self, 0x10));
		switch (OV_I(self, 8)) {
		case 0:
			CLSamples_PlayGeneric(0x18, 0x7f, 0, 0, NULL);
			CLSamples_PlayCommentary(2, -1, 0xff, 0, 0, NULL);
			break;
		case 1:
			CLSamples_PlayCommentary(0, 2, 0xff, 0, 0, NULL);
			FUN_000848c0();
			break;
		case 2:
			CLSamples_PlayCommentary(0, 1, 0xff, 0, 0, NULL);
			break;
		case 3:
			CLSamples_PlayCommentary(0, 0, 0xff, 0, 0, NULL);
			break;
		case 4:
			OV_S(OV_I(self, 0x180), 0x54) = 1;
			OV_S(OV_I(self, 0x184), 0x54) = 1;
			OV_S(OV_I(self, 0x188), 0x54) = 1;
			OV_S(OV_I(self, 0x18c), 0x54) = 1;
			CLSamples_PlayCommentary(1, -1, 0xff, 0, 0, NULL);
			if (OV_I(self, 0x88) == 0)
				CLMusic_PlayRandom(0x3ffff0, 0, 0xff, -0x14, 0x14, 0);
			break;
		}
	}
	if (DAT_002baebc != 0 && OV_F(OV_I(self, 0xd4), 0x374) == OV_F(OV_I(self, 0xd4), 0x38c) &&
	    clu_versus_state == 0) {
		clu_versus_state = 1;
		box_a = OV_I(self, 0xf0 + DAT_002baec4 * 4);
		box_b = OV_I(self, 0xf0 + DAT_002baec8 * 4);
		OV_F(box_a, 0x2e0) = 1.0f;
		OV_F(box_b, 0x2e0) = 1.0f;
		OV_F(OV_I(self, 0xf0 + DAT_002baec4 * 4), 0x64) = OV_F(OV_I(self, 0xf0 + DAT_002baec4 * 4), 0x300) - 8.0f;
		OV_F(OV_I(self, 0xf0 + DAT_002baec8 * 4), 0x64) = OV_F(OV_I(self, 0xf0 + DAT_002baec8 * 4), 0x300) + 8.0f;
	}
	if (OV_F(self, 4) < 0.0f) {
		OV_U(self, 4) = OV_U(self, 0);
		OV_I(self, 8) = OV_I(self, 8) + 1;
		CLHud_RestoreRamp();
		switch (OV_I(self, 8)) {
		case 1:
			if (DAT_002baebc != 0) {
				OV_S(OV_I(self, 0xd4), 0x54) = 0;
				OV_I(OV_I(self, 0xd4), 0x2f4) = 1;
				OV_I(OV_I(self, 0xd4), 0x2f8) = 1;
				OV_S(OV_I(self, 0xd8), 0x54) = 0;
				OV_I(OV_I(self, 0xd8), 0x2f4) = 1;
				OV_I(OV_I(self, 0xd8), 0x2f8) = 1;
				CLSamples_Play((int)FUN_0004bb30(0, "scorebarping.wav", 2, 0xdefa), 0xff, 0, 0, NULL);
			}
			OV_I(self, 0x10) = CLHud_ShowCountdown(1);
			OV_F(self, 0xc) = OV_F(self, 0) * 0.33333334f;
			CLU_DebugPrintf("countdown 3\n");
			return 0;
		case 2:
			CLHud_MakePopup(OV_I(self, 0x10));
			OV_I(self, 0x10) = CLHud_ShowCountdown(2);
			CLU_DebugPrintf("countdown 2\n");
			break;
		case 3:
			CLHud_MakePopup(OV_I(self, 0x10));
			OV_I(self, 0x10) = CLHud_ShowCountdown(3);
			CLU_DebugPrintf("countdown 1\n");
			return 0;
		case 4:
			CLHud_MakePopup(OV_I(self, 0x10));
			OV_I(self, 0x10) = CLHud_ShowCountdown(4);
			CLU_DebugPrintf("countdown GO....\n");
			return 0;
		case 5:
			CLHud_MakePopup(OV_I(self, 0x10));
			OV_I(self, 0x10) = 0;
			CLU_DebugPrintf("countdown PLAYING....\n");
			FUN_00084e20();
			return 1;
		}
	}
	return 0;
}

/* ---------- results */

/* 0x00290bf4  volume of the end whistle (generic sample 0) */
#define clu_end_whistle_volume CLU_VAR(int, 0x00290bf4)
/* 0x00290ca0  sound delays of the rank markers by rank (4 floats: 0, 20, 40, 60) */
#define clu_rank_delays CLU_ARR(unsigned int, 0x00290ca0)
/* 0x002ecf50  winners (player numbers 1-4, -1 ends the list) */
#define clu_winners CLU_ARR(int, 0x002ecf50)
/* 0x002ecf70  rank markers of the four players */
#define clu_rank_markers CLU_ARR(int, 0x002ecf70)

extern int DAT_002baec0;      /* core: team members */
extern int DAT_002baecc;
extern int DAT_002baeb4;      /* core: game type (commentator line offset) */
extern int DAT_002bae98;      /* core */
extern int DAT_00293928;      /* charlib: voice volume */
extern unsigned int DAT_002ba4cc; /* core */

/* 0x0007ad50  ClassLib/Hud.cpp (inferred; thiscall)
CreateWinnerText: ends the game with the whistle, works out the winner(s) from the
scores (teams in team games), shows "Player n wins" or "n and m draw" with the
commentator, then the rank markers on the score boxes. Returns the winner (player
index or team), -1 for a draw. */
int CLHud_CreateWinnerText(int self, int *scores, int with_game_type)
{
	undefined4 desc[0xec];
	char names[132];
	char clock[16];
	int count;
	int best;
	int result;
	int channel;
	int team_a, team_b;
	int player;
	int rank;
	int character;
	int marker;

	memset(clock, 0, sizeof(clock));
	memset(desc, 0, sizeof(desc));
	memcpy(clock, "0:00", 5);
	count = 0;
	best = -1;
	DAT_002ba4cc = DAT_002baea8 == 0;
	result = -1;
	CLRumble_StopAll((int)CLU_ADDR(0x002ed324));
	memset(desc, 0, sizeof(desc));
	DESC_I(desc, 0x288) = 2;
	DESC_I(desc, 0x44) = OV_I(self, 0x20);
	if (OV_I(self, 0x20) == 0)
		CLU_DebugPrintf("Hud.cpp, CreateWinnerText(): couldn't find font handgotb.ttf.\n");
	DESC_S(desc, 0x24c) = 1;
	DESC_S(desc, 0x24e) = 1;
	DESC_F(desc, 0x2a4) = 0.0f;
	DESC_F(desc, 0x2a8) = 0.0f;
	DESC_F(desc, 0x2ac) = 0.0f;
	DESC_F(desc, 0x250) = 127.0f;
	DESC_F(desc, 0x254) = 127.0f;
	DESC_F(desc, 0x258) = 127.0f;
	DESC_F(desc, 0x25c) = 127.0f;
	DESC_U(desc, 0x290) = 0x400;
	if (OV_I(self, 0x8c) != 0 && DAT_002baeb8 == 0) {
		CLMusic_FadeOut(-10);
		OV_I(self, 0x88) = 0;
	}
	channel = CLSamples_PlayGeneric(0, clu_end_whistle_volume, 0, 0, NULL);
	if (DAT_002baebc != 0) {
		team_a = scores[DAT_002baec0] + scores[DAT_002baec4];
		team_b = scores[DAT_002baecc] + scores[DAT_002baec8];
		if (team_a > team_b) {
			CL_WSprintf((unsigned short *)((unsigned char *)desc + 0x48), CL_TEXT(0x1820), 1);
			result = 0;
			CLSamples_QueueLine(-1, channel, 0x39, 0, 0xff, 0, 0);
		} else if (team_a < team_b) {
			CL_WSprintf((unsigned short *)((unsigned char *)desc + 0x48), CL_TEXT(0x1820), 2);
			result = 1;
			CLSamples_QueueLine(-1, channel, 0x39, 1, 0xff, 0, 0);
		} else {
			CL_WSprintf((unsigned short *)((unsigned char *)desc + 0x48), CL_TEXT(0x1830));
			CLSamples_QueueLine(-1, channel, 0x10, -1, 0xff, 0, 0);
			result = -1;
		}
		goto create;
	}
	if (scores[0] == -1) {
		clu_winners[0] = 1;
		count = 1;
	} else if (scores[0] > -1) {
		clu_winners[0] = 1;
		best = scores[0];
		count = 1;
	}
	if (scores[1] == best) {
		clu_winners[count] = 2;
		count++;
	} else if (scores[1] > best) {
		clu_winners[0] = 2;
		best = scores[1];
		count = 1;
	}
	if (scores[2] == best) {
		clu_winners[count] = 3;
		count++;
	} else if (scores[2] > best) {
		clu_winners[0] = 3;
		best = scores[2];
		count = 1;
	}
	if (scores[3] == best) {
		clu_winners[count] = 4;
		count++;
	} else if (scores[3] > best) {
		count = 1;
		clu_winners[0] = 4;
	}
	switch (count) {
	case 1:
		CL_WSprintf((unsigned short *)((unsigned char *)desc + 0x48), CL_TEXT(0x10e0), clu_winners[0]);
		result = clu_winners[0] - 1;
		character = DAT_002baed4[result * 10];
		if (channel >= 0) {
			if (with_game_type != 0) {
				CLSamples_QueueLine(-1, channel, DAT_002baeb4 + 0x2f, character, 0xff, 0, 0);
			} else {
				CLSamples_QueueLine(-1, channel, character + 7, -1, 0xff, 0, 0);
				if (DAT_002bae98 != 1)
					break;
			}
			CLSamples_QueueLine(character, -channel, 9, -1, DAT_00293928, 0, 0);
		} else if (with_game_type != 0) {
			CLSamples_PlayCommentary(DAT_002baeb4 + 0x2f, DAT_002baeb4, character, 0, 0, NULL);
		} else {
			CLSamples_PlayCommentary(character + 7, -1, 0xff, 0, 0, NULL);
		}
		break;
	case 2:
		sprintf(names, "%d and %d", clu_winners[0], clu_winners[1]);
		if (channel >= 0)
			CLSamples_QueueLine(-1, channel, 0x10, -1, 0xff, 0, 0);
		else
			CLSamples_PlayCommentary(0x10, -1, 0xff, 0, 0, NULL);
		break;
	case 3:
		sprintf(names, "%d, %d and %d", clu_winners[0], clu_winners[1], clu_winners[2]);
		if (channel >= 0)
			CLSamples_QueueLine(-1, channel, 0x11, -1, 0xff, 0, 0);
		else
			CLSamples_PlayCommentary(0x11, -1, 0xff, 0, 0, NULL);
		break;
	case 4:
		sprintf(names, "%d, %d, %d and %d", clu_winners[0], clu_winners[1], clu_winners[2], clu_winners[3]);
		if (channel >= 0)
			CLSamples_QueueLine(-1, channel, 0x12, -1, 0xff, 0, 0);
		else
			CLSamples_PlayCommentary(0x12, -1, 0xff, 0, 0, NULL);
		break;
	}
	if (count < 4)
		clu_winners[count] = -1;
	if (count > 1)
		CL_WSprintf((unsigned short *)((unsigned char *)desc + 0x48), CL_TEXT(0x10f0), names);
create:
	DESC_U(desc, 0x290) = DESC_U(desc, 0x290) | 0x9000;
	DESC_I(desc, 0x298) = 4;
	CLU_DebugPrintf("Creating WinnerText\n");
	OV_I(self, 0x38) = COverlay_Create((int *)desc);
	OV_I(self, 0xcc) = 1;
	OV_F(self, 0x90) = 0.0f;
	if (OV_I(self, 0xd0) != 0)
		FUN_0004a330((ushort *)(OV_I(self, 0xd0) + 0xa4), (byte *)clock);

	memset(desc, 0, sizeof(desc));
	rank = 3;
	for (player = 0; player < 4; player++) {
		CLHud_DescCharacterColour(desc, DAT_002baed4[player * 10]);
		DESC_I(desc, 0x288) = 0;
		DESC_F(desc, 0x2b4) = 35.0f;
		DESC_F(desc, 0x2b8) = 36.0f;
		DESC_F(desc, 0x2bc) = 0.0f;
		DESC_F(desc, 0x25c) = 0.0f;
		DESC_F(desc, 0x308) = 32.0f;
		DESC_F(desc, 0x30c) = 32.0f;
		DESC_I(desc, 0x28c) = 3;
		DESC_I(desc, 0x260) = 4;
		DESC_I(desc, 0x268) = 0;
		DESC_I(desc, 0x264) = 0x7f;
		DESC_I(desc, 0x37c) = 0;
		DESC_F(desc, 0x31c) = -0.8f;
		DESC_F(desc, 0x324) = -0.8f;
		DESC_F(desc, 0x32c) = -0.8f;
		DESC_F(desc, 0x318) = 20.0f;
		DESC_F(desc, 0x320) = 20.0f;
		DESC_F(desc, 0x328) = 20.0f;
		DESC_F(desc, 0x334) = 20.0f;
		DESC_F(desc, 0x33c) = 20.0f;
		DESC_F(desc, 0x344) = 20.0f;
		DESC_F(desc, 0x330) = 1.0f;
		DESC_F(desc, 0x338) = 1.0f;
		DESC_F(desc, 0x340) = 1.0f;
		if (DAT_002baebc != 0) {
			team_a = scores[DAT_002baec0] + scores[DAT_002baec4];
			team_b = scores[DAT_002baecc] + scores[DAT_002baec8];
			if (DAT_002baed4[player * 10 + 8] == 0) {
				rank = team_a < team_b ? 1 : 0;
				DESC_I(desc, 4) = clu_position_textures[rank];
			} else if (DAT_002baed4[player * 10 + 8] == 1) {
				rank = team_b < team_a ? 1 : 0;
				DESC_I(desc, 4) = clu_position_textures[rank];
			}
		} else {
			rank = 3;
			if (player != 0 && scores[player] >= scores[0])
				rank = 2;
			if (player != 1 && scores[player] >= scores[1])
				rank--;
			if (player != 2 && scores[player] >= scores[2])
				rank--;
			if (player != 3 && scores[player] >= scores[3])
				rank--;
			DESC_I(desc, 4) = clu_position_textures[rank];
		}
		if (DAT_002baed0 == 0) {
			marker = COverlay_CreateChild(OV_I(self, 0xf0 + player * 4), desc);
			clu_rank_markers[player] = marker;
			OV_I(marker, 0x408) = (int)FUN_0004bb30(0, "scorebarping.wav", 2, 0xdefa);
			OV_U(clu_rank_markers[player], 0x3e4) = clu_rank_delays[rank];
			OV_S(OV_I(self, 0xe0 + player * 4), 0x54) = 1;
			OV_S(clu_rank_markers[player], 0x54) = 1;
			rank = 3;
		}
	}
	if (DAT_002baed0 == 0) {
		for (player = 0; player < 4; player++) {
			marker = clu_rank_markers[player];
			OV_S(marker, 0x54) = 0;
			OV_I(marker, 0x2f4) = 1;
			OV_I(marker, 0x2f8) = 1;
			OV_U(marker, 0x2ec) = OV_U(marker, 0x2ec) | 0x8000;
		}
	}
	return result;
}

/* ---------- ground rings of the AI players */

/* 0x002ecfd8  quad of the ground ring */
#define clu_ring_quad CLU_ARR(struct CLQuadVertex, 0x002ecfd8)

extern unsigned int DAT_002bc960;   /* bdisplay: alpha 0x?? colour component */
extern unsigned int DAT_002bca94;   /* bdisplay: alpha colour component of the ring */

/* 0x0007b6f0  ClassLib/Hud.cpp (inferred)
Builds a textured quad of half size `size` lying on the plane with normal `normal`
at `pos`, turned by `angle`, with grey level `brightness`. */
void CLHud_BuildGroundQuad(struct CLQuadVertex *quad, float *pos, float *normal, float size, float angle,
			   float brightness)
{
	float n[3];
	float v[4];
	float a[3];
	float b[3];
	double ax, bx, s1, s2, d1, d2;
	float ay, by, az, bz, s3, d3;
	int level;
	unsigned int colour;

	n[0] = normal[0];
	n[1] = normal[1];
	n[2] = normal[2];
	v[0] = 1.0f;
	v[1] = 0.0f;
	v[2] = 0.0f;
	FUN_00051170(v, v, angle);
	a[0] = (float)((double)v[2] * (double)n[1] - (double)v[1] * (double)n[2]);
	a[1] = (float)((double)v[0] * (double)n[2] - (double)v[2] * (double)n[0]);
	a[2] = (float)((double)v[1] * (double)n[0] - (double)v[0] * (double)n[1]);
	b[0] = (float)((double)a[1] * (double)n[2] - (double)a[2] * (double)n[1]);
	b[1] = (float)((double)a[2] * (double)n[0] - (double)a[0] * (double)n[2]);
	b[2] = (float)((double)a[0] * (double)n[1] - (double)a[1] * (double)n[0]);
	FUN_000514a0(a, a);
	FUN_000514a0(b, b);
	bx = (double)b[0] * (double)size;
	ax = (double)a[0] * (double)size;
	s1 = ax + bx;
	quad[0].x = (float)((double)pos[0] - s1);
	by = (float)((double)b[1] * (double)size);
	ay = (float)((double)a[1] * (double)size);
	s2 = (double)a[1] * (double)size + (double)by;
	quad[0].y = (float)((double)pos[1] - s2);
	bz = (float)((double)b[2] * (double)size);
	az = (float)((double)a[2] * (double)size);
	s3 = (float)((double)a[2] * (double)size + (double)bz);
	quad[0].z = (float)((double)pos[2] - (double)s3);
	level = (int)brightness;
	colour = DAT_002bd640[level] | DAT_002bc490[level] | DAT_002bc960 | DAT_002bce18[level];
	quad[0].colour = colour;
	quad[0].v = 1.0f;
	quad[0].u = 0.0f;
	quad[3].x = (float)(s1 + (double)pos[0]);
	quad[3].y = (float)(s2 + (double)pos[1]);
	quad[3].z = (float)((double)s3 + (double)pos[2]);
	quad[3].colour = colour;
	quad[3].u = 1.0f;
	quad[3].v = 0.0f;
	d1 = ax - bx;
	quad[2].x = (float)(d1 + (double)pos[0]);
	d2 = (double)ay - (double)by;
	quad[2].y = (float)(d2 + (double)pos[1]);
	d3 = (float)((double)az - (double)bz);
	quad[2].z = (float)((double)d3 + (double)pos[2]);
	quad[2].colour = colour;
	quad[2].u = 1.0f;
	quad[2].v = 1.0f;
	quad[1].x = (float)((double)pos[0] - d1);
	quad[1].y = (float)((double)pos[1] - d2);
	quad[1].z = (float)((double)pos[2] - (double)d3);
	quad[1].colour = colour;
	quad[1].u = 0.0f;
	quad[1].v = 0.0f;
}

/* 0x0007b970  ClassLib/Hud.cpp (inferred; the callee pops its arguments)
Draws the ring of AI player `player` on the ground below `pos` (found with a ray down),
pointing at `target` (a ring without arrow when target is NULL). */
void CLHud_DrawAIRing(int player, float *pos, float *target)
{
	int saved_state[30];
	float start[4];
	float dir[4];
	float hit[4];
	float normal[4];
	float angle;
	const unsigned char *rgb;
	unsigned int colour;

	FUN_00046d40((undefined4 *)saved_state);
	start[0] = pos[0];
	start[1] = pos[1] + 100.0f;
	start[2] = pos[2];
	start[3] = pos[3];
	dir[0] = (float)((double)0.0f * (double)1.4f);
	dir[1] = (float)((((double)pos[1] - (double)2000.0f) - (double)start[1]) * (double)1.4f);
	dir[2] = (float)((double)0.0f * (double)1.4f);
	dir[3] = pos[3];
	if (((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(start, dir, hit, normal) == 0.0f)
		return;
	hit[1] = hit[1] + 10.0f;
	if (target != NULL)
		angle = (float)(CLU_FPATAN((double)target[0] - (double)pos[0], (double)target[2] - (double)pos[2]) +
				(double)0.7853981f + (double)3.1415925f);
	else
		angle = 0.0f;
	CLHud_BuildGroundQuad(clu_ring_quad, hit, normal, 96.0f, angle, 80.0f);
	rgb = (const unsigned char *)&clu_characters[DAT_002baed4[player * 10]].colour;
	colour = DAT_002bd640[rgb[2]] | DAT_002bc490[rgb[1]] | DAT_002bce18[rgb[0]] | DAT_002bca94;
	clu_ring_quad[0].colour = colour;
	clu_ring_quad[1].colour = colour;
	clu_ring_quad[2].colour = colour;
	clu_ring_quad[3].colour = colour;
	FUN_000416c0(0, (struct bdTexture *)(target != NULL ? clu_ai_ring : clu_ai_ring_nogoal));
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 1, 0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(7, 2, 0);
	FUN_0003f2d0(4, (void *)clu_ring_quad, 4);
	FUN_00046d60(saved_state);
}

/* 0x002ed068  quad of the second ground ring */
#define clu_ring2_quad CLU_ARR(struct CLQuadVertex, 0x002ed068)

/* 0x0007bb70  ClassLib/Hud.cpp (inferred)
Like CLHud_DrawAIRing, with the arrow turned by `angle` (no arrow for 0). */
void CLHud_DrawAIRingAngle(int player, float *pos, float angle)
{
	int saved_state[30];
	float start[4];
	float dir[4];
	float hit[4];
	float normal[4];
	float turn;
	const unsigned char *rgb;
	unsigned int colour;

	FUN_00046d40((undefined4 *)saved_state);
	start[0] = pos[0];
	start[1] = pos[1] + 100.0f;
	start[2] = pos[2];
	start[3] = pos[3];
	dir[0] = (float)((double)0.0f * (double)1.4f);
	dir[1] = (float)((((double)pos[1] - (double)2000.0f) - (double)start[1]) * (double)1.4f);
	dir[2] = (float)((double)0.0f * (double)1.4f);
	dir[3] = pos[3];
	if (((float (*)(float *, float *, float *, float *))FUN_000b4bd0)(start, dir, hit, normal) == 0.0f)
		return;
	hit[1] = hit[1] + 10.0f;
	if (angle == 0.0f)
		turn = 0.0f;
	else
		turn = angle + 3.9269905f;
	CLHud_BuildGroundQuad(clu_ring2_quad, hit, normal, 96.0f, turn, 80.0f);
	rgb = (const unsigned char *)&clu_characters[DAT_002baed4[player * 10]].colour;
	colour = DAT_002bd640[rgb[2]] | DAT_002bc490[rgb[1]] | DAT_002bce18[rgb[0]] | DAT_002bca94;
	clu_ring2_quad[0].colour = colour;
	clu_ring2_quad[1].colour = colour;
	clu_ring2_quad[2].colour = colour;
	clu_ring2_quad[3].colour = colour;
	FUN_000416c0(0, (struct bdTexture *)(angle != 0.0f ? clu_ai_ring : clu_ai_ring_nogoal));
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 1, 0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(5, 0, 0);
	FUN_00044b00(7, 2, 0);
	FUN_0003f2d0(4, (void *)clu_ring2_quad, 4);
	FUN_00046d60(saved_state);
}

/* 0x0007bd80  ClassLib/Hud.cpp (inferred; thiscall)
Sets the target widths of the score bars from the scores (team sums in team games),
relative to the score range (from 0 or the lowest score). */
void CLHud_UpdateBars(int self)
{
	float low;
	double high;
	float team_a;
	double team_b;
	double range;
	double value;
	int player;
	int mate;

	if (DAT_002baebc == 0) {
		low = OV_F(self, 0x3c);
		high = (double)OV_F(self, 0x40);
	} else {
		team_a = OV_F(self, 0x4c + DAT_002baec4 * 4) + OV_F(self, 0x4c + DAT_002baec0 * 4);
		team_b = (double)OV_F(self, 0x4c + DAT_002baecc * 4) + (double)OV_F(self, 0x4c + DAT_002baec8 * 4);
		if ((double)team_a <= team_b) {
			low = team_a;
			high = team_b;
		} else {
			low = (float)team_b;
			high = (double)team_a;
		}
	}
	if (low > 0.0f)
		low = 0.0f;
	if (high == (double)low)
		high = high + (double)1.0f;
	if (DAT_002bae94 <= 0)
		return;
	range = high - (double)low;
	for (player = 0; player < DAT_002bae94; player++) {
		if (DAT_002baebc != 0) {
			mate = (&DAT_002baec4)[DAT_002baed4[player * 10 + 8] * 2 - DAT_002baed4[player * 10 + 9]];
			value = (double)OV_F(self, 0x4c + mate * 4) + (double)OV_F(self, 0x4c + player * 4);
		} else {
			value = (double)OV_F(self, 0x4c + player * 4);
		}
		OV_F(OV_I(self, 0x78 + player * 4), 0x36c) = (float)((value - (double)low) / range * (double)clu_bar_scale);
	}
}

extern const char *DAT_00283898[6]; /* core: results images of the characters ("dub_g.bmp") */

/* 0x0007be80  ClassLib/Hud.cpp (inferred)
Turns the score boxes into result boxes: the character image (the portrait, 25%
bigger, for absent players), full colour, moved up 8 with their children down 12. */
void CLHud_ShowResultBoxes(void)
{
	int player;
	int box;
	int node;

	for (player = 0; player < 4; player++) {
		OV_S(OV_I(clu_hud, 0xe0 + player * 4), 0x54) = 1;
		if (DAT_002baed8[player * 10] == 0) {
			OV_I(OV_I(clu_hud, 0xf0 + player * 4), 0x60) =
				(int)FUN_0004bb30(0, (char *)clu_characters[DAT_002baed4[player * 10]].portrait, 0, 0xdefa);
			OV_F(OV_I(clu_hud, 0xf0 + player * 4), 0x364) =
				(float)((double)OV_F(OV_I(clu_hud, 0xf0 + player * 4), 0x364) * 1.25);
			OV_F(OV_I(clu_hud, 0xf0 + player * 4), 0x368) =
				(float)((double)OV_F(OV_I(clu_hud, 0xf0 + player * 4), 0x368) * 1.25);
		} else {
			OV_I(OV_I(clu_hud, 0xf0 + player * 4), 0x60) =
				(int)FUN_0004bb30(0, (char *)DAT_00283898[DAT_002baed4[player * 10]], 0, 0xdefa);
		}
		box = OV_I(clu_hud, 0xf0 + player * 4);
		OV_F(box, 0x2ac) = 127.0f;
		OV_F(box, 0x2b0) = 127.0f;
		OV_F(box, 0x2b4) = 127.0f;
		OV_F(box, 0x2b8) = 127.0f;
		OV_F(OV_I(clu_hud, 0xf0 + player * 4), 0x304) = OV_F(OV_I(clu_hud, 0xf0 + player * 4), 0x304) - 8.0f;
		for (node = (int)CLList_Next((unsigned int *)OV_I(clu_hud, 0xf0 + player * 4), NULL); node != 0;
		     node = (int)CLList_Next((unsigned int *)OV_I(clu_hud, 0xf0 + player * 4), (unsigned int *)node))
			OV_F(node, 0x304) = OV_F(node, 0x304) + 12.0f;
	}
}

/* 0x0007bfd0  ClassLib/Hud.cpp (inferred; thiscall)
Takes the score bar geometry from score4.tga: the corner angles and the inner margins
relative to its half size. */
void CLHud_InitBarGeometry(int self)
{
	int texture;
	double w36;
	double half;

	texture = (int)FUN_0004bb30(0, "score4.tga", 0, 0xdefa);
	OV_I(self, 0x5c) = texture;
	w36 = (double)(int)(*(unsigned short *)(texture + 0x2e) - 0x24);
	OV_F(self, 0x68) = (float)atan2(w36, (double)(int)(*(unsigned short *)(texture + 0x30) - 0x23));
	OV_F(self, 0x6c) = (float)atan2(w36, -35.0);
	OV_F(self, 0x70) = (float)(atan2(-36.0, -35.0) + (double)6.283185f);
	half = (double)(int)*(unsigned short *)(texture + 0x2e) * (double)0.5f;
	OV_F(self, 0x60) = (float)(((double)36.0f - half) / half);
	OV_I(self, 0x74) = 0;
	half = (double)(int)*(unsigned short *)(texture + 0x30) * (double)0.5f;
	OV_F(self, 100) = (float)(((double)35.0f - half) / half);
}

/* 0x002ed0f8  vertices of the score bar fan (up to 8) */
#define clu_bar_quad CLU_ARR(struct CLQuadVertex, 0x002ed0f8)

static void CLHud_BarVertex(int n, float x, float y, unsigned int colour, float u, float v)
{
	clu_bar_quad[n].x = x;
	clu_bar_quad[n].y = y;
	clu_bar_quad[n].z = 0.0f;
	clu_bar_quad[n].colour = colour;
	clu_bar_quad[n].u = u;
	clu_bar_quad[n].v = v;
}

/* 0x0007c080  ClassLib/Hud.cpp (inferred; thiscall, the callee pops its arguments)
Draws the four round score bars with score4.tga (geometry from CLHud_InitBarGeometry).
Each score is clamped to [0,1] and fills an arc of score * 1.6 pi around the centre of the
player's score box; the arc is cut at the corner angles self+0x68/0x6c/0x70. A score at
or above `threshold` pulses its alpha with self+0x74. Nothing is drawn while the results
are shown. The x87 code keeps 0.0 on the stack for the vertex z; y0 + H * 0.0 is written
as y0 and x0 + W * 0.0 as x0. */
void CLHud_DrawScoreBars(int self, float s0, float s1, float s2, float s3, float threshold)
{
	float score[4];
	int i;
	int texture;
	int box;
	int alpha;
	const unsigned char *rgb;
	unsigned int colour;
	double w, h;
	double cx, cy, cu, cv;
	double ad, t;
	float a, sin_a, cos_a, tf;
	float x0, y0, f60, f64;

	if (DAT_002baeb8 != 0)
		return;
	score[0] = s0;
	score[1] = s1;
	score[2] = s2;
	score[3] = s3;
	FUN_00043b70(0);
	FUN_00044b00(3, 0, 0);
	FUN_00044b00(6, 1, 0);
	FUN_00044b00(10, 0, 0);
	FUN_00044b00(4, 0, 0);
	FUN_00044b00(7, 1, 0);
	FUN_00043930((undefined4 *)DAT_002855f0);
	FUN_000416c0(0, (struct bdTexture *)OV_I(self, 0x5c));
	for (i = 0; i < 4; i++) {
		if (score[i] > 1.0f)
			score[i] = 1.0f;
		else if (0.0f > score[i])
			score[i] = 0.0f;
		rgb = (const unsigned char *)&clu_characters[DAT_002baed4[i * 10]].colour;
		if (!(score[i] >= threshold))
			alpha = 0x7f;
		else
			alpha = (int)((sin((double)OV_F(self, 0x74) * (double)0.10471975f) + 1.0) * 63.0);
		ad = (double)score[i] * (double)5.026548f;
		a = (float)ad;
		box = OV_I(clu_hud, 0xf0 + i * 4);
		texture = OV_I(self, 0x5c);
		sin_a = (float)sin(ad);
		cos_a = (float)cos((double)a);
		x0 = OV_F(box, 0x300);
		y0 = (float)((double)OV_F(box, 0x304) - (double)(int)*(unsigned short *)(texture + 0x30));
		w = (double)(int)*(unsigned short *)(texture + 0x2e);
		h = (double)(int)*(unsigned short *)(texture + 0x30);
		f60 = OV_F(self, 0x60);
		f64 = OV_F(self, 100);
		colour = DAT_002bc898[alpha] | DAT_002bd640[rgb[2]] | DAT_002bc490[rgb[1]] | DAT_002bce18[rgb[0]];
		cx = w * ((double)f60 + 1.0) * 0.5 + x0;
		cy = (1.0 - (double)f64) * h * 0.5 + y0;
		cu = ((double)f60 + 1.0) * 0.5;
		cv = ((double)f64 + 1.0) * 0.5;
		if (a <= OV_F(self, 0x68)) {
			/* top edge, right of the centre */
			tf = (float)((double)sin_a / (double)cos_a);
			t = (1.0 - (double)f60) * tf + f60 + 1.0;
			CLHud_BarVertex(0, (float)cx, (float)cy, colour, (float)cu, (float)cv);
			CLHud_BarVertex(1, (float)cx, y0, colour, (float)cu, 1.0f);
			CLHud_BarVertex(2, (float)(t * w * 0.5 + x0), y0, colour, (float)(t * 0.5), 1.0f);
			FUN_0003f2d0(4, (void *)clu_bar_quad, 3);
		} else if (a <= OV_F(self, 0x6c)) {
			/* right edge */
			t = tan((double)a - (double)1.5707963f);
			t = ((double)f64 - 1.0) * t + f64;
			CLHud_BarVertex(0, (float)cx, (float)cy, colour, (float)cu, (float)cv);
			CLHud_BarVertex(1, (float)cx, y0, colour, (float)cu, 1.0f);
			CLHud_BarVertex(2, (float)(w + x0), (float)((1.0 - t) * h * 0.5 + y0), colour, 1.0f,
					(float)((t + 1.0) * 0.5));
			CLHud_BarVertex(3, (float)(w + x0), y0, colour, 1.0f, 1.0f);
			FUN_0003f2d0(4, (void *)clu_bar_quad, 4);
		} else if (a <= OV_F(self, 0x70)) {
			/* bottom edge */
			t = -((double)sin_a / (double)cos_a);
			t = ((double)f60 + 1.0) * t + f60 + 1.0;
			CLHud_BarVertex(0, (float)cx, y0, colour, (float)cu, 1.0f);
			CLHud_BarVertex(1, (float)(w + x0), y0, colour, 1.0f, 1.0f);
			CLHud_BarVertex(2, (float)cx, (float)cy, colour, (float)cu, (float)cv);
			CLHud_BarVertex(3, (float)(w + x0), (float)(h + y0), colour, 1.0f, 0.0f);
			CLHud_BarVertex(4, (float)(t * w * 0.5 + x0), (float)(h + y0), colour, (float)(t * 0.5), 0.0f);
			FUN_0003f2d0(4, (void *)clu_bar_quad, 5);
		} else {
			/* left edge */
			tf = (float)tan((double)a - (double)4.712389f);
			t = ((double)f64 + 1.0) * tf + f64;
			CLHud_BarVertex(0, (float)cx, y0, colour, (float)cu, 1.0f);
			CLHud_BarVertex(1, (float)(w + x0), y0, colour, 1.0f, 1.0f);
			CLHud_BarVertex(2, (float)cx, (float)cy, colour, (float)cu, (float)cv);
			CLHud_BarVertex(3, (float)(w + x0), (float)(h + y0), colour, 1.0f, 0.0f);
			CLHud_BarVertex(4, x0, (float)(h + y0), colour, 0.0f, 0.0f);
			CLHud_BarVertex(5, x0, (float)(h + y0), colour, 0.0f, 0.0f);
			CLHud_BarVertex(6, (float)cx, (float)cy, colour, (float)cu, (float)cv);
			CLHud_BarVertex(7, x0, (float)((1.0 - t) * h * 0.5 + y0), colour, 0.0f, (float)((t + 1.0) * 0.5));
			FUN_0003f2d0(4, (void *)clu_bar_quad, 8);
		}
	}
	FUN_000416c0(0, 0);
	FUN_00044b00(7, 3, 0);
}
