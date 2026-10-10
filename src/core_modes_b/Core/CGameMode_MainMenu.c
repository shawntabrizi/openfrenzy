/*
CGAMEMODE_MAINMENU.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_MainMenu.cpp

The main menu (Party Game, Tournament, Options, XDemos) and, emitted into the same
translation unit, the inline parts of CGameMode_MainOptions (constructor, Stop, the
language/settings text refresh, Update) and the menu's BG_Effects text box subclass.
*/

#include "xtl.h"

#include "core_modes_b.h"

static const char cmb_file_mainmenu[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_MainMenu.cpp";

/* 0x00255400  CGameMode_MainMenu vtable */
const void *const vtable_00255400[14] = {
	(const void *)FUN_0001d1e0, /* [0] scalar deleting destructor */
	(const void *)FUN_0001c310, /* [1] Stop */
	(const void *)FUN_0001c890, /* [2] Start */
	(const void *)FUN_0001c300, /* [3] Resume */
	(const void *)FUN_0001c2d0, /* [4] Load */
	(const void *)FUN_0002fe60, /* [5] core: button repeat dispatch */
	(const void *)FUN_0001c470, /* [6] Update */
	(const void *)FUN_0001c400, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] engine_misc: empty */
	(const void *)FUN_00012930, /* [9] core_modes_a: background-load poll */
	(const void *)FUN_000128e0, /* [10] core_modes_a: FinishedLoading */
	(const void *)FUN_001d5950, /* [11] empty */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_0001cb20, /* [13] OnButton */
};

/* 0x00255448  vtable of the main menu's BG_Effects text box subclass (0x4c bytes) */
const void *const vtable_00255448[7] = {
	(const void *)FUN_0001c860, /* [0] scalar deleting destructor */
	(const void *)FUN_00011160, /* [1] core_modes_a BG_Effects */
	(const void *)FUN_00011170, /* [2] core_modes_a BG_Effects: update */
	(const void *)FUN_00011210, /* [3] core_modes_a BG_Effects */
	(const void *)FUN_000135c0, /* [4] core_modes_a */
	(const void *)FUN_000135d0, /* [5] core_modes_a */
	(const void *)FUN_00011930, /* [6] core_modes_a BG_Effects */
};

/* 0x002554dc  CGameMode_MainOptions vtable */
const void *const vtable_002554dc[14] = {
	(const void *)FUN_0001d1e0, /* [0] scalar deleting destructor */
	(const void *)FUN_0001cd90, /* [1] Stop */
	(const void *)FUN_0001d200, /* [2] Start */
	(const void *)FUN_001d5950, /* [3] empty */
	(const void *)FUN_001d5950, /* [4] empty */
	(const void *)FUN_0002fe60, /* [5] core: button repeat dispatch */
	(const void *)FUN_0001cf90, /* [6] Update */
	(const void *)FUN_0001dd30, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a */
	(const void *)FUN_000128e0, /* [10] core_modes_a */
	(const void *)FUN_001d5950, /* [11] empty */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_0001d790, /* [13] OnButton */
};

/* 0x0001c2a0  Core/CGameMode_MainMenu.cpp (unplaced in the table) */
struct CGameMode_MainMenu *CGameMode_MainMenu_ctor(struct CGameMode_MainMenu *self)
{
	CMB_GAMEMODE_CTOR(self, "MainMenu");
	self->field_b8 = 0;
	self->font = 0;
	self->selection = 0;
	self->base.refs++;
	self->base.vtable = vtable_00255400;
	return self;
}

/* 0x0001c2d0  Core/CGameMode_MainMenu.cpp */
void CGameMode_MainMenu_Load(struct CGameMode_MainMenu *self)
{
	self->font = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
}

/* 0x0001c300  Core/CGameMode_MainMenu.cpp: thunk */
void CGameMode_MainMenu_Resume(struct CGameMode_MainMenu *self)
{
	((void (*)(void *))FUN_000129d0)(self);
}

