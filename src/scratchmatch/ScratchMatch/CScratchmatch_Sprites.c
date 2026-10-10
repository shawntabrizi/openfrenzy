/*
ScratchMatch/CScratchmatch_Sprites.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CScratchmatch_Sprites.cpp
(module scratchmatch)

The HUD of Scratch Match ("mapSprites", 27 sprites): the frame of the hit line with one lane
per player, the button icons that scroll up each lane to the hit line ("maxIconSprites",
SC_ICONLISTSIZE = 100, struct SMIcon), the pulse of a lane when its player hits, the floating
rating and combo texts (16, struct SMText), the lives of the survival game, the combo
counters, and the frame rate. Sprites are drawn as screen-space quads (SMSprites_DrawSprite).
*/

#include "scratchmatch.h"

/* "%u %S" and "%S" (UTF-16, 0x00278c54 and 0x002566bc) */
static const ushort s_CountFormat[] = { '%', 'u', ' ', '%', 'S', 0 };
static const ushort s_TextFormat[] = { '%', 'S', 0 };

/* the x of a player's lane */
static float sm_LaneX(unsigned int player)
{
	return sm_ufloat(player) * sm_LaneSpacing - 210.0f;
}

/* an entry of the text table (classlib_ui's language strings): a UTF-16 string pointer */
#define sm_TextEntry(offset) (*(ushort **)(*(char **)((char *)DAT_002ecc74 + 0x20) + (offset)))

/* 0x001bfff0  (gap; the sprites' initialiser): the HUD layout, no icons and texts */
void SMSprites_Init(void)
{
	int i;

	sm_IconFade = 96.0f;
	sm_LaneSpacing = 140.0f;
	sm_IconScale = 0.35f;
	sm_HitLineY = 105.0f;
	sm_PulseTime = 0.2f;
	SM_I(0x00311b00) = 0;
	SM_I(0x00311b04) = 7;
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		sm_Pulsing[i] = 0;
		sm_Pulse[i] = 0.0f;
	}
	for (i = 0; i < SM_NUM_TEXTS; i++)
		sm_Texts[i].type = 0;
	for (i = 0; i < SM_NUM_ICONS; i++) {
		sm_Icons[i].buttons = 0;
		sm_Icons[i].x = 0.0f;
		sm_Icons[i].y = 0.0f;
		sm_Icons[i].colour = 0xffffff;
		sm_Icons[i].scale = 0.0f;
		sm_Icons[i].hitTime = 0.0f;
		sm_Icons[i].birthTime = 0.0f;
		sm_Icons[i].state = 0;
	}
	sm_FirstFreeText = 0;
	sm_FirstFreeIcon = 0;
	sm_IconStartY = -140;
	sm_IconEndY = sm_HitLineY + sm_IconFade;
}

/* 0x001c00f0  ScratchMatch/CScratchmatch_Sprites.cpp: loads the 27 HUD sprites */
void SMSprites_Load(void)
{
	int i;

	for (i = 0; i < 27; i++) {
		sm_Sprites[i] = (struct bdTexture *)bLoadResource(NULL, sm_SpriteNames[i], 0, 0xdefa);
		if (sm_Sprites[i] == NULL)
			SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x9c,
				"POINTER ASSERTION FAILURE: mapSprites[ un32SpriteIndex ]");
	}
}

/* 0x001c0140  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): forgets the sprites (they
are freed with their package) */
void SMSprites_Free(void)
{
	int i;

	for (i = 0; i < 27; i++)
		sm_Sprites[i] = NULL;
}

