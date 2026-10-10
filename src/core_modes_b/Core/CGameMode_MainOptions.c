/*
CGAMEMODE_MAINOPTIONS.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_MainOptions.cpp

The options screen: effects and music volume, vibration, soundtrack, track, and the
language/voice screen (CGameMode_Credits-like mode of core_modes_a, FUN_00017c70).
The constructor, Stop, Update and the text refresh are in CGameMode_MainMenu.c (they were
emitted into that translation unit).
*/

#include "core_modes_b.h"

static const char cmb_file_mainoptions[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_MainOptions.cpp";

/* core_modes_a BG_Effects text boxes: (this, x, y, text, font) and (this, x, y, length, font) */
#define CMB_TEXTBOX(memory, x, y, text, font) \
	((void *(*)(void *, int, int, const unsigned short *, int))FUN_00011c50)((memory), (x), (y), (text), (font))
#define CMB_EDITBOX(memory, x, y, length, font) \
	((void *(*)(void *, int, int, int, int))FUN_00011cd0)((memory), (x), (y), (length), (font))

/* add a text box to a group's list (classlib_ui FUN_00071070) and start it (vtable +4) */
static void cmb_group_add(void *group, void *box)
{
	((void (*)(void *, void *, int))FUN_00071070)((char *)group + 0x10, box == NULL ? NULL : (char *)box + 4, 1);
	CMB_VFN(box, 0x04, void (*)(void *))(box);
}

/* 0x0001d200  Core/CGameMode_MainOptions.cpp */
void CGameMode_MainOptions_Start(struct CGameMode_MainOptions *self)
{
	void *memory;
	void *box;
	void *root;
	int font_bold;
	int font_light;
	int player;
	int button;
	int i;

	((void (*)(int))FUN_0004c290)(self->base.package);
	((void (*)(int))FUN_00054690)(DAT_002bae6c);
	DAT_00284408 = 0;
	self->music_level = DAT_002bae68 * 11 / 256;
	self->effects_level = DAT_002bae6c * 11 / 256;
	for (player = 0; player < 4; player++) {
		for (button = 0; button < 9; button++)
			CMB_I32(DAT_002baf78[player][button], 0x20) = 2;
	}
	self->sample = CMB_RESOURCE4(0, "frontend_02b.wav", 2, 0xdefa);
	font_bold = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	font_light = CMB_RESOURCE4(0, "handgotl.ttf", 3, 0xdefa);

	memory = CMB_NEW(0x20, cmb_file_mainoptions, 0x5a);
	self->group = memory == NULL ? NULL : ((void *(*)(void *))FUN_00013820)(memory);
	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x5c);
	self->items[0] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -200, 0, CMB_TEXT(9), font_bold);
	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x5d);
	self->items[1] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -200, 0, CMB_TEXT(8), font_bold);
	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x5e);
	self->items[2] = memory == NULL ? NULL : CMB_EDITBOX(memory, -200, 0, 0x100, font_bold);
	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x5f);
	self->items[3] = memory == NULL ? NULL : CMB_EDITBOX(memory, -200, 0, 0x100, font_bold);
	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x60);
	self->items[4] = memory == NULL ? NULL : CMB_EDITBOX(memory, -200, 0, 0x100, font_bold);
	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x61);
	self->items[5] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -200, 0, CMB_TEXT(0x14f), font_bold);

	if (DAT_002bae84 != 0) {
		/* custom soundtracks: stop the root mode's streams */
		root = ((void *(*)(void *, const char *))FUN_0002b5b0)(CMB_MODESTACK, "Root");
		if (CMB_I32(root, 0x54) != 0) {
			((void (*)(int))FUN_000563f0)(CMB_I32(root, 0x54));
			CMB_I32(root, 0x54) = 0;
		}
		if (CMB_I32(root, 0x58) != 0) {
			((void (*)(int))FUN_000563f0)(CMB_I32(root, 0x58));
			CMB_I32(root, 0x58) = 0;
		}
		DAT_0028ffb8 = -1;
		((void (*)(int, int, int, int, int, int, int))FUN_000722d0)(
			CMB_I32(DAT_002bae84, 0x10), 0, 0xff, 0x32, 0, 0, 0);
	}
	if (self->option >= 5 || self->option < 0)
		self->option = 0;
	for (i = 0; i <= 5; i++) {
		CMB_F32(self->items[i], 0x34) = 0.75f;
		CMB_F32(self->items[i], 0x14) = 100.0f - (float)i * 40.0f;
		if (i == self->option)
			cmb_text_select(self->items[i], 0.0f);
		else
			cmb_text_deselect(self->items[i]);
		cmb_group_add(self->group, self->items[i]);
	}

	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x92);
	box = memory == NULL ? NULL : CMB_TEXTBOX(memory, -200, 0x91, CMB_TEXT(7), font_bold);
	((void (*)(void *))FUN_00012430)(box);
	cmb_group_add(self->group, box);

	memory = CMB_NEW(0x90, cmb_file_mainoptions, 0x96);
	box = memory == NULL ? NULL : CMB_EDITBOX(memory, -310, -190, 0x100, font_light);
	self->hint = box;
	CMB_I32(box, 0x8c) = 0x30;
	CMB_F32(self->hint, 0x7c) = 560.0f;
	CMB_F32(self->hint, 0x34) = 0.75f;
	((void (*)(void *))FUN_000114e0)(self->hint);
	cmb_group_add(self->group, self->hint);

	((void (*)(void *, int, const char *))FUN_00030210)(self,
		CMB_RESOURCE4(0, "over_option.obe", 1, 0xdefa), NULL);
	CGameMode_MainOptions_RefreshTexts(self);
}

