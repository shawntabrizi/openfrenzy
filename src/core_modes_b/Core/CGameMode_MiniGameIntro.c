/*
CGAMEMODE_MINIGAMEINTRO.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_MiniGameIntro.cpp

The screen before a minigame: its name, three lines of rules, the controls, and
(outside practice) "play / practice". When everybody is ready it fades out, pops itself
and starts the minigame mode below it on the stack.

Only 0x00023a00 is tagged with this file; the other methods (0x00023440-0x000239e0 and
0x000242d0-0x00024510) are in this class's vtable (0x00255ba8) and unplaced in the table.
*/

#include "core_modes_b.h"

static const char cmb_file_intro[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_MiniGameIntro.cpp";

#define CMB_TEXTBOX(memory, x, y, text, font) \
	((void *(*)(void *, int, int, const unsigned short *, int))FUN_00011c50)((memory), (x), (y), (text), (font))
#define cmb_add_to_background(effect) (((void (*)(void *, int))FUN_000137e0)((effect), 1))
/* BG_Effects text box, vtable +0x1c: start typing (char delay, start delay, sound) */
#define cmb_text_start(text, char_delay, start_delay) \
	(CMB_VFN(text, 0x1c, void (*)(void *, float, float, int))((text), (char_delay), (start_delay), 0))
#define cmb_text_highlight(text) (((void (*)(void *))FUN_000114e0)(text))
#define CMB_OD_SIZE 0x3b0
#define cmb_overlay_create(d) (((void *(*)(void *))FUN_00074500)(d))

/* the mode on top of the stack */
static void *cmb_top_mode(void)
{
	return ((void **)CMB_MODESTACK)[CMB_I32(CMB_MODESTACK, 0x50) - 1];
}

/* 0x00255ba8  CGameMode_MiniGameIntro vtable */
const void *const vtable_00255ba8[14] = {
	(const void *)FUN_000239e0, /* [0] scalar deleting destructor */
	(const void *)FUN_000235d0, /* [1] Suspend */
	(const void *)FUN_00023590, /* [2] Start */
	(const void *)FUN_00024510, /* [3] Exit */
	(const void *)FUN_000242d0, /* [4] Enter */
	(const void *)FUN_0002fe60, /* [5] core: button repeat dispatch */
	(const void *)FUN_000237b0, /* [6] Update */
	(const void *)FUN_00023900, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a: background-load poll */
	(const void *)FUN_000128e0, /* [10] core_modes_a: FinishedLoading */
	(const void *)FUN_001d5950, /* [11] empty */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_00023600, /* [13] OnButton */
};

/* 0x00023440  (unplaced): constructor (game: text index of the game, name: its package) */
struct CGameMode_MiniGameIntro *CGameMode_MiniGameIntro_ctor(struct CGameMode_MiniGameIntro *self,
	int game, const char *name)
{
	CMB_GAMEMODE_CTOR(self, "MiniGameIntro");
	self->base.vtable = vtable_00255ba8;
	((void (*)(void *))FUN_00071050)(self->list);
	self->game = game;
	self->frames = 0;
	self->fade_start = 0;
	strcpy(self->name, name);
	self->base.flags |= 2;
	DAT_00292c50 = 0x18;
	DAT_00292c4c = 0x18;
	DAT_00292c78 = 0x240;
	CMB_DEBUG_PRINTF("\n\nNOW LAUNCHING: %S (%d %d)\n\n\n", CMB_TEXT(self->game + 0xa5),
		DAT_002baeac, DAT_002baeb0);
	return self;
}

/* 0x00023530  (unplaced): destructor */
void CGameMode_MiniGameIntro_dtor(struct CGameMode_MiniGameIntro *self)
{
	self->base.vtable = vtable_00255ba8;
	((void (*)(void *))FUN_001d5950)(self->list);
	CMB_GAMEMODE_DTOR(self);
}

/* 0x00023590  (unplaced): Start */
void CGameMode_MiniGameIntro_Start(struct CGameMode_MiniGameIntro *self)
{
	DAT_00284408 = 1;
	if (DAT_002b95d0 != 0)
		CMB_U8(DAT_002b95d0, 0x58) = 1;
	self->saved_fov[0] = DAT_002be588;
	self->saved_fov[1] = DAT_002be58c;
	_DAT_002ecde8 = 0;
}

/* 0x000235d0  (unplaced): Suspend */
void CGameMode_MiniGameIntro_Suspend(struct CGameMode_MiniGameIntro *self)
{
	if (DAT_002b95d0 != 0)
		CMB_U8(DAT_002b95d0, 0x58) = 0;
	((void (*)(float, float))FUN_00044a00)(self->saved_fov[0], self->saved_fov[1]);
}

/* 0x00023600  (unplaced): OnButton; returns 1 while not ready */
int CGameMode_MiniGameIntro_OnButton(struct CGameMode_MiniGameIntro *self, int button, int pad)
{
	if (self->ready == 0 || self->fade_start != 0)
		return 1;
	switch (button) {
	case 0: /* up */
		if (DAT_002baed0 != 0)
			break;
		cmb_text_deselect(self->choice[self->selection]);
		self->selection--;
		if (self->selection < 0) {
			self->selection = 0;
			cmb_text_select(self->choice[self->selection], 0.0f);
			return 0;
		}
		CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		cmb_text_select(self->choice[self->selection], 0.0f);
		return 0;
	case 2: /* down */
		if (DAT_002baed0 != 0)
			break;
		cmb_text_deselect(self->choice[self->selection]);
		self->selection++;
		if (self->selection > 1)
			self->selection = 1;
		else
			CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		cmb_text_select(self->choice[self->selection], 0.0f);
		break;
	case 4:
	case 5: /* go */
		self->fade_start = self->frames;
		if (DAT_002baeac != 0 && (DAT_002baeb8 == 0 || DAT_002baeac == 0xb))
			((void (*)(int))FUN_00071ff0)(-10);
		CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
		return 0;
	case 6: /* back, in a tournament */
		if (DAT_002bae98 == 1) {
			CMB_PLAY_SAMPLE(10, 0x7f, 0, 0, 0);
			CMB_POP_MODE(1);
			((void (*)(void))FUN_0002c2e0)();
			return 1;
		}
		break;
	}
	return 0;
}

/* 0x000237b0  (unplaced): Update */
void CGameMode_MiniGameIntro_Update(struct CGameMode_MiniGameIntro *self, int active)
{
	int character;
	int package;

	if (DAT_002ecdec != 0)
		return;
	((void (*)(int))FUN_00030000)(active);
	if (self->loaded != 0) {
		/* wait until the characters' packages are loaded */
		self->ready = 1;
		for (character = 0; character < 6; character++) {
			if (DAT_002b95b0[character] == 0)
				continue;
			package = DAT_00290754[character * 0x18];
			DAT_002b95b0[character] = 0;
			if (package != 0) {
				((int (*)(int, int, int, int))FUN_0004b870)(package, -1, 0xdefa, 0);
				((void (*)(int))FUN_00071170)(character);
			}
			self->ready = 0;
		}
	}
	if (self->ready != 0) {
		self->frames++;
		if (DAT_002baeb8 != 0 || DAT_003055e0 != 0) {
			/* nobody to press A: go */
			if (self->fade_start == 0)
				CMB_VFN(self, CMB_VT_ONBUTTON, int (*)(void *, int, int))(self, 4, 0);
			return;
		}
		if (CMB_I32(self->logo, 0x2bc) == 0) {
			CMB_VFN(self->choice[0], 0x10, void (*)(void *, int))(self->choice[0], 0);
			cmb_text_select(self->choice[0], 0.0f);
			cmb_add_to_background(self->choice[0]);
			if (DAT_002baed0 == 0) {
				CMB_VFN(self->choice[1], 0x10, void (*)(void *, int))(self->choice[1], 0);
				cmb_text_deselect(self->choice[1]);
				cmb_add_to_background(self->choice[1]);
			}
			CMB_I32(self->logo, 0x2bc) = -4;
		}
	}
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074be0)();
}