/* 0x001c0160  ScratchMatch/CScratchmatch_Sprites.cpp: the frame rate (tweak "Show Framerate") */
void SMSprites_DrawFramerate(void)
{
	char text[16];
	ushort wide[16];
	float fps;
	int length;

	if (sm_ShowFramerate == 1) {
		bdSetWorldMatrix(DAT_002855f0);
		fps = (float)(1.0f / SMTimer_FrameTime());
		if (!(fps < 1000.0f))
			SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x1f7, "ASSERTION FAILURE: fFPS < 1000.0f");
		_snprintf(text, 10, "fps: %.1f", (double)fps);
		bkStringToWide(wide, text);
		length = (unsigned short)lstrlenA(text);
		bdPrintFontLite(sm_Font, wide, length, -280.0f, 220.0f, 0x7f, 0x7f, 0x7f, 0x40, NULL, NULL);
	}
}

/* 0x001c0220  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): starts the pulse of a lane */
void SMSprites_StartPulse(int player)
{
	if (sm_Pulsing[player] == 0) {
		sm_Pulsing[player] = 1;
		SMTimer_Reset(SMT_PULSE0 + player);
		sm_Pulse[player] = 0.0f;
	}
}

/* 0x001c0260  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): a floating text over a lane:
a rating (types 1-5, white, from the hit line) or a combo count (type 6, the player's
colour); not in the survival game */
void SMSprites_AddText(int player, int type, unsigned int value)
{
	struct SMText *text;
	unsigned int colour;

	if (sm_GameVariation == 3 || sm_FirstFreeText >= SM_NUM_TEXTS)
		return;
	text = &sm_Texts[sm_FirstFreeText];
	text->type = type;
	text->age = 0.0f;
	text->rgba[3] = 0xc0;
	text->speed = -100.0f;
	text->x = sm_LaneX((unsigned int)player);
	if (type == 6) {
		colour = sm_Players[player].brightColour;
		text->value = value;
		text->y = -50.0f;
		text->dim = 0;
		text->duration = 1.0f;
		text->rgba[0] = (unsigned char)(colour >> 16);
		text->rgba[1] = (unsigned char)(colour >> 8);
		text->rgba[2] = (unsigned char)colour;
	} else {
		text->value = 0;
		text->y = sm_HitLineY - sm_IconFade;
		text->dim = 1;
		text->duration = 0.5f;
		text->rgba[0] = 0xff;
		text->rgba[1] = 0xff;
		text->rgba[2] = 0xff;
	}
	sm_FirstFreeText = sm_FirstFreeText + 1;
	while (sm_FirstFreeText < SM_NUM_TEXTS && sm_Texts[sm_FirstFreeText].type != 0)
		sm_FirstFreeText = sm_FirstFreeText + 1;
}

/* 0x001c03a0  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): the texts rise and expire */
void SMSprites_UpdateTexts(void)
{
	struct SMText *text;
	double dt, age;
	unsigned int i;

	dt = SMTimer_FrameTime();
	for (i = 0; i < SM_NUM_TEXTS; i++) {
		text = &sm_Texts[i];
		if (text->type != 0) {
			age = dt + text->age;
			text->age = (float)age;
			if (age <= text->duration) {
				text->y = (float)(dt * text->speed + text->y);
			} else {
				text->type = 0;
				if (i < sm_FirstFreeText)
					sm_FirstFreeText = i;
			}
		}
	}
}

/* 0x001c0420  ScratchMatch/CScratchmatch_Sprites.cpp: a new icon in a player's lane that
reaches the hit line at `hitTime` */
void SMSprites_AddIcon(int player, int buttons, float hitTime)
{
	struct SMIcon *icon;

	if (sm_FirstFreeIcon >= SM_NUM_ICONS)
		return;
	if (buttons == 0)
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x2df, "ASSERTION FAILURE: un32Buttons != SCBBUF_NONE");
	icon = &sm_Icons[sm_FirstFreeIcon];
	icon->player = player;
	icon->x = sm_LaneX((unsigned int)player);
	icon->y = (float)sm_IconStartY;
	icon->buttons = (unsigned int)buttons;
	icon->colour = 0xffffff;
	icon->scale = 1.0f;
	icon->birthTime = (float)SMTimer_Elapsed(SMT_BEAT);
	icon->hitTime = hitTime;
	icon->seqPos = sm_Players[player].seqPos;
	icon->state = 0;
	icon->bonus = buttons == SM_BONUS;
	if (icon->hitTime <= icon->birthTime)
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x2fb,
			"ASSERTION FAILURE: maxIconSprites[ mun32FirstFreeIconSlot ].fExpectedHitTime > "
			"maxIconSprites[ mun32FirstFreeIconSlot ].fBirthTime");
	sm_FirstFreeIcon = sm_FirstFreeIcon + 1;
	while (sm_FirstFreeIcon < SM_NUM_ICONS && sm_Icons[sm_FirstFreeIcon].buttons != 0)
		sm_FirstFreeIcon = sm_FirstFreeIcon + 1;
}

