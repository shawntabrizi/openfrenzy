/*
Core/CGameMode_DemoSplash.c  <-  C:\BlitzParty\Game\Src\Core\CGameMode_DemoSplash.cpp

The demo disc's closing splash: a picture with a typed message; after five
seconds (or a button) it fades and launches the demo launcher (XDemos.xbe or
the title that started the demo). The constructor (0x00018350) is unplaced in
the table; it is this class's.
*/

#include "core_modes_a.h"
#include "core_modes_a_modes.h"
#include "core_modes_a_globals.h"

/* CGameMode_DemoSplash (vtable 0x00254ed0) */
struct CGameMode_DemoSplash {
	struct CGameModeMenu menu;      /* +0x00 */
	int package;                    /* +0x98 Packages\demosplash */
	int picture;                    /* +0x9c demosplash.bmp */
	int unka0;                      /* +0xa0 */
	int unka4;                      /* +0xa4 */
	float fade;                     /* +0xa8 0 clear .. 265 black */
	float timer;                    /* +0xac */
	int leaving;                    /* +0xb0 */
	struct CMA_List list;           /* +0xb4 */
	struct CBGTypeText *text;       /* +0xc0 */
};
OF_ASSERT_OFFSET(struct CGameMode_DemoSplash, text, 0xc0);

/* platform (XDK) */
unsigned long XLaunchNewImageA(const char *path, void *launchData);
long D3DDevice_PersistDisplay(void);

extern char DAT_002ba9d8[];         /* core: the title's own path */
extern int DAT_002d0750;            /* bkernel: launch data type (0: launched by a title) */
extern unsigned char DAT_002d08a8[]; /* bkernel: LAUNCH_DATA; +0xc the launching title's path */

/* bkernel FUN_0004db30: load a package now (type, name, flags) */
#define CMA_LoadPackage(type, name, flags) ((int (*)(int, const char *, int))FUN_0004db30)((type), (name), (flags))

static const char s_DemoSplash_cpp[] = "C:\\BlitzParty\\Game\\Src\\Core\\CGameMode_DemoSplash.cpp";

/* 0x00254ed0 CGameMode_DemoSplash */
const void *const vtable_00254ed0[] = {
	(const void *)CGameMode_DemoSplash_Delete,
	(const void *)CGameMode_DemoSplash_Suspend,
	(const void *)CGameMode_DemoSplash_Resume,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)FUN_0002fe60,
	(const void *)CGameMode_DemoSplash_Update,
	(const void *)CGameMode_DemoSplash_Render,
	(const void *)FUN_001ec260,
	(const void *)CGameMode_UpdateLoading,
	(const void *)CGameMode_FinishedLoading,
	(const void *)FUN_001d5950,
	(const void *)FUN_001d5950,
	(const void *)CGameMode_DemoSplash_HandleButton,
};

/* 0x00018350  Core (unplaced; CGameMode_DemoSplash) */
struct CGameMode_DemoSplash *CGameMode_DemoSplash_ctor(struct CGameMode_DemoSplash *self)
{
	FUN_00030090((undefined4 *)self, (undefined4 *)"DemoSplash");
	self->menu.base.vtbl = vtable_00254ed0;
	CMA_List_Init(&self->list);
	return self;
}

/* 0x000183b0  Core/CGameMode_DemoSplash.cpp: suspend */
void CGameMode_DemoSplash_Suspend(struct CGameMode_DemoSplash *self)
{
	if (self->text != 0)
		CMA_VFN(self->text, 0, void *(*)(void *, unsigned int))(self->text, 1);
	FUN_0004db40((int *)self->package);
}

/* 0x000183e0  Core/CGameMode_DemoSplash.cpp: fade in, wait 300 frames; when leaving, fade out
   and launch the demo launcher */