/* 0x0001c310  Core/CGameMode_MainMenu.cpp */
void CGameMode_MainMenu_Stop(struct CGameMode_MainMenu *self)
{
	int i;

	for (i = 0; i < 4; i++) {
		if (self->items[i] == NULL)
			break;
		CMB_VFN(self->items[i], 0x14, void (*)(void *, float))(self->items[i], 0.0f);
		self->items[i] = NULL;
	}
	((void (*)(void *, void *))FUN_0002ff40)(self, NULL);
	if (self->group_a8 != NULL) {
		CMB_VFN(self->group_a8, 0x14, void (*)(void *, float))(self->group_a8, 0.0f);
		self->group_a8 = NULL;
	}
	if (self->group_ac != NULL) {
		CMB_VFN(self->group_ac, 0x14, void (*)(void *, float))(self->group_ac, 0.0f);
		((void (*)(void *, int))FUN_000137e0)(self->group_ac, 1);
		self->group_ac = NULL;
	}
}

/* 0x0001c390  Core/CGameMode_MainMenu.cpp: "Party Game" */
void CGameMode_MainMenu_StartParty(struct CGameMode_MainMenu *self)
{
	CScoreTable_Clear(&g_score_table);
	DAT_002bae98 = 0;
	DAT_002bae74 = 0;
	DAT_002bae88 = DAT_002844a0;
	CMB_PUSH_MODE(DAT_002ba550, 1);
}

/* 0x0001c3d0  Core/CGameMode_MainMenu.cpp: "Tournament" */
void CGameMode_MainMenu_StartTournament(struct CGameMode_MainMenu *self)
{
	CScoreTable_Clear(&g_score_table);
	DAT_002bae98 = 1;
	CMB_PUSH_MODE(DAT_002ba554, 1);
}

/* 0x0001c400  Core/CGameMode_MainMenu.cpp */
void CGameMode_MainMenu_Render(struct CGameMode_MainMenu *self, int active)
{
	float matrix[16];

	((void (*)(void *, int))FUN_00030310)(self, active);
	if (active != 0) {
		/* a copy of the identity matrix at 0x002855f0, scaled by 0.5 and moved */
		memset(matrix, 0, sizeof matrix);
		matrix[0] = 0.5f;
		matrix[5] = 0.5f;
		matrix[10] = 0.5f;
		matrix[15] = 1.0f;
		matrix[12] = 140.0f;
		matrix[13] = -190.0f;
		((void (*)(float *))FUN_00043930)(matrix);
		CMB_VFN(self->group_ac, 0x0c, void (*)(void *))(self->group_ac);
	}
}

/* 0x0001c470  Core/CGameMode_MainMenu.cpp */
void CGameMode_MainMenu_Update(struct CGameMode_MainMenu *self, int active)
{
	float idle;

	if (DAT_002ecdec != 0)
		return;
	((void (*)(int))FUN_00030000)(active);
	if (active == 0)
		return;
	idle = DAT_0028fda8 + self->idle_time;
	self->idle_time = idle;
	/* idle for DAT_00283760 frames: start the attract mode, unless the other timer runs */
	if (DAT_00283760 < idle && (_DAT_002ba9c8 <= 0.0f || 600.0f < _DAT_002ba9c8)) {
		self->idle_time = 0.0f;
		CMB_PUSH_MODE(DAT_002b95cc, 1);
		return;
	}
	CMB_VFN(self->group_ac, 0x08, void (*)(void *))(self->group_ac);
}

/* the main menu text box: a BG_Effects box (core_modes_a FUN_000117b0) with its own
   vtable. Real prototype of FUN_000117b0: (this, float x, float y, int columns, int rows,
   int length, float speed, int colour[4]). */
static void *cmb_bgbox_ctor(void *self, float x, float y, int columns, int rows, int length,
                            float speed, int *colour)
{
	return CMB_DUAL(FUN_000117b0,
		undefined4 *(*)(undefined4 *, undefined4, undefined4, int, int, undefined4, undefined4,
		                undefined4 *),
		void *(*)(void *, float, float, int, int, int, float, int *),
		(self, cmb_fbits(x), cmb_fbits(y), columns, rows, length, cmb_fbits(speed), colour),
		(self, x, y, columns, rows, length, speed, colour));
}

