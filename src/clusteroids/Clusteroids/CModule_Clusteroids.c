/*
Clusteroids/CModule_Clusteroids.c  <-  C:\BlitzParty\Game\Src\Clusteroids\CModule_Clusteroids.cpp

The Clusteroids game entry (a CGame, core): its name, the four variant names, and the
start of a variant (pushes the Clusteroids game mode and the minigame intro).
0x000d9390 and 0x000d93c0 (gap) are the constructor and destructor that store this
file's vtable 0x00268d34.
*/

#include "clusteroids.h"
#include "clusteroids_game.h"

/* 0x00268d34  vtable of CModule_Clusteroids (rebuilt from the XBE) */
const void *const vtable_00268d34[4] = {
	(const void *)CModuleClusteroids_DeletingDtor, /* +0x00 */
	(const void *)CModuleClusteroids_Init,         /* +0x04 */
	(const void *)FUN_001d5950,                    /* +0x08 (empty) */
	(const void *)CModuleClusteroids_Start,        /* +0x0c */
};

/* 0x000d9390  Clusteroids/CModule_Clusteroids.cpp (inferred: stores this file's vtable) */
void *CModuleClusteroids_Ctor(void *self)
{
	CGame_Ctor6((unsigned int *)self, "Clusteroids", (unsigned int)"Tails",
		(unsigned int)"Pod Hog", (unsigned int)"Mines", (unsigned int)"Collector");
	*(const void *const **)self = vtable_00268d34;
	return self;
}

/* 0x000d93c0  Clusteroids/CModule_Clusteroids.cpp (inferred) */
void CModuleClusteroids_Dtor(void *self)
{
	*(const void *const **)self = vtable_00268d34;
	CGame_Dtor((unsigned int *)self);
}

/* 0x000d93d0  Clusteroids/CModule_Clusteroids.cpp
vtable slot 1. */
int CModuleClusteroids_Init(void *self)
{
	(void)self;
	bkPrintf("CModule_Clusteroids::Init()\n");
	return 1;
}

/* 0x000d93f0  Clusteroids/CModule_Clusteroids.cpp
The deleting destructor (flags bit 0: free the object). */
void *CModuleClusteroids_DeletingDtor(void *self, unsigned int flags)
{
	CModuleClusteroids_Dtor(self);
	if ((flags & 1) != 0)
		CL_DELETE(self);
	return self;
}

/* 0x000d9410  Clusteroids/CModule_Clusteroids.cpp
vtable slot 3: starts variant `variant` (1-4): pushes a new Clusteroids game mode, then
the minigame intro with the picture "clust<n>.bmp". Returns 0 when the game mode could
not be pushed. (The original guards the allocations with a C++ exception frame; the port
has no exceptions.) */
int CModuleClusteroids_Start(void *self, int variant)
{
	char picture[64];
	void *mode;

	(void)self;
	g_ClVariant = (short)variant;
	mode = CL_NEW(0x2c, CL_FILE_MODULE, 0x62, 0x1001);
	if (mode != NULL)
		mode = CGameModeClusteroids_Ctor(mode);
	if (CGameModeStack_Push(CL_MODESTACK, (struct CGameMode *)mode, 1) == 0)
		return 0;
	_snprintf(picture, 0x3f, "clust%d.bmp", (int)g_ClVariant);
	mode = CL_NEW(0x300, CL_FILE_MODULE, 0x66, 0x1001);
	if (mode != NULL)
		mode = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)mode,
			g_ClVariant + 0x33, picture);
	CGameModeStack_Push(CL_MODESTACK, (struct CGameMode *)mode, 1);
	return 1;
}
