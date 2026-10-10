/*
Core/CGameMode_Credits.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_Credits.cpp

The credits: two columns that type the credit groups (a title from the text
table, then the names in pages of up to five lines). The mode's methods
before CGameMode_Credits.cpp's tagged function (0x00017c70-0x00017e00) and
after it (0x000182e0) are in the table as unplaced; they are this class's
(vtable 0x00254e58).
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

/* a credits column (0xf4 bytes) */
struct CMA_CreditsColumn {
	int x;                          /* +0x00 */
	int y;                          /* +0x04 */
	int group;                      /* +0x08 credit group shown (0x21: done) */
	struct CBGTypeText *title;      /* +0x0c */
	int lineCount;                  /* +0x10 names in the group */
	int perPage;                    /* +0x14 */
	int first;                      /* +0x18 first name of the page */
	int nextPage;                   /* +0x1c */
	int yOffset;                    /* +0x20 */
	struct CBGTypeText *lines[50];  /* +0x24 */
	float timer;                    /* +0xec */
	unsigned int style;             /* +0xf0 */
};
OF_ASSERT_SIZE(struct CMA_CreditsColumn, 0xf4);

/* CGameMode_Credits (vtable 0x00254e58) */
struct CGameMode_Credits {
	struct CGameModeMenu menu;              /* +0x000 */
	int sharedGroup;                        /* +0x098 a group with "**" spans both columns */
	int nextGroup;                          /* +0x09c */
	struct CMA_CreditsColumn columns[2];    /* +0x0a0 */
	int font1;                              /* +0x288 handgotb.ttf */
	int font2;                              /* +0x28c handgotl.ttf */
};
OF_ASSERT_OFFSET(struct CGameMode_Credits, font1, 0x288);

/* 0x002b9714: the credits mode */
struct CGameMode_Credits *DAT_002b9714;

/* 0x00281c40: the names of the 33 credit groups, 50 lines each; "*" ends a group ("**": the
   group goes on in the other column, "***": wait for it) */
