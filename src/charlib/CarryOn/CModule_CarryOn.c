/*
CarryOn/CModule_CarryOn.c  (charlib)
Original: C:\BlitzParty\Game\Src\CarryOn\CModule_CarryOn.cpp (the file name is in the
allocation at 0x000b1b2a).

The "Fireworks" minigame module (CarryOn): its game entry with the four variant names
and the start of a variant. These four functions lie inside the CharLib text block, so
the module table gives them to charlib (docs/modules.tsv); they belong with the carryon
modules (reported to the architect).
*/

#include "charlib.h"

/* 0x002ba750  the game mode stack (core) */
extern unsigned char DAT_002ba750[0x58];
/* 0x00293ad8  the CFire_Game object (carryon .data) */
extern unsigned char DAT_00293ad8[0xc];
/* 0x00293ae4  the CarryOn game mode (carryon .data) */
extern void *DAT_00293ae4;
/* 0x00293b28  (carryon .data) */
extern int DAT_00293b28;

/* core: CGame_Ctor6(self, name, variant names ...), CGame_Dtor(self),
CGameModeStack_Push(stack, mode, resume) */
#define cm_GameCtor \
	((void (*)(void *, const char *, const char *, const char *, const char *, const char *))FUN_0002bf30)
#define cm_GameDtor ((void (*)(void *))FUN_0002bf60)
#define cm_PushMode ((int (*)(void *, void *, int))FUN_0002b640)
/* core_modes_b: CGameMode_MiniGameIntro_ctor(self, game, picture name) */
#define cm_MiniGameIntroCtor ((void *(*)(void *, int, const char *))FUN_00023440)
/* carryon_a: CFire_Game methods (self, variant) */
#define cm_FireGameSetVariant ((void (*)(void *, int))FUN_000bd820)
#define cm_FireGameSetLevel ((void (*)(void *, int))FUN_000babe0)

/* 0x00267000  the vtable of the module (rebuilt from the XBE) */
static const void *const vtable_00267000[] = {
	(const void *)CModuleCarryOn_DeletingDtor, /* [0] +0x00 */
	(const void *)FUN_000a6fa0,                /* [1] +0x04 */
	(const void *)FUN_001d5950,                /* [2] +0x08 (empty) */
	(const void *)CModuleCarryOn_Start,        /* [3] +0x0c */
};

/* 0x000b1a60  CarryOn/CModule_CarryOn.cpp (in the CharLib block) */
void *CModuleCarryOn_Ctor(void *self)
{
	cm_GameCtor(self, "Fireworks", "Let's do Launch", "Up the glitter", "Firework Dismay",
		"Carnival Knowledge");
	*(const void *const **)self = vtable_00267000;
	return self;
}

/* 0x000b1a90  CarryOn/CModule_CarryOn.cpp (in the CharLib block) */
void CModuleCarryOn_Dtor(void *self)
{
	*(const void *const **)self = vtable_00267000;
	cm_GameDtor(self);
}

/* 0x000b1aa0  CarryOn/CModule_CarryOn.cpp (in the CharLib block)
vtable slot 3: starts variant `variant` (1-based): sets up the CFire_Game object, pushes
the CarryOn game mode, then the minigame intro mode with the picture "fire<n>.bmp".
Returns nonzero when both modes were pushed. (The original guards the allocation with a
C++ exception frame; the port has no exceptions.) */
int CModuleCarryOn_Start(void *self, int variant)
{
	char picture[20];
	void *mode;

	(void)self;
	cm_FireGameSetVariant(DAT_00293ad8, variant);
	cm_FireGameSetLevel(DAT_00293ad8, variant - 1);
	DAT_00293b28 = 0;
	if (cm_PushMode(DAT_002ba750, DAT_00293ae4, 1) == 0)
		return 0;
	sprintf(picture, "fire%d.bmp", variant);
	mode = chl_Alloc(0x300, "C:\\BlitzParty\\Game\\Src\\CarryOn\\CModule_CarryOn.cpp", 0x67, 0x1001);
	if (mode == NULL)
		mode = NULL;
	else
		mode = cm_MiniGameIntroCtor(mode, variant + 0x2f, picture);
	return cm_PushMode(DAT_002ba750, mode, 1) != 0;
}

/* 0x000b1b90  CarryOn/CModule_CarryOn.cpp (in the CharLib block)
The deleting destructor (flags bit 0: free the object). */
void *CModuleCarryOn_DeletingDtor(void *self, unsigned int flags)
{
	CModuleCarryOn_Dtor(self);
	if ((flags & 1) != 0)
		chl_Free(self);
	return self;
}
