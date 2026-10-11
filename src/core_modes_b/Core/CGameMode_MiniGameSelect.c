/*
CGAMEMODE_MINIGAMESELECT.C  <-  C:\BlitzParty\Game\Src\Core\CGameMode_MiniGameSelect.cpp

The tournament's minigame choice: a carousel of every available game variant, sorted by
name, with the info panel of the selected one (core FUN_0002e0d0). Starting a game hands
over to the game's module (DAT_002ba954); coming back adds the scores to the tournament
totals and shows the score screen.

The constructor (0x00024620) and destructor (0x000247b0) are unplaced in the table.
*/

#include "core_modes_b.h"

static const char cmb_file_select[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_MiniGameSelect.cpp";

#define CMB_TEXTBOX(memory, x, y, text, font) \
	((void *(*)(void *, int, int, const unsigned short *, int))FUN_00011c50)((memory), (x), (y), (text), (font))
#define CMB_OD_SIZE 0x3b0
#define cmb_overlay_create(d) (((void *(*)(void *))FUN_00074500)(d))

/* 0x00283900  file-name prefix of each game's pictures (.data pointer table) */
static const char *const cmb_game_prefix[13] = {
	"meta", "bump", "fire", "clust", "dem", "drill", "tag", "hop", "bug", "roll", "row",
	"scratch", "tank",
};

/* 0x00255d0c  CGameMode_MiniGameSelect vtable */
const void *const vtable_00255d0c[14] = {
	(const void *)FUN_00024fe0, /* [0] scalar deleting destructor */
	(const void *)FUN_00024880, /* [1] Suspend */
	(const void *)FUN_00025000, /* [2] Start */
	(const void *)FUN_001d5950, /* [3] empty */
	(const void *)FUN_00024820, /* [4] Enter */
	(const void *)FUN_000248f0, /* [5] button repeat dispatch */
	(const void *)FUN_00024940, /* [6] Update */
	(const void *)FUN_00024a00, /* [7] Render */
	(const void *)FUN_001ec260, /* [8] empty */
	(const void *)FUN_00012930, /* [9] core_modes_a: background-load poll */
	(const void *)FUN_000128e0, /* [10] core_modes_a: FinishedLoading */
	(const void *)FUN_00025290, /* [11] Loaded */
	(const void *)FUN_001d5950, /* [12] empty */
	(const void *)FUN_00024d40, /* [13] OnButton */
};

/* 0x000245f0 (inside the gap): qsort comparison of the carousel, by name */
static int cmb_compare_entries(const void *a, const void *b)
{
	return wcscmp((const wchar_t *)CMB_TEXT(((const struct CMiniGameEntry *)a)->text),
		(const wchar_t *)CMB_TEXT(((const struct CMiniGameEntry *)b)->text));
}

/* 0x00024620  (unplaced): constructor; builds the carousel from the availability table */
struct CGameMode_MiniGameSelect *CGameMode_MiniGameSelect_ctor(struct CGameMode_MiniGameSelect *self)
{
	struct CMiniGameEntry *entry;
	int game;
	int variant;

	CMB_GAMEMODE_CTOR(self, "MiniGameSelect");
	self->base.vtable = vtable_00255d0c;
	((void (*)(void *))FUN_00071050)(self->list);
	self->base.flags |= 2;
	self->base.refs++;
	self->info = NULL;
	DAT_002ba4dc = 0;
	entry = DAT_002ba238;
	for (game = 1; game <= 12; game++) {
		for (variant = 1; variant <= 4; variant++) {
			if (DAT_002839f0[game * 4 + variant - 4] != 0) {
				entry->game = game;
				entry->variant = variant;
				entry->text = 0xd1 + (game - 1) * 4 + (variant - 1);
				DAT_002ba4dc++;
				entry++;
			}
		}
	}
	qsort(DAT_002ba238, DAT_002ba4dc, 0xc, cmb_compare_entries);
	self->base.repeat_rate[2] = 4;
	self->base.repeat_rate[0] = 4;
	self->base.repeat_delay[2] = 0xf;
	self->base.repeat_delay[0] = 0xf;
	self->field_9c = 1;
	return self;
}

/* 0x000247b0  (unplaced): destructor */
void CGameMode_MiniGameSelect_dtor(struct CGameMode_MiniGameSelect *self)
{
	self->base.vtable = vtable_00255d0c;
	if ((int)self == DAT_002ba55c)
		DAT_002ba55c = 0;
	((void (*)(void *))FUN_001d5950)(self->list);
	CMB_GAMEMODE_DTOR(self);
}

/* 0x00024820  Core/CGameMode_MiniGameSelect.cpp: Enter (a new tournament) */
void CGameMode_MiniGameSelect_Enter(struct CGameMode_MiniGameSelect *self)
{
	self->leave = 0;
	self->totals[0] = 0;
	self->totals[1] = 0;
	self->totals[2] = 0;
	self->totals[3] = 0;
	memset(self->items, 0, sizeof self->items);
	self->base.repeat_rate[8] = 10000000;
	self->base.repeat_rate[7] = 10000000;
	self->base.repeat_rate[6] = 100000000;
	self->base.repeat_rate[5] = 100000000;
	self->base.repeat_rate[4] = 100000000;
}

/* 0x00024880  Core/CGameMode_MiniGameSelect.cpp: Suspend */
void CGameMode_MiniGameSelect_Suspend(struct CGameMode_MiniGameSelect *self)
{
	int i;

	for (i = 0; i < DAT_002ba4dc; i++) {
		if (self->items[i] != NULL) {
			CMB_VFN(self->items[i], 0x00, void *(*)(void *, int))(self->items[i], 1);
			self->items[i] = NULL;
		}
	}
	((void (*)(void *))FUN_00072720)(self->list);
	((void (*)(void))FUN_00074ba0)();
	((void (*)(void *, void *))FUN_0002ff40)(self, NULL);
	if (self->info != NULL) {
		CMB_VFN(self->info, 0x00, void *(*)(void *, int))(self->info, 1);
		self->info = NULL;
	}
}

/* 0x000248f0  Core/CGameMode_MiniGameSelect.cpp: button repeat, and leave on back */
void CGameMode_MiniGameSelect_Buttons(struct CGameMode_MiniGameSelect *self, int active)
{
	if (active == 0 || self->base.state != 0)
		return;
	CMB_AT(self, 0x18, int)++;
	((void (*)(void *, int))FUN_0002fe60)(self, active);
	if (self->leave != 0)
		CMB_POP_MODE(1);
}

/* 0x00024940  Core/CGameMode_MiniGameSelect.cpp */
void CGameMode_MiniGameSelect_Update(struct CGameMode_MiniGameSelect *self, int active)
{
	int i;

	if (DAT_002ecdec != 0)
		return;
	((void (*)(int))FUN_00030000)(active);
	if (active == 0)
		return;
	if (self->base.state != 0) {
		CMB_VFN(self, 0x24, void (*)(void *))(self);
		return;
	}
	if (DAT_002ba4d0 != 0) {
		_DAT_002ba4c0 = _DAT_00283988 * DAT_0028fda8 + _DAT_002ba4c0;
		if (_DAT_002ba4c0 > 1.0f) {
			DAT_002ba4d0 = 0;
			_DAT_002ba4c0 = 0.0f;
			cmb_text_select(self->items[DAT_002ba4d8], 0.0f);
		}
	}
	for (i = 0; i < DAT_002ba4dc; i++)
		CMB_VFN(self->items[i], 0x08, void (*)(void *))(self->items[i]);
	((void (*)(void))FUN_00074be0)();
	CMB_VFN(self->info, 0x04, void (*)(void *))(self->info);
}

/* 0x00024a00  Core/CGameMode_MiniGameSelect.cpp: the carousel, nine entries visible */
void CGameMode_MiniGameSelect_Render(struct CGameMode_MiniGameSelect *self, int active)
{
	static const float scale[9] = { 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f };
	static const float xs[9] = {
		-200.0f, -196.0f, -192.0f, -188.0f, -184.0f, -188.0f, -192.0f, -196.0f, -200.0f,
	};
	static const float ys[9] = { 88.0f, 76.0f, 60.0f, 40.0f, 16.0f, -14.0f, -39.0f, -59.0f, -74.0f };
	float table[27];
	void *item;
	float t;
	int dir;
	int top;
	int k;
	int i;

	/* the original builds the table on the stack: scale[9], x[9], y[9] */
	memcpy(table, scale, sizeof scale);
	memcpy(table + 9, xs, sizeof xs);
	memcpy(table + 18, ys, sizeof ys);
	if (DAT_002ba4e4 != 0) {
		((void (*)(int, int))FUN_00071c20)(0xff, 0);
		DAT_002ba4e4 = 0;
	}
	((void (*)(void *, int))FUN_00030310)(self, active);
	if (active == 0 || self->base.state != 0)
		return;
	top = DAT_002ba4d4;
	if (DAT_002ba4d0 == 0) {
		CMB_F32(self->items[(DAT_002ba4d4 - 1 + DAT_002ba4dc) % DAT_002ba4dc], 0x14) = 100.0f;
		for (i = top; i < DAT_002ba4d4 + 9; i++) {
			k = i - DAT_002ba4d4;
			item = self->items[i % DAT_002ba4dc];
			CMB_F32(item, 0x34) = table[k];
			CMB_F32(item, 0x10) = table[k + 9];
			CMB_F32(item, 0x14) = table[k + 18];
			CMB_VFN(item, 0x0c, void (*)(void *))(item);
		}
	} else {
		CMB_F32(self->items[(DAT_002ba4dc - 1 + DAT_002ba4d4) % DAT_002ba4dc], 0x14) = 100.0f;
		for (i = top; i < DAT_002ba4d4 + 9; i++) {
			dir = DAT_002ba4d0;
			if (!((dir > 0 && i != DAT_002ba4d4 + 8) || (dir < 0 && i != DAT_002ba4d4)))
				continue;
			k = i - DAT_002ba4d4;
			t = _DAT_002ba4c0;
			item = self->items[i % DAT_002ba4dc];
			CMB_F32(item, 0x34) = t * table[k] + (1.0f - t) * table[k + dir];
			CMB_F32(item, 0x10) = t * table[k + 9] + (1.0f - t) * table[DAT_002ba4d0 + k + 9];
			CMB_F32(item, 0x14) = t * table[k + 18] + (1.0f - t) * table[DAT_002ba4d0 + k + 18];
			CMB_VFN(item, 0x0c, void (*)(void *))(item);
		}
	}
	((void (*)(int))FUN_00073900)(1);
	((void (*)(void *))FUN_0002d830)(self->info);
}

/* 0x00024cc0  Core/CGameMode_MiniGameSelect.cpp: show entry `index` in the info panel */
void CGameMode_MiniGameSelect_ShowGame(struct CGameMode_MiniGameSelect *self, int index, int from_below)
{
	char name[64];
	int picture;

	sprintf(name, "%s%d.bmp", cmb_game_prefix[DAT_002ba238[index].game], DAT_002ba238[index].variant);
	picture = CMB_RESOURCE4(0, name, 0, 0xdefa);
	((void (*)(void *, int, int, int, char))FUN_0002da90)(self->info, DAT_002ba238[index].game,
		DAT_002ba238[index].variant, picture, from_below != 0);
}

/* the arrow overlay: glow it */
static void cmb_flash_arrow(void *arrow)
{
	CMB_F32(arrow, 0x2b8) = 127.0f;
	CMB_I32(arrow, 0x2bc) = -6;
	CMB_I32(arrow, 0x2f4) = 1;
}

/* start the selected game: its module (vtable +0xc) starts the variant */
static void cmb_start_selected(void)
{
	int game = DAT_002ba238[DAT_002ba4d8].game;
	int variant = DAT_002ba238[DAT_002ba4d8].variant;
	int availability;

	DAT_002baeac = game;
	DAT_002baeb0 = variant;
	availability = DAT_002839f0[variant + game * 4 - 4];
	if ((availability < 0 ? -availability : availability) == 3)
		DAT_002baebc = 1;
	CMB_VFN(DAT_002ba954[game], 0x0c, void (*)(void *, int))(DAT_002ba954[game], variant);
}

/* 0x00024d40  Core/CGameMode_MiniGameSelect.cpp */
int CGameMode_MiniGameSelect_OnButton(struct CGameMode_MiniGameSelect *self, int button, int pad)
{
	int player;

	if (CMB_AT(self, 0x18, int) < DAT_00283984 || DAT_002ba4d0 != 0)
		return 0;
	switch (button) {
	case 0:
	case 3: /* previous */
		DAT_002ba4d0 = -1;
		_DAT_002ba4c0 = 0.0f;
		cmb_flash_arrow(self->up_arrow);
		CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		DAT_002ba4d4 = (DAT_002ba4dc - 1 + DAT_002ba4d4) % DAT_002ba4dc;
		cmb_text_deselect(self->items[DAT_002ba4d8]);
		DAT_002ba4d8--;
		if (DAT_002ba4d8 == -1)
			DAT_002ba4d8 = DAT_002ba4dc - 1;
		CGameMode_MiniGameSelect_ShowGame(self, DAT_002ba4d8, 0);
		return 0;
	case 1:
	case 2: /* next */
		_DAT_002ba4c0 = 0.0f;
		DAT_002ba4d0 = 1;
		cmb_flash_arrow(self->down_arrow);
		CMB_PLAY_SAMPLE(7, 0x7f, 0, 0, 0);
		DAT_002ba4d4 = (DAT_002ba4d4 + 1) % DAT_002ba4dc;
		cmb_text_deselect(self->items[DAT_002ba4d8]);
		DAT_002ba4d8++;
		if (DAT_002ba4d8 == DAT_002ba4dc)
			DAT_002ba4d8 = 0;
		CGameMode_MiniGameSelect_ShowGame(self, DAT_002ba4d8, 1);
		return 0;
	case 4:
	case 5: /* play */
		CMB_PLAY_SAMPLE(9, 0x7f, 0, 0, 0);
		DAT_002bae98 = 1;
		DAT_002bae9c = 2;
		for (player = 0; player < DAT_002bae94; player++) {
			if (DAT_002baed4[player][0] == -1)
				DAT_002baed4[player][0] = player;
		}
		DAT_002838f4 = DAT_002ba238[DAT_002ba4d8].game;
		DAT_002838f8 = DAT_002ba238[DAT_002ba4d8].variant;
		DAT_0028198c = 1;
		DAT_002ba4cc = 0;
		CMB_VFN(self, CMB_VT_SUSPEND, void (*)(void *))(self);
		cmb_start_selected();
		return 0;
	case 6: /* back */
		CMB_PLAY_SAMPLE(10, 0x7f, 0, 0, 0);
		self->leave = 1;
		break;
	}
	return 0;
}

/* 0x00024fe0  Core/CGameMode_MiniGameSelect.cpp */
void *CGameMode_MiniGameSelect_DeletingDtor(struct CGameMode_MiniGameSelect *self, unsigned char flags)
{
	CGameMode_MiniGameSelect_dtor(self);
	if ((flags & 1) != 0)
		CMB_DELETE(self);
	return self;
}

/* 0x00025000  Core/CGameMode_MiniGameSelect.cpp: Start; also back from a game */
void CGameMode_MiniGameSelect_Start(struct CGameMode_MiniGameSelect *self)
{
	int winner;
	int points[4];
	void *memory;
	void *mode;

	CMB_AT(self, 0x18, int) = 0;
	DAT_00292c50 = 0x18;
	DAT_00292c4c = 0x18;
	DAT_002ba4e4 = 1;
	DAT_00284408 = 1;
	DAT_00292c78 = 0x240;
	if (DAT_002ba4cc != 0) {
		/* back from a game: add its points */
		((void (*)(int *, int *, int))FUN_00028ab0)(&winner, points, 1);
		DAT_002ba4cc = 0;
		self->totals[0] += points[0];
		self->totals[1] += points[1];
		self->totals[2] += points[2];
		self->totals[3] += points[3];
	}
	DAT_002baebc = 0;
	if (DAT_0028198c == -1) {
		if (DAT_002bae9c == 0) {
			DAT_002ba4d8 = 0;
			DAT_002ba4d4 = DAT_002ba4dc - 4 < 0 ? 0 : DAT_002ba4dc - 4;
		} else if (DAT_002bae9c == 2) {
			/* the tournament's end: the scores screen */
			memory = CMB_NEW(0xed0, cmb_file_select, 0x124);
			mode = memory == NULL ? NULL : ((void *(*)(void *, int))FUN_000284e0)(memory, 0);
			CMB_PUSH_MODE(mode, 1);
			return;
		}
		DAT_002bae9c = 1;
		((void (*)(int, int, int, int, int, int))FUN_000724b0)(4, 0, 0xff, -20, 0x14, 0);
		((void (*)(void *))FUN_00072720)(self->list);
		((void (*)(void *))FUN_000796c0)((void *)DAT_002ecfac);
		((void (*)(void *, int *))FUN_0007a200)((void *)DAT_002ecfac, self->totals);
		((void (*)(void *))FUN_0007be80)((void *)DAT_002ecfac);
		self->base.state = 0;
		CMB_VFN(self, 0x2c, void (*)(void *))(self);
		if (DAT_002b95d0 != 0)
			CMB_U8(DAT_002b95d0, 0x58) = 1;
		((void (*)(void *, int, const char *))FUN_00030210)(self,
			CMB_RESOURCE4(0, "over_minisel.obe", 1, 0xdefa), NULL);
	} else if (DAT_002baea8 == 0) {
		/* the intro asked for the rules again */
		memory = CMB_NEW(0xc0, cmb_file_select, 0x11d);
		mode = memory == NULL ? NULL : ((void *(*)(void *))FUN_00012b50)(memory);
		CMB_PUSH_MODE(mode, 1);
	} else {
		/* play the chosen game */
		CMB_VFN(self, CMB_VT_SUSPEND, void (*)(void *))(self);
		cmb_start_selected();
	}
	((void (*)(int))FUN_0004eed0)(1);
}

/* 0x00025290  Core/CGameMode_MiniGameSelect.cpp: the screen */
void CGameMode_MiniGameSelect_Loaded(struct CGameMode_MiniGameSelect *self)
{
	unsigned int desc[CMB_OD_SIZE / 4];
	char name[64];
	void *memory;
	int texture;
	int half;
	int i;

	memset(desc, 0, sizeof desc);
	DAT_002ba4c4 = CMB_RESOURCE4(0, "handgotb.ttf", 3, 0xdefa);
	if (DAT_002ba4c4 == 0 || (DAT_002ba230 = CMB_RESOURCE4(0, "handgotl.ttf", 3, 0xdefa)) == 0) {
		CMB_DEBUG_PRINTF("CGameMode_CharacterSelect::GainFocus *** Could not load font 'boombox.bmp' ***\n");
		return;
	}
	self->up_texture = CMB_RESOURCE4(0, "up_arrow.bmp", 0, 0xdefa);
	self->down_texture = CMB_RESOURCE4(0, "down_arrow.bmp", 0, 0xdefa);

	/* the headings */
	memset(desc, 0, sizeof desc);
	CMB_I32(desc, 0x44) = DAT_002ba4c4;
	CMB_I32(desc, 0x288) = 2;
	CMB_F32(desc, 0x250) = (float)(DAT_0028452c[0] / 2);
	CMB_F32(desc, 0x2a4) = 150.0f;
	CMB_F32(desc, 0x2a8) = 175.0f;
	CMB_AT(desc, 0x24c, unsigned short) = 2;
	CMB_F32(desc, 0x254) = (float)(DAT_0028452c[1] / 2);
	CMB_F32(desc, 0x258) = (float)(DAT_0028452c[2] / 2);
	CMB_F32(desc, 0x25c) = (float)(DAT_0028452c[3] / 2);
	CMB_AT(desc, 0x248, const unsigned short *) = CMB_TEXT(0x108);
	cmb_overlay_create(desc);
	CMB_I32(desc, 0x288) = 2;
	CMB_I32(desc, 0x44) = DAT_002ba230;
	CMB_F32(desc, 0x2a4) = 0.0f;
	CMB_F32(desc, 0x2a8) = -100.0f;
	CMB_F32(desc, 0x2ac) = 0.0f;
	CMB_F32(desc, 0x250) = 127.0f;
	CMB_F32(desc, 0x254) = 127.0f;
	CMB_F32(desc, 0x258) = 127.0f;
	CMB_F32(desc, 0x25c) = 127.0f;
	CMB_AT(desc, 0x248, const unsigned short *) = CMB_TEXT(0x109);
	self->heading_left = cmb_overlay_create(desc);
	CMB_I32(desc, 0x288) = 2;
	CMB_I32(desc, 0x44) = DAT_002ba230;
	CMB_F32(desc, 0x2a4) = 220.0f;
	CMB_F32(desc, 0x2a8) = -100.0f;
	CMB_F32(desc, 0x2ac) = 0.0f;
	CMB_F32(desc, 0x250) = 127.0f;
	CMB_F32(desc, 0x254) = 127.0f;
	CMB_F32(desc, 0x258) = 127.0f;
	CMB_F32(desc, 0x25c) = 127.0f;
	CMB_AT(desc, 0x248, const unsigned short *) = CMB_TEXT(0x10a);
	self->heading_right = cmb_overlay_create(desc);

	/* one text per entry */
	for (i = 0; i < DAT_002ba4dc; i++) {
		memory = CMB_NEW(0x90, cmb_file_select, 399);
		self->items[i] = memory == NULL ? NULL
			: CMB_TEXTBOX(memory, -180, 0, CMB_TEXT(DAT_002ba238[i].text), DAT_002ba4c4);
		cmb_text_deselect(self->items[i]);
		CMB_F32(self->items[i], 0x34) = 0.75f;
		if (i == DAT_002ba4d8)
			cmb_text_select(self->items[i], 0.0f);
	}

	/* the arrows, plain and glowing */
	memset(desc, 0, sizeof desc);
	half = DAT_002844dc[0] / 2;
	CMB_F32(desc, 0x250) = (float)half;
	CMB_I32(desc, 0x04) = self->up_texture;
	CMB_F32(desc, 0x254) = (float)half;
	CMB_I32(desc, 0x288) = 0;
	CMB_I32(desc, 0x268) = 0;
	CMB_I32(desc, 0x264) = 0x7f;
	CMB_F32(desc, 0x258) = (float)half;
	CMB_I32(desc, 0x28c) = 1;
	CMB_F32(desc, 0x25c) = 0.0f;
	CMB_F32(desc, 0x2a4) = -260.0f;
	CMB_F32(desc, 0x2a8) = 110.0f - _DAT_002ba4c8;
	texture = CMB_I32(desc, 0x04);
	CMB_F32(desc, 0x308) = (float)CMB_AT(texture, 0x2e, unsigned short);
	CMB_F32(desc, 0x30c) = (float)CMB_AT(texture, 0x30, unsigned short);
	self->up_arrow = cmb_overlay_create(desc);
	half = DAT_002844dc[0] / 2;
	CMB_F32(desc, 0x250) = (float)half;
	CMB_I32(desc, 0x04) = self->down_texture;
	CMB_F32(desc, 0x254) = (float)half;
	CMB_F32(desc, 0x25c) = 0.0f;
	CMB_F32(desc, 0x258) = (float)half;
	CMB_F32(desc, 0x2a8) = -70.0f - _DAT_002ba4c8;
	texture = CMB_I32(desc, 0x04);
	CMB_F32(desc, 0x308) = (float)CMB_AT(texture, 0x2e, unsigned short);
	CMB_F32(desc, 0x30c) = (float)CMB_AT(texture, 0x30, unsigned short);
	self->down_arrow = cmb_overlay_create(desc);
	CMB_F32(desc, 0x2a8) = 110.0f - _DAT_002ba4c8;
	CMB_I32(desc, 0x04) = self->up_texture;
	CMB_F32(desc, 0x250) = 127.0f;
	CMB_F32(desc, 0x254) = 127.0f;
	CMB_F32(desc, 0x258) = 127.0f;
	CMB_F32(desc, 0x25c) = 127.0f;
	texture = CMB_I32(desc, 0x04);
	CMB_F32(desc, 0x308) = (float)CMB_AT(texture, 0x2e, unsigned short);
	CMB_F32(desc, 0x30c) = (float)CMB_AT(texture, 0x30, unsigned short);
	self->up_glow = cmb_overlay_create(desc);
	CMB_F32(desc, 0x2a8) = -70.0f - _DAT_002ba4c8;
	CMB_I32(desc, 0x04) = self->down_texture;
	CMB_F32(desc, 0x250) = 127.0f;
	CMB_F32(desc, 0x254) = 127.0f;
	CMB_F32(desc, 0x258) = 127.0f;
	CMB_F32(desc, 0x25c) = 127.0f;
	texture = CMB_I32(desc, 0x04);
	CMB_F32(desc, 0x308) = (float)CMB_AT(texture, 0x2e, unsigned short);
	CMB_F32(desc, 0x30c) = (float)CMB_AT(texture, 0x30, unsigned short);
	self->down_glow = cmb_overlay_create(desc);

	/* the info panel */
	memory = CMB_NEW(0x70, cmb_file_select, 0x1c2);
	self->info = memory == NULL ? NULL : ((void *(*)(void *))FUN_0002e0d0)(memory);
	sprintf(name, "%s%d.bmp", cmb_game_prefix[DAT_002ba238[DAT_002ba4d8].game],
		DAT_002ba238[DAT_002ba4d8].variant);
	((void (*)(void *, int, int, int))FUN_0002da70)(self->info, DAT_002ba238[DAT_002ba4d8].game,
		DAT_002ba238[DAT_002ba4d8].variant, CMB_RESOURCE4(0, name, 0, 0xdefa));
	CMB_VFN(self, CMB_VT_UPDATE, void (*)(void *, int))(self, 1);
}