static const wchar16 *const PTR_u_Blitz_Games_00281c40[33 * 50] = {
	/* 0 */
	[0 * 50 + 0] = u"Blitz Games",
	[0 * 50 + 1] = u"*",
	/* 1 */
	[1 * 50 + 0] = u"Darren \"Skip\" Wood",
	[1 * 50 + 1] = u"*",
	/* 2 */
	[2 * 50 + 0] = u"Nick \"Nads\" Adams",
	[2 * 50 + 1] = u"Martyn Ash",
	[2 * 50 + 2] = u"Rich \"Bob\" Jones",
	[2 * 50 + 3] = u"David \"Dibs\" Manuel",
	[2 * 50 + 4] = u"Mark Neesam",
	[2 * 50 + 5] = u"Chris Swan",
	[2 * 50 + 6] = u"*",
	/* 3 */
	[3 * 50 + 0] = u"Steve \"Shads\" Bond",
	[3 * 50 + 1] = u"Tom Drummond",
	[3 * 50 + 2] = u"Chris \"Crispy\" Fry",
	[3 * 50 + 3] = u"Rob Grant",
	[3 * 50 + 4] = u"Dave \"Buffalo\" Harries",
	[3 * 50 + 5] = u"James Healey",
	[3 * 50 + 6] = u"Michael Higgs",
	[3 * 50 + 7] = u"Toby \"Rat Boy\" Jones",
	[3 * 50 + 8] = u"Craig Rushforth",
	[3 * 50 + 9] = u"David Swift",
	[3 * 50 + 10] = u"Matthew \"Chimpy\" Hayward",
	[3 * 50 + 11] = u"R Fred Williams",
	[3 * 50 + 12] = u"Barry Paterson",
	[3 * 50 + 13] = u"Neil Campbell",
	[3 * 50 + 14] = u"Phil Palmer",
	[3 * 50 + 15] = u"*",
	/* 4 */
	[4 * 50 + 0] = u"John \"Chief\" Whigham",
	[4 * 50 + 1] = u"Andy Slater",
	[4 * 50 + 2] = u"Lyndon Homewood",
	[4 * 50 + 3] = u"Tom Whittock",
	[4 * 50 + 4] = u"Andy Sidwell",
	[4 * 50 + 5] = u"*",
	/* 5 */
	[5 * 50 + 0] = u"Rehaan Akhtar",
	[5 * 50 + 1] = u"Mark \"Winx\" Capewell",
	[5 * 50 + 2] = u"Johnathan Evans",
	[5 * 50 + 3] = u"Lauren Grindrod",
	[5 * 50 + 4] = u"Gordon Hargreaves",
	[5 * 50 + 5] = u"Simon \"Purple\" Hayes",
	[5 * 50 + 6] = u"Paul Jennings",
	[5 * 50 + 7] = u"Rich \"Bob\" Jones",
	[5 * 50 + 8] = u"Simon Little",
	[5 * 50 + 9] = u"Nadine Mathias",
	[5 * 50 + 10] = u"Matthew \"General\" Norledge",
	[5 * 50 + 11] = u"Mike Procter",
	[5 * 50 + 12] = u"Jon Walsh",
	[5 * 50 + 13] = u"*",
	/* 6 */
	[6 * 50 + 0] = u"Tony Cartwright",
	[6 * 50 + 1] = u"Simon \"Purple\" Hayes",
	[6 * 50 + 2] = u"Duncan Nimmo",
	[6 * 50 + 3] = u"Ian Pestridge",
	[6 * 50 + 4] = u"Mike Rooker",
	[6 * 50 + 5] = u"Richard Whale",
	[6 * 50 + 6] = u"*",
	/* 7 */
	[7 * 50 + 0] = u"Tony Cartwright",
	[7 * 50 + 1] = u"Sandro DaCruz",
	[7 * 50 + 2] = u"Danny Murdock",
	[7 * 50 + 3] = u"Mark \"Winx\" Capewell",
	[7 * 50 + 4] = u"Simon Little",
	[7 * 50 + 5] = u"*",
	/* 8 */
	[8 * 50 + 0] = u"Rob Lord",
	[8 * 50 + 1] = u"*",
	/* 9 */
	[9 * 50 + 0] = u"Andy Morris",
	[9 * 50 + 1] = u"*",
	/* 10 */
	[10 * 50 + 0] = u"Rachel Rodgers",
	[10 * 50 + 1] = u"Posy Miller",
	[10 * 50 + 2] = u"Damian Gaskin",
	[10 * 50 + 3] = u"Danny Wilson",
	[10 * 50 + 4] = u"Jay Benedict",
	[10 * 50 + 5] = u"Everal Walsh",
	[10 * 50 + 6] = u"*",
	/* 11 */
	[11 * 50 + 0] = u"Philip Oliver",
	[11 * 50 + 1] = u"Andrew Oliver",
	[11 * 50 + 2] = u"Nigel Davies",
	[11 * 50 + 3] = u"Susie Thorburn",
	[11 * 50 + 4] = u"Mark Buckingham",
	[11 * 50 + 5] = u"James Corrigan",
	[11 * 50 + 6] = u"Mary Daly",
	[11 * 50 + 7] = u"Caroline Hart",
	[11 * 50 + 8] = u"Oliver Hiatt",
	[11 * 50 + 9] = u"Natalie Griffith",
	[11 * 50 + 10] = u"Laynie Osborn",
	[11 * 50 + 11] = u"Alison Parker",
	[11 * 50 + 12] = u"Jackie Pinnock",
	[11 * 50 + 13] = u"Norma Randall",
	[11 * 50 + 14] = u"Jonathan Scarcliffe",
	[11 * 50 + 15] = u"Simon Smith",
	[11 * 50 + 16] = u"Alex Still",
	[11 * 50 + 17] = u"Carla Stringer",
	[11 * 50 + 18] = u"Lucy Tarrant",
	[11 * 50 + 19] = u"Caroline Thornicroft",
	[11 * 50 + 20] = u"Jacqui Lyons",
	[11 * 50 + 21] = u"Guy Herbert",
	[11 * 50 + 22] = u"**",
	/* 12 */
	[12 * 50 + 0] = u"Gordon Hee",
	[12 * 50 + 1] = u"***",
	/* 13 */
	[13 * 50 + 0] = u"Robert Lamb",
	[13 * 50 + 1] = u"*",
	/* 14 */
	[14 * 50 + 0] = u"Barbara Hagen",
	[14 * 50 + 1] = u"*",
	/* 15 */
	[15 * 50 + 0] = u"Phil Platt",
	[15 * 50 + 1] = u"*",
	/* 16 */
	[16 * 50 + 0] = u"J.D. Alley",
	[16 * 50 + 1] = u"Bruce Sharp",
	[16 * 50 + 2] = u"*",
	/* 17 */
	[17 * 50 + 0] = u"Matt Coggan",
	[17 * 50 + 1] = u"Roy Doron",
	[17 * 50 + 2] = u"Collin Moore",
	[17 * 50 + 3] = u"Jeff Stephens",
	[17 * 50 + 4] = u"Harris Thurmond",
	[17 * 50 + 5] = u"Katie \"Sweets\" Stone",
	[17 * 50 + 6] = u"David Eim",
	[17 * 50 + 7] = u"*",
	/* 18 */
	[18 * 50 + 0] = u"Jacob \"Bataar\" Fulwiler",
	[18 * 50 + 1] = u"Bill Metters",
	[18 * 50 + 2] = u"Matt Edington",
	[18 * 50 + 3] = u"Cory Alexander",
	[18 * 50 + 4] = u"*",
	/* 19 */
	[19 * 50 + 0] = u"Chris Burns",
	[19 * 50 + 1] = u"Jeannie Davison",
	[19 * 50 + 2] = u"Dana Fos",
	[19 * 50 + 3] = u"Eric Haddock",
	[19 * 50 + 4] = u"Marjorie Osterhout",
	[19 * 50 + 5] = u"Pat Rytkonen",
	[19 * 50 + 6] = u"Jason Groce",
	[19 * 50 + 7] = u"*",
	/* 20 */
	[20 * 50 + 0] = u"Ray Kowalewski",
	[20 * 50 + 1] = u"Keith Steury",
	[20 * 50 + 2] = u"*",
	/* 21 */
	[21 * 50 + 0] = u"LouAn Williams",
	[21 * 50 + 1] = u"*",
	/* 22 */
	[22 * 50 + 0] = u"Jama Cantrell",
	[22 * 50 + 1] = u"Sue Stickney",
	[22 * 50 + 2] = u"*",
	/* 23 */
	[23 * 50 + 0] = u"Lief Thompson",
	[23 * 50 + 1] = u"*",
	/* 24 */
	[24 * 50 + 0] = u"Peter Fitzpatrick",
	[24 * 50 + 1] = u"*",
	/* 25 */
	[25 * 50 + 0] = u"Jonathon Young",
	[25 * 50 + 1] = u"*",
	/* 26 */
	[26 * 50 + 0] = u"Suzanne Boylan",
	[26 * 50 + 1] = u"*",
	/* 27 */
	[27 * 50 + 0] = u"Cosmo Greco",
	[27 * 50 + 1] = u"Michael Ivory",
	[27 * 50 + 2] = u"*",
	/* 28 */
	[28 * 50 + 0] = u"David Foster",
	[28 * 50 + 1] = u"*",
	/* 29 */
	[29 * 50 + 0] = u"Enda Kelly",
	[29 * 50 + 1] = u"*",
	/* 30 */
	[30 * 50 + 0] = u"Humberto Castaneda",
	[30 * 50 + 1] = u"Shane Kim",
	[30 * 50 + 2] = u"James Miller",
	[30 * 50 + 3] = u"David Norris",
	[30 * 50 + 4] = u"Alexey Pajitnov",
	[30 * 50 + 5] = u"Bonnie Ross",
	[30 * 50 + 6] = u"Yvette Wagner",
	[30 * 50 + 7] = u"Keith Wintraub",
	[30 * 50 + 8] = u"**",
	/* 31 */
	[31 * 50 + 0] = u" ",
	[31 * 50 + 1] = u" ",
	[31 * 50 + 2] = u" ",
	[31 * 50 + 3] = u" ",
	[31 * 50 + 4] = u" ",
	[31 * 50 + 5] = u"***",
	/* 32 */
	[32 * 50 + 0] = u" ",
	[32 * 50 + 1] = u" ",
	[32 * 50 + 2] = u" ",
	[32 * 50 + 3] = u" ",
	[32 * 50 + 4] = u" ",
	[32 * 50 + 5] = u"***",
};