/* 0x00023900  (unplaced): Render; fades out and starts the minigame */
void CGameMode_MiniGameIntro_Render(struct CGameMode_MiniGameIntro *self, int active)
{
	void *game;

	((void (*)(void *, int))FUN_00030310)(self, active);
	((void (*)(int))FUN_00043b70)(0);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(int))FUN_00073900)(1);
	if (self->fade_start == 0)
		return;
	((void (*)(int, int))FUN_00071c20)((self->frames - self->fade_start) * 0x109 / DAT_002838f0, 1);
	if (self->frames >= self->fade_start + DAT_002838f0 + 2) {
		DAT_002baea8 = self->selection;
		CMB_POP_MODE(1);
		game = cmb_top_mode();
		CMB_VFN(game, 0x2c, void (*)(void *))(game);
		game = cmb_top_mode();
		CMB_VFN(game, 0x30, void (*)(void *))(game);
		CMB_I32(cmb_top_mode(), 4) = 1;
		CMB_DEBUG_PRINTF("\n\nMiniGameIntro initing game on frame: %d\n\n", DAT_002ba99c);
	}
}

/* 0x000239e0  (unplaced): scalar deleting destructor */
void *CGameMode_MiniGameIntro_DeletingDtor(struct CGameMode_MiniGameIntro *self, unsigned char flags)
{
	CGameMode_MiniGameIntro_dtor(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* 0x00023a00  Core/CGameMode_MiniGameIntro.cpp: the texts and overlays */
void CGameMode_MiniGameIntro_CreateTexts(struct CGameMode_MiniGameIntro *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];
	void *memory;

	memset(desc, 0, sizeof desc);
	((void (*)(void *))FUN_00072720)(self->list);
	if (DAT_002baeac == 0) {
		memory = CMB_NEW(0x24, cmb_file_intro, 0x5c);
		self->backdrop = memory == NULL ? NULL : ((void *(*)(void *))FUN_00012730)(memory);
		cmb_add_to_background(self->backdrop);
	} else {
		self->backdrop = NULL;
	}

	/* the name */
	if (DAT_002baed0 != 0) {
		self->game = 0x4d;
		memory = CMB_NEW(0x90, cmb_file_intro, 0x68);
		self->title = memory == NULL ? NULL : CMB_TEXTBOX(memory, -245, 175, CMB_TEXT(0x184), DAT_002ba224);
		_DAT_002ecda8 = 0x184;
	} else {
		memory = CMB_NEW(0x90, cmb_file_intro, 0x6d);
		self->title = memory == NULL ? NULL
			: CMB_TEXTBOX(memory, -245, 175, CMB_TEXT(self->game + 0xa5), DAT_002ba224);
		_DAT_002ecda8 = self->game + 0xa5;
	}
	_DAT_002ecd9c = self->game;
	_DAT_002ecda4 = self->game + 0x36;
	_DAT_002ecda0 = self->game + 0x6c;
	cmb_add_to_background(self->title);
	cmb_text_start(self->title, 1.0f, 10.0f);
	CMB_I32(self->title, 0x6c) = 200;
	cmb_text_select(self->title, 0.0f);

	/* the rules */
	if (DAT_002baed0 != 0) {
		memory = CMB_NEW(0x90, cmb_file_intro, 0x7b);
		self->text[0] = memory == NULL ? NULL
			: CMB_TEXTBOX(memory, -235, DAT_002838e4[0], CMB_TEXT(0x185), DAT_002ba220);
	} else {
		memory = CMB_NEW(0x90, cmb_file_intro, 0x7d);
		self->text[0] = memory == NULL ? NULL
			: CMB_TEXTBOX(memory, -235, DAT_002838e4[0], CMB_TEXT(self->game), DAT_002ba220);
	}
	CMB_F32(self->text[0], 0x7c) = 470.0f;
	CMB_I32(self->text[0], 0x80) = 1;
	cmb_add_to_background(self->text[0]);
	cmb_text_start(self->text[0], 1.0f, 10.0f);
	cmb_text_highlight(self->text[0]);
	if (DAT_002baed0 != 0) {
		memory = CMB_NEW(0x90, cmb_file_intro, 0x86);
		self->text[1] = memory == NULL ? NULL
			: CMB_TEXTBOX(memory, -235, DAT_002838e4[1], CMB_TEXT(0x186), DAT_002ba220);
	} else {
		memory = CMB_NEW(0x90, cmb_file_intro, 0x88);
		self->text[1] = memory == NULL ? NULL
			: CMB_TEXTBOX(memory, -235, DAT_002838e4[1], CMB_TEXT(self->game + 0x36), DAT_002ba220);
	}
	CMB_F32(self->text[1], 0x7c) = 470.0f;
	CMB_I32(self->text[1], 0x80) = 1;
	cmb_add_to_background(self->text[1]);
	cmb_text_start(self->text[1], 1.0f, 40.0f);
	CMB_I32(self->text[1], 0x6c) = 200;
	cmb_text_highlight(self->text[1]);
	memory = CMB_NEW(0x90, cmb_file_intro, 0x90);
	self->text[2] = memory == NULL ? NULL
		: CMB_TEXTBOX(memory, -235, DAT_002838e4[2], CMB_TEXT(self->game + 0x6c), DAT_002ba220);
	CMB_F32(self->text[2], 0x7c) = 400.0f;
	CMB_I32(self->text[2], 0x80) = 1;
	cmb_add_to_background(self->text[2]);
	cmb_text_start(self->text[2], 1.0f, 70.0f);
	CMB_I32(self->text[2], 0x6c) = 200;
	cmb_text_highlight(self->text[2]);

	/* the controls headings */
	memory = CMB_NEW(0x90, cmb_file_intro, 0x98);
	self->controls[0] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -245, 0x6f, CMB_TEXT(0x124), DAT_002ba220);
	cmb_add_to_background(self->controls[0]);
	cmb_text_start(self->controls[0], 1.0f, 10.0f);
	CMB_I32(self->controls[0], 0x6c) = 200;
	cmb_text_select(self->controls[0], 0.0f);
	memory = CMB_NEW(0x90, cmb_file_intro, 0x9e);
	self->controls[1] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -245, -4, CMB_TEXT(0x125), DAT_002ba220);
	cmb_add_to_background(self->controls[1]);
	cmb_text_start(self->controls[1], 1.0f, 10.0f);
	cmb_text_select(self->controls[1], 0.0f);

	/* play / practice (added when the loading has finished) */
	memory = CMB_NEW(0x90, cmb_file_intro, 0xa4);
	self->choice[0] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -110, -130, CMB_TEXT(0xd), DAT_002ba220);
	CMB_U32(self->choice[0], 0x8c) |= 8;
	memory = CMB_NEW(0x90, cmb_file_intro, 0xa8);
	self->choice[1] = memory == NULL ? NULL : CMB_TEXTBOX(memory, -110, -150, CMB_TEXT(0x126), DAT_002ba220);
	CMB_U32(self->choice[1], 0x8c) |= 8;

	/* the animated logo */
	memset(desc, 0, sizeof desc);
	sprintf((char *)desc + 0x2c4, "animlogo.gif");
	CMB_I32(desc, 0x288) = 0;
	CMB_I32(desc, 0x290) = 0x280;
	CMB_I32(desc, 0x268) = 0;
	CMB_I32(desc, 0x264) = 0x7f;
	CMB_I32(desc, 0x28c) = 1;
	CMB_I32(desc, 0x298) = 1;
	CMB_F32(desc, 0x2a4) = 173.0f;
	CMB_F32(desc, 0x2a8) = -120.0f;
	CMB_F32(desc, 0x308) = 64.0f;
	CMB_F32(desc, 0x30c) = 64.0f;
	CMB_F32(desc, 0x284) = 20.0f;
	CMB_F32(desc, 0x250) = 127.0f;
	CMB_F32(desc, 0x254) = 127.0f;
	CMB_F32(desc, 0x258) = 127.0f;
	CMB_F32(desc, 0x25c) = 127.0f;
	CMB_I32(desc, 0x26c) = 0xff;
	CMB_I32(desc, 0x278) = 0;
	CMB_I32(desc, 0x270) = 0xff;
	CMB_I32(desc, 0x27c) = 0;
	CMB_I32(desc, 0x274) = 0xff;
	CMB_I32(desc, 0x280) = 0;
	CMB_I32(desc, 0x264) = 0xff;
	CMB_I32(desc, 0x268) = 0;
	self->logo = cmb_overlay_create(desc);

	/* the bar that slides in */
	memset(desc, 0, sizeof desc);
	CMB_I32(desc, 0x288) = 0;
	CMB_I32(desc, 0x294) = 1;
	CMB_F32(desc, 0x284) = 16.0f;
	CMB_F32(desc, 0x250) = 0.0f;
	CMB_F32(desc, 0x254) = 75.0f;
	CMB_F32(desc, 0x258) = 151.0f;
	CMB_F32(desc, 0x25c) = 255.0f;
	CMB_F32(desc, 0x2a4) = -960.0f;
	CMB_F32(desc, 0x2a8) = -100.0f;
	CMB_F32(desc, 0x2ac) = 0.0f;
	CMB_F32(desc, 0x08) = -320.0f;
	CMB_F32(desc, 0x0c) = -100.0f;
	CMB_F32(desc, 0x10) = 0.0f;
	CMB_I32(desc, 0x28c) = 4;
	CMB_F32(desc, 0x308) = 640.0f;
	CMB_F32(desc, 0x30c) = 4.0f;
	cmb_overlay_create(desc);
}

