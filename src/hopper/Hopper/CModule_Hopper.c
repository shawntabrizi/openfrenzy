/*
Hopper/CModule_Hopper.c  <-  C:\BlitzParty\Game\Src\Hopper\CModule_Hopper.cpp
(module hopper)

The Hopper entry of core's game list (a CGame, vtable 0x0026e14c): name "Hopper",
variations "Prop Rotation", "Centrifugal Farce", "Careless Torque..." and "Twisted System".
Start makes the game mode (CGameMode "Hopper"), the route tables the CharLib AI players
share, and pushes the game mode and the minigame intro screen ("hop<variation>.bmp").
*/

#include "hopper.h"

static const char s_ModuleFile[] = "C:\\BlitzParty\\Game\\Src\\Hopper\\CModule_Hopper.cpp";

/* 0x0026e14c  CModule_Hopper vtable (the CGame virtuals) */
const void *const vtable_0026e14c[4] = {
	(const void *)FUN_0013b9d0, /* [0] scalar deleting destructor */
	(const void *)FUN_000a6fa0, /* [1] init (returns 1) */
	(const void *)FUN_001d5950, /* [2] shutdown (empty) */
	(const void *)FUN_0013b9f0, /* [3] start (variation) */
};

/* 0x0013b990  (gap): CModule_Hopper::CModule_Hopper */
unsigned int *CModule_Hopper_Ctor(unsigned int *self)
{
	CGame_Ctor6(self, "Hopper", (unsigned int)(uintptr_t)"Prop Rotation",
		(unsigned int)(uintptr_t)"Centrifugal Farce", (unsigned int)(uintptr_t)"Careless Torque...",
		(unsigned int)(uintptr_t)"Twisted System");
	self[0] = (unsigned int)(uintptr_t)vtable_0026e14c;
	return self;
}

/* 0x0013b9c0  (gap): CModule_Hopper::~CModule_Hopper */
void CModule_Hopper_Dtor(unsigned int *self)
{
	self[0] = (unsigned int)(uintptr_t)vtable_0026e14c;
	CGame_Dtor(self);
}

/* 0x0013b9d0  Hopper/CModule_Hopper.cpp: scalar deleting destructor */
unsigned int *CModule_Hopper_Destroy(unsigned int *self, byte flags)
{
	CModule_Hopper_Dtor(self);
	if ((flags & 1) != 0)
		HOP_DELETE(self);
	return self;
}

/* 0x000ad830  CharLib/Route.cpp: constructor of one 0x18-byte route table (a short and a
byte per grid cell). Not in the function table: only "eh vector constructor iterator"
calls reach it; a local copy (charlib owns the original; mousewhack and carryon_a keep
copies too). */
static void *hop_RouteTable_Ctor(void *self)
{
	HOP_AP(self, 0x0) = bkHeapAlloc(DAT_00292c78 * 2, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23d, 0x2001);
	HOP_AP(self, 0x4) = bkHeapAlloc(DAT_00292c78, "C:\\BlitzParty\\Game\\Src\\CharLib\\Route.cpp", 0x23e, 0x2001);
	return self;
}

/* 0x0013b9f0  Hopper/CModule_Hopper.cpp: start (thiscall, `self` unused). Makes the game
mode (it keeps the variation at +0x28), one route table per grid cell for the shared route
object 0x003024c8, then pushes the mode and the intro ("hop<variation>.bmp"). Returns 1 if
both pushes succeeded. */
int CModule_Hopper_Start(unsigned int *self, int variation)
{
	char picture[12];
	int *mode;
	int *block;
	char *tables;
	void *intro;
	int count;
	int i;

	(void)self;
	mode = (int *)HOP_NEW(0x2c, s_ModuleFile, 0x59);
	if (mode == NULL)
		hop_Mode = NULL;
	else
		hop_Mode = (int *)CGameMode_Hopper_Ctor(mode);
	hop_Mode[10] = variation;
	/* new CLRouteTable[route cells] (eh vector constructor iterator; destructor 0x000ad880) */
	count = DAT_00292c78;
	block = (int *)HOP_NEW(count * 0x18 + 4, s_ModuleFile, 0x5e);
	if (block == NULL) {
		tables = NULL;
	} else {
		tables = (char *)(block + 1);
		*block = count;
		for (i = 0; i < count; i++)
			hop_RouteTable_Ctor(tables + i * 0x18);
	}
	hop_RouteTables = tables;
	FUN_000c39b0((undefined4)(uintptr_t)hop_RouteObject, (undefined4)(uintptr_t)tables, 0x18, 0x18);
	if (CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)hop_Mode, 1) == 0)
		return 0;
	sprintf(picture, "hop%d.bmp", variation);
	intro = HOP_NEW(0x300, s_ModuleFile, 0x68);
	if (intro != NULL)
		intro = CGameMode_MiniGameIntro_ctor((struct CGameMode_MiniGameIntro *)intro, variation + 0x43, picture);
	return CGameModeStack_Push((struct CGameModeStack *)DAT_002ba750, (struct CGameMode *)intro, 1) != 0;
}