/* 0x00283608 */
static const float DAT_00283608 = -0.4f;

static const char s_Credits_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_Credits.cpp";

#define CMA_CREDIT(group, line) PTR_u_Blitz_Games_00281c40[(group) * 50 + (line)]

/* 0x00254e58 CGameMode_Credits */
const void *const vtable_00254e58[] = {
	(const void *)FUN_0001d1e0,     /* core_modes_b: deleting destructor of menu modes */
	(const void *)CGameMode_Credits_Suspend,
	(const void *)CGameMode_Credits_Resume,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_0002fe60,
	(const void *)CGameMode_Credits_Update,
	(const void *)CGameMode_Credits_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_Credits_HandleButton,
};

/* 0x00017c70  Core (unplaced; CGameMode_Credits) */
struct CGameMode_Credits *CGameMode_Credits_ctor(struct CGameMode_Credits *self)
{
	FUN_00030090((undefined4 *)self, (undefined4 *)"Credits");
	self->menu.base.vtbl = vtable_00254e58;
	return self;
}

/* 0x00017c90  Core (unplaced; CGameMode_Credits): resume: hide the button prompts, two
   columns, the fonts */
void CGameMode_Credits_Resume(struct CGameMode_Credits *self)
{
	int i;

	self->sharedGroup = 0;
	DAT_00284408 = 0;
	DAT_002b9714 = self;
	for (i = 0; i < 36; i++)
		CMA_FIELD(int, DAT_002baf78[i], 0x20) = 2;
	self->nextGroup = 1;
	for (i = 0; i < 2; i++) {
		struct CMA_CreditsColumn *column = &self->columns[i];

		column->group = i;
		column->title = 0;
		column->first = 0;
		column->nextPage = 0;
		column->timer = (float)(-i) * 60.0f;
		column->perPage = 0;
	}
	self->columns[0].x = 220;
	self->columns[0].y = 80;
	self->columns[0].style = 0x10;
	self->columns[1].x = -220;
	self->columns[1].y = -100;
	self->columns[1].style = 0;
	self->font1 = FUN_0004bb30(0, "handgotb.ttf", 3, 0xdefa);
	self->font2 = FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa);
}

