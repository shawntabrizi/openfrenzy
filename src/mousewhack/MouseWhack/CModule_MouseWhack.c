/*
MouseWhack/CModule_MouseWhack.c  <-  C:\BlitzParty\Game\Src\MouseWhack\CModule_MouseWhack.cpp
(module mousewhack)

The Bug Bash entry of core's game list (a CGame, vtable 0x00271b94): name "MouseWhack",
variations "Brawl", "Tremors", "Burgers" and "Pied Pipers". Start pushes the game mode
(CGameMode_MouseWhack) and the minigame intro screen, then makes the route tables the
CharLib AI players share.
*/

#include "mousewhack.h"

static const char s_ModuleFile[] = "C:\\BlitzParty\\Game\\Src\\MouseWhack\\CModule_MouseWhack.cpp";

/* core: the mode stack (0x002ba750) */
extern unsigned char DAT_002ba750[];

/* 0x00271b94  CModule_MouseWhack vtable (the CGame virtuals) */
const void *const vtable_00271b94[4] = {
	(const void *)FUN_0016cd20, /* [0] scalar deleting destructor */
	(const void *)FUN_000a6fa0, /* [1] init (returns 1) */
	(const void *)FUN_001d5950, /* [2] shutdown (empty) */
	(const void *)FUN_0016cd40, /* [3] start (variation) */
};

/* 0x0016cce0  (gap): CModule_MouseWhack::CModule_MouseWhack */
unsigned int *CModule_MouseWhack_Ctor(unsigned int *self)
{
	CGame_Ctor6(self, "MouseWhack", (unsigned int)(uintptr_t)"Brawl", (unsigned int)(uintptr_t)"Tremors",
		(unsigned int)(uintptr_t)"Burgers", (unsigned int)(uintptr_t)"Pied Pipers");
	self[0] = (unsigned int)(uintptr_t)vtable_00271b94;
	return self;
}

/* 0x0016cd10  (gap): CModule_MouseWhack::~CModule_MouseWhack */
void CModule_MouseWhack_Dtor(unsigned int *self)
{
	self[0] = (unsigned int)(uintptr_t)vtable_00271b94;
	CGame_Dtor(self);
}

/* 0x0016cd20  MouseWhack/CModule_MouseWhack.cpp: scalar deleting destructor */
unsigned int *CModule_MouseWhack_Destroy(unsigned int *self, byte flags)
{
	CModule_MouseWhack_Dtor(self);
	if ((flags & 1) != 0)
		MW_DELETE(self);
	return self;
}

/* 0x000ad830  CharLib/Route.cpp: constructor of one 0x18-byte route height table (two
arrays over the route grid). Not in the function table: only the "eh vector constructor
iterator" call reaches it; a local copy (charlib owns the original). */
static void *mw_RouteTable_Ctor(void *self)
{
	MW_AP(self, 0x0) = bkHeapAlloc(DAT_00292c78 * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23d, 0x2001);
	MW_AP(self, 0x4) = bkHeapAlloc(DAT_00292c78, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23e, 0x2001);
	return self;
}

/* 0x0016cd40  MouseWhack/CModule_MouseWhack.cpp: start (thiscall, `self` unused): pushes the
game mode, then the minigame intro ("bug<variation>.bmp"); on success makes the 4 route
tables and gives them to the shared route object 0x0030a0f0. Returns 1 if both pushes
succeeded. */
int CModule_MouseWhack_Start(unsigned int *self, int variation)
{
	char picture[64];
	void *mode;
	int *block;
	char *tables;
	int i;

	(void)self;
	sprintf(picture, "bug%d.bmp", variation);
	mw_Variation = (short)(variation - 1);
	mode = bkHeapAlloc(0x34, s_ModuleFile, 0x62, 0x1001);
	if (mode != NULL)
		mode = CGameMode_MouseWhack_Ctor(mode);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1) == 0)
		return 0;
	mode = bkHeapAlloc(0x300, s_ModuleFile, 0x68, 0x1001);
	if (mode != NULL)
		mode = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)mode, variation + 0x47, picture);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)mode, 1) == 0)
		return 0;
	bkPrintf("Bug Bash variation=%d\n", variation);
	/* new CLRouteTable[4] (eh vector constructor iterator; destructor 0x000ad880) */
	block = (int *)bkHeapAlloc(100, s_ModuleFile, 0x70, 0x1001);
	if (block == NULL) {
		tables = NULL;
	} else {
		tables = (char *)(block + 1);
		*block = 4;
		for (i = 0; i < 4; i++)
			mw_RouteTable_Ctor(tables + i * 0x18);
	}
	mw_RouteTables = tables;
	FUN_000c39b0((undefined4)(uintptr_t)DAT_0030a0f0, (undefined4)(uintptr_t)tables, 2, 2);
	return 1;
}
