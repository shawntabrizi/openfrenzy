/*
RollerBall/CModule_RollerBall.c  <-  C:\BlitzParty\Game\Src\RollerBall\CModule_RollerBall.cpp

The RollerBall game entry (a CGame, core): its name and the four variation names, the
creation and deletion of the game object (DAT_0030ef04), and the start of a variation.
0x001877b0 and 0x001877e0 (gap) are the constructor and destructor that store this file's
vtable 0x002738b4.
*/

#include "rollerball_a.h"

/* 0x0030ef04  the CRollerBall_Game object (.bss; rollerball_b reads it too) */
unsigned char *DAT_0030ef04;

/* 0x002738b4  vtable of CModule_RollerBall (rebuilt from the XBE) */
const void *const vtable_002738b4[4] = {
	(const void *)CModuleRollerBall_DeletingDtor, /* +0x00 */
	(const void *)CModuleRollerBall_Init,         /* +0x04 */
	(const void *)CModuleRollerBall_End,          /* +0x08 */
	(const void *)CModuleRollerBall_Start,        /* +0x0c */
};

/* 0x001877b0  RollerBall (gap; CModule_RollerBall.cpp inferred: stores this file's vtable) */
void *CModuleRollerBall_Ctor(void *self)
{
	CGame_Ctor6((unsigned int *)self, "RollerBall", (unsigned int)"Sphere of Destiny",
		(unsigned int)"Ball Fighting", (unsigned int)"Roll of Honour",
		(unsigned int)"Rolls Race");
	*(const void *const **)self = vtable_002738b4;
	return self;
}

/* 0x001877e0  RollerBall (gap; CModule_RollerBall.cpp inferred) */
void CModuleRollerBall_Dtor(void *self)
{
	*(const void *const **)self = vtable_002738b4;
	CGame_Dtor((unsigned int *)self);
}

/* 0x00187820  RollerBall/CModule_RollerBall.cpp
The deleting destructor (flags bit 0: free the object). */
void *CModuleRollerBall_DeletingDtor(void *self, unsigned int flags)
{
	CModuleRollerBall_Dtor(self);
	if ((flags & 1) != 0)
		RB_DELETE(self);
	return self;
}

/* 0x00187840  RollerBall/CModule_RollerBall.cpp
vtable slot 1: creates the game object (0x2ae0 bytes). Returns the result of the shared
"return 1" function 0x000a6fa0. (The original guards the allocation with a C++ exception
frame; the port has no exceptions.) */
int CModuleRollerBall_Init(void *self)
{
	void *game;

	(void)self;
	game = RB_NEW(0x2ae0, RB_FILE_MODULE, 0x39);
	if (game == NULL)
		DAT_0030ef04 = NULL;
	else
		DAT_0030ef04 = CRollerBallGame_Ctor(game);
	return BCarFloaty_DeleteItem();
}

/* 0x001877f0  RollerBall/CModule_RollerBall.cpp
vtable slot 2: deletes the game object (virtual deleting destructor). */
void CModuleRollerBall_End(void *self)
{
	(void)self;
	FUN_001d5950(); /* empty (ECX = the game object) */
	if (DAT_0030ef04 != NULL)
		RB_VFN(DAT_0030ef04, 0x00, void *(*)(void *, unsigned int))(DAT_0030ef04, 1);
}

/* 0x00187810  RollerBall/CModule_RollerBall.cpp
vtable slot 3: starts variation `variation` (1-4) on the game object (jmp to 0x00188cb0). */
int CModuleRollerBall_Start(void *self, int variation)
{
	(void)self;
	return CRollerBallGame_Start(DAT_0030ef04, variation);
}