/* 0x00017db0  Core (unplaced; CGameMode_Credits): render (a jump to core's menu render) */
void CGameMode_Credits_Render(struct CGameMode_Credits *self, int active)
{
	FUN_00030310((int)self, active);
}

/* 0x00017dc0  Core (unplaced; CGameMode_Credits): A, start or B leave */
int CGameMode_Credits_HandleButton(struct CGameMode_Credits *self, int button, int player)
{
	(void)self;
	(void)player;
	if (button > 3 && button < 7) {
		CMA_ModeStack_Pop(1);
		CMA_PlayGeneric(10, 0x7f, 0, 0, 0);
		return 1;
	}
	return 0;
}

/* 0x00017e00  Core (unplaced; CGameMode_Credits): suspend: fade every text out */
void CGameMode_Credits_Suspend(struct CGameMode_Credits *self)
{
	int c;

	for (c = 0; c < 2; c++) {
		struct CMA_CreditsColumn *column = &self->columns[c];
		int i;

		if (column->title != 0)
			CMA_VFN(column->title, 0x14, void (*)(void *, float))(column->title, 10.0f);
		for (i = 0; i < column->perPage; i++)
			if (column->lines[i] != 0)
				CMA_VFN(column->lines[i], 0x14, void (*)(void *, float))(column->lines[i], 10.0f);
	}
}

/* 0x00017e60  Core/CGameMode_Credits.cpp: one column: type the next page when its time comes,
   fade it after its time; returns 1 when the column has shown every group */