/* 0x001c0560  ScratchMatch/CScratchmatch_Sprites.cpp: moves the icons up (linearly, to reach
the hit line at their hit time), fades them in at the bottom and out above the hit line, and
frees those past the top */
void SMSprites_UpdateIcons(void)
{
	struct SMIcon *icon;
	unsigned int alpha;
	float now, start, y;
	unsigned int i;

	SMTimer_FrameTime();
	now = (float)SMTimer_Elapsed(SMT_BEAT);
	for (i = 0; i < SM_NUM_ICONS; i++) {
		icon = &sm_Icons[i];
		if (icon->buttons == 0)
			continue;
		start = (float)sm_IconStartY;
		y = (float)((sm_HitLineY - start) * ((now - icon->birthTime) / (icon->hitTime - icon->birthTime)) + start);
		icon->y = y;
		if (sm_IconFade + start > y)
			alpha = (unsigned int)(unsigned char)SM_FTOL((y - start) / sm_IconFade * 255.0f) << 24;
		else if (y <= sm_HitLineY)
			alpha = 0xff000000u;
		else
			alpha = (unsigned int)(unsigned char)SM_FTOL((sm_IconEndY - y) * 255.0f / (sm_IconEndY - sm_HitLineY)) << 24;
		icon->colour = (icon->colour & 0x00ffffffu) | alpha;
		if (icon->y > sm_IconEndY) {
			icon->buttons = 0;
			if (i < sm_FirstFreeIcon)
				sm_FirstFreeIcon = i;
		}
	}
}

/* 0x001c06c0  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): frees an icon */
void SMSprites_RemoveIcon(unsigned int icon)
{
	sm_Icons[icon].buttons = 0;
	if (icon < sm_FirstFreeIcon)
		sm_FirstFreeIcon = icon;
}

/* 0x001c06f0  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): the buttons of an icon (0 for
no icon) */
unsigned int SMSprites_GetIconButtons(int icon)
{
	if (icon == SM_NO_ICON)
		return 0;
	return sm_Icons[icon].buttons;
}

/* 0x001c0710  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): the hit time of an icon */
float SMSprites_GetIconHitTime(int icon)
{
	if (icon == SM_NO_ICON)
		return 0.0f;
	return sm_Icons[icon].hitTime;
}

/* 0x001c0730  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): the buttons of a player's
current icon */
unsigned int SMSprites_GetPlayerIconButtons(int player)
{
	if (sm_Players[player].icon == SM_NO_ICON)
		return 0;
	return sm_Icons[sm_Players[player].icon].buttons;
}

/* 0x001c0760  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): frees all icons of a player */
void SMSprites_RemovePlayerIcons(int player)
{
	unsigned int i;

	for (i = 0; i < SM_NUM_ICONS; i++) {
		if (sm_Icons[i].player == player) {
			sm_Icons[i].buttons = 0;
			if (i < sm_FirstFreeIcon)
				sm_FirstFreeIcon = i;
		}
	}
}

