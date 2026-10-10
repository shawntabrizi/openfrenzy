/*
Drillers/CModule_Drillers.c  <-  C:\BlitzParty\Game\Src\Drillers\CModule_Drillers.cpp

The Drillers game entry (a CGame, core): its name, the four variant names, and the start
of a variant (pushes the Drillers game mode and the minigame intro).
0x0010f830 and 0x0010f860 (gap) are the constructor and destructor that store this file's
vtable 0x0026bf48.
*/

#include "drillers.h"

/* 0x0026bf48  vtable of CModule_Drillers (rebuilt from the XBE) */
const void *const vtable_0026bf48[4] = {
	(const void *)CModuleDrillers_DeletingDtor, /* +0x00 */
	(const void *)FUN_000a6fa0,                 /* +0x04 Init: returns 1 */
	(const void *)FUN_001d5950,                 /* +0x08 (empty) */
	(const void *)CModuleDrillers_Start,        /* +0x0c */
};

/* 0x0010f830  Drillers (gap; CModule_Drillers.cpp inferred: stores this file's vtable) */
void *CModuleDrillers_Ctor(void *self)
{
	CGame_Ctor6((unsigned int *)self, "Drillers", (unsigned int)"Rubble Alliance",
		(unsigned int)"Erodeo", (unsigned int)"TailBlazer", (unsigned int)"Quantum Snare");
	*(const void *const **)self = vtable_0026bf48;
	return self;
}

/* 0x0010f860  Drillers (gap; CModule_Drillers.cpp inferred) */
void CModuleDrillers_Dtor(void *self)
{
	*(const void *const **)self = vtable_0026bf48;
	CGame_Dtor((unsigned int *)self);
}

/* 0x0010f870  Drillers/CModule_Drillers.cpp
The deleting destructor (flags bit 0: free the object). */
void *CModuleDrillers_DeletingDtor(void *self, unsigned int flags)
{
	CModuleDrillers_Dtor(self);
	if ((flags & 1) != 0)
		DR_DELETE(self);
	return self;
}

/* 0x0010f890  Drillers/CModule_Drillers.cpp
vtable slot 3: starts variant `variant` (1-4): pushes a new Drillers game mode (it keeps
the variant at +0x28), then the minigame intro with the picture "drill<n>.bmp", then sets
the four HUD bar values (HUD +0xa8..+0xb4) to 175. Returns 0 when a mode could not be
pushed. (The original guards the allocations with a C++ exception frame; the port has no
exceptions.) */
int CModuleDrillers_Start(void *self, int variant)
{
	char picture[12];
	void *mode;
	int offset;

	(void)self;
	mode = DR_NEW(0x2c, DR_FILE_MODULE, 0x59, 0x1001);
	if (mode == NULL)
		g_DrGameMode = NULL;
	else
		g_DrGameMode = CGameModeDrillers_Ctor(mode);
	DR_I32(g_DrGameMode, 0x28) = variant;
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750,
			(struct CGameMode *)g_DrGameMode, 1) == 0)
		return 0;
	sprintf(picture, "drill%d.bmp", variant);
	mode = DR_NEW(0x300, DR_FILE_MODULE, 99, 0x1001);
	if (mode != NULL)
		mode = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)mode,
			variant + 0x3b, picture);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode,
			1) == 0)
		return 0;
	for (offset = 0xa8; offset < 0xb8; offset += 4)
		DR_F32(DAT_002ecfac, offset) = 175.0f;
	return 1;
}