/* 0x0001d460  Core/CGameMode_MainOptions.cpp: not a function. 0x0001d460 is an instruction
   inside CGameMode_MainOptions_Start (0x0001d200-0x0001d785) that Ghidra took for a
   function start; nothing calls it. Kept only so the table entry has a definition. */
void FUN_0001d460(void)
{
	of_unimplemented(0x0001d460, "FUN_0001d460 (not a function: inside 0x0001d200)");
}

/* stick deflection of a pad's direction input k (the input objects keep it at +0x14) */
static int cmb_axis(int pad, int k)
{
	int value = CMB_I32(DAT_002baf78[pad][k], 0x14);

	return value < 0 ? -value : value;
}

/* vibration on/off; when switched on, a short rumble on the pad that pressed */
static void cmb_toggle_vibration(int pad)
{
	DAT_002bae7c = DAT_002bae7c == 0;
	if (DAT_002bae7c != 0) {
		CMB_F32(DAT_002ecfac, 0x90) = 1.0f;
		DAT_002baed8[pad][0] = 1;
		((int (*)(void *, int, int, int, int, int, int, int, int))FUN_00080400)(
			DAT_002ed324, pad, 1, 0x7ff8, 200, 10, 0x5a, 0, 3);
		((int (*)(void *, int, int, int, int, int, int, int, int))FUN_00080400)(
			DAT_002ed324, pad, 0, 0x7ff8, 200, 10, 0x5a, 0, 3);
		DAT_002baed8[pad][0] = 0;
		CMB_F32(DAT_002ecfac, 0x90) = 0.0f;
	}
	CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
}

static void cmb_set_effects_level(struct CGameMode_MainOptions *self, int level)
{
	self->effects_level = level;
	DAT_002bae6c = level * 0xff / 10;
	((void (*)(int))FUN_00054690)(DAT_002bae6c);
	CMB_PLAY_SAMPLE(8, 0xff, 0, 0, 0);
}

static void cmb_set_music_level(struct CGameMode_MainOptions *self, int level)
{
	self->music_level = level;
	DAT_002bae68 = level * 0xff / 10;
	CMB_PLAY_SAMPLE(8, 0xff, 0, 0, 0);
}

/* 0x0001d790  Core/CGameMode_MainOptions.cpp. Buttons 0-3 are the directions; they act
   only when the stick (or pad) points mostly that way. */