/* 0x001c07a0  ScratchMatch/CScratchmatch_Sprites.cpp: changes the buttons of a player's current
icon (a bonus icon becomes the pattern) */
void SMSprites_SetPlayerIconButtons(int player, int buttons)
{
	if (buttons == 0)
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x3f9, "ASSERTION FAILURE: un32Buttons != SCBBUF_NONE");
	if (sm_Players[player].icon != SM_NO_ICON)
		sm_Icons[sm_Players[player].icon].buttons = (unsigned int)buttons;
}

/* 0x001c07f0  ScratchMatch/CScratchmatch_Sprites.cpp: the sequence line of a player's current
icon. With no icon the original asserts twice and then reads far past the icon list; the
port returns 0 there instead of reading outside its data (only an assertion path). */
unsigned int SMSprites_GetPlayerIconSequence(int player)
{
	unsigned int icon = (unsigned int)sm_Players[player].icon;

	if (icon == SM_NO_ICON) {
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x40b, "ASSERTION FAILURE: un32Icon != 0xFFFF");
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x40c, "ASSERTION FAILURE: un32Icon < SC_ICONLISTSIZE");
	} else if (icon >= SM_NUM_ICONS) {
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x40c, "ASSERTION FAILURE: un32Icon < SC_ICONLISTSIZE");
	}
	if (icon >= SM_NUM_ICONS)
		return 0;
	return sm_Icons[icon].seqPos;
}

/* 0x001c0850  ScratchMatch/CScratchmatch_Sprites.cpp (bracketed): crosses out a player's
previous icon if it is still above the hit line */
void SMSprites_CrossPreviousIcon(int player)
{
	int icon = sm_Players[player].prevIcon;

	if (icon != SM_NO_ICON && sm_HitLineY < sm_Icons[icon].y)
		sm_Icons[icon].state = 2;
}

/* 0x001c0890  ScratchMatch/CScratchmatch_Sprites.cpp: 1 for a bonus icon. An invalid icon
asserts (and, unlike the original, reads nothing: 0). */
int SMSprites_IsBonusIcon(unsigned int icon)
{
	if (icon == SM_NO_ICON) {
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x428, "ASSERTION FAILURE: un32Icon != 0xFFFF");
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x429, "ASSERTION FAILURE: un32Icon < SC_ICONLISTSIZE");
	} else if (icon >= SM_NUM_ICONS) {
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0x429, "ASSERTION FAILURE: un32Icon < SC_ICONLISTSIZE");
	}
	if (icon >= SM_NUM_ICONS)
		return 0;
	return sm_Icons[icon].bonus;
}

/* half the size of a texture, scaled unless the scale is (nearly) 0 or 1 */
static float sm_HalfSize(unsigned short size, float scale)
{
	if (fabsf(scale) < 0.01f || fabsf(scale - 1.0f) < 0.001f)
		return (float)(int)(size >> 1);
	return (float)(int)(size >> 1) * scale;
}