void CGameMode_DemoSplash_Update(struct CGameMode_DemoSplash *self, int active)
{
	double t;

	if (active == 0)
		return;
	CMA_VFN(self->text, 0x08, int (*)(void *))(self->text);
	if (self->leaving != 0) {
		t = (double)DAT_0028fda8 * 12.0 + (double)self->fade;
		self->fade = (float)t;
		if (t > 265.0) {
			int package = CMA_LoadPackage(0, "Packages\\XDemo", 0);
			int picture;
			int i;
			unsigned long result;

			FUN_0004b870((struct bkPackage *)package, 0xffffffff, 0xdefa, 0);
			picture = (int)FUN_0004bb30(0, "wait.bmp", 5, 0xdefa);
			for (i = 0; i < 3; i++) {
				FUN_00046dd0((struct bdTexture *)picture, -1, -1);
				FUN_00041ee0(0, 0, 0, 0xff);
			}
			FUN_00046dd0((struct bdTexture *)picture, -1, -1);
			CMA_DebugPrintf("\n\n%s\n\n", (char *)DAT_002d08a8 + 0xc);
			strcpy(DAT_002d0758, DAT_002ba9d8);
			FUN_0001c800();
			D3DDevice_PersistDisplay();
			FUN_0004ed60(1);
			result = XLaunchNewImageA(DAT_002d0750 == 0 ? (char *)DAT_002d08a8 + 0xc
				: "d:\\XDemos\\XDemos.xbe", DAT_002d08a8);
			CMA_DebugPrintf("\n\nXLaunchNewImage returned %d\n\n", result);
		}
		return;
	}
	t = (double)self->fade - (double)DAT_0028fda8 * 5.0;
	self->fade = (float)t;
	if (t < 0.0)
		self->fade = 0.0f;
	t = (double)DAT_0028fda8 + (double)self->timer;
	self->timer = (float)t;
	if (t > 300.0)
		self->leaving = 1;
}

/* 0x00018560  Core/CGameMode_DemoSplash.cpp: the picture, the text, the fade */
void CGameMode_DemoSplash_Render(struct CGameMode_DemoSplash *self, int active)
{
	if (active == 0)
		return;
	if (self->picture != 0) {
		FUN_00044b00(2, 0xc, 0);
		FUN_00044ac0(2);
		FUN_00046dd0((struct bdTexture *)self->picture, -1, -1);
		FUN_00044ae0(2);
	}
	CMA_VFN(self->text, 0x0c, void (*)(void *))(self->text);
	if (self->fade != 0.0f)
		FUN_00071c20((int)self->fade, 1);
}

/* 0x000185e0  Core/CGameMode_DemoSplash.cpp: A or start leaves after the 300 frames */
int CGameMode_DemoSplash_HandleButton(struct CGameMode_DemoSplash *self, int button, int player)
{
	(void)player;
	if (button > 3 && button < 6 && self->leaving == 0 && self->timer >= 300.0f) {
		CMA_PlayGeneric(9, 0x7f, 0, 0, 0);
		self->leaving = 1;
	}
	return 0;
}

/* 0x00018630  Core/CGameMode_DemoSplash.cpp */
struct CGameMode_DemoSplash *CGameMode_DemoSplash_Delete(struct CGameMode_DemoSplash *self,
	unsigned int flags)
{
	CGameMode_DemoSplash_dtor(self);
	if (flags & 1)
		CMA_Delete(self);
	return self;
}

/* 0x00018650  Core/CGameMode_DemoSplash.cpp */
void CGameMode_DemoSplash_dtor(struct CGameMode_DemoSplash *self)
{
	FUN_001d5950(); /* ~CMA_List (nothing) */
	FUN_00030190((undefined4 *)self);
}

/* 0x000186a0  Core/CGameMode_DemoSplash.cpp: resume: the package, the picture, the message */
void CGameMode_DemoSplash_Resume(struct CGameMode_DemoSplash *self)
{
	void *memory;

	self->timer = 0.0f;
	self->fade = 265.0f;
	self->package = CMA_LoadPackage(0, "Packages\\demosplash", 0);
	FUN_0004b870((struct bkPackage *)self->package, 0xffffffff, 0xdefa, 0);
	self->picture = (int)FUN_0004bb30(0, "demosplash.bmp", 5, 0xdefa);
	self->leaving = 0;
	memory = CMA_New(0x90, s_DemoSplash_cpp, 0x2c, 0x1001);
	self->text = memory == 0 ? 0
		: CBGTypeText_ctor(memory, 0, 100, CMA_TEXT(0x1b9), (int)FUN_0004bb30(0, "handgotl.ttf", 3, 0xdefa));
	CBGTypeText_Select(self->text, 0.0f);
	self->text->style |= 0xc;
	self->text->style &= ~1u;
	self->text->boxWidth = 240.0f;
	CMA_VFN(self->text, 0x1c, void (*)(void *, float, float, int))(self->text, 1.0f, 60.0f, 0);
	self->text->beepVolume = 200;
	self->text->wrap = 1;
}
