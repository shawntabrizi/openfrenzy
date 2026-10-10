/*
TankedUp/CModule_TankedUp.c  <-  C:\BlitzParty\Game\Src\TankedUp\CModule_TankedUp.cpp

The TankedUp game entry (a CGame, core): its name, the four variation names, and the start
of a variation (starts CTank_Game, then pushes the minigame intro with "tank<n>.bmp").
0x001c1760 and 0x001c1790 (gap) are the constructor and destructor that store this file's
vtable 0x00278d1c.
*/

#include "tankedup_a.h"

/* 0x00278d1c  vtable of CModule_TankedUp (rebuilt from the XBE) */
const void *const vtable_00278d1c[4] = {
	(const void *)CModuleTankedUp_DeletingDtor, /* +0x00 */
	(const void *)CModuleTankedUp_Init,         /* +0x04 */
	(const void *)CModuleTankedUp_Slot2,        /* +0x08 */
	(const void *)CModuleTankedUp_Start,        /* +0x0c */
};

/* 0x001c1760  TankedUp/CModule_TankedUp.cpp (inferred: stores this file's vtable) */
void *CModuleTankedUp_Ctor(void *self)
{
	CGame_Ctor6((unsigned int *)self, "TankedUp", (unsigned int)"Command",
		(unsigned int)"Drop Zone", (unsigned int)"Trench Warfare", (unsigned int)"Race");
	*(const void *const **)self = vtable_00278d1c;
	return self;
}

/* 0x001c1790  TankedUp/CModule_TankedUp.cpp (inferred) */
void CModuleTankedUp_Dtor(void *self)
{
	*(const void *const **)self = vtable_00278d1c;
	CGame_Dtor((unsigned int *)self);
}

/* 0x001c17a0  TankedUp/CModule_TankedUp.cpp
vtable slot 1: forwards to a CTank_Game method that the compiler folded into the shared
`return 1` function 0x000a6fa0. */
int CModuleTankedUp_Init(void *self)
{
	(void)self;
	return 1;
}

/* 0x001c17b0  TankedUp/CModule_TankedUp.cpp
vtable slot 2: forwards to an empty CTank_Game method (0x001d5950). */
void CModuleTankedUp_Slot2(void *self)
{
	(void)self;
}

/* 0x001c17c0  TankedUp/CModule_TankedUp.cpp
The deleting destructor (flags bit 0: free the object). */
void *CModuleTankedUp_DeletingDtor(void *self, unsigned int flags)
{
	CModuleTankedUp_Dtor(self);
	if ((flags & 1) != 0)
		TU_DELETE(self);
	return self;
}

/* 0x001c17e0  TankedUp/CModule_TankedUp.cpp
vtable slot 3: starts variation `variation` (1-4): CTank_Game pushes its game mode, then
this pushes the minigame intro with the picture "tank<n>.bmp". Returns CTank_Game's result.
(The original guards the allocation with a C++ exception frame; the port has none.) */
int CModuleTankedUp_Start(void *self, int variation)
{
	char picture[64];
	int result;
	void *intro;

	(void)self;
	result = CTankGame_Start(TU_GAME, variation);
	_snprintf(picture, 0x3f, "tank%d.bmp", variation);
	intro = TU_NEW(0x300, TU_FILE_MODULE, 0x5f);
	if (intro != NULL)
		intro = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)intro,
			variation + 0x57, picture);
	CGameModeStack_Push(TU_MODESTACK, (struct CGameMode *)intro, 1);
	return result;
}