/* 0x001c08e0  ScratchMatch/CScratchmatch_Sprites.cpp: draws `texture` centred at (x, y) in
screen space, scaled and turned by `rotation`, in colour `rgb` with `alpha` */
void SMSprites_DrawSprite(struct bdTexture *texture, float x, float y, unsigned int rgb, unsigned char alpha,
	float scaleX, float scaleY, float rotation)
{
	float corners[4][4];
	struct SMVertex *quad;
	unsigned int colour;
	float halfW, halfH;
	int k;

	if (texture == NULL)
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0xcc, "POINTER ASSERTION FAILURE: pTexture");
	if (!(-(float)sm_HalfWidth <= x && x <= (float)sm_HalfWidth))
		SM_ASSERT_FAIL("CScratchmatch_Sprites.cpp", 0xcd,
			"ASSERTION FAILURE: ( fXPos >= -(float)( gxGameVariables.nScreenHalfWidth )) && "
			"( fXPos <= (float)( gxGameVariables.nScreenHalfWidth ))");
	halfW = sm_HalfSize(SM_AT(texture, 0x2e, unsigned short), scaleX);
	halfH = sm_HalfSize(SM_AT(texture, 0x30, unsigned short), scaleY);
	corners[0][0] = -halfW;
	corners[0][1] = halfH;
	corners[1][0] = halfW;
	corners[1][1] = halfH;
	corners[2][0] = halfW;
	corners[2][1] = -halfH;
	corners[3][0] = -halfW;
	corners[3][1] = -halfH;
	quad = sm_Quad;
	for (k = 0; k < 4; k++) {
		corners[k][2] = 0.0f;
		corners[k][3] = 1.0f;
		if (!(fabsf(rotation) <= 0.001f))
			bmVectorRotateZ(corners[k], corners[k], rotation);
		quad[k].pos[2] = corners[k][2];
		quad[k].pos[0] = x + corners[k][0];
		quad[k].pos[1] = y + corners[k][1];
	}
	colour = DAT_002bce18[(rgb >> 16) & 0xff] | DAT_002bc490[(rgb >> 8) & 0xff] | DAT_002bd640[rgb & 0xff] |
		DAT_002bc898[alpha];
	quad[0].uv[0] = 0.0f;
	quad[0].uv[1] = 0.0f;
	quad[1].uv[0] = 1.0f;
	quad[1].uv[1] = 0.0f;
	quad[2].uv[0] = 1.0f;
	quad[2].uv[1] = 1.0f;
	quad[3].uv[0] = 0.0f;
	quad[3].uv[1] = 1.0f;
	quad[0].diffuse = colour;
	quad[1].diffuse = colour;
	quad[2].diffuse = colour;
	quad[3].diffuse = colour;
	bdSetTexture(0, texture);
	bdDrawPrimitiveFVF(5, quad, 4, 0x152);
}