/* 0x000242d0  (unplaced): Enter */
void CGameMode_MiniGameIntro_Enter(struct CGameMode_MiniGameIntro *self)
{
	int used[6];
	int player;
	int character;

	memset(used, 0, sizeof used);
	DAT_00290960 = 1;
	((void (*)(int))FUN_0002c5b0)(-1);
	((void (*)(int))FUN_0004eed0)(0);
	DAT_002ba4e0++;
	DAT_002f28ac = 0;
	((void (*)(int))FUN_00054690)(DAT_002bae6c);
	CMB_I32(DAT_002ecfac, 0x88) = 0;
	CMB_I32(DAT_002ecfac, 0x8c) = 1;
	((void (*)(void))FUN_0003bc90)();
	((void (*)(void))FUN_0003b700)();
	CMB_I32(DAT_002ecfac, 0xc8) = 1;
	for (player = 0; player < 4; player++) {
		CMB_F32(DAT_002ecfac, 0xa8 + player * 4) = 275.0f;
		DAT_002baed4[player][5] = 0;
		DAT_00290be4[player] = 50.0f;
		DAT_002ecf90[player] = 0;
	}
	if (DAT_002bae98 == 2 && DAT_002bae8c == 0) {
		DAT_002bae90--;
		DAT_002bae8c = 1;
		DAT_002baed4[0][1] = 1;
	}
	if (DAT_002ba6a4 != 0) {
		/* attract mode: four CPU players with different characters */
		DAT_002baeb8 = 1;
		for (player = 0; player < 4; player++) {
			DAT_002baed4[player][1] = 0;
			do
				character = cmb_rand() % 6;
			while (used[character] != 0);
			DAT_002baed4[player][0] = character;
			used[character] = 1;
		}
		DAT_002bae90 = 4;
		DAT_002bae8c = 0;
	}
	if (DAT_002baeb8 == 0)
		((void (*)(int, int, int, int, int, int))FUN_000724b0)(8, 0, 0xff, -20, 0x14, 0);
	self->loaded = 0;
	DAT_002ba224 = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	if (DAT_002ba224 == 0 ||
	    (DAT_002ba220 = CMB_RESOURCE4(0, "handgotl.ttf", 3, 0xdefa)) == 0) {
		CMB_DEBUG_PRINTF("CGameMode_CharacterSelect::GainFocus *** Could not load font 'boombox.bmp' ***\n");
		return;
	}
	self->loading_model = CMB_RESOURCE4(0, "over_loading.obe", 1, 0xdefa);
	((void (*)(void *, int, const char *))FUN_00030210)(self, self->loading_model, NULL);
	CGameMode_MiniGameIntro_CreateTexts(self);
	self->selection = 0;
	self->ready = 0;
	CMB_VFN(self, CMB_VT_UPDATE, void (*)(void *, int))(self, 1);
}