int CGameMode_MainOptions_OnButton(struct CGameMode_MainOptions *self, int button, int pad)
{
	void *memory;
	void *mode;
	int deflection;

	switch (button) {
	case 0: /* up */
		deflection = cmb_axis(pad, 0);
		if (deflection > cmb_axis(pad, 3) * 2 && deflection > cmb_axis(pad, 1) * 2) {
			cmb_text_deselect(self->items[self->option]);
			self->option--;
			if (self->option < 0)
				self->option = 5;
			cmb_text_select(self->items[self->option], 0.0f);
			CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		}
		break;
	case 1: /* right */
		deflection = cmb_axis(pad, 1);
		if (deflection > cmb_axis(pad, 0) * 2 && deflection > cmb_axis(pad, 2) * 2) {
			switch (self->option) {
			case 0:
				if (self->effects_level < 10)
					cmb_set_effects_level(self, self->effects_level + 1);
				break;
			case 1:
				if (self->music_level < 10)
					cmb_set_music_level(self, self->music_level + 1);
				break;
			case 2:
				cmb_toggle_vibration(pad);
				break;
			case 3:
				((int (*)(int))FUN_000750f0)(1);
				break;
			case 4:
				CGameMode_MainOptions_PlayTrack(1);
				break;
			}
		}
		break;
	case 2: /* down */
		deflection = cmb_axis(pad, 2);
		if (deflection > cmb_axis(pad, 3) * 2 && deflection > cmb_axis(pad, 1) * 2) {
			cmb_text_deselect(self->items[self->option]);
			self->option++;
			if (self->option > 5)
				self->option = 0;
			cmb_text_select(self->items[self->option], 0.0f);
			CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		}
		break;
	case 3: /* left */
		deflection = cmb_axis(pad, 3);
		if (deflection > cmb_axis(pad, 0) * 2 && deflection > cmb_axis(pad, 2) * 2) {
			switch (self->option) {
			case 0:
				if (self->effects_level > 0)
					cmb_set_effects_level(self, self->effects_level - 1);
				break;
			case 1:
				if (self->music_level > 0)
					cmb_set_music_level(self, self->music_level - 1);
				break;
			case 2:
				cmb_toggle_vibration(pad);
				break;
			case 3:
				((int (*)(int))FUN_000750f0)(-1);
				break;
			case 4:
				CGameMode_MainOptions_PlayTrack(-1);
				break;
			}
		}
		break;
	case 4:
	case 5: /* A, start */
		switch (self->option) {
		case 2:
			cmb_toggle_vibration(pad);
			break;
		case 3:
			((int (*)(int))FUN_000750f0)(1);
			break;
		case 4:
			CGameMode_MainOptions_PlayTrack(1);
			break;
		case 5: /* language and voices */
			memory = CMB_NEW(0x290, cmb_file_mainoptions, 0x16e);
			mode = memory == NULL ? NULL : ((void *(*)(void *))FUN_00017c70)(memory);
			CMB_PUSH_MODE(mode, 1);
			CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
			return 1;
		}
		break;
	case 6: /* back */
		CMB_PLAY_SAMPLE(10, 0x7f, 0, 0, 0);
		CMB_POP_MODE(1);
		return 1;
	}
	return 0;
}

/* 0x0001dd30  Core/CGameMode_MainOptions.cpp */
void CGameMode_MainOptions_Render(struct CGameMode_MainOptions *self, int active)
{
	((void (*)(void *, int))FUN_00030310)(self, active);
	if (active == 0)
		return;
	((void (*)(int))FUN_00043b70)(0);
	((void (*)(void))FUN_000438b0)();
	CGameMode_MainOptions_DrawMeter(80.0f, 95.0f, self->effects_level, self->option == 0);
	CGameMode_MainOptions_DrawMeter(80.0f, 55.0f, self->music_level, self->option == 1);
	if (self->group != NULL)
		CMB_VFN(self->group, 0x0c, void (*)(void *))(self->group);
}
