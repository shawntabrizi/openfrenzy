/*
BumperCars/CModule_BumperCars.c  <-  C:\BlitzParty\Game\Src\BumperCars\CModule_BumperCars.cpp
(module bumpercars_a)

The BumperCars entry of core's game list (a CGame, vtable 0x0026352c): its four variations
are "Hockey", "Pinball", "Knockout" and "Bumper Race". Start pushes the game mode and the
minigame intro screen. The constructor and destructor are gap functions before the tagged
ones (file inferred from the vtable).
*/

#include "bumpercars_a.h"

static const char s_ModuleFile[] = "C:\\BlitzParty\\Game\\Src\\BumperCars\\CModule_BumperCars.cpp";

/* core: the mode stack (0x002ba750) */
extern unsigned char DAT_002ba750[];

/* 0x0026352c  CModule_BumperCars vtable (the CGame virtuals: core_globals.h CGameVtbl) */
const void *const vtable_0026352c[4] = {
	(const void *)FUN_00082930, /* [0] scalar deleting destructor */
	(const void *)FUN_00082910, /* [1] init */
	(const void *)FUN_00082920, /* [2] shutdown (empty) */
	(const void *)FUN_00082950, /* [3] start (variation) */
};

/* 0x000828d0  (gap): CModule_BumperCars::CModule_BumperCars */
unsigned int *CModule_BumperCars_Ctor(unsigned int *self)
{
	CGame_Ctor6(self, "BumperCars", (unsigned int)(uintptr_t)"Hockey", (unsigned int)(uintptr_t)"Pinball",
		(unsigned int)(uintptr_t)"Knockout", (unsigned int)(uintptr_t)"Bumper Race");
	self[0] = (unsigned int)(uintptr_t)vtable_0026352c;
	return self;
}

/* 0x00082900  (gap): CModule_BumperCars::~CModule_BumperCars */
void CModule_BumperCars_Dtor(unsigned int *self)
{
	self[0] = (unsigned int)(uintptr_t)vtable_0026352c;
	CGame_Dtor(self);
}

/* 0x00082910  BumperCars/CModule_BumperCars.cpp: init (the game's one-time init) */
int CModule_BumperCars_Init(unsigned int *self)
{
	(void)self;
	return FUN_00086c80(&DAT_002ed458);
}

/* 0x00082920  BumperCars/CModule_BumperCars.cpp: shutdown (the game's empty shutdown) */
void CModule_BumperCars_Shutdown(unsigned int *self)
{
	(void)self;
	FUN_001d5950();
}

/* 0x00082930  BumperCars/CModule_BumperCars.cpp: scalar deleting destructor */
unsigned int *CModule_BumperCars_Destroy(unsigned int *self, byte flags)
{
	CModule_BumperCars_Dtor(self);
	if ((flags & 1) != 0)
		BCA_DELETE(self);
	return self;
}

/* 0x00082950  BumperCars/CModule_BumperCars.cpp: start: pushes the game mode, then the
minigame intro ("bump<variation>.bmp") on top of it. Returns the game mode push result. */
int CModule_BumperCars_Start(unsigned int *self, int variation)
{
	char picture[64];
	void *intro;
	int result;

	(void)self;
	result = CBCar_Game_PushMode(&DAT_002ed458, variation);
	snprintf(picture, 0x3f, "bump%d.bmp", variation);
	intro = BCA_NEW(0x300, s_ModuleFile, 0x5c, 0x1001);
	if (intro != NULL)
		intro = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)intro, variation + 0x2b, picture);
	CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)intro, 1);
	return result;
}