/* 0x001c0ba0  (gap; placed with the HUD): draws the HUD while playing: the hit-line frame, the
lanes (pulsing with the buttons just hit), the lives (survival) or the combo counters, the
floating texts and the icons (a cross over missed ones) */
void SMSprites_Draw(void)
{
	ushort text[64];
	struct SMIcon *icon;
	struct SMText *label;
	struct SMPlayer *player;
	unsigned int buttons, lives, maxLives, k, i;
	unsigned char alpha, grey;
	float x, y, width, height, scale;
	int base, length, a;

	SMSprites_DrawSprite(sm_Sprites[SMSPR_FRAMEBACK], 0.0f, sm_HitLineY, 0xffffff, 0x40, 1.12f, 0.5f, 0.0f);
	for (i = 0; i < SM_NUM_PLAYERS; i++)
		SMSprites_DrawSprite(sm_Sprites[SMSPR_TOPFRAME1 + i], 0.0f, sm_HitLineY, sm_Players[i].brightColour, 0x80,
			1.12f, 0.5f, 0.0f);
	if (sm_Flash == 1)
		SMSprites_DrawSprite(sm_Sprites[SMSPR_FRAMEFLASH], 0.0f, sm_HitLineY, 0xffffff,
			(unsigned char)SM_FTOL(sm_FlashAlpha * 255.0f), 1.12f, 0.5f, 0.0f);
	y = sm_HitLineY;
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		x = sm_LaneX(i);
		grey = (unsigned char)SM_FTOL(sm_Pulse[i] * 127.0f);
		SMSprites_DrawSprite(sm_Sprites[SMSPR_CLUSTEREMPTY], x, y, ((unsigned int)grey + 0x80) * 0x10101, 0xff,
			sm_IconScale, sm_IconScale, 0.0f);
		if (sm_Pulsing[i] == 1) {
			grey = (unsigned char)SM_FTOL(sm_Pulse[i] * 127.0f);
			bdPushRenderState(6, 1, 0);
			bdPushRenderState(10, 1, 0);
			SMSprites_DrawSprite(sm_Sprites[SMSPR_BUTTONFLASH], x, y, (unsigned int)grey * 0x10101, 0xff,
				sm_IconScale, sm_IconScale, 0.0f);
			buttons = sm_Players[i].hitButtons;
			for (k = 0; k < 4; k++) {
				if ((buttons & (1u << k)) != 0)
					SMSprites_DrawSprite(sm_Sprites[SMSPR_CLUSTERA + k], x, y, 0xffffff, 0xff, sm_IconScale,
						sm_IconScale, 0.0f);
			}
			bdPopRenderState(6);
			bdPopRenderState(10);
		}
	}
	if (sm_GameVariation == 3) {
		/* the lives: a row of lights per player (a lost one blinks for a second) */
		maxLives = (unsigned int)(DAT_002bae70 * -2 + 8);
		for (i = 0; i < SM_NUM_PLAYERS; i++) {
			player = &sm_Players[i];
			switch (i) {
			case 0:
				base = -251;
				break;
			case 1:
				base = -107;
				break;
			case 2:
				base = 37;
				break;
			default:
				base = 181;
				break;
			}
			base = base + (int)(8 - maxLives) * 5;
			for (k = 1; k <= maxLives; k++) {
				lives = (unsigned int)player->lives;
				if (SMTimer_Elapsed(SMT_BEAT) - player->lifeLostTime < 1.0f && sm_Blink != 0)
					lives++;
				SMSprites_DrawSprite(sm_Sprites[lives < k ? SMSPR_LIFEOFF : SMSPR_LIFEON], (float)base, 139.0f,
					player->brightColour, 0xff, 1.0f, 1.0f, 0.0f);
				base = base + 10;
			}
		}
	} else {
		/* the combo counters */
		for (i = 0; i < SM_NUM_PLAYERS; i++) {
			player = &sm_Players[i];
			if (player->hits > 4) {
				bdSetWorldMatrix(DAT_002855f0);
				bkWideSprintf(text, s_CountFormat, player->hits, sm_TextEntry(0x1980));
				length = bkWideStrlen(text);
				bdGetStringSize(sm_Font, text, length, &width, &height);
				bdPrintFontLite(sm_Font, text, length, sm_LaneX(i) - width * 0.5f, 0.0f,
					(player->brightColour >> 16) & 0xff, (player->brightColour >> 8) & 0xff,
					player->brightColour & 0xff, 0x70, NULL, NULL);
			}
		}
	}
	for (i = 0; i < SM_NUM_TEXTS; i++) {
		label = &sm_Texts[i];
		if (label->type == 0)
			continue;
		bdSetWorldMatrix(DAT_002855f0);
		if (label->type == 6)
			bkWideSprintf(text, s_CountFormat, label->value, sm_TextEntry(0x19e0));
		else
			bkWideSprintf(text, s_TextFormat, sm_TextEntry((label->type + 0x198) * 0x10));
		length = bkWideStrlen(text);
		bdGetStringSize(sm_Font, text, length, &width, &height);
		a = (sm_Blink == 0 && label->dim == 1) ? 0x40 : 0xff;
		bdPrintFontLite(sm_Font, text, length, label->x - width * 0.5f, label->y, label->rgba[0], label->rgba[1],
			label->rgba[2], a, NULL, NULL);
	}
	for (i = 0; i < SM_NUM_ICONS; i++) {
		icon = &sm_Icons[i];
		buttons = icon->buttons;
		if (buttons == 0)
			continue;
		x = icon->x;
		y = icon->y;
		alpha = (unsigned char)(icon->colour >> 24);
		SMSprites_DrawSprite(sm_Sprites[SMSPR_CLUSTEREMPTY], x, y, 0xffffff, alpha, sm_IconScale, sm_IconScale, 0.0f);
		if (buttons == SM_BONUS)
			SMSprites_DrawSprite(sm_Sprites[SMSPR_QUESTION], x, y, 0xffffff, alpha, sm_IconScale, sm_IconScale, 0.0f);
		for (k = 0; k < 4; k++) {
			if ((buttons & (1u << k)) != 0)
				SMSprites_DrawSprite(sm_Sprites[SMSPR_CLUSTERA + k], x, y, 0xffffff, alpha, sm_IconScale,
					sm_IconScale, 0.0f);
		}
		if (icon->state == 2) {
			scale = (255.0f - (float)alpha) * 0.00392156886f * 3.0f - 0.899999976f;
			if (scale > 1.0f)
				scale = 1.0f;
			SMSprites_DrawSprite(sm_Sprites[SMSPR_CROSS], x, y, 0xffffff, alpha, sm_IconScale * scale,
				sm_IconScale * scale, 0.0f);
		}
	}
}