int Credits_UpdateColumn(struct CMA_CreditsColumn *self)
{
	double t;
	int i;

	if (self->group > 0x20)
		return 1;
	t = (double)DAT_0028fda8 + (double)self->timer;
	self->timer = (float)t;
	if (t > 0.0 && (self->title == 0 || self->nextPage != 0)) {
		int remaining;

		if (self->nextPage == 0) {
			self->lineCount = 0;
			while (*CMA_CREDIT(self->group, self->lineCount) != '*')
				self->lineCount = self->lineCount + 1;
			if (DAT_002b9714->sharedGroup == 0) {
				const wchar16 *end = CMA_CREDIT(self->group, self->lineCount);

				if (CMA_WStrLen(end) > 1 && end[1] == '*')
					DAT_002b9714->sharedGroup = 1;
			} else {
				const wchar16 *end = CMA_CREDIT(self->group, self->lineCount);

				if (CMA_WStrLen(end) > 2 && end[2] == '*') {
					self->timer = self->timer - DAT_0028fda8;
					return 0;
				}
			}
		}
		remaining = self->lineCount - self->first;
		self->perPage = remaining < 5 ? remaining : 5;
		if (self->perPage != 0) {
			int pages = remaining / self->perPage;

			if (remaining % self->perPage != 0)
				pages = pages + 1;
			self->perPage = remaining / pages;
		}
		if (self->nextPage == 0)
			self->yOffset = (self->perPage + 5) * 10;
		self->nextPage = 0;
		if (self->title == 0) {
			void *memory = CMA_New(0x90, s_Credits_cpp, 0x117, 0x1001);

			self->title = memory == 0 ? 0
				: CBGTypeText_ctor(memory, self->x, self->yOffset + self->y,
					CMA_TEXT(self->group + 0x150), DAT_002b9714->font1);
			self->title->style |= self->style;
			CBGTypeText_Select(self->title, 0.0f);
			if (self->group > 0x1e)
				self->title->style &= ~1u;
			CMA_VFN(self->title, 0x1c, void (*)(void *, float, float, int))(self->title, 2.0f, 0.0f, 0);
			CBGEffect_AddToBackground(&self->title->buf.text.base, 1);
		}
		for (i = 0; i < self->perPage; i++) {
			void *memory = CMA_New(0x90, s_Credits_cpp, 0x124, 0x1001);
			struct CBGTypeText *line;

			line = memory == 0 ? 0
				: CBGTypeText_ctor(memory, self->x, self->yOffset + self->y - i * 20 - 50,
					(unsigned short *)CMA_CREDIT(self->group, self->first + i), DAT_002b9714->font2);
			self->lines[i] = line;
			line->style |= self->style;
			CBGTypeText_Select(line, (float)(i + 1) * DAT_00283608);
			CMA_VFN(line, 0x1c, void (*)(void *, float, float, int))(line, 2.0f, (float)(20 + i * 10), 0);
			CBGEffect_AddToBackground(&line->buf.text.base, 1);
		}
	}

	if ((double)self->timer >= (double)((self->perPage * 5 + 0x1e) * 4)) {
		float hide;

		self->first = self->first + self->perPage;
		if (self->first == self->lineCount) {
			const wchar16 *end = CMA_CREDIT(self->group, self->lineCount);

			if (CMA_WStrLen(end) > 1 && end[1] == '*')
				DAT_002b9714->sharedGroup = 0;
			self->group = DAT_002b9714->nextGroup + 1;
			DAT_002b9714->nextGroup = self->group;
			hide = self->group < 0x21 ? 0.0f : 10.0f;
			self->first = 0;
			self->timer = -30.0f;
			CMA_VFN(self->title, 0x14, void (*)(void *, float))(self->title, hide);
			self->title = 0;
		} else {
			hide = 10.0f;
			self->timer = -6.0f;
			self->nextPage = 1;
		}
		for (i = 0; i < self->perPage; i++) {
			CMA_VFN(self->lines[i], 0x14, void (*)(void *, float))(self->lines[i], hide);
			self->lines[i] = 0;
		}
	}
	return 0;
}

/* 0x000182e0  Core (unplaced; CGameMode_Credits): update both columns; leave when both are done */
void CGameMode_Credits_Update(struct CGameMode_Credits *self, int active)
{
	int done = 0;
	int i;

	if (DAT_002ecdec != 0)
		return;
	CMA_MenuMode_Process(self, active);
	if (active == 0)
		return;
	for (i = 0; i < 2; i++)
		done = done + Credits_UpdateColumn(&self->columns[i]);
	if (done == 2)
		CMA_ModeStack_Pop(1);
}
