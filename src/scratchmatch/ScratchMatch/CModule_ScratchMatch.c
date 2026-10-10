/*
ScratchMatch/CModule_ScratchMatch.c  <-  C:\BlitzParty\Game\Src\ScratchMatch\CModule_ScratchMatch.cpp
(module scratchmatch)

The Scratch Match entry of core's game list (a CGame, vtable 0x00276764): name "ScratchMatch",
variations "Rhythm", "Pattern", "Scratch" and "Hardcore". Start stores the variation, pushes
the game mode (CGameMode_ScratchMatch) and then the minigame intro screen.
*/

#include "scratchmatch.h"

static const char s_ModuleFile[] = "C:\\BlitzParty\\Game\\Src\\ScratchMatch\\CModule_ScratchMatch.cpp";

/* 0x00276764  CModule_ScratchMatch vtable (the CGame virtuals) */
const void *const vtable_00276764[4] = {
	(const void *)FUN_001ba5a0, /* [0] scalar deleting destructor */
	(const void *)FUN_000a6fa0, /* [1] init (returns 1) */
	(const void *)FUN_001d5950, /* [2] shutdown (empty) */
	(const void *)FUN_001ba5c0, /* [3] start (variation) */
};

/* 0x001ba560  (gap; the module's constructor): CModule_ScratchMatch::CModule_ScratchMatch */
unsigned int *CModule_ScratchMatch_Ctor(unsigned int *self)
{
	CGame_Ctor6(self, "ScratchMatch", (unsigned int)(uintptr_t)"Rhythm", (unsigned int)(uintptr_t)"Pattern",
		(unsigned int)(uintptr_t)"Scratch", (unsigned int)(uintptr_t)"Hardcore");
	self[0] = (unsigned int)(uintptr_t)vtable_00276764;
	return self;
}

/* 0x001ba590  (gap; the module's destructor): CModule_ScratchMatch::~CModule_ScratchMatch */
void CModule_ScratchMatch_Dtor(unsigned int *self)
{
	self[0] = (unsigned int)(uintptr_t)vtable_00276764;
	CGame_Dtor(self);
}

/* 0x001ba5a0  ScratchMatch/CModule_ScratchMatch.cpp: scalar deleting destructor */
unsigned int *CModule_ScratchMatch_Destroy(unsigned int *self, unsigned char flags)
{
	CModule_ScratchMatch_Dtor(self);
	if ((flags & 1) != 0)
		SM_DELETE(self);
	return self;
}

/* 0x001ba5c0  ScratchMatch/CModule_ScratchMatch.cpp: start (thiscall, `self` unused): stores
the variation, pushes the game mode, then the minigame intro ("scratch<variation>.bmp").
Returns 0 when the game mode could not be pushed, else 1 (the intro's push is not checked). */
int CModule_ScratchMatch_Start(unsigned int *self, int variation)
{
	char picture[64];
	void *mode;

	(void)self;
	sm_Variation = variation;
	mode = bkHeapAlloc(0x2c, s_ModuleFile, 0x5f, 0x1001);
	if (mode != NULL)
		mode = CGameMode_ScratchMatch_Ctor(mode);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1) == 0)
		return 0;
	_snprintf(picture, 0x3f, "scratch%d.bmp", variation);
	mode = bkHeapAlloc(0x300, s_ModuleFile, 99, 0x1001);
	if (mode != NULL)
		mode = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)mode, variation + 0x53, picture);
	CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1);
	DAT_00290960 = 0;
	return 1;
}