/* 0x001c12b0  (gap; placed with the HUD): the lane pulses: half a sine over sm_PulseTime */
void SMSprites_UpdatePulses(void)
{
	int i;

	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		if (sm_Pulsing[i] == 1) {
			if (SMTimer_Elapsed(SMT_PULSE0 + i) <= sm_PulseTime) {
				sm_Pulse[i] = (float)sin(SMTimer_Elapsed(SMT_PULSE0 + i) / sm_PulseTime * 3.1415925f);
			} else {
				sm_Pulsing[i] = 0;
				sm_Pulse[i] = 0.0f;
			}
		}
	}
}

/* 0x001c1320  (gap; placed with the HUD): each player's current icon is the icon of theirs
within half a beat of the hit line. When the current icon changes and it was not hit (after
the first second), the previous icon is crossed out and a life is lost. */
void SMSprites_UpdatePlayerIcons(void)
{
	struct SMPlayer *player;
	float now;
	int found, icon, i;

	now = (float)SMTimer_Elapsed(SMT_BEAT);
	for (i = 0; i < SM_NUM_PLAYERS; i++) {
		player = &sm_Players[i];
		for (found = 0; found < SM_NUM_ICONS; found++) {
			if (sm_Icons[found].player == i && fabsf(sm_Icons[found].hitTime - now) < sm_BeatPeriod * 0.5f)
				break;
		}
		if (found == SM_NUM_ICONS) {
			found = SM_NO_ICON;
			if (player->icon == SM_NO_ICON)
				continue;
			if (player->hit == 0 && SMTimer_Elapsed(SMT_BEAT) > 1.0f) {
				icon = player->prevIcon;
				if (icon != SM_NO_ICON && sm_HitLineY < sm_Icons[icon].y)
					sm_Icons[icon].state = 2;
				player->lives = player->lives - 1;
			}
		} else if (player->icon != found) {
			if (player->hit == 0 && player->icon != SM_NO_ICON && SMTimer_Elapsed(SMT_BEAT) > 1.0f) {
				icon = player->prevIcon;
				if (icon != SM_NO_ICON && sm_HitLineY < sm_Icons[icon].y)
					sm_Icons[icon].state = 2;
				player->lives = player->lives - 1;
			}
		} else {
			continue;
		}
		icon = player->icon;
		player->prevIcon = icon;
		player->prevButtons = icon == SM_NO_ICON ? 0 : (int)sm_Icons[icon].buttons;
		player->icon = found;
		player->hitButtons = 0;
		player->hit = 0;
	}
}

/* 0x001c14e0  (gap; placed with the HUD): the HUD's frame: pulses, the blink, texts, the
frame flash, icons */
int SMSprites_Update(void)
{
	double flash;

	SMSprites_UpdatePulses();
	if (SMTimer_Elapsed(SMT_BLINK) > 0.1f) {
		SMTimer_Reset(SMT_BLINK);
		sm_Blink = sm_Blink == 0;
	}
	SMSprites_UpdateTexts();
	flash = sm_FlashAlpha - SMTimer_FrameTime() * 10.0f;
	sm_FlashAlpha = (float)flash;
	if (flash <= 0.0) {
		sm_FlashAlpha = 0.0f;
		sm_Flash = 0;
	}
	SMSprites_UpdateIcons();
	return 0;
}