/* 0x0001c510  Core/CGameMode_MainMenu.cpp: the background text boxes */
void CGameMode_MainMenu_CreateBoxes(struct CGameMode_MainMenu *self)
{
	int colour[4];
	void *memory;
	unsigned int *box;

	colour[0] = DAT_002844dc[0];
	colour[1] = DAT_002844dc[1];
	colour[2] = DAT_002844dc[2];
	colour[3] = 0x18;
	memory = CMB_NEW(0x48, cmb_file_mainmenu, 0x1cb);
	box = memory == NULL ? NULL : cmb_bgbox_ctor(memory, -240.0f, 240.0f, 0x12, 0x18, 0x12, 8.0f, colour);
	((void (*)(void *, void *, int))FUN_00071070)((char *)self->group_a8 + 0x10, box == NULL ? NULL : box + 1, 1);
	CMB_VFN(box, 0x04, void (*)(void *))(box);

	colour[3] = 0x18;
	box = CMB_NEW(0x4c, cmb_file_mainmenu, 0x1d7);
	if (box != NULL) {
		cmb_bgbox_ctor(box, -340.0f, 115.0f, 0x20, 1, 1, 2.0f, colour);
		box[0] = (unsigned int)vtable_00255448;
		box[0x12] = 0;
	}
	CMB_AT(box, 0x44, float) = 2.8f;
	((void (*)(void *, void *, int))FUN_00071070)((char *)self->group_a8 + 0x10, box + 1, 1);
	CMB_VFN(box, 0x04, void (*)(void *))(box);

	colour[3] = 0x10;
	box = CMB_NEW(0x4c, cmb_file_mainmenu, 0x1dc);
	if (box != NULL) {
		cmb_bgbox_ctor(box, -340.0f, 60.0f, 0x10, 1, 1, 2.0f, colour);
		box[0] = (unsigned int)vtable_00255448;
		box[0x12] = 0;
	}
	CMB_AT(box, 0x44, float) = 5.6f;
	((void (*)(void *, void *, int))FUN_00071070)((char *)self->group_a8 + 0x10, box + 1, 1);
	CMB_VFN(box, 0x04, void (*)(void *))(box);

	colour[3] = 0x18;
	box = CMB_NEW(0x4c, cmb_file_mainmenu, 0x1e1);
	if (box != NULL) {
		cmb_bgbox_ctor(box, -340.0f, -190.0f, 0x20, 1, 1, 1.0f, colour);
		box[0] = (unsigned int)vtable_00255448;
		box[0x12] = 0;
	}
	CMB_AT(box, 0x44, float) = 2.8f;
	((void (*)(void *, void *, int))FUN_00071070)((char *)self->group_a8 + 0x10, box + 1, 1);
	CMB_VFN(box, 0x04, void (*)(void *))(box);

	memory = CMB_NEW(0x48, cmb_file_mainmenu, 0x1e5);
	box = memory == NULL ? NULL : cmb_bgbox_ctor(memory, 180.0f, 240.0f, 1, 0xc, 1, 2.0f, colour);
	CMB_AT(box, 0x44, float) = 2.8f;
	((void (*)(void *, void *, int))FUN_00071070)((char *)self->group_a8 + 0x10, box + 1, 1);
	CMB_VFN(box, 0x04, void (*)(void *))(box);
}

/* 0x0001c800  Core/CGameMode_MainMenu.cpp: release everything before XLaunchNewImage */
void CGameMode_MainMenu_PrepareLaunch(void)
{
	CMB_DEBUG_PRINTF("\nPrep start\n");
	((void (*)(int))FUN_0004eed0)(0);
	((void (*)(int))FUN_00071ff0)(-1000);
	((void (*)(void))FUN_00054730)();
	cmb_kick_off_and_wait_for_idle();
	while (((unsigned int (*)(int))FUN_0004cf90)(1) == 0)
		Sleep(0);
	CMB_DEBUG_PRINTF("\nPrep end\n");
}

