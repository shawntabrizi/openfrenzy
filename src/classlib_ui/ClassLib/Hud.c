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
	self[7] = FUN_0004bb30(0, "mslcd.ttf", 3, 0xdefa);
	clu_font_light = FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa);
	self[0xe] = 0;
	self[0x10] = 0;
	self[8] = FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
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
			*(short *)((int)self[0x60] + 0x54) = 0;
		if (DAT_002baed8[10] != 0)
			*(short *)((int)self[0x61] + 0x54) = 0;
		if (DAT_002baed8[20] != 0)
			*(short *)((int)self[0x62] + 0x54) = 0;
		if (DAT_002baed8[30] != 0)
			*(short *)((int)self[99] + 0x54) = 0;
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
Creates the HUD and loads its marker textures. */
void CLHud_Create(undefined4 context)
{
	int memory;
	int i;

	memory = (int)((void *(*)(unsigned int, const char *, int, unsigned int, undefined4))FUN_0004c480)(
		400, "C:\\BlitzParty\\Game\\Src\\ClassLib\\Hud.cpp", 0x3f, 0x1001, context);
	if (memory == 0)
		clu_hud = 0;
	else
		clu_hud = (int)CLHud_Construct((int *)memory);
	clu_ai_ring = FUN_0004bb30(0, "ai_player_ring.bmp", 0, 0xdefa);
	clu_ai_ring_nogoal = FUN_0004bb30(0, "ai_player_ringnogoal.bmp", 0, 0xdefa);
	for (i = 0; i < 4; i++) {
		OV_F(clu_hud, 0xa8 + i * 4) = 275.0f;
		clu_marker_y[i] = 50.0f;
		clu_marker_x[i] = 0.0f;
	}
	clu_position_textures[0] = FUN_0004bb30(0, "pos1.bmp", 0, 0xdefa);
	clu_position_textures[1] = FUN_0004bb30(0, "pos2.bmp", 0, 0xdefa);
	clu_position_textures[2] = FUN_0004bb30(0, "pos3.bmp", 0, 0xdefa);
	clu_position_textures[3] = FUN_0004bb30(0, "pos4.bmp", 0, 0xdefa);
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