/* 0x00024510  (unplaced): Exit */
void CGameMode_MiniGameIntro_Exit(struct CGameMode_MiniGameIntro *self)
{
	int i;

	CMB_VFN(self->title, 0x14, void (*)(void *, float))(self->title, -1.0f);
	CMB_VFN(self->text[0], 0x14, void (*)(void *, float))(self->text[0], -1.0f);
	CMB_VFN(self->text[1], 0x14, void (*)(void *, float))(self->text[1], -1.0f);
	CMB_VFN(self->text[2], 0x14, void (*)(void *, float))(self->text[2], -1.0f);
	CMB_VFN(self->controls[0], 0x14, void (*)(void *, float))(self->controls[0], -1.0f);
	CMB_VFN(self->controls[1], 0x14, void (*)(void *, float))(self->controls[1], -1.0f);
	if (self->backdrop != NULL) {
		((int (*)(void *, void *))FUN_000710b0)((char *)DAT_002b95d0 + 0x74, (char *)self->backdrop + 4);
		if (self->backdrop != NULL)
			CMB_VFN(self->backdrop, 0x00, void *(*)(void *, int))(self->backdrop, 1);
		self->backdrop = NULL;
	}
	for (i = 0; i < 2; i++) {
		if (self->choice[i] != NULL)
			CMB_VFN(self->choice[i], 0x14, void (*)(void *, float))(self->choice[i], -1.0f);
	}
	((void (*)(void *))FUN_0002ffd0)(self);
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074ba0)();
}