/* 0x0001c860  Core/CGameMode_MainMenu.cpp: text box scalar deleting destructor */
void *CMainMenuBox_DeletingDtor(void *self, unsigned char flags)
{
	FUN_0001c880(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* 0x0001c880  Core/CGameMode_MainMenu.cpp: text box destructor (thunk to the BG_Effects one) */
void CMainMenuBox_Dtor(void *self)
{
	((void (*)(void *))FUN_00011890)(self);
}

/* 0x0001c890  Core/CGameMode_MainMenu.cpp */
void CGameMode_MainMenu_Start(struct CGameMode_MainMenu *self)
{
	int text_ids[4];
	int *slot;
	void *memory;
	void *group;
	void *item;
	int index;
	int y;

	text_ids[0] = 0xf;
	text_ids[1] = 0xe;
	text_ids[2] = 7;
	text_ids[3] = 0x1b5;
	if (DAT_002baad8 != 0) {
		DAT_002baae8 = 1;
		DAT_002baad8 = 0;
	}
	self->idle_time = 0.0f;
	((void (*)(int))FUN_00054690)(DAT_002bae6c);
	/* no auto-repeat for buttons 4, 5 and 6 */
	self->base.repeat_rate[6] = 100000000;
	self->base.repeat_rate[5] = 100000000;
	self->base.repeat_rate[4] = 100000000;
	DAT_00284408 = 0;
	for (slot = DAT_002ba570; slot < DAT_002ba570 + 0x31; slot++) {
		if (*slot != 0)
			*slot = 0;
	}
	DAT_002bae9c = 0;

	memory = CMB_NEW(0x20, cmb_file_mainmenu, 0x94);
	group = memory == NULL ? NULL : ((void *(*)(void *))FUN_00013820)(memory);
	self->group_a8 = group;
	((void (*)(void *, int))FUN_000137e0)(group, 1);
	memory = CMB_NEW(0x20, cmb_file_mainmenu, 0x97);
	self->group_ac = memory == NULL ? NULL : ((void *(*)(void *))FUN_00013820)(memory);
	if (DAT_002b95d0 != 0)
		CMB_U8(DAT_002b95d0, 0x58) = 1;
	CGameMode_MainMenu_CreateBoxes(self);

	index = 0;
	for (y = 100; y > -100; y -= 50) {
		memory = CMB_NEW(0x90, cmb_file_mainmenu, 0xa4);
		item = memory == NULL ? NULL
			: ((void *(*)(void *, int, int, const unsigned short *, int))FUN_00011c50)(
				memory, -200, y, CMB_TEXT(text_ids[index]), self->font);
		self->items[index] = item;
		CMB_AT(item, 0x34, float) = 0.75f;
		if (index == self->selection)
			((void (*)(void *, int))FUN_000114b0)(self->items[index], 0);
		else
			((void (*)(void *))FUN_00011500)(self->items[index]);
		item = self->items[index];
		((void (*)(void *, void *, int))FUN_00071070)((char *)self->group_ac + 0x10,
			item == NULL ? NULL : (char *)item + 4, 1);
		CMB_VFN(item, 0x04, void (*)(void *))(item);
		index++;
	}
	((void (*)(int, int, int, int, int, int))FUN_000724b0)(2, 0, 0xff, -20, 0x14, 0);
	((void (*)(void *, int, const char *))FUN_00030210)(self,
		CMB_RESOURCE4(0, "over_main.obe", 1, 0xdefa), NULL);
	for (index = 0; index < 4; index++) {
		((void (*)(int))FUN_00048d40)(DAT_002baf78[index][5]);
		((void (*)(int))FUN_00048d40)(DAT_002baf78[index][4]);
	}
}

/* 0x0001cb20  Core/CGameMode_MainMenu.cpp */
int CGameMode_MainMenu_OnButton(struct CGameMode_MainMenu *self, int button, int pad)
{
	unsigned char launch_data[3072];
	int image;
	int count;
	unsigned int result;

	self->idle_time = 0.0f;
	switch (button) {
	case 0: /* up */
		((void (*)(void *))FUN_00011500)(self->items[self->selection]);
		self->selection--;
		if (self->selection < 0)
			self->selection = 3;
		((void (*)(void *, int))FUN_000114b0)(self->items[self->selection], 0);
		CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		return 0;
	case 2: /* down */
		((void (*)(void *))FUN_00011500)(self->items[self->selection]);
		self->selection++;
		if (self->selection == 4)
			self->selection = 0;
		((void (*)(void *, int))FUN_000114b0)(self->items[self->selection], 0);
		CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		return 0;
	case 4:
	case 5: /* A, start */
		CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
		switch (self->selection) {
		case 0:
			CGameMode_MainMenu_StartParty(self);
			return 1;
		case 1:
			CGameMode_MainMenu_StartTournament(self);
			return 1;
		case 2:
			CMB_PUSH_MODE(DAT_002b9c78, 1);
			return 1;
		case 3: /* the XDemos launcher */
			((void (*)(int, int, int, int))FUN_0004b870)(
				((int (*)(int, const char *, int))FUN_0004db30)(0, "Packages\\XDemo", 0), -1, 0xdefa, 0);
			image = CMB_RESOURCE4(0, "wait.bmp", 5, 0xdefa);
			for (count = 3; count != 0; count--) {
				((void (*)(int, int, int))FUN_00046dd0)(image, -1, -1);
				((void (*)(int, int, int, int))FUN_00041ee0)(0, 0, 0, 0xff);
			}
			((void (*)(int, int, int))FUN_00046dd0)(image, -1, -1);
			D3DDevice_PersistDisplay();
			CGameMode_MainMenu_PrepareLaunch();
			result = XLaunchNewImageA("D:\\XDemos\\XDemos.xbe", (PLAUNCH_DATA)launch_data);
			CMB_DEBUG_PRINTF("\n\nXLaunchNewImage returned %d\n\n", result);
			Sleep(100000);
			break;
		}
		break;
	}
	return 0;
}

/* 0x0001cd50  Core/CGameMode_MainMenu.cpp: CGameMode_MainOptions constructor */
struct CGameMode_MainOptions *CGameMode_MainOptions_ctor(struct CGameMode_MainOptions *self)
{
	CMB_GAMEMODE_CTOR(self, "MainOptions");
	self->package = 0;
	self->option = 0;
	self->base.repeat_rate[6] = 100000000;
	self->base.repeat_rate[5] = 100000000;
	self->base.repeat_rate[4] = 100000000;
	self->base.refs++;
	self->base.vtable = vtable_002554dc;
	return self;
}

/* 0x0001cd90  Core/CGameMode_MainMenu.cpp: CGameMode_MainOptions::Stop */
void CGameMode_MainOptions_Stop(struct CGameMode_MainOptions *self)
{
	if (self->package != 0) {
		((void (*)(int))FUN_0004db40)(self->package);
		self->package = 0;
	}
	if (self->group != NULL) {
		CMB_VFN(self->group, 0x14, void (*)(void *, float))(self->group, 30.0f);
		((void (*)(void *, int))FUN_000137e0)(self->group, 1);
	}
	self->group = NULL;
	((void (*)(void *, void *))FUN_0002ff40)(self, NULL);
	((void (*)(void))FUN_0004c2c0)();
}

/* 0x0001cdf0  Core/CGameMode_MainMenu.cpp: CGameMode_MainOptions, refresh the setting texts */
void CGameMode_MainOptions_RefreshTexts(struct CGameMode_MainOptions *self)
{
	unsigned short text[64];
	int length;
	const unsigned short *value;

	/* text 0x174/0x173: on/off of the DAT_002bae7c setting */
	_snwprintf((wchar_t *)text, 0x40, L"%s %s", CMB_TEXT(0x172),
		DAT_002bae7c == 0 ? CMB_TEXT(0x174) : CMB_TEXT(0x173));
	CMB_VFN(self->items[2], 0x18, void (*)(void *, unsigned short *))(self->items[2], text);

	length = ((int (*)(const unsigned short *))FUN_0004b110)(CMB_TEXT(0x17d));
	value = ((const unsigned short *(*)(int, int))FUN_00075000)(DAT_002bae84, 0x1e - length);
	CMB_WSPRINTF(text, L"%S: %S", CMB_TEXT(0x17d), value);
	CMB_VFN(self->items[3], 0x18, void (*)(void *, unsigned short *))(self->items[3], text);

	length = ((int (*)(const unsigned short *))FUN_0004b110)(CMB_TEXT(0x1b2));
	value = ((const unsigned short *(*)(int, int, int))FUN_00075070)(DAT_002bae84, DAT_0028ffb8,
		0x1e - length);
	CMB_WSPRINTF(text, L"%S: %S", CMB_TEXT(0x1b2), value);
	CMB_VFN(self->items[4], 0x18, void (*)(void *, unsigned short *))(self->items[4], text);

	if (self->option >= 0) {
		if (self->option < 5)
			CMB_WSPRINTF(CMB_PTR(self->hint, 0x3c), L"%S %S", CMB_TEXT(0x18f), CMB_TEXT(0x18e));
		else if (self->option == 5)
			CMB_WSPRINTF(CMB_PTR(self->hint, 0x3c), L"%S %S", CMB_TEXT(0x18d), CMB_TEXT(0x18e));
	}
}

/* 0x0001cf90  Core/CGameMode_MainMenu.cpp: CGameMode_MainOptions::Update */
void CGameMode_MainOptions_Update(struct CGameMode_MainOptions *self, int active)
{
	if (DAT_002ecdec != 0)
		return;
	((void (*)(int))FUN_00030000)(active);
	if (active != 0) {
		CGameMode_MainOptions_RefreshTexts(self);
		if (self->group != NULL)
			CMB_VFN(self->group, 0x08, void (*)(void *))(self->group);
	}
}

/* 0x0001cfd0  Core/CGameMode_MainMenu.cpp: CGameMode_MainOptions, play the next/previous
   soundtrack (DAT_0028ffb8 is the current one) */
void CGameMode_MainOptions_PlayTrack(int step)
{
	int track;
	int count;

	CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
	if (DAT_002bae84 == 0) {
		track = step + DAT_0028ffb8;
		if (track == 0x1c)
			track = 6;
		else if (track == 5)
			track = 0x1b;
		((void (*)(int, int, int, int, int, int))FUN_00072440)(track, 0xff, -50, 0x32, 0, 0);
		return;
	}
	count = ((int (*)(int))FUN_00055fd0)(DAT_002bae84);
	((void (*)(int, int, int, int, int, int))FUN_00072440)((DAT_0028ffb8 + count + step) % count,
		0xff, -50, 0x32, 0, 0);
}

/* 0x0001d060  Core/CGameMode_MainMenu.cpp: a 10-bar level meter (no caller found; the
   fourth stack argument is not used) */
void CGameMode_MainOptions_DrawMeter(float x, float y, int level, int unused)
{
	unsigned int colour;
	float top;
	int bar;

	colour = DAT_002bce18[DAT_00283768[0]] | DAT_002bc490[DAT_00283768[1]] |
		DAT_002bd640[DAT_00283768[2]] | DAT_002bc898[DAT_00283768[3]];
	for (bar = 0; bar < 10; bar++) {
		DAT_002b9c80[bar][0].colour = colour;
		DAT_002b9c80[bar][1].colour = colour;
		DAT_002b9c80[bar][2].colour = colour;
		DAT_002b9c80[bar][3].colour = colour;
	}
	y = y - 20.0f;
	for (bar = 0; bar < 10; bar++) {
		struct cmb_meter_vertex *quad = DAT_002b9c80[bar];

		if (level <= bar) {
			colour = DAT_002bce18[DAT_00283768[4]] | DAT_002bc490[DAT_00283768[5]] |
				DAT_002bd640[DAT_00283768[6]] | DAT_002bc898[DAT_00283768[7]];
			quad[0].colour = colour;
			quad[1].colour = colour;
			quad[2].colour = colour;
			quad[3].colour = colour;
		}
		top = (float)(bar + 1) * 2.5f + y;
		quad[0].x = x;
		quad[0].y = top;
		quad[0].z = 0.0f;
		quad[1].x = x + 8.0f;
		quad[1].y = top;
		quad[1].z = 0.0f;
		quad[2].x = x + 8.0f;
		quad[2].y = y;
		quad[2].z = 0.0f;
		quad[3].x = x;
		quad[3].y = y;
		quad[3].z = 0.0f;
		((int (*)(int, int))FUN_000416c0)(0, 0);
		((void (*)(int, void *, int))FUN_0003f2d0)(5, quad, 4);
		x = x + 10.0f;
	}
}

/* 0x0001d1e0  Core/CGameMode_MainMenu.cpp: game mode scalar deleting destructor (shared by
   CGameMode_MainMenu and CGameMode_MainOptions) */
void *CGameMode_MainMenu_DeletingDtor(void *self, unsigned char flags)
{
	((void (*)(void *))FUN_0001ac80)(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}
